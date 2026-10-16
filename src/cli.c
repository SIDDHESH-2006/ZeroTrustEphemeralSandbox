#include "cli.h"
#include <getopt.h>
#include <sys/stat.h>

void cli_print_version(void) {
    printf(ANSI_BOLD "Zero-Trust Ephemeral Sandbox (Landlock+Seccomp) v%s\n" ANSI_RESET, SANDBOX_VERSION);
    printf("BCSE303P Operating Systems Lab - Vellore Institute of Technology\n");
    printf("Project Review - 1 (Fall Semester 2026-2027)\n\n");
    printf(ANSI_BOLD "Authors:\n" ANSI_RESET);
    printf("  - Mohammad Ammar Abdullah (24BYB0017)\n");
    printf("  - Siddhesh Srinivasan     (24BYB0035)\n");
    printf("  - Aditi Soni              (24BDE0144)\n\n");
    printf(ANSI_BOLD "Faculty Guide:\n" ANSI_RESET);
    printf("  - Dr. PORKODI S (Assistant Professor Senior Grade - 1)\n");
}

void cli_print_usage(const char *prog_name) {
    printf(ANSI_BOLD "Usage:" ANSI_RESET " %s [options] -- <command> [args...]\n\n", prog_name);
    printf(ANSI_BOLD "Options:\n" ANSI_RESET);
    printf("  -r, --ro <path>          Allow read-only access to <path> (repeatable)\n");
    printf("  -w, --rw <path>          Allow read-write access to <path> (repeatable)\n");
    printf("  -n, --no-net             Deny all network socket operations (default)\n");
    printf("      --allow-net          Permit network socket syscalls\n");
    printf("      --unshare-net        Unshare network and user namespace (CLONE_NEWNET)\n");
    printf("      --drop-caps          Drop all ambient, permitted and inheritable capabilities\n");
    printf("      --no-default-paths   Do not auto-allowlist system shared library paths (/usr, /lib, etc.)\n");
    printf("  -v, --verbose            Enable verbose diagnostic output\n");
    printf("  -h, --help               Display this help message and exit\n");
    printf("      --version            Display project details and version\n\n");
    printf(ANSI_BOLD "Examples:\n" ANSI_RESET);
    printf("  # Run untrusted bash script with read-only /usr and read-write ./tmp\n");
    printf("  %s --ro /usr --rw ./tmp -- /bin/bash untrusted.sh\n\n", prog_name);
    printf("  # Run Python script with strict network blocking\n");
    printf("  %s --rw . -- /usr/bin/python3 script.py\n\n", prog_name);
    printf("  # Completely isolated execution with dropped capabilities\n");
    printf("  %s --drop-caps --rw /tmp -- /bin/ls -la /tmp\n", prog_name);
}

static int add_rule(struct sandbox_config *config, const char *path, path_access_t access, bool is_default) {
    if (config->rule_count >= MAX_PATH_RULES) {
        sb_log_error("Maximum number of path rules (%d) exceeded", MAX_PATH_RULES);
        return -1;
    }

    if (!is_default) {
        struct stat st;
        if (stat(path, &st) != 0) {
            sb_log_error("Specified path does not exist or cannot be accessed: %s (%s)", path, strerror(errno));
            return -1;
        }
    }

    char resolved[PATH_MAX];
    const char *target = path;

    /* If path can be resolved via realpath, use canonical path */
    if (realpath(path, resolved) != NULL) {
        target = resolved;
    }

    /* Check for duplicate path */
    for (size_t i = 0; i < config->rule_count; i++) {
        if (strcmp(config->rules[i].path, target) == 0) {
            /* If existing is RO and new is RW, upgrade to RW */
            if (access == ACCESS_RW) {
                config->rules[i].access = ACCESS_RW;
            }
            if (!is_default) {
                config->rules[i].is_default = false;
            }
            return 0;
        }
    }

    snprintf(config->rules[config->rule_count].path,
             sizeof(config->rules[config->rule_count].path),
             "%s", target);
    config->rules[config->rule_count].access = access;
    config->rules[config->rule_count].is_default = is_default;
    config->rule_count++;

    return 0;
}

