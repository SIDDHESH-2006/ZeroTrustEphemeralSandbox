#ifndef SANDBOX_ISOLATE_H
#define SANDBOX_ISOLATE_H

#include "sandbox.h"

/*
 * Enforces PR_SET_NO_NEW_PRIVS to prevent privilege escalation via setuid
 * and required by Landlock and unprivileged Seccomp.
 */
int isolate_set_no_new_privs(void);

/*
 * Clears ambient capabilities and zeroes inheritable/permitted capability sets.
 */
int isolate_drop_capabilities(void);

/*
 * Decouples user and network namespaces via unshare().
 */
int isolate_namespaces(bool unshare_net, bool verbose);

#endif /* SANDBOX_ISOLATE_H */
