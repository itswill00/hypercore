/*
 * HyperCore - Sysfs Utilities Implementation
 * Author: @itswill00
 */

#include "sysfs.hpp"

void sysfs_write(const char *path, const char *val) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return;
    write(fd, val, strlen(val));
    close(fd);
}

int sysfs_read_int(const char *path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return atoi(buf);
}

void update_module_prop_status(const char *status) {
    static char s_last_status[128] = "";
    static time_t s_last_write_time = 0;
    time_t now = time(NULL);

    if (status && strcmp(s_last_status, status) == 0 && (now - s_last_write_time) < 30) {
        return;
    }

    if (status) {
        strncpy(s_last_status, status, sizeof(s_last_status) - 1);
        s_last_status[sizeof(s_last_status) - 1] = '\0';
    }
    s_last_write_time = now;

    char prop_path[256];
    snprintf(prop_path, sizeof(prop_path), "%s/module.prop", g_nodes.mod_dir);

    FILE *f = fopen(prop_path, "r");
    if (!f) return;

    char lines[32][256];
    int count = 0;
    while (fgets(lines[count], sizeof(lines[count]), f) && count < 32) {
        count++;
    }
    fclose(f);

    f = fopen(prop_path, "w");
    if (!f) return;

    for (int i = 0; i < count; i++) {
        if (strncmp(lines[i], "description=", 12) == 0) {
            fprintf(f, "description=[Active: %s] Smart kernel optimizer & gaming daemon for Helio G99 Ultra.\n", status);
        } else {
            fputs(lines[i], f);
        }
    }
    fclose(f);
}
