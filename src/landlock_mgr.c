#include "landlock_mgr.h"
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>

int landlock_get_abi_version(void) {
    return syscall(SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
}

int landlock_apply_sandbox(const struct sandbox_config *config) {
    int abi = landlock_get_abi_version();
    if (abi < 0) {
        if (errno == ENOSYS || errno == EOPNOTSUPP) {
            sb_log_warn("Landlock LSM is not supported or not enabled in this kernel (errno=%d)", errno);
            return 0; /* Continue with Seccomp protection */
        }
        sb_log_error("Landlock ABI query failed: %s", strerror(errno));
        return -1;
    }

    sb_log_debug(config->verbose, "Detected Landlock LSM ABI version: %d", abi);

    /* Construct handled filesystem access rights based on detected ABI */
    uint64_t ro_access = LANDLOCK_ACCESS_FS_EXECUTE |
                         LANDLOCK_ACCESS_FS_READ_FILE |
                         LANDLOCK_ACCESS_FS_READ_DIR;

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

    if (abi >= 2) {
        rw_access |= LANDLOCK_ACCESS_FS_REFER;
    }
    if (abi >= 3) {
        rw_access |= LANDLOCK_ACCESS_FS_TRUNCATE;
    }

    struct landlock_ruleset_attr ruleset_attr = {
        .handled_access_fs = rw_access,
    };

    int ruleset_fd = syscall(SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);
    if (ruleset_fd < 0) {
        sb_log_error("Failed to create Landlock ruleset: %s", strerror(errno));
        return -1;
    }

    /* Populate rules for all allowlisted paths */
    for (size_t i = 0; i < config->rule_count; i++) {
        const char *path = config->rules[i].path;
        path_access_t access_type = config->rules[i].access;

        int fd = open(path, O_PATH | O_CLOEXEC);
        if (fd < 0) {
            sb_log_debug(config->verbose, "Skipping non-existent or inaccessible path: %s (%s)", path, strerror(errno));
            continue;
        }

        struct stat st;
        if (fstat(fd, &st) != 0) {
            sb_log_warn("Failed to stat path '%s': %s", path, strerror(errno));
            close(fd);
            continue;
        }

        uint64_t allowed;
        if (S_ISDIR(st.st_mode)) {
            allowed = (access_type == ACCESS_RW) ? rw_access : ro_access;
        } else {
            /* For non-directory files, Landlock only permits file-specific access rights */
            uint64_t file_ro = LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_READ_FILE;
            uint64_t file_rw = file_ro | LANDLOCK_ACCESS_FS_WRITE_FILE;
            if (abi >= 3) {
                file_rw |= LANDLOCK_ACCESS_FS_TRUNCATE;
            }
            allowed = (access_type == ACCESS_RW) ? file_rw : file_ro;
        }

        struct landlock_path_beneath_attr path_attr = {
            .allowed_access = allowed,
            .parent_fd = fd,
        };

        if (syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &path_attr, 0) != 0) {
            sb_log_warn("Failed to add Landlock rule for '%s': %s", path, strerror(errno));
        } else {
            sb_log_debug(config->verbose, "Allowed %s: %s",
                         (access_type == ACCESS_RW) ? "RW" : "RO", path);
        }

        close(fd);
    }

    /* Enforce Landlock restriction on calling process and all future children */
    if (syscall(SYS_landlock_restrict_self, ruleset_fd, 0) != 0) {
        sb_log_error("Failed to enforce Landlock restrictions: %s", strerror(errno));
        close(ruleset_fd);
        return -1;
    }

    close(ruleset_fd);
    sb_log_debug(config->verbose, "Landlock LSM containment barrier active");
    return 0;
}
