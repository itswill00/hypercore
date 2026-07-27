/*
 * HyperCore - Storage I/O Queue Tuning Implementation
 * Author: @itswill00
 */

#include "io.hpp"

void apply_io_tuning(void) {
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "sd", 2) == 0 || strncmp(ent->d_name, "mmcblk", 6) == 0) {
            char p[256];
            snprintf(p, sizeof(p), "/sys/block/%s/queue/scheduler", ent->d_name);
            sysfs_write(p, "none");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
            sysfs_write(p, "128");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/iostats", ent->d_name);
            sysfs_write(p, "0");
            snprintf(p, sizeof(p), "/sys/block/%s/queue/nr_requests", ent->d_name);
            sysfs_write(p, "64");
        }
    }
    closedir(d);
}
