#ifndef SANDBOX_SECCOMP_BPF_H
#define SANDBOX_SECCOMP_BPF_H

#include "sandbox.h"

/*
 * Assembles and loads a classic Berkeley Packet Filter (BPF) array into the kernel
 * via SYS_seccomp or prctl(PR_SET_SECCOMP).
 *
 * Filters system calls without any third-party dependencies:
 * - Denies socket(), socketpair(), connect(), bind(), listen(), accept(), accept4() (unless allow_net)
 * - Denies ptrace()
 * - Denies chroot(), pivot_root(), kexec_load(), kexec_file_load()
 * - Returns SECCOMP_RET_ERRNO with EPERM for unauthorized calls.
 * - Allows all other standard system calls (SECCOMP_RET_ALLOW).
 */
int seccomp_apply_filter(const struct sandbox_config *config);

#endif /* SANDBOX_SECCOMP_BPF_H */
