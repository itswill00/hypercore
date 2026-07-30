/*
 * HyperCore - Storage I/O Queue Tuning Implementation
 * Author: @itswill00
 */

#include "io.hpp"

void set_read_ahead(const char *kb_val) {
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "sd", 2) == 0 ||
            strncmp(ent->d_name, "mmcblk", 6) == 0 ||
            strncmp(ent->d_name, "dm-", 3) == 0 ||
            strncmp(ent->d_name, "ufs", 3) == 0) {
            char p[256];
            snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
            sysfs_write(p, kb_val);
        }
    }
    closedir(d);
}

void apply_io_tuning(void) {
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "sd", 2) == 0 ||
            strncmp(ent->d_name, "mmcblk", 6) == 0 ||
            strncmp(ent->d_name, "dm-", 3) == 0 ||
            strncmp(ent->d_name, "ufs", 3) == 0) {
            char p[256];
            snprintf(p, sizeof(p), "/sys/block/%s/queue/scheduler", ent->d_name);
            sysfs_write(p, "none");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
            sysfs_write(p, "256");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/iostats", ent->d_name);
            sysfs_write(p, "0");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/nr_requests", ent->d_name);
            sysfs_write(p, "64");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/add_random", ent->d_name);
            sysfs_write(p, "0");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/rq_affinity", ent->d_name);
            sysfs_write(p, "2");
        }
    }
    closedir(d);
}

void apply_irq_tuning(void) {
    DIR *dir = opendir("/proc/irq");
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        int irq = atoi(ent->d_name);
        if (irq <= 0) continue;

        char name_path[128];
        snprintf(name_path, sizeof(name_path), "/proc/irq/%d/action", irq);
        int fd = open(name_path, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            char buf[64];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
                if (strstr(buf, "mali") || strstr(buf, "ged") || strstr(buf, "kgsl") || strstr(buf, "dsi")) {
                    char aff_path[128];
                    snprintf(aff_path, sizeof(aff_path), "/proc/irq/%d/smp_affinity", irq);
                    sysfs_write(aff_path, "c0");
                }
            }
        }
    }
    closedir(dir);
}
