#ifndef SANDBOX_SECCOMP_BPF_H
#define SANDBOX_SECCOMP_BPF_H

/* Include core sandbox structures and configurations */
#include "sandbox.h"

/*
 * seccomp_apply_filter:
 * Assembles and loads a classic Berkeley Packet Filter (BPF) program into the Linux kernel
 * using SYS_seccomp (or fallback to prctl(PR_SET_SECCOMP)).
 *
 * Filters system calls without any third-party dependencies:
 * - Denies io_uring operations: io_uring_setup, io_uring_enter, io_uring_register
 * - Denies cross-process memory inspection: process_vm_readv, process_vm_writev
 * - Denies system & container tampering: ptrace, chroot, pivot_root, kexec_load,
 *   kexec_file_load, mount, umount2, swapon, swapoff, reboot
 * - Denies network socket & data operations (unless allow_net):
 *   socket, socketpair, connect, bind, listen, accept, accept4,
 *   sendto, recvfrom, sendmsg, recvmsg, sendmmsg, recvmmsg,
 *   setsockopt, getsockopt, shutdown, socketcall
 * - Returns SECCOMP_RET_ERRNO with EPERM for unauthorized calls.
 * - Allows all other standard system calls (SECCOMP_RET_ALLOW).
 */
int seccomp_apply_filter(const struct sandbox_config *config);

#endif /* SANDBOX_SECCOMP_BPF_H */
