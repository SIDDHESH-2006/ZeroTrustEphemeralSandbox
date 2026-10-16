#ifndef SANDBOX_H
#define SANDBOX_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#define SANDBOX_VERSION "1.0.0"
#define MAX_PATH_RULES 256
#define MAX_DEFAULT_PATHS 16

/* Terminal colors for supervisor logs */
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"

#define sb_log_info(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_BLUE "[sandbox:info]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

#define sb_log_success(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_GREEN "[sandbox:ok]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

#define sb_log_warn(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_YELLOW "[sandbox:warn]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

#define sb_log_error(...) \
    do { \
        fprintf(stderr, ANSI_BOLD ANSI_RED "[sandbox:error]" ANSI_RESET " "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

#define sb_log_debug(verbose, ...) \
    do { \
        if (verbose) { \
            fprintf(stderr, ANSI_CYAN "[sandbox:debug]" ANSI_RESET " "); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while (0)

typedef enum {
    ACCESS_RO = 0,
    ACCESS_RW = 1
} path_access_t;

struct path_rule {
    char path[PATH_MAX];
    path_access_t access;
    bool is_default;      /* true if auto-added default system path */
};

struct sandbox_config {
    struct path_rule rules[MAX_PATH_RULES];
    size_t rule_count;

    bool allow_net;           /* false by default: network sockets are blocked */
    bool unshare_net;         /* call unshare(CLONE_NEWNET) if set */
    bool drop_caps;           /* explicitly drop ambient & inheritable caps */
    bool no_default_paths;    /* omit standard /usr, /lib, /bin system paths */
    bool verbose;             /* output step-by-step security actions */

    char **payload_argv;      /* target payload command and arguments */
    int payload_argc;
};

#endif /* SANDBOX_H */
