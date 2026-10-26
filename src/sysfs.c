#include "sysfs.h"
#include "log.h"

int sysfs_read_str(const char *path, char *out_buf, size_t max_len) {
    if (!path || path[0] == '\0' || !out_buf || max_len == 0) return 0;
    out_buf[0] = '\0';

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    ssize_t n = read(fd, out_buf, max_len - 1);
    close(fd);
    if (n <= 0) return 0;
    out_buf[n] = '\0';

    size_t len = strlen(out_buf);
    while (len > 0 && (out_buf[len - 1] == '\r' || out_buf[len - 1] == '\n' || out_buf[len - 1] == ' ')) {
        out_buf[--len] = '\0';
    }
    return 1;
}

int sysfs_read_str_fallback(const char *const paths[], char *out_buf, size_t max_len) {
    if (!paths || !out_buf || max_len == 0) return 0;
    for (int i = 0; paths[i]; i++) {
        if (paths[i][0] != '\0' && sysfs_read_str(paths[i], out_buf, max_len)) return 1;
    }
    return 0;
}

int sysfs_read_int(const char *path) {
    char buf[32];
    if (!sysfs_read_str(path, buf, sizeof(buf))) return 0;
    return (int)strtol(buf, NULL, 10);
}

/* Per-path error log throttling. A tuning daemon that writes ~60 nodes on every
 * transition must not flood the log when a ROM lacks half of them. */
#define SYSFS_ERR_SLOTS 16
static struct {
    char path[160];
    time_t last_err;
} s_err_tbl[SYSFS_ERR_SLOTS];

static int sysfs_should_log_err(const char *path) {
    if (!path || path[0] == '\0') return 0;
    time_t now = time(NULL);
    unsigned h = 0;
    for (const char *p = path; *p; p++) h = h * 31u + (unsigned char)*p;
    int slot = (int)(h & (SYSFS_ERR_SLOTS - 1));

    if (s_err_tbl[slot].path[0] != '\0' &&
        strcmp(s_err_tbl[slot].path, path) == 0 &&
        now - s_err_tbl[slot].last_err < 60) {
        return 0;
    }

    snprintf(s_err_tbl[slot].path, sizeof(s_err_tbl[slot].path), "%s", path);
    s_err_tbl[slot].last_err = now;
    return 1;
}

static void trim_value(char *dst, size_t cap, const char *val) {
    snprintf(dst, cap, "%s", val);
    size_t len = strlen(dst);
    while (len > 0 && (dst[len - 1] == '\r' || dst[len - 1] == '\n' || dst[len - 1] == ' ')) {
        dst[--len] = '\0';
    }
}

/* Write one node. Returns 1 when the value is already set, the write landed,
 * or the node does not exist (nothing to do); 0 when the node exists but the
 * write failed. Callers that walk candidate lists use 0 to try the next path,
 * which is what makes a v1 path that exists-but-read-only fall through to the
 * v2 path instead of silently leaving the old value behind. */
static int write_one(const char *path, const char *val) {
    if (!path || path[0] == '\0') return 0;

    char clean[64];
    trim_value(clean, sizeof(clean), val);

    char current[64];
    if (sysfs_read_str(path, current, sizeof(current)) && strcmp(current, clean) == 0) {
        return 1;
    }

    int fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0 && errno == EACCES) {
        /* Widen briefly, then put the original mode straight back while the fd
         * is already held — the writable window is one open() call. */
        struct stat st;
        int have_saved = (stat(path, &st) == 0);
        chmod(path, 0666);
        fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if (have_saved) chmod(path, st.st_mode & 0777);
    }
    if (fd < 0) {
        if (errno == ENOENT) return 1; /* node absent: not an error for this target */
        if (sysfs_should_log_err(path)) {
            log_info("Kernel", "Cannot open sysfs node (errno=%d): %s", errno, path);
        }
        return 0;
    }

    ssize_t written = write(fd, clean, strlen(clean));
    int saved_errno = errno;
    close(fd);

    if (written < 0) {
        if (sysfs_should_log_err(path)) {
            log_info("Kernel", "Write rejected by kernel (errno=%d, value='%s'): %s",
                     saved_errno, clean, path);
        }
        return 0;
    }
    return 1;
}

void sysfs_write(const char *path, const char *val) {
    if (!val) return;
    (void)write_one(path, val);
}

void sysfs_write_fallback(const char *const paths[], const char *val) {
    if (!paths || !val) return;
    for (int i = 0; paths[i]; i++) {
        if (paths[i][0] == '\0') continue;
        if (write_one(paths[i], val)) return;
    }
}

void sysfs_write_temp_mode(const char *path, const char *val, mode_t temp_mode) {
    if (!path || path[0] == '\0' || !val) return;
    struct stat st;
    int have_saved = (stat(path, &st) == 0);
    if (!have_saved) return; /* nothing to widen; write_one would just fail */

    mode_t saved = st.st_mode & 0777;
    if (chmod(path, temp_mode) != 0) return;
    sysfs_write(path, val);
    chmod(path, saved);
}
