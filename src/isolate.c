/* Include isolation interface declarations and shared headers */
#include "isolate.h"

/* Linux system call and process control headers */
#include <sys/prctl.h>       /* Operations on process control states (PR_SET_NO_NEW_PRIVS, PR_CAP_AMBIENT) */
#include <sys/syscall.h>     /* Direct syscall interface (SYS_capset) */
#include <linux/capability.h>/* Linux capability data structures and version constants */
#include <sched.h>           /* Namespace manipulation flags (CLONE_NEWUSER, CLONE_NEWNET, unshare) */

/* Fallback constant for PR_SET_NO_NEW_PRIVS if missing in legacy libc headers */
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif

/* Fallback constants for ambient capability management if missing */
#ifndef PR_CAP_AMBIENT
#define PR_CAP_AMBIENT 47
#define PR_CAP_AMBIENT_CLEAR_ALL 4
#endif

/*
 * isolate_set_no_new_privs:
 * Enforces PR_SET_NO_NEW_PRIVS to prevent setuid/setgid execution and required for unprivileged Landlock.
 */
int isolate_set_no_new_privs(void) {
    /* Invoke prctl system call with PR_SET_NO_NEW_PRIVS set to 1 */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        /* Log error if the kernel refuses the prctl call */
        sb_log_error("Failed to set PR_SET_NO_NEW_PRIVS: %s", strerror(errno));
        /* Return failure code */
        return -1;
    }
    /* Return success code */
    return 0;
}

/*
 * isolate_drop_capabilities:
 * Strips Linux ambient, permitted, effective, and inheritable capabilities.
 */
int isolate_drop_capabilities(void) {
    /* Step 1: Clear all ambient capabilities */
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) != 0) {
        /* If the kernel doesn't support ambient capabilities, EINVAL is non-fatal */
        if (errno != EINVAL) {
            /* Emit warning if ambient capabilities clearing failed */
            sb_log_warn("Failed to clear ambient capabilities: %s", strerror(errno));
        }
    }

    /* Step 2: Clear effective, permitted, and inheritable capabilities via raw capset */
    struct __user_cap_header_struct hdr;
    /* Allocate capability data blocks for 64-bit capabilities (V3) */
    struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];

    /* Zero-initialize the capability header structure */
    memset(&hdr, 0, sizeof(hdr));
    /* Zero-initialize all capability data entries (effective, permitted, inheritable masks = 0) */
    memset(data, 0, sizeof(data));

    /* Specify capability API version 3 for 64-bit capability bitmasks */
    hdr.version = _LINUX_CAPABILITY_VERSION_3;
    /* Target PID 0 indicates the calling process */
    hdr.pid = 0;

    /* Execute the capset system call with cleared masks */
    if (syscall(SYS_capset, &hdr, data) != 0) {
        /* If running unprivileged, capset may return harmless errors if caps already 0 */
        sb_log_debug(true, "Capset drop returned: %s", strerror(errno));
    }

    /* Return success */
    return 0;
}

/*
 * isolate_namespaces:
 * Disconnects the calling process from host network and user namespaces via unshare().
 */
int isolate_namespaces(bool unshare_net, bool verbose) {
    /* If network unshare was not requested by the user, return immediately */
    if (!unshare_net) {
        /* Return success */
        return 0;
    }

    /*
     * In an unprivileged context without root, unsharing CLONE_NEWNET requires CLONE_NEWUSER.
     * Attempt to decouple both user and network namespaces simultaneously.
     */
    if (unshare(CLONE_NEWUSER | CLONE_NEWNET) != 0) {
        /* Log debug notice if compound unshare failed */
        sb_log_debug(verbose, "Unshare user+network namespace failed (%s), trying network only", strerror(errno));
        /* Attempt fallback to network-only unshare (succeeds if running with CAP_SYS_ADMIN) */
        if (unshare(CLONE_NEWNET) != 0) {
            /* Emit warning and advise that Seccomp socket filter remains active */
            sb_log_warn("Unable to unshare network namespace (%s). Falling back to Seccomp socket filter.", strerror(errno));
            /* Return 0 to allow execution to proceed safely under Seccomp enforcement */
            return 0;
        }
    }

    /* Log success when network namespace decoupling succeeds */
    sb_log_debug(verbose, "Network namespace isolated successfully");
    /* Return success */
    return 0;
}
