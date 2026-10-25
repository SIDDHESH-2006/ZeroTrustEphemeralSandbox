/* Include Seccomp BPF interface declarations and configurations */
#include "seccomp_bpf.h"

/* Linux kernel Seccomp, BPF, and audit headers */
#include <linux/seccomp.h>   /* Seccomp mode constants (SECCOMP_SET_MODE_FILTER, SECCOMP_RET_*) */
#include <linux/filter.h>    /* Classic BPF filter structures (sock_filter, sock_fprog, BPF_* macros) */
#include <linux/audit.h>     /* CPU architecture definitions (AUDIT_ARCH_X86_64, etc.) */
#include <sys/syscall.h>     /* Direct system call numbers (__NR_* / SYS_*) */
#include <sys/prctl.h>       /* Operations on process control states */
#include <stddef.h>          /* Standard definitions including offsetof macro */

/* Fallback constant for seccomp mode filter if missing in header */
#ifndef SECCOMP_SET_MODE_FILTER
#define SECCOMP_SET_MODE_FILTER 1
#endif

/* Architecture-specific audit identification constant to prevent 32/64-bit ABI confusion */
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

/* Fallback definitions for io_uring syscalls (Linux 5.1+) */
#ifndef SYS_io_uring_setup
#ifdef __NR_io_uring_setup
#define SYS_io_uring_setup __NR_io_uring_setup
#elif defined(__x86_64__) || defined(__aarch64__) || defined(__arm__) || defined(__i386__)
#define SYS_io_uring_setup 425
#endif
#endif

#ifndef SYS_io_uring_enter
#ifdef __NR_io_uring_enter
#define SYS_io_uring_enter __NR_io_uring_enter
#elif defined(__x86_64__) || defined(__aarch64__) || defined(__arm__) || defined(__i386__)
#define SYS_io_uring_enter 426
#endif
#endif

#ifndef SYS_io_uring_register
#ifdef __NR_io_uring_register
#define SYS_io_uring_register __NR_io_uring_register
#elif defined(__x86_64__) || defined(__aarch64__) || defined(__arm__) || defined(__i386__)
#define SYS_io_uring_register 427
#endif
#endif

/* Fallback definitions for cross-process memory inspection (Linux 3.2+) */
#ifndef SYS_process_vm_readv
#ifdef __NR_process_vm_readv
#define SYS_process_vm_readv __NR_process_vm_readv
#endif
#endif

#ifndef SYS_process_vm_writev
#ifdef __NR_process_vm_writev
#define SYS_process_vm_writev __NR_process_vm_writev
#endif
#endif

/* Fallback definitions for network-adjacent system calls */
#ifndef SYS_sendto
#ifdef __NR_sendto
#define SYS_sendto __NR_sendto
#endif
#endif

#ifndef SYS_recvfrom
#ifdef __NR_recvfrom
#define SYS_recvfrom __NR_recvfrom
#endif
#endif

#ifndef SYS_sendmsg
#ifdef __NR_sendmsg
#define SYS_sendmsg __NR_sendmsg
#endif
#endif

#ifndef SYS_recvmsg
#ifdef __NR_recvmsg
#define SYS_recvmsg __NR_recvmsg
#endif
#endif

#ifndef SYS_sendmmsg
#ifdef __NR_sendmmsg
#define SYS_sendmmsg __NR_sendmmsg
#endif
#endif

#ifndef SYS_recvmmsg
#ifdef __NR_recvmmsg
#define SYS_recvmmsg __NR_recvmmsg
#endif
#endif

#ifndef SYS_setsockopt
#ifdef __NR_setsockopt
#define SYS_setsockopt __NR_setsockopt
#endif
#endif

#ifndef SYS_getsockopt
#ifdef __NR_getsockopt
#define SYS_getsockopt __NR_getsockopt
#endif
#endif

#ifndef SYS_shutdown
#ifdef __NR_shutdown
#define SYS_shutdown __NR_shutdown
#endif
#endif

#ifndef SYS_socketcall
#ifdef __NR_socketcall
#define SYS_socketcall __NR_socketcall
#endif
#endif

