#ifndef SANDBOX_SECCOMP_BPF_H
#define SANDBOX_SECCOMP_BPF_H

/* Include core sandbox structures and configurations */
#include "sandbox.h"

/*
 * seccomp_apply_filter:
 * Assembles and loads a classic Berkeley Packet Filter (BPF) program into the Linux kernel
 * using SYS_seccomp (or fallback to prctl(PR_SET_SECCOMP)).
 *
 * This implementation enforces strict zero-dependency containment:
 * 1. Validates host CPU architecture (AUDIT_ARCH_X86_64) to prevent ABI evasion.
 * 2. Denies dangerous system calls (ptrace, chroot, pivot_root, kexec_load, reboot, mount).
 * 3. Denies networking syscalls (socket, connect, bind, listen, accept) unless config->allow_net is true.
 * 4. Returns SECCOMP_RET_ERRNO with EPERM for all denied operations.
 * 5. Permits all other legitimate application system calls with SECCOMP_RET_ALLOW.
 *
 * @param config: Pointer to sandbox_config indicating network policies.
 * @return 0 on success, or -1 on error.
 */
int seccomp_apply_filter(const struct sandbox_config *config);

#endif /* SANDBOX_SECCOMP_BPF_H */
