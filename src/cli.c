/* Include CLI interface header and sandbox data structures */
#include "cli.h"

/* Standard argument parsing and filesystem headers */
#include <getopt.h>  /* GNU getopt_long function and option structures */
#include <sys/stat.h>/* POSIX file status definitions (stat function) */

/*
 * cli_print_version:
 * Outputs project title, university course, student authors, and faculty guide.
 */
void cli_print_version(void) {
    /* Print project title and release version with bold ANSI formatting */
    printf(ANSI_BOLD "Zero-Trust Ephemeral Sandbox (Landlock+Seccomp) v%s\n" ANSI_RESET, SANDBOX_VERSION);
    /* Print academic department and university info */
    printf("BCSE303P Operating Systems Lab - Vellore Institute of Technology\n");
    /* Print academic review milestone */
    printf("Project Review - 1 (Fall Semester 2026-2027)\n\n");
    /* Print team header */
    printf(ANSI_BOLD "Authors:\n" ANSI_RESET);
    /* Print student author 1 */
    printf("  - Mohammad Ammar Abdullah (24BYB0017)\n");
    /* Print student author 2 */
    printf("  - Siddhesh Srinivasan     (24BYB0035)\n");
    /* Print student author 3 */
    printf("  - Aditi Soni              (24BDE0144)\n\n");
    /* Print faculty guide header */
    printf(ANSI_BOLD "Faculty Guide:\n" ANSI_RESET);
    /* Print faculty guide details */
    printf("  - Dr. PORKODI S (Assistant Professor Senior Grade - 1)\n");
}

/*
 * cli_print_usage:
 * Prints syntax documentation and usage examples to stdout.
 */
void cli_print_usage(const char *prog_name) {
    /* Print basic command invocation syntax */
    printf(ANSI_BOLD "Usage:" ANSI_RESET " %s [options] -- <command> [args...]\n\n", prog_name);
    /* Print options section header */
    printf(ANSI_BOLD "Options:\n" ANSI_RESET);
    /* Describe read-only allowlist flag */
    printf("  -r, --ro <path>          Allow read-only access to <path> (repeatable)\n");
    /* Describe read-write allowlist flag */
    printf("  -w, --rw <path>          Allow read-write access to <path> (repeatable)\n");
    /* Describe network denial flag */
    printf("  -n, --no-net             Deny all network socket operations (default)\n");
    /* Describe network permission flag */
    printf("      --allow-net          Permit network socket syscalls\n");
    /* Describe network namespace decoupling flag */
    printf("      --unshare-net        Unshare network and user namespace (CLONE_NEWNET)\n");
    /* Describe capability dropping flag */
    printf("      --drop-caps          Drop all ambient, permitted and inheritable capabilities\n");
    /* Describe flag to disable default system library allowlisting */
    printf("      --no-default-paths   Do not auto-allowlist system shared library paths (/usr, /lib, etc.)\n");
    /* Describe verbose logging flag */
    printf("  -v, --verbose            Enable verbose diagnostic output\n");
    /* Describe help manual flag */
    printf("  -h, --help               Display this help message and exit\n");
    /* Describe version info flag */
    printf("      --version            Display project details and version\n\n");
    /* Print example commands header */
    printf(ANSI_BOLD "Examples:\n" ANSI_RESET);
    /* Example 1: running bash script with ro and rw rules */
    printf("  # Run untrusted bash script with read-only /usr and read-write ./tmp\n");
    printf("  %s --ro /usr --rw ./tmp -- /bin/bash untrusted.sh\n\n", prog_name);
    /* Example 2: running Python script with network blocked */
    printf("  # Run Python script with strict network blocking\n");
    printf("  %s --rw . -- /usr/bin/python3 script.py\n\n", prog_name);
    /* Example 3: running fully isolated ls command */
    printf("  # Completely isolated execution with dropped capabilities\n");
    printf("  %s --drop-caps --rw /tmp -- /bin/ls -la /tmp\n", prog_name);
}

