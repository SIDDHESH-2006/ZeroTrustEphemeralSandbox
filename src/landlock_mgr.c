/* Include Landlock manager header containing declarations and fallback syscall IDs */
#include "landlock_mgr.h"

/* Standard Linux file and system call headers */
#include <fcntl.h>       /* File control options (open, O_PATH, O_CLOEXEC) */
#include <sys/syscall.h> /* Direct syscall invocation function */
#include <sys/stat.h>    /* File status flags and macros (fstat, S_ISDIR) */

/*
 * landlock_get_abi_version:
 * Invokes SYS_landlock_create_ruleset with LANDLOCK_CREATE_RULESET_VERSION to query kernel support.
 */
int landlock_get_abi_version(void) {
    /* Landlock ABI version query system call */
    return syscall(SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
}

/*
 * landlock_apply_sandbox:
 * Instantiates the Landlock ruleset, adds allowlist rules for all registered paths,
 * and permanently locks the calling process within the restricted sandbox.
 */
int landlock_apply_sandbox(const struct sandbox_config *config) {
    /* Query the active kernel's Landlock ABI version */
    int abi = landlock_get_abi_version();

    /* Check if the query returned an error */
    if (abi < 0) {
        /* Check if Landlock is simply not compiled or enabled in this kernel */
        if (errno == ENOSYS || errno == EOPNOTSUPP) {
            /* Warn user that Landlock is unavailable, but Seccomp will still protect */
            sb_log_warn("Landlock LSM is not supported or not enabled in this kernel (errno=%d)", errno);
            /* Continue safely */
            return 0;
        }
        /* Report fatal error if ABI query failed unexpectedly */
        sb_log_error("Landlock ABI query failed: %s", strerror(errno));
        /* Return failure */
        return -1;
    }

    /* Print diagnostic ABI version message in verbose mode */
    sb_log_debug(config->verbose, "Detected Landlock LSM ABI version: %d", abi);

    /* Construct base read-only access flags: execute binary, read files, list directory contents */
    uint64_t ro_access = LANDLOCK_ACCESS_FS_EXECUTE |
                         LANDLOCK_ACCESS_FS_READ_FILE |
                         LANDLOCK_ACCESS_FS_READ_DIR;

    /* Construct read-write access flags: RO flags plus write, delete, and file/directory creation */
    uint64_t rw_access = ro_access |
                         LANDLOCK_ACCESS_FS_WRITE_FILE |
                         LANDLOCK_ACCESS_FS_REMOVE_DIR |
                         LANDLOCK_ACCESS_FS_REMOVE_FILE |
                         LANDLOCK_ACCESS_FS_MAKE_CHAR |
                         LANDLOCK_ACCESS_FS_MAKE_DIR |
                         LANDLOCK_ACCESS_FS_MAKE_REG |
                         LANDLOCK_ACCESS_FS_MAKE_SOCK |
                         LANDLOCK_ACCESS_FS_MAKE_FIFO |
                         LANDLOCK_ACCESS_FS_MAKE_BLOCK |
                         LANDLOCK_ACCESS_FS_MAKE_SYM;

    /* Landlock ABI version 2 introduced the REFER right for linking/renaming across directories */
    if (abi >= 2) {
        /* Enable REFER flag if ABI version is 2 or higher */
        rw_access |= LANDLOCK_ACCESS_FS_REFER;
    }

    /* Landlock ABI version 3 introduced the TRUNCATE right for truncating files */
    if (abi >= 3) {
        /* Enable TRUNCATE flag if ABI version is 3 or higher */
        rw_access |= LANDLOCK_ACCESS_FS_TRUNCATE;
    }

    /* Prepare ruleset attribute structure defining handled filesystem access rights */
    struct landlock_ruleset_attr ruleset_attr = {
        .handled_access_fs = rw_access, /* Declare all possible access rights this ruleset controls */
    };

    /* Invoke SYS_landlock_create_ruleset to obtain a new ruleset file descriptor */
    int ruleset_fd = syscall(SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);
    /* Validate that ruleset file descriptor was created successfully */
    if (ruleset_fd < 0) {
        /* Log error if ruleset creation failed */
        sb_log_error("Failed to create Landlock ruleset: %s", strerror(errno));
        /* Return failure */
        return -1;
    }

    /* Iterate through all registered path rules to add them to the ruleset */
    for (size_t i = 0; i < config->rule_count; i++) {
        /* Extract current target path string */
        const char *path = config->rules[i].path;
        /* Extract access mode (ACCESS_RO or ACCESS_RW) */
        path_access_t access_type = config->rules[i].access;

        /* Open path with O_PATH to obtain a lightweight file descriptor without read/write overhead */
        int fd = open(path, O_PATH | O_CLOEXEC);
        /* Check if opening the path failed (e.g., path does not exist on this machine) */
        if (fd < 0) {
            /* Skip non-existent path and log debug notice */
            sb_log_debug(config->verbose, "Skipping non-existent or inaccessible path: %s (%s)", path, strerror(errno));
            /* Continue to next rule */
            continue;
        }

        /* Retrieve file metadata to determine if path is a directory or regular file */
        struct stat st;
        /* Execute fstat on opened file descriptor */
        if (fstat(fd, &st) != 0) {
            /* Log warning if stat fails */
            sb_log_warn("Failed to stat path '%s': %s", path, strerror(errno));
            /* Close file descriptor */
            close(fd);
            /* Continue to next rule */
            continue;
        }

        /* Calculate allowed access rights for this specific rule */
        uint64_t allowed;
        /* Check if target is a directory */
        if (S_ISDIR(st.st_mode)) {
            /* Assign full directory access rights based on RO or RW designation */
            allowed = (access_type == ACCESS_RW) ? rw_access : ro_access;
        } else {
            /* For non-directory regular files, directory-only rights cause EINVAL; mask file rights only */
            uint64_t file_ro = LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_READ_FILE;
            /* Build file read-write rights */
            uint64_t file_rw = file_ro | LANDLOCK_ACCESS_FS_WRITE_FILE;
            /* Append TRUNCATE if ABI 3 is supported */
            if (abi >= 3) {
                file_rw |= LANDLOCK_ACCESS_FS_TRUNCATE;
            }
            /* Set allowed rights for regular file */
            allowed = (access_type == ACCESS_RW) ? file_rw : file_ro;
        }

        /* Populate Landlock path beneath attribute with the file descriptor and allowed rights */
        struct landlock_path_beneath_attr path_attr = {
            .allowed_access = allowed, /* Permitted access bitmask */
            .parent_fd = fd,           /* File descriptor pointing to the allowlisted anchor path */
        };

        /* Call SYS_landlock_add_rule to register this allowlist path with the ruleset */
        if (syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &path_attr, 0) != 0) {
            /* Log warning if adding the rule fails */
            sb_log_warn("Failed to add Landlock rule for '%s': %s", path, strerror(errno));
        } else {
            /* Log successful rule addition in debug mode */
            sb_log_debug(config->verbose, "Allowed %s: %s",
                         (access_type == ACCESS_RW) ? "RW" : "RO", path);
        }

        /* Close the directory/file anchor descriptor once rule is committed */
        close(fd);
    }

    /* Enforce the Landlock ruleset on the calling process and all its future child processes */
    if (syscall(SYS_landlock_restrict_self, ruleset_fd, 0) != 0) {
        /* Log error if self-restriction fails */
        sb_log_error("Failed to enforce Landlock restrictions: %s", strerror(errno));
        /* Close ruleset descriptor */
        close(ruleset_fd);
        /* Return failure */
        return -1;
    }

    /* Close ruleset descriptor as restriction is permanently locked in the kernel */
    close(ruleset_fd);
    /* Emit debug notice that Landlock is fully operational */
    sb_log_debug(config->verbose, "Landlock LSM containment barrier active");
    /* Return success */
    return 0;
}
