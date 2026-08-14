
#include "memory.hpp"
#include "log.hpp"

static int s_total_ram_kb = 0;

static void detect_total_ram(void) {
    if (s_total_ram_kb > 0) return;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            char *p = line + 9;
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p) {
                s_total_ram_kb = atoi(p);
                break;
            }
        }
    }
    fclose(f);
}

void apply_memory_tuning(void) {
    detect_total_ram();
    sysfs_write("/proc/sys/vm/swappiness", "60");
    sysfs_write("/proc/sys/vm/dirty_ratio", "15");
    sysfs_write("/proc/sys/vm/dirty_background_ratio", "5");
    sysfs_write("/proc/sys/vm/vfs_cache_pressure", "100");
    sysfs_write("/proc/sys/vm/stat_interval", "5");
    sysfs_write("/proc/sys/vm/compaction_proactiveness", "20");
    sysfs_write("/proc/sys/vm/page-cluster", "0");
    sysfs_write("/proc/sys/vm/extfrag_threshold", "750");
}

void tune_memory_pressure(void) {
    detect_total_ram();
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    /* Use MemAvailable only — it reflects realistic usable RAM including reclaimable caches. */
    int mem_avail_kb = 0;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemAvailable:", 13) == 0) {
            char *p = line + 13;
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p) {
                mem_avail_kb = atoi(p);
                break;
            }
        }
    }
    fclose(f);

    /* Adapt threshold based on total physical RAM:
     * Devices with < 6GB RAM (e.g. 4GB) trigger pressure relief earlier (< 450MB),
     * while 6GB+ devices trigger at < 350MB. */
    int pressure_threshold = (s_total_ram_kb > 0 && s_total_ram_kb < 6291456) ? 460800 : 358400;

    if (mem_avail_kb > 0 && mem_avail_kb < pressure_threshold) {
        sysfs_write("/proc/sys/vm/swappiness", "80");
        sysfs_write("/proc/sys/vm/compaction_proactiveness", "50");
    } else {
        sysfs_write("/proc/sys/vm/swappiness", "60");
        sysfs_write("/proc/sys/vm/compaction_proactiveness", "20");
    }
}

int trigger_purge_ram_cache(void) {
    sync();
    sysfs_write("/proc/sys/vm/drop_caches", "3");

    /* Raise compaction_proactiveness instead of writing compact_memory directly to avoid synchronous mmap_lock stalls */
    sysfs_write("/proc/sys/vm/compaction_proactiveness", "60");
    sysfs_write("/proc/sys/vm/stat_interval", "1");
    sysfs_write("/sys/module/lowmemorykiller/parameters/minfree", "1");

    log_info("Memory", "RAM & Cache deep purge executed: drop_caches(3), async kcompactd triggered (compaction_proactiveness=60).");
    return 1;
}