static int add_rule(struct sandbox_config *config, const char *path, path_access_t access, bool is_default) {
    if (config->rule_count >= MAX_PATH_RULES) {
        /* Output error notification if path limit exceeded */
        sb_log_error("Maximum number of path rules (%d) exceeded", MAX_PATH_RULES);
        /* Return failure */
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
    /* Default target pointer points to original path */
    const char *target = path;

    /* If realpath resolves successfully, use canonical path to eliminate symlinks/dot-dots */
    if (realpath(path, resolved) != NULL) {
        /* Update target pointer to resolved canonical buffer */
        target = resolved;
    }

    /* Iterate through previously registered rules to avoid duplicate entries */
    for (size_t i = 0; i < config->rule_count; i++) {
        /* Compare current rule path with new target path */
        if (strcmp(config->rules[i].path, target) == 0) {
            /* If existing rule was RO and incoming rule is RW, upgrade access level to RW */
            if (access == ACCESS_RW) {
                /* Upgrade access */
                config->rules[i].access = ACCESS_RW;
            }
            if (!is_default) {
                config->rules[i].is_default = false;
            }
            return 0;
        }
    }

    /* Safely copy path string into the configuration rules array */
    snprintf(config->rules[config->rule_count].path,
             sizeof(config->rules[config->rule_count].path),
             "%s", target);
    /* Store access level (ACCESS_RO or ACCESS_RW) */
    config->rules[config->rule_count].access = access;
    config->rules[config->rule_count].is_default = is_default;
    config->rule_count++;

    /* Return success */
    return 0;
}

/*
 * cli_add_default_paths:
 * Allowlists standard system library, binary, and configuration directories necessary
 * for dynamic linkers and basic utilities to operate.
 */
int cli_add_default_paths(struct sandbox_config *config) {
    /* If user specified --no-default-paths, skip automatic allowlisting */
    if (config->no_default_paths) {
        /* Return success */
        return 0;
    }

    /* Array of standard system read-only directories required by dynamically linked ELF binaries */
    static const char *default_ro_paths[] = {
        "/usr",      /* User utilities, binaries, and shared libraries */
        "/lib",      /* Core shared libraries and kernel modules */
        "/lib64",    /* 64-bit dynamic linker and glibc libraries */
        "/bin",      /* Essential system executables */
        "/sbin",     /* Essential system administrative binaries */
        "/etc"       /* Configuration files (ld.so.cache, alternatives, etc.) */
    };

    /* Compute number of default read-only paths */
    size_t count = sizeof(default_ro_paths) / sizeof(default_ro_paths[0]);
    /* Iterate over each default RO path */
    for (size_t i = 0; i < count; i++) {
        /* Buffer for stat check */
        struct stat st;
        /* Check if path exists on the host filesystem */
        if (stat(default_ro_paths[i], &st) == 0) {
            if (add_rule(config, default_ro_paths[i], ACCESS_RO, true) != 0) {
                return -1;
            }
        }
    }

    /* Array of default pseudo-devices required for stdout/stderr redirection and random generation */
    static const char *default_rw_paths[] = {
        "/dev"       /* Pseudo-devices (/dev/null, /dev/zero, /dev/urandom) */
    };
    /* Compute number of default read-write paths */
    count = sizeof(default_rw_paths) / sizeof(default_rw_paths[0]);
    /* Iterate over default read-write paths */
    for (size_t i = 0; i < count; i++) {
        /* Buffer for stat check */
        struct stat st;
        /* Check if path exists */
        if (stat(default_rw_paths[i], &st) == 0) {
            if (add_rule(config, default_rw_paths[i], ACCESS_RW, true) != 0) {
                return -1;
            }
        }
    }

    /* Return success */
    return 0;
}

/*
 * cli_parse_args:
 * Parses command-line arguments, validates syntax, sets flags, and isolates payload command.
 */
int cli_parse_args(int argc, char *argv[], struct sandbox_config *config) {
    /* Clear and zero-initialize the entire configuration structure */
    memset(config, 0, sizeof(*config));
    /* Apply zero-trust default: network is blocked */
    config->allow_net = false;
    /* Apply zero-trust default: network namespace unshare is off unless requested */
    config->unshare_net = false;
    /* Apply zero-trust default: capabilities are dropped */
    config->drop_caps = true;
    /* Standard shared library paths are included by default */
    config->no_default_paths = false;
    /* Verbose diagnostic tracing is off by default */
    config->verbose = false;

    /* Define long option mappings for GNU getopt_long */
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

    /* Variable to hold option character */
    int opt;
    /* Variable to store index of matched option */
    int option_index = 0;

    /* Preserve original argc before modifying for getopt_long */
    int orig_argc = argc;
    /* Index tracking where target payload command starts */
    int payload_start = -1;

    /* Scan arguments looking for the '--' payload separator */
    for (int i = 1; i < orig_argc; i++) {
        /* Check if current argument is "--" */
        if (strcmp(argv[i], "--") == 0) {
            /* Payload command begins immediately after '--' */
            payload_start = i + 1;
            /* Replace '--' with NULL to truncate argument array for getopt_long */
            argv[i] = NULL;
            /* Update argc for getopt_long */
            argc = i;
            /* Stop searching */
            break;
        }
    }

    /* Process all command-line options preceding the '--' separator */
    while ((opt = getopt_long(argc, argv, "+r:w:nvh", long_options, &option_index)) != -1) {
        /* Switch on option character or ID */
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
            case 'n': /* -n or --no-net */
                /* Explicitly enforce network denial */
                config->allow_net = false;
                break;
            case 1001: /* --allow-net */
                /* Permit network socket operations */
                config->allow_net = true;
                break;
            case 1002: /* --unshare-net */
                /* Enable network namespace decoupling */
                config->unshare_net = true;
                break;
            case 1003: /* --drop-caps */
                /* Explicitly enable capability dropping */
                config->drop_caps = true;
                break;
            case 1004: /* --no-default-paths */
                /* Disable auto-inclusion of default system library paths */
                config->no_default_paths = true;
                break;
            case 'v': /* -v or --verbose */
                /* Enable diagnostic verbose tracing */
                config->verbose = true;
                break;
            case 'h': /* -h or --help */
                /* Print usage documentation */
                cli_print_usage(argv[0]);
                /* Exit immediately */
                exit(EXIT_SUCCESS);
            case 1005: /* --version */
                /* Print version and team credits */
                cli_print_version();
                /* Exit immediately */
                exit(EXIT_SUCCESS);
            case '?': /* Unknown option */
            default:
                /* Print usage and return failure */
                cli_print_usage(argv[0]);
                return -1;
        }
    }

    /* Validate payload command extraction */
    if (payload_start == -1) {
        /* If no '--' separator was passed, check if unparsed positional arguments remain */
        if (optind < orig_argc) {
            /* Assign start of payload argument vector */
            config->payload_argv = &argv[optind];
            /* Calculate payload argument count */
            config->payload_argc = orig_argc - optind;
        } else {
            /* Emit error that no command was specified */
            sb_log_error("No target command specified. Use '-- <command> [args...]'");
            /* Print usage synopsis */
            cli_print_usage(argv[0]);
            /* Return failure */
            return -1;
        }
    } else if (payload_start >= orig_argc) {
        /* Emit error if '--' was provided but nothing followed it */
        sb_log_error("No target command specified after '--'");
        /* Print usage synopsis */
        cli_print_usage(argv[0]);
        /* Return failure */
        return -1;
    } else {
        /* Assign start of payload argument vector after '--' */
        config->payload_argv = &argv[payload_start];
        /* Calculate payload argument count */
        config->payload_argc = orig_argc - payload_start;
    }

    /* Verify payload argument count and non-NULL executable path */
    if (config->payload_argc <= 0 || config->payload_argv[0] == NULL) {
        /* Output error notification */
        sb_log_error("Target command cannot be empty");
        /* Return failure */
        return -1;
    }

    /* Include standard shared library paths unless user disabled them */
    if (cli_add_default_paths(config) != 0) {
        /* Return failure if default path setup fails */
        return -1;
    }

    /* Return success */
    return 0;
}
