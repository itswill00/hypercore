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

    char lines[300][256];
    int count = 0;
    while (fgets(lines[count % 300], sizeof(lines[0]), in)) {
        count++;
    }
    fclose(in);

    FILE *out = fopen(LOG_PATH, "w");
    if (!out) return;

    int start = (count > 150) ? (count - 150) : 0;
    for (int i = start; i < count; i++) {
        fputs(lines[i % 300], out);
    }
    fclose(out);
}
