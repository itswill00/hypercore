#include "memory.h"
#include "nodes.h"
#include "log.h"

static int s_total_ram_kb = 0;
static int s_swap_total_kb = -1;

static int s_last_swap = -1;
static int s_last_compact = -1;

/* One pass over /proc/meminfo for all three values. The previous version
 * opened and parsed the file twice per tick, and a third time to detect swap. */
static void read_meminfo(int *mem_total, int *mem_avail, int *swap_total) {
    *mem_total = 0;
    *mem_avail = 0;
    *swap_total = -1;

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char *p = strchr(line, ':');
        if (!p) continue;
        *p = '\0';
        char *v = p + 1;
        while (*v && (*v < '0' || *v > '9')) v++;
        if (*v == '\0') continue;
        int val = atoi(v);

        if (strcmp(line, "MemTotal") == 0) *mem_total = val;
        else if (strcmp(line, "MemAvailable") == 0) *mem_avail = val;
        else if (strcmp(line, "SwapTotal") == 0) *swap_total = val;
    }
    fclose(f);
}

void tune_memory_pressure(void) {
    int gaming = (g_state.current_profile == PROFILE_Gaming ||
                  g_state.current_profile == PROFILE_Gaming_MOBA);
    if (!gaming) {
        /* The profile matrix owns the VM knobs outside Gaming; invalidate the
         * cache so re-entering Gaming always writes its own values. */
        s_last_swap = -1;
        s_last_compact = -1;
        return;
    }

    int total = 0, avail = 0, swap = -1;
    read_meminfo(&total, &avail, &swap);

    if (s_total_ram_kb <= 0 && total > 0) s_total_ram_kb = total;
    if (s_swap_total_kb < 0 && swap >= 0) s_swap_total_kb = swap;

    /* Sub-6 GB devices need relief a bit earlier. */
    int threshold = (s_total_ram_kb > 0 && s_total_ram_kb < 6291456) ? 460800 : 358400;
    int under_pressure = (avail > 0 && avail < threshold);
    int no_swap = (s_swap_total_kb == 0);

    /* With no swap pool, a high swappiness just makes kswapd scan anonymous
     * pages it can never reclaim. */
    int target_swap = no_swap ? 20 : (under_pressure ? 80 : 60);
    int target_compact = under_pressure ? 50 : 20;

    if (target_swap != s_last_swap) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", target_swap);
        sysfs_write_fallback(N_VM_SWAPPINESS, buf);
        s_last_swap = target_swap;
    }
    if (target_compact != s_last_compact) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", target_compact);
        sysfs_write_fallback(N_VM_COMPACTION, buf);
        s_last_compact = target_compact;
    }
}

int trigger_purge_ram_cache(void) {
    sync();
    sysfs_write_fallback(N_VM_COMPACTION, "60");

    /* Raise compaction_proactiveness instead of writing compact_memory
     * directly; the latter stalls the caller on mmap_lock. */
    log_info("Memory", "RAM/cache purge: drop_caches(3), async compaction requested");
    sysfs_write("/proc/sys/vm/drop_caches", "3");
    return 1;
}
