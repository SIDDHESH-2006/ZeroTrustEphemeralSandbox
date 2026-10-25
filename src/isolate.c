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
        /* If PR_CAP_AMBIENT is not supported by older kernels, continue */
        if (errno != EINVAL && errno != EOPNOTSUPP) {
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
        int capset_err = errno;
        /*
         * Verify whether capabilities are actually held.
         * If capset() failed but the process already has no effective or permitted
         * capabilities, then privileges are not retained (e.g. unprivileged user).
         * If any prohibited capability is retained, fail safely.
         */
        struct __user_cap_data_struct cur_data[_LINUX_CAPABILITY_U32S_3];
        memset(cur_data, 0, sizeof(cur_data));
        if (syscall(SYS_capget, &hdr, cur_data) == 0) {
            bool retains_caps = false;
            for (int i = 0; i < _LINUX_CAPABILITY_U32S_3; i++) {
                if (cur_data[i].effective != 0 || cur_data[i].permitted != 0) {
                    retains_caps = true;
                    break;
                }
            }
            if (retains_caps) {
                sb_log_error("Failed to drop capabilities and process retains privileges: %s", strerror(capset_err));
                return -1;
            }
            sb_log_debug(true, "Capset returned %s but process has no capabilities (safe)", strerror(capset_err));
        } else {
            sb_log_error("Capset failed (%s) and capget verification failed: %s",
                         strerror(capset_err), strerror(errno));
            return -1;
        }
    } else {
        /* Verify that effective capabilities are indeed zero */
        struct __user_cap_data_struct check_data[_LINUX_CAPABILITY_U32S_3];
        memset(check_data, 0, sizeof(check_data));
        if (syscall(SYS_capget, &hdr, check_data) == 0) {
            for (int i = 0; i < _LINUX_CAPABILITY_U32S_3; i++) {
                if (check_data[i].effective != 0) {
                    sb_log_error("Failed to clear effective capabilities");
                    return -1;
                }
            }
        }
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
            sb_log_error("Failed to unshare network namespace: %s", strerror(errno));
            return -1; /* Fail safely: requested namespace operation could not be established */
        }
    }

    /* Log success when network namespace decoupling succeeds */
    sb_log_debug(verbose, "Network namespace isolated successfully");
    /* Return success */
    return 0;
}
