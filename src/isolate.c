#include "isolate.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/capability.h>
#include <sched.h>

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif

#ifndef PR_CAP_AMBIENT
#define PR_CAP_AMBIENT 47
#define PR_CAP_AMBIENT_CLEAR_ALL 4
#endif

int isolate_set_no_new_privs(void) {
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        sb_log_error("Failed to set PR_SET_NO_NEW_PRIVS: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int isolate_drop_capabilities(void) {
    /* 1. Clear ambient capabilities */
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) != 0) {
        /* If PR_CAP_AMBIENT is not supported by kernel, continue */
        if (errno != EINVAL) {
            sb_log_warn("Failed to clear ambient capabilities: %s", strerror(errno));
        }
    }

    /* 2. Clear effective, permitted, and inheritable capabilities via raw capset */
    struct __user_cap_header_struct hdr;
    struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];

    memset(&hdr, 0, sizeof(hdr));
    memset(data, 0, sizeof(data));

    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    hdr.pid = 0; /* current thread */

    if (syscall(SYS_capset, &hdr, data) != 0) {
        /* Not fatal if unprivileged user is already non-root, but report warning */
        sb_log_debug(true, "Capset drop returned: %s", strerror(errno));
    }

    return 0;
}

int isolate_namespaces(bool unshare_net, bool verbose) {
    if (!unshare_net) {
        return 0;
    }

    /*
     * Attempt to unshare network namespace.
     * In unprivileged context, CLONE_NEWNET requires CLONE_NEWUSER.
     */
    if (unshare(CLONE_NEWUSER | CLONE_NEWNET) != 0) {
        sb_log_debug(verbose, "Unshare user+network namespace failed (%s), trying network only", strerror(errno));
        if (unshare(CLONE_NEWNET) != 0) {
            sb_log_warn("Unable to unshare network namespace (%s). Falling back to Seccomp socket filter.", strerror(errno));
            return 0;
        }
    }

    sb_log_debug(verbose, "Network namespace isolated successfully");
    return 0;
}
