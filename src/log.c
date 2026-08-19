
#include "log.hpp"
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

static const char *s_level_names[] = {
    "Info ",
    "State",
    "Warn ",
    "Error"
};

/* Persistent log file descriptor to eliminate open/close I/O overhead */
static int s_log_fd = -1;

static void ensure_log_open(void) {
    if (s_log_fd >= 0) return;
    s_log_fd = open(LOG_PATH, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    if (s_log_fd < 0) {
        s_log_fd = open("/data/adb/hypercore/hypercore.log", O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    }
}

void log_reopen(void) {
    if (s_log_fd >= 0) { close(s_log_fd); s_log_fd = -1; }
    ensure_log_open();
}

void log_write(log_level_t level, const char *tag, const char *fmt, ...) {
    ensure_log_open();
    if (s_log_fd < 0) return;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    /* Reentrant localtime_r for thread safety */
    struct tm tm_buf;
    struct tm *tm_info = localtime_r(&tv.tv_sec, &tm_buf);

    char tbuf[32];
    if (!tm_info) {
        snprintf(tbuf, sizeof(tbuf), "0000-00-00 00:00:00.000");
    } else {
        size_t tlen = strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);
        snprintf(tbuf + tlen, sizeof(tbuf) - tlen, ".%03d", (int)(tv.tv_usec / 1000));
    }

    const char *lvl_str = (level >= 0 && level <= LOG_LEVEL_Error) ? s_level_names[level] : "Info ";
    const char *tag_str = (tag && tag[0] != '\0') ? tag : "System";

    /* Format line into single buffer for atomic O_APPEND write */
    char line[512];
    int hlen = snprintf(line, sizeof(line), "[%s] [%s] [%-8s] ", tbuf, lvl_str, tag_str);
    if (hlen < 0 || hlen >= (int)sizeof(line)) return;

    va_list args;
    va_start(args, fmt);
    int blen = vsnprintf(line + hlen, sizeof(line) - hlen - 1, fmt, args);
    va_end(args);
    if (blen < 0) return;

    int total = hlen + blen;
    if (total >= (int)sizeof(line) - 1) total = (int)sizeof(line) - 2;
    line[total++] = '\n';
    line[total]   = '\0';

    write(s_log_fd, line, (size_t)total);
}

void rotate_log(void) {
    struct stat st;
    if (stat(LOG_PATH, &st) != 0 || st.st_size <= 524288) return; /* rotate at 512 KB */

    if (s_log_fd >= 0) { close(s_log_fd); s_log_fd = -1; }

    #define MAX_RING_LINES 250
    #define MAX_LINE_LEN   256
    static char s_ring_buffer[MAX_RING_LINES][MAX_LINE_LEN];

    FILE *in = fopen(LOG_PATH, "r");
    if (!in) { ensure_log_open(); return; }

    char buf[MAX_LINE_LEN];
    int count = 0;
    while (fgets(buf, sizeof(buf), in)) {
        int slot = count % MAX_RING_LINES;
        strncpy(s_ring_buffer[slot], buf, MAX_LINE_LEN - 1);
        s_ring_buffer[slot][MAX_LINE_LEN - 1] = '\0';
        count++;
    }
    fclose(in);

    /* Atomic rotate: write to temp file then rename */
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", LOG_PATH);

    FILE *out = fopen(tmp_path, "w");
    if (!out) { ensure_log_open(); return; }

    int total = (count < MAX_RING_LINES) ? count : MAX_RING_LINES;
    int start_idx = (count < MAX_RING_LINES) ? 0 : (count % MAX_RING_LINES);

    for (int i = 0; i < total; i++) {
        int slot = (start_idx + i) % MAX_RING_LINES;
        fputs(s_ring_buffer[slot], out);
    }
    fclose(out);
    rename(tmp_path, LOG_PATH);

    /* Reopen persistent fd pointing to the freshly rotated file */
    ensure_log_open();
}
