#ifndef SANDBOX_LANDLOCK_MGR_H
#define SANDBOX_LANDLOCK_MGR_H

/* Include core types and configuration structures */
#include "sandbox.h"

/* Linux kernel Landlock header providing structs and access flag definitions */
#include <linux/landlock.h>

/* Fallback Landlock system call numbers for x86_64 if not defined in older C libraries */
#ifndef SYS_landlock_create_ruleset
#define SYS_landlock_create_ruleset 444  /* Syscall number to instantiate a new Landlock ruleset */
#endif

#ifndef SYS_landlock_add_rule
#define SYS_landlock_add_rule 445        /* Syscall number to add an access rule to an existing ruleset */
#endif

#ifndef SYS_landlock_restrict_self
#define SYS_landlock_restrict_self 446   /* Syscall number to enforce ruleset restrictions on self */
#endif

/* Fallback bitmask access flags for Landlock ABI version 1 */
#ifndef LANDLOCK_ACCESS_FS_EXECUTE
#define LANDLOCK_ACCESS_FS_EXECUTE            (1ULL << 0)   /* Execute regular files */
#endif

#ifndef LANDLOCK_ACCESS_FS_WRITE_FILE
#define LANDLOCK_ACCESS_FS_WRITE_FILE         (1ULL << 1)   /* Write / overwrite existing regular files */
#endif

#ifndef LANDLOCK_ACCESS_FS_READ_FILE
#define LANDLOCK_ACCESS_FS_READ_FILE          (1ULL << 2)   /* Read content of regular files */
#endif

#ifndef LANDLOCK_ACCESS_FS_READ_DIR
#define LANDLOCK_ACCESS_FS_READ_DIR           (1ULL << 3)   /* Read directory listings / iterate entries */
#endif

#ifndef LANDLOCK_ACCESS_FS_REMOVE_DIR
#define LANDLOCK_ACCESS_FS_REMOVE_DIR         (1ULL << 4)   /* Delete an empty directory (rmdir) */
#endif

#ifndef LANDLOCK_ACCESS_FS_REMOVE_FILE
#define LANDLOCK_ACCESS_FS_REMOVE_FILE        (1ULL << 5)   /* Unlink / delete regular files */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_CHAR
#define LANDLOCK_ACCESS_FS_MAKE_CHAR          (1ULL << 6)   /* Create character special devices */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_DIR
#define LANDLOCK_ACCESS_FS_MAKE_DIR           (1ULL << 7)   /* Create new directories (mkdir) */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_REG
#define LANDLOCK_ACCESS_FS_MAKE_REG           (1ULL << 8)   /* Create new regular files (open with O_CREAT) */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_SOCK
#define LANDLOCK_ACCESS_FS_MAKE_SOCK          (1ULL << 9)   /* Create UNIX domain sockets on filesystem */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_FIFO
#define LANDLOCK_ACCESS_FS_MAKE_FIFO          (1ULL << 10)  /* Create named pipes / FIFOs (mkfifo) */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_BLOCK
#define LANDLOCK_ACCESS_FS_MAKE_BLOCK         (1ULL << 11)  /* Create block special devices */
#endif

#ifndef LANDLOCK_ACCESS_FS_MAKE_SYM
#define LANDLOCK_ACCESS_FS_MAKE_SYM           (1ULL << 12)  /* Create symbolic links (symlink) */
#endif

/* Fallback access flags introduced in Landlock ABI version 2 */
#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER              (1ULL << 13)  /* Link or rename files between directories */
#endif

/* Fallback access flags introduced in Landlock ABI version 3 */
#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE           (1ULL << 14)  /* Truncate regular files (truncate, ftruncate) */
#endif

/*
 * landlock_get_abi_version:
 * Queries the Linux kernel to determine the supported Landlock ABI version.
 * @return Supported ABI version (e.g., 1, 2, 3), or negative errno on error.
 */
int landlock_get_abi_version(void);

/*
 * landlock_apply_sandbox:
 * Creates the Landlock ruleset, populates it with all RO and RW allowlisted paths,
 * and restricts the calling process permanently.
 *
 * @param config: Pointer to the sandbox configuration containing allowlisted paths.
 * @return 0 on success, or -1 on failure.
 */
int landlock_apply_sandbox(const struct sandbox_config *config);

#endif /* SANDBOX_LANDLOCK_MGR_H */
