/*
 * HyperCore - Logging Implementation
 * Author: @itswill00
 */

#include "log.hpp"

void log_write(const char *fmt, ...) {
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) return;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);

    va_list args;
    va_start(args, fmt);
    fprintf(f, "%s ", tbuf);
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);

    fclose(f);
}

void rotate_log(void) {
    struct stat st;
    if (stat(LOG_PATH, &st) != 0 || st.st_size <= 102400) return;

    FILE *in = fopen(LOG_PATH, "r");
    if (!in) return;

    char **lines = (char **)malloc(200 * sizeof(char *));
    if (!lines) { fclose(in); return; }

    char buf[256];
    int count = 0;
    while (fgets(buf, sizeof(buf), in)) {
        int slot = count % 200;
        if (count < 200) {
            lines[slot] = (char *)malloc(strlen(buf) + 1);
        } else {
            free(lines[slot]);
            lines[slot] = (char *)malloc(strlen(buf) + 1);
        }
        if (lines[slot]) strcpy(lines[slot], buf);
        count++;
    }
    fclose(in);

    FILE *out = fopen(LOG_PATH, "w");
    if (!out) {
        int total = (count < 200) ? count : 200;
        for (int i = 0; i < total; i++) free(lines[i]);
        free(lines);
        return;
    }

    int total = (count < 200) ? count : 200;
    int start_idx = (count < 200) ? 0 : (count % 200);
    for (int i = 0; i < total; i++) {
        int slot = (start_idx + i) % 200;
        if (lines[slot]) {
            fputs(lines[slot], out);
            free(lines[slot]);
        }
    }
    fclose(out);
    free(lines);
}
