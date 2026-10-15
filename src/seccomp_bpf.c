#include "seccomp_bpf.h"
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <stddef.h>

#ifndef SECCOMP_SET_MODE_FILTER
#define SECCOMP_SET_MODE_FILTER 1
#endif

#if defined(__x86_64__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_X86_64
#elif defined(__i386__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_I386
#elif defined(__aarch64__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_AARCH64
#elif defined(__arm__)
#define SECCOMP_AUDIT_ARCH AUDIT_ARCH_ARM
#else
#error "Host architecture is not supported by Seccomp-BPF filter"
#endif

int seccomp_apply_filter(const struct sandbox_config *config) {
    /* Maximum possible filter instructions needed */
    struct sock_filter filter[128];
    size_t idx = 0;

    /* 1. Verify system architecture to prevent 32/64-bit ABI confusion */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch));
    filter[idx++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SECCOMP_AUDIT_ARCH, 1, 0);
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);

    /* 2. Load system call number into accumulator */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr));

    /* Helper macro: if syscall == nr, return EPERM; else skip return instruction */
    #define DENY_SYSCALL(syscall_num) do { \
        filter[idx++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (syscall_num), 0, 1); \
        filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)); \
    } while (0)

    /* 3. Always deny dangerous system calls */
    #ifdef SYS_ptrace
    DENY_SYSCALL(SYS_ptrace);
    #endif
    #ifdef SYS_chroot
    DENY_SYSCALL(SYS_chroot);
    #endif
    #ifdef SYS_pivot_root
    DENY_SYSCALL(SYS_pivot_root);
    #endif
    #ifdef SYS_kexec_load
    DENY_SYSCALL(SYS_kexec_load);
    #endif
    #ifdef SYS_kexec_file_load
    DENY_SYSCALL(SYS_kexec_file_load);
    #endif
    #ifdef SYS_mount
    DENY_SYSCALL(SYS_mount);
    #endif
    #ifdef SYS_umount2
    DENY_SYSCALL(SYS_umount2);
    #endif
    #ifdef SYS_swapon
    DENY_SYSCALL(SYS_swapon);
    #endif
    #ifdef SYS_swapoff
    DENY_SYSCALL(SYS_swapoff);
    #endif
    #ifdef SYS_reboot
    DENY_SYSCALL(SYS_reboot);
    #endif

    /* 4. Deny network socket system calls unless explicitly permitted */
    if (!config->allow_net) {
        #ifdef SYS_socket
        DENY_SYSCALL(SYS_socket);
        #endif
        #ifdef SYS_socketpair
        DENY_SYSCALL(SYS_socketpair);
        #endif
        #ifdef SYS_connect
        DENY_SYSCALL(SYS_connect);
        #endif
        #ifdef SYS_bind
        DENY_SYSCALL(SYS_bind);
        #endif
        #ifdef SYS_listen
        DENY_SYSCALL(SYS_listen);
        #endif
        #ifdef SYS_accept
        DENY_SYSCALL(SYS_accept);
        #endif
        #ifdef SYS_accept4
        DENY_SYSCALL(SYS_accept4);
        #endif
    }

    #undef DENY_SYSCALL

    /* 5. Allow all other authorized system calls */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);

    struct sock_fprog prog = {
        .len = (unsigned short)idx,
        .filter = filter,
    };

    /* Apply BPF filter via SYS_seccomp or prctl fallback */
    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) != 0) {
        if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0) != 0) {
            sb_log_error("Failed to install Seccomp-BPF filter: %s", strerror(errno));
            return -1;
        }
    }

    sb_log_debug(config->verbose, "Seccomp-BPF filter injected (%zu BPF instructions, net=%s)",
                 idx, config->allow_net ? "allowed" : "blocked");
    return 0;
}
