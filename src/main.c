/* Include core sandbox structures, error logging macros, and version information */
#include "sandbox.h"

/* Include subsystem modules for CLI parsing, process isolation, Landlock, and Seccomp */
#include "cli.h"
#include "isolate.h"
#include "landlock_mgr.h"
#include "seccomp_bpf.h"

/*
 * main:
 * Entry point and supervisor execution lifecycle for the Zero-Trust Ephemeral Sandbox.
 *
 * Sequence of security enforcement:
 * 1. Parse command-line flags and extract payload command.
 * 2. Enforce PR_SET_NO_NEW_PRIVS (mandatory kernel restriction).
 * 3. Decouple namespaces if requested (--unshare-net).
 * 4. Drop Linux capabilities (ambient and permitted sets).
 * 5. Apply Landlock LSM path-based filesystem barrier.
 * 6. Inject Seccomp-BPF system call filtering barrier.
 * 7. Dispatch payload via execvp() replacing memory image with zero parent overhead.
 */
int main(int argc, char *argv[]) {
    /* Declare unified sandbox configuration structure on stack */
    struct sandbox_config config;

    /* Step 1: Parse command-line options and payload arguments */
    if (cli_parse_args(argc, argv, &config) != 0) {
        /* Exit with failure code if argument parsing encountered an error */
        return EXIT_FAILURE;
    }

    /* Output verbose diagnostic startup banner if verbose mode is active */
    if (config.verbose) {
        /* Print initialization message */
        sb_log_info("Initializing Zero-Trust Ephemeral Sandbox v%s", SANDBOX_VERSION);
        /* Print target payload binary */
        sb_log_debug(true, "Target payload: %s", config.payload_argv[0]);
        /* Print network policy status */
        sb_log_debug(true, "Allow network: %s", config.allow_net ? "yes" : "no");
        /* Print capability policy status */
        sb_log_debug(true, "Drop capabilities: %s", config.drop_caps ? "yes" : "no");
        /* Print total registered allowlisted paths */
        sb_log_debug(true, "Total allowlisted filesystem paths: %zu", config.rule_count);
    }

    /* Step 2: Enforce PR_SET_NO_NEW_PRIVS (kernel prerequisite for unprivileged Landlock/Seccomp) */
    if (isolate_set_no_new_privs() != 0) {
        /* Exit immediately if privilege escalation barrier fails */
        return EXIT_FAILURE;
    }

    /* Step 3: Decouple network & user namespaces if requested */
    if (config.unshare_net) {
        /* Call isolate_namespaces */
        if (isolate_namespaces(config.unshare_net, config.verbose) != 0) {
            /* Exit on failure */
            return EXIT_FAILURE;
        }
    }

    /* Step 4: Drop Linux capabilities (ambient, effective, permitted, inheritable) */
    if (config.drop_caps) {
        /* Call isolate_drop_capabilities */
        if (isolate_drop_capabilities() != 0) {
            /* Exit on failure */
            return EXIT_FAILURE;
        }
    }

    /* Step 5: Apply Landlock LSM filesystem containment barrier */
    if (landlock_apply_sandbox(&config) != 0) {
        /* Log error if filesystem jail initialization fails */
        sb_log_error("Failed to establish Landlock filesystem barrier");
        /* Exit with failure */
        return EXIT_FAILURE;
    }

    /* Step 6: Inject Seccomp-BPF system call filter barrier into kernel */
    if (seccomp_apply_filter(&config) != 0) {
        /* Log error if system call filter installation fails */
        sb_log_error("Failed to establish Seccomp-BPF syscall filter barrier");
        /* Exit with failure */
        return EXIT_FAILURE;
    }

    /* Output diagnostic confirmation if verbose mode is active */
    if (config.verbose) {
        /* Print confirmation that security barriers are locked */
        sb_log_success("Security barriers locked. Dispatching execution handoff...");
    }

    /* Step 7: Zero-overhead execution handoff to the target payload via execvp */
    execvp(config.payload_argv[0], config.payload_argv);

    /* If execvp returns, execution has failed (e.g. binary not found or permissions denied) */
    sb_log_error("Failed to execute '%s': %s", config.payload_argv[0], strerror(errno));

    /* Check if executable was not found */
    if (errno == ENOENT) {
        /* Return standard POSIX exit code 127 for command not found */
        return 127;
    } else if (errno == EACCES) {
        /* Return standard POSIX exit code 126 for command not executable */
        return 126;
    }

    /* Generic execution failure */
    return EXIT_FAILURE;
}
