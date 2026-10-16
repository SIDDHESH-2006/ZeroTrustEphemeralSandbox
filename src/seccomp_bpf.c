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

/*
 * seccomp_apply_filter:
 * Assembles a raw classic BPF program to filter unauthorized system calls and loads it into the kernel.
 */
int seccomp_apply_filter(const struct sandbox_config *config) {
    /* Array holding raw BPF instructions */
    struct sock_filter filter[128];
    /* Index tracking current instruction position in filter array */
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
        filter[idx++] = (struct sock_filter)BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (syscall_num), 0, 1); \
        filter[idx++] = (struct sock_filter)BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)); \
    } while (0)

    /*
     * Instruction Block 3: Dangerous Syscall Denials
     * Block process tracing, debugging, and escape mechanisms.
     */
    #ifdef SYS_ptrace
    DENY_SYSCALL(SYS_ptrace);          /* Deny ptrace() to block inspection or debugger attachment */
    #endif

    #ifdef SYS_chroot
    DENY_SYSCALL(SYS_chroot);          /* Deny chroot() to prevent chroot escape attempts */
    #endif

    #ifdef SYS_pivot_root
    DENY_SYSCALL(SYS_pivot_root);      /* Deny pivot_root() */
    #endif

    #ifdef SYS_kexec_load
    DENY_SYSCALL(SYS_kexec_load);      /* Deny loading new kernel images */
    #endif

    #ifdef SYS_kexec_file_load
    DENY_SYSCALL(SYS_kexec_file_load); /* Deny file-based kernel execution */
    #endif

    #ifdef SYS_mount
    DENY_SYSCALL(SYS_mount);           /* Deny mounting filesystems */
    #endif

    #ifdef SYS_umount2
    DENY_SYSCALL(SYS_umount2);         /* Deny unmounting filesystems */
    #endif

    #ifdef SYS_swapon
    DENY_SYSCALL(SYS_swapon);          /* Deny manipulating swap memory devices */
    #endif

    #ifdef SYS_swapoff
    DENY_SYSCALL(SYS_swapoff);         /* Deny disabling swap memory devices */
    #endif

    #ifdef SYS_reboot
    DENY_SYSCALL(SYS_reboot);          /* Deny triggering machine reboot or shutdown */
    #endif

    /*
     * Instruction Block 4: Network Socket Denials
     * If allow_net is false, deny all network socket creation and connection calls.
     */
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
    }

    #undef DENY_SYSCALL

    /*
     * Instruction Block 5: Default Action
     * Permit all other standard system calls necessary for valid application execution.
     */
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