int seccomp_apply_filter(const struct sandbox_config *config) {
    /* Maximum possible filter instructions needed (sized generously with bounds check) */
    struct sock_filter filter[256];
    size_t idx = 0;

    /*
     * Instruction Block 1: Architecture Validation
     * Load architecture identifier from seccomp_data struct into BPF accumulator.
     */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch));
    /* If accumulator equals expected architecture, jump 1 instruction (skip kill); else proceed to kill */
    filter[idx++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SECCOMP_AUDIT_ARCH, 1, 0);
    /* Kill process immediately if architecture does not match to prevent ABI bypass */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);

    /*
     * Instruction Block 2: System Call Number Retrieval
     * Load system call number (nr) from seccomp_data struct into BPF accumulator.
     */
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr));

    /*
     * Macro DENY_SYSCALL:
     * Compares accumulator against target syscall_num.
     * If matched (jt = 0): executes immediately following return instruction (SECCOMP_RET_ERRNO | EPERM).
     * If not matched (jf = 1): jumps over return instruction to continue testing the next syscall.
     */
    #define DENY_SYSCALL(syscall_num) do { \
        if (idx + 2 >= sizeof(filter) / sizeof(filter[0])) { \
            sb_log_error("Seccomp filter instruction limit exceeded"); \
            return -1; \
        } \
        filter[idx++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (syscall_num), 0, 1); \
        filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)); \
    } while (0)

    /* 3. Deny debugger inspection and process tracing */
    #ifdef SYS_ptrace
    DENY_SYSCALL(SYS_ptrace);
    #endif

    /* 4. Deny cross-process memory inspection and tampering (Task 5.2) */
    #ifdef SYS_process_vm_readv
    DENY_SYSCALL(SYS_process_vm_readv);
    #endif
    #ifdef SYS_process_vm_writev
    DENY_SYSCALL(SYS_process_vm_writev);
    #endif

    /* 5. Deny asynchronous io_uring subsystem (Task 5.1) */
    #ifdef SYS_io_uring_setup
    DENY_SYSCALL(SYS_io_uring_setup);
    #endif
    #ifdef SYS_io_uring_enter
    DENY_SYSCALL(SYS_io_uring_enter);
    #endif
    #ifdef SYS_io_uring_register
    DENY_SYSCALL(SYS_io_uring_register);
    #endif

    /* 6. Deny filesystem/namespace jail escapes and host system modifications */
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

    /* 7. Deny network socket and data transfer syscalls unless explicitly permitted (Task 5.3) */
    if (!config->allow_net) {
        #ifdef SYS_socket
        DENY_SYSCALL(SYS_socket);      /* Deny socket() creation */
        #endif

        #ifdef SYS_socketpair
        DENY_SYSCALL(SYS_socketpair);  /* Deny socketpair() creation */
        #endif

        #ifdef SYS_connect
        DENY_SYSCALL(SYS_connect);     /* Deny connect() to outbound remote hosts */
        #endif

        #ifdef SYS_bind
        DENY_SYSCALL(SYS_bind);        /* Deny bind() to local network ports */
        #endif

        #ifdef SYS_listen
        DENY_SYSCALL(SYS_listen);      /* Deny listen() for incoming connections */
        #endif

        #ifdef SYS_accept
        DENY_SYSCALL(SYS_accept);      /* Deny accept() of network connections */
        #endif

        #ifdef SYS_accept4
        DENY_SYSCALL(SYS_accept4);     /* Deny accept4() with flags */
        #endif
        #ifdef SYS_sendto
        DENY_SYSCALL(SYS_sendto);
        #endif
        #ifdef SYS_recvfrom
        DENY_SYSCALL(SYS_recvfrom);
        #endif
        #ifdef SYS_sendmsg
        DENY_SYSCALL(SYS_sendmsg);
        #endif
        #ifdef SYS_recvmsg
        DENY_SYSCALL(SYS_recvmsg);
        #endif
        #ifdef SYS_sendmmsg
        DENY_SYSCALL(SYS_sendmmsg);
        #endif
        #ifdef SYS_recvmmsg
        DENY_SYSCALL(SYS_recvmmsg);
        #endif
        #ifdef SYS_setsockopt
        DENY_SYSCALL(SYS_setsockopt);
        #endif
        #ifdef SYS_getsockopt
        DENY_SYSCALL(SYS_getsockopt);
        #endif
        #ifdef SYS_shutdown
        DENY_SYSCALL(SYS_shutdown);
        #endif
        #ifdef SYS_socketcall
        DENY_SYSCALL(SYS_socketcall);
        #endif
    }

    #undef DENY_SYSCALL

    /* 8. Allow all other authorized system calls */
    if (idx >= sizeof(filter) / sizeof(filter[0])) {
        sb_log_error("Seccomp filter instruction limit exceeded");
        return -1;
    }
    filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);

    /* Construct sock_fprog structure containing instruction count and filter buffer pointer */
    struct sock_fprog prog = {
        .len = (unsigned short)idx, /* Total count of assembled BPF instructions */
        .filter = filter,           /* Pointer to filter instruction array */
    };

    /* Install filter program using SYS_seccomp system call */
    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) != 0) {
        /* Fallback to prctl(PR_SET_SECCOMP) if SYS_seccomp returned non-zero */
        if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0) != 0) {
            /* Log error if both kernel injection mechanisms fail */
            sb_log_error("Failed to install Seccomp-BPF filter: %s", strerror(errno));
            /* Return failure */
            return -1;
        }
    }

    /* Print diagnostic confirmation in verbose mode */
    sb_log_debug(config->verbose, "Seccomp-BPF filter injected (%zu BPF instructions, net=%s)",
                 idx, config->allow_net ? "allowed" : "blocked");

    /* Return success */
    return 0;
}
