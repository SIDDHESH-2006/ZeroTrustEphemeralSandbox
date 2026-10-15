#include "sandbox.h"
#include "cli.h"
#include "isolate.h"
#include "landlock_mgr.h"
#include "seccomp_bpf.h"

int main(int argc, char *argv[]) {
    struct sandbox_config config;

    /* 1. Parse command line arguments */
    if (cli_parse_args(argc, argv, &config) != 0) {
        return EXIT_FAILURE;
    }

    if (config.verbose) {
        sb_log_info("Initializing Zero-Trust Ephemeral Sandbox v%s", SANDBOX_VERSION);
        sb_log_debug(true, "Target payload: %s", config.payload_argv[0]);
        sb_log_debug(true, "Allow network: %s", config.allow_net ? "yes" : "no");
        sb_log_debug(true, "Drop capabilities: %s", config.drop_caps ? "yes" : "no");
        sb_log_debug(true, "Total allowlisted filesystem paths: %zu", config.rule_count);
    }

    /* 2. Enforce PR_SET_NO_NEW_PRIVS (mandatory before Landlock and unprivileged Seccomp) */
    if (isolate_set_no_new_privs() != 0) {
        return EXIT_FAILURE;
    }

    /* 3. Decouple namespaces if requested */
    if (config.unshare_net) {
        if (isolate_namespaces(config.unshare_net, config.verbose) != 0) {
            return EXIT_FAILURE;
        }
    }

    /* 4. Drop Linux capabilities */
    if (config.drop_caps) {
        if (isolate_drop_capabilities() != 0) {
            return EXIT_FAILURE;
        }
    }

    /* 5. Apply Landlock LSM filesystem containment barrier */
    if (landlock_apply_sandbox(&config) != 0) {
        sb_log_error("Failed to establish Landlock filesystem barrier");
        return EXIT_FAILURE;
    }

    /* 6. Apply Seccomp-BPF system call filter barrier */
    if (seccomp_apply_filter(&config) != 0) {
        sb_log_error("Failed to establish Seccomp-BPF syscall filter barrier");
        return EXIT_FAILURE;
    }

    if (config.verbose) {
        sb_log_success("Security barriers locked. Dispatching execution handoff...");
    }

    /* 7. Zero-overhead execution handoff to target payload */
    execvp(config.payload_argv[0], config.payload_argv);

    /* If execvp returns, an error occurred */
    sb_log_error("Failed to execute '%s': %s", config.payload_argv[0], strerror(errno));

    if (errno == ENOENT) {
        return 127; /* Standard command not found exit code */
    } else if (errno == EACCES) {
        return 126; /* Standard permission denied exit code */
    }

    return EXIT_FAILURE;
}