int cli_add_default_paths(struct sandbox_config *config) {
    if (config->no_default_paths) {
        return 0;
    }

    /* Default paths necessary for loading dynamic linkers, libc, and system tools */
    static const char *default_ro_paths[] = {
        "/usr",
        "/lib",
        "/lib64",
        "/bin",
        "/sbin",
        "/etc"
    };

    size_t count = sizeof(default_ro_paths) / sizeof(default_ro_paths[0]);
    for (size_t i = 0; i < count; i++) {
        struct stat st;
        if (stat(default_ro_paths[i], &st) == 0) {
            if (add_rule(config, default_ro_paths[i], ACCESS_RO, true) != 0) {
                return -1;
            }
        }
    }

    /* Allow standard pseudo-devices */
    static const char *default_rw_paths[] = {
        "/dev"
    };
    count = sizeof(default_rw_paths) / sizeof(default_rw_paths[0]);
    for (size_t i = 0; i < count; i++) {
        struct stat st;
        if (stat(default_rw_paths[i], &st) == 0) {
            if (add_rule(config, default_rw_paths[i], ACCESS_RW, true) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

int cli_parse_args(int argc, char *argv[], struct sandbox_config *config) {
    memset(config, 0, sizeof(*config));
    config->allow_net = false;       /* zero-trust: network blocked by default */
    config->unshare_net = false;
    config->drop_caps = true;        /* zero-trust: drop capabilities by default */
    config->no_default_paths = false;
    config->verbose = false;

    static struct option long_options[] = {
        {"ro",               required_argument, 0, 'r'},
        {"rw",               required_argument, 0, 'w'},
        {"no-net",           no_argument,       0, 'n'},
        {"deny-net",         no_argument,       0, 'n'},
        {"allow-net",        no_argument,       0, 1001},
        {"unshare-net",      no_argument,       0, 1002},
        {"drop-caps",        no_argument,       0, 1003},
        {"no-default-paths", no_argument,       0, 1004},
        {"verbose",          no_argument,       0, 'v'},
        {"help",             no_argument,       0, 'h'},
        {"version",          no_argument,       0, 1005},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    int orig_argc = argc;
    int payload_start = -1;
    for (int i = 1; i < orig_argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            payload_start = i + 1;
            argv[i] = NULL;
            argc = i;
            break;
        }
    }

    while ((opt = getopt_long(argc, argv, "+r:w:nvh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'r':
                if (add_rule(config, optarg, ACCESS_RO, false) != 0) {
                    return -1;
                }
                break;
            case 'w':
                if (add_rule(config, optarg, ACCESS_RW, false) != 0) {
                    return -1;
                }
                break;
            case 'n':
                config->allow_net = false;
                break;
            case 1001: /* --allow-net */
                config->allow_net = true;
                break;
            case 1002: /* --unshare-net */
                config->unshare_net = true;
                break;
            case 1003: /* --drop-caps */
                config->drop_caps = true;
                break;
            case 1004: /* --no-default-paths */
                config->no_default_paths = true;
                break;
            case 'v':
                config->verbose = true;
                break;
            case 'h':
                cli_print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 1005: /* --version */
                cli_print_version();
                exit(EXIT_SUCCESS);
            case '?':
            default:
                cli_print_usage(argv[0]);
                return -1;
        }
    }

    /* Check payload arguments */
    if (payload_start == -1) {
        /* If no '--' was given, check if positional arguments exist */
        if (optind < orig_argc) {
            config->payload_argv = &argv[optind];
            config->payload_argc = orig_argc - optind;
        } else {
            sb_log_error("No target command specified. Use '-- <command> [args...]'");
            cli_print_usage(argv[0]);
            return -1;
        }
    } else if (payload_start >= orig_argc) {
        sb_log_error("No target command specified after '--'");
        cli_print_usage(argv[0]);
        return -1;
    } else {
        config->payload_argv = &argv[payload_start];
        config->payload_argc = orig_argc - payload_start;
    }

    if (config->payload_argc <= 0 || config->payload_argv[0] == NULL) {
        sb_log_error("Target command cannot be empty");
        return -1;
    }

    /* Add default system paths unless explicitly disabled */
    if (cli_add_default_paths(config) != 0) {
        return -1;
    }

    return 0;
}
