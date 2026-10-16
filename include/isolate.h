#ifndef SANDBOX_ISOLATE_H
#define SANDBOX_ISOLATE_H

/* Include core definitions, error logging macros, and configuration structs */
#include "sandbox.h"

/*
 * isolate_set_no_new_privs:
 * Locks the process execution state via prctl(PR_SET_NO_NEW_PRIVS, 1).
 * Prevents the process and any descendants from acquiring elevated privileges
 * through setuid/setgid binaries or filesystem capabilities.
 * This is a mandatory kernel requirement before loading unprivileged Landlock rulesets
 * or Seccomp-BPF filters.
 *
 * @return 0 on success, or -1 on system call failure.
 */
int isolate_set_no_new_privs(void);

/*
 * isolate_drop_capabilities:
 * Strips Linux capabilities to enforce zero-trust execution.
 * 1. Invokes prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL) to clear ambient capabilities.
 * 2. Invokes raw capset() system call to zero out permitted, effective, and inheritable masks.
 *
 * @return 0 on success, or -1 on failure.
 */
int isolate_drop_capabilities(void);

/*
 * isolate_namespaces:
 * Optionally unshares user and network namespaces using unshare(CLONE_NEWUSER | CLONE_NEWNET).
 * Allows network isolation at the namespace level in addition to Seccomp socket blocking.
 *
 * @param unshare_net: Boolean flag indicating if network namespace should be decoupled.
 * @param verbose: Boolean flag to enable diagnostic debug output.
 * @return 0 on success, or -1 on critical failure.
 */
int isolate_namespaces(bool unshare_net, bool verbose);

#endif /* SANDBOX_ISOLATE_H */
