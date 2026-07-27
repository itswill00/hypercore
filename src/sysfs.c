/*
 * HyperCore - Sysfs Utilities Implementation
 * Author: @itswill00
 */

#include "sysfs.hpp"

void sysfs_write(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, val, strlen(val));
    close(fd);
}

int sysfs_read_int(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return atoi(buf);
}
