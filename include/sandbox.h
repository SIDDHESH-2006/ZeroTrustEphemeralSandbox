#ifndef SANDBOX_H
#define SANDBOX_H

/* Ensure GNU-specific extensions and system call constants are exposed */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Standard POSIX and C library headers */
#include <stdio.h>       /* Standard I/O operations (printf, fprintf, perror) */
#include <stdlib.h>      /* Memory allocation, process exit codes (EXIT_SUCCESS, EXIT_FAILURE) */
#include <stdint.h>      /* Exact-width integer types (uint64_t, uint32_t) */
#include <stdbool.h>     /* Boolean data type (bool, true, false) */
#include <string.h>      /* String manipulation routines (strcmp, snprintf, memset) */
#include <limits.h>      /* Architecture limits including maximum path length (PATH_MAX) */
#include <errno.h>       /* System error number definitions and strerror() */
#include <unistd.h>      /* Standard symbolic constants and types (close, execvp) */

/* Software release version string */
#define SANDBOX_VERSION "1.0.0"

/* Upper bound limit for path allowlist rules configurable by user */
#define MAX_PATH_RULES 256

/* Upper bound limit for default system shared library paths */
#define MAX_DEFAULT_PATHS 16

/* ANSI escape code definitions for colorized terminal diagnostic logging */
#define ANSI_RESET   "\033[0m"   /* Reset all text attributes to terminal default */
#define ANSI_BOLD    "\033[1m"   /* Enable bold text styling */
#define ANSI_RED     "\033[31m"  /* Foreground red color for error messages */
#define ANSI_GREEN   "\033[32m"  /* Foreground green color for success messages */
#define ANSI_YELLOW  "\033[33m"  /* Foreground yellow color for warning messages */
#define ANSI_BLUE    "\033[34m"  /* Foreground blue color for informational messages */
#define ANSI_CYAN    "\033[36m"  /* Foreground cyan color for verbose debug messages */

/* Macro to output formatted informational messages to standard error */
#define sb_log_info(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_BLUE "[sandbox:info]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

/* Macro to output formatted success notifications to standard error */
#define sb_log_success(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_GREEN "[sandbox:ok]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

/* Macro to output formatted warning messages to standard error */
#define sb_log_warn(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_YELLOW "[sandbox:warn]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

/* Macro to output formatted error messages to standard error */
#define sb_log_error(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_RED "[sandbox:error]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

/* Macro to output conditional debug messages only when verbose mode is enabled */
#define sb_log_debug(verbose, ...) \
    do { \
        if (verbose) { \
            fprintf(stderr, ANSI_CYAN "[sandbox:debug]" ANSI_RESET " "); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while (0)

/* Enumeration of filesystem access levels supported by the sandbox ruleset */
typedef enum {
    ACCESS_RO = 0,   /* Read-only access: allows file reading, directory listing, execution */
    ACCESS_RW = 1    /* Read-write access: permits full write, creation, truncation, removal */
} path_access_t;

/* Representation of an individual filesystem path rule */
struct path_rule {
    char path[PATH_MAX];
    path_access_t access;
    bool is_default;      /* true if auto-added default system path */
};

/* Unified sandbox runtime configuration structure */
struct sandbox_config {
    struct path_rule rules[MAX_PATH_RULES]; /* Array of allowlisted path rules */
    size_t rule_count;                      /* Total number of active path rules registered */

    bool allow_net;           /* Network policy: false blocks socket creation via Seccomp */
    bool unshare_net;         /* Namespace policy: true decouples network & user namespace */
    bool drop_caps;           /* Capabilities policy: true clears ambient & permitted sets */
    bool no_default_paths;    /* Library policy: true omits auto-allowlisting system paths */
    bool verbose;             /* Diagnostic policy: true prints step-by-step debug traces */

    char **payload_argv;      /* Argument vector for target payload command to be executed */
    int payload_argc;         /* Number of arguments present in target payload vector */
};

#endif /* SANDBOX_H */
