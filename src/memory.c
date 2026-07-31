/*
 * HyperCore - Virtual Memory & ZRAM Tuning Implementation
 * Author: @itswill00
 */

#include "memory.hpp"
#include "log.hpp"

void apply_memory_tuning(void) {
    sysfs_write("/proc/sys/vm/swappiness", "15");
    sysfs_write("/proc/sys/vm/dirty_ratio", "15");
    sysfs_write("/proc/sys/vm/dirty_background_ratio", "5");
    sysfs_write("/proc/sys/vm/vfs_cache_pressure", "100");
    sysfs_write("/proc/sys/vm/stat_interval", "5");
    sysfs_write("/proc/sys/vm/compaction_proactiveness", "10");
    sysfs_write("/proc/sys/vm/page-cluster", "0");
    sysfs_write("/proc/sys/vm/extfrag_threshold", "750");
}

void compact_memory_on_sleep(void) {
    sysfs_write("/proc/sys/vm/compact_memory", "1");
    log_info("MEMORY", "Proactive memory compaction executed for sleep state");
}
