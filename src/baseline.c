#include "baseline.h"
#include "nodes.h"
#include "sysfs.h"
#include "io.h"
#include "gpu.h"
#include "util.h"
#include "log.h"

stock_baseline_t g_stock_baseline;

/* One row per captured value. `paths` is the same candidate list used for
 * capture and restore, which is what makes "apply wrote cgroup v2 but restore
 * wrote cgroup v1" impossible by construction. */
typedef struct {
    const char *key;
    int is_int;
    void *field;
    size_t cap;
    const char *const *paths;
    const char *def;
    mode_t restore_mode; /* 0 = plain write, else temporary chmod for the write */
    int no_restore;      /* restored by dedicated code (paired freq writes, IO glob) */
} bl_entry_t;

#define BL_STR(key, nodes, def) \
    { #key, 0, g_stock_baseline.key, sizeof(g_stock_baseline.key), nodes, def, 0, 0 }
#define BL_STR_MODE(key, nodes, def, mode) \
    { #key, 0, g_stock_baseline.key, sizeof(g_stock_baseline.key), nodes, def, mode, 0 }
#define BL_INT(key, nodes, def) \
    { #key, 1, &g_stock_baseline.key, sizeof(int), nodes, def, 0, 1 }
#define BL_STR_NORE(key, nodes, def) \
    { #key, 0, g_stock_baseline.key, sizeof(g_stock_baseline.key), nodes, def, 0, 1 }

static const bl_entry_t g_bl[] = {
    /* CPU */
    BL_STR(gov0, N_CPU_GOV0, "sugov_ext"),
    BL_STR(gov6, N_CPU_GOV6, "sugov_ext"),
    BL_INT(lit_min_freq, N_CPU_POL0_MIN, "500000"),
    BL_INT(lit_max_freq, N_CPU_POL0_MAX, "2000000"),
    BL_INT(big_min_freq, N_CPU_POL6_MIN, "725000"),
    BL_INT(big_max_freq, N_CPU_POL6_MAX, "2200000"),
    BL_STR(pol0_up_rate, N_RATE_P0_UP, "1000"),
    BL_STR(pol0_down_rate, N_RATE_P0_DOWN, "1000"),
    BL_STR(pol6_up_rate, N_RATE_P6_UP, "1000"),
    BL_STR(pol6_down_rate, N_RATE_P6_DOWN, "1000"),

    /* Cgroups / uclamp */
    BL_STR(bg_cpus, N_CPUSET_BG, "0-3"),
    BL_STR(sys_bg_cpus, N_CPUSET_SYS_BG, "0-5"),
    BL_STR(top_app_cpus, N_CPUSET_TOP_APP, "0-7"),
    BL_STR(bg_shares, N_CPUCTL_BG_SHARES, "1024"),
    BL_STR(bg_uclamp_min, N_CPUCTL_BG_MIN, "0"),
    BL_STR(bg_uclamp_max, N_CPUCTL_BG_MAX, "max"),
    BL_STR(sys_bg_uclamp_max, N_CPUCTL_SYS_BG_MAX, "max"),
    BL_STR(top_app_shares, N_CPUCTL_TOP_SHARES, "1024"),
    BL_STR(top_app_uclamp_min, N_CPUCTL_TOP_MIN, "0"),
    BL_STR(top_app_uclamp_max, N_CPUCTL_TOP_MAX, "max"),

    /* GPU devfreq. max must be restored before min or the kernel rejects the
     * min write with -EINVAL. */
    BL_STR(mali_policy, N_MALI_POLICY, "coarse_demand"),
    BL_STR(mali_gpu_gov, N_MALI_GOV, ""),
    BL_STR(mali_max_freq, N_MALI_MAX, "1003000000"),
    BL_STR(mali_min_freq, N_MALI_MIN, "390000000"),
    BL_STR(mali_poll_int, N_MALI_POLL, "0"),
    BL_STR(mali_upthresh, N_MALI_UPTHRESH, "80"),
    BL_STR(mali_downdiff, N_MALI_DOWNDIFF, "20"),

    /* MediaTek GED / FPSGO */
    BL_STR(boost_gpu_enable, N_GED_BOOST_GPU, "0"),
    BL_STR(ged_smart_boost, N_GED_SMART_BOOST, "0"),
    BL_STR(ged_boost_enable, N_GED_BOOST_ENABLE, "1"),
    BL_STR(enable_gpu_boost, N_GED_ENABLE_GPU_BOOST, "1"),
    BL_STR(gpu_cust_boost_freq, N_GED_CUST_BOOST_FREQ, "390000"),
    BL_STR(gpu_cust_upbound_freq, N_GED_CUST_UPBOUND, "1003000"),
    BL_STR(gpu_bottom_freq, N_GED_GPU_BOTTOM, "390000"),
    BL_STR(g_fb_dvfs_threshold, N_GED_FB_DVFS_THRESHOLD, "80"),
    BL_STR(gx_fb_dvfs_margin, N_GED_GX_MARGIN, "40"),
    BL_STR(gx_game_mode, N_GED_GX_GAME_MODE, "0"),
    BL_STR(fpsgo_force_onoff, N_FPSGO_FORCE, "0"),
    BL_STR(fpsgo_boost_ta, N_FPSGO_BOOST_TA, "0"),
    BL_STR(fpsgo_ultra_rescue, N_FPSGO_ULTRA, "0"),
    BL_STR(fpsgo_light_loading, N_FPSGO_LIGHT, "0"),
    BL_STR(fpsgo_idleprefer, N_FPSGO_IDLEPREFER, "0"),
    BL_STR(fpsgo_thrm_enable, N_FPSGO_THRM, "1"),

    /* Vendor thermal policy (kept at stock in every profile) */
    BL_STR_MODE(sconfig, N_SCONFIG, "0", 0666),

    /* Touch */
    BL_STR(touch_smooth, N_TOUCH_SMOOTH, "0"),
    BL_STR(touch_noise, N_TOUCH_NOISE, "0"),
    BL_STR(touch_game, N_TOUCH_GAME, "0"),
    BL_STR(touch_sens, N_TOUCH_SENS, "0"),
    BL_STR(touch_edge, N_TOUCH_EDGE, "0"),

    /* VM */
    BL_STR(vm_swappiness, N_VM_SWAPPINESS, "100"),
    BL_STR(vm_dirty_ratio, N_VM_DIRTY_RATIO, "20"),
    BL_STR(vm_dirty_bg_ratio, N_VM_DIRTY_BG_RATIO, "10"),
    BL_STR(vm_vfs_cache_pressure, N_VM_VFS_CACHE, "100"),
    BL_STR(vm_stat_interval, N_VM_STAT_INTERVAL, "1"),
    BL_STR(vm_dirty_writeback, N_VM_DIRTY_WRITEBACK, "500"),
    BL_STR(vm_page_cluster, N_VM_PAGE_CLUSTER, "3"),
    BL_STR(vm_compaction, N_VM_COMPACTION, "20"),

    /* Storage IO is applied to every block device via io.c, not to one node. */
    BL_STR_NORE(io_read_ahead, N_NET_READ_AHEAD, "1024"),
    BL_STR_NORE(io_nr_requests, N_NET_NR_REQUESTS, "128"),

    /* Scheduler */
    BL_STR(sched_migration_cost, N_SCHED_MIGRATION, "200000"),
    BL_STR(sched_latency, N_SCHED_LATENCY, "10000000"),
    BL_STR(sched_nr_migrate, N_SCHED_NR_MIGRATE, "32"),
};

#define BL_COUNT (sizeof(g_bl) / sizeof(g_bl[0]))

static int bl_read_field(const bl_entry_t *e) {
    char buf[64] = "";
    if (!sysfs_read_str_fallback(e->paths, buf, sizeof(buf)) || buf[0] == '\0') {
        if (!e->def || e->def[0] == '\0') return 0;
        snprintf(buf, sizeof(buf), "%s", e->def);
    }
    if (e->is_int) {
        *(int *)e->field = atoi(buf);
    } else {
        snprintf((char *)e->field, e->cap, "%s", buf);
    }
    return 1;
}

static void bl_write_field(const bl_entry_t *e) {
    if (e->no_restore) return;
    const char *val = (const char *)e->field;
    if (!val[0]) {
        if (!e->def || e->def[0] == '\0') return;
        val = e->def;
    }

    if (e->restore_mode) {
        for (int i = 0; e->paths[i]; i++) {
            if (access(e->paths[i], F_OK) != 0) continue;
            sysfs_write_temp_mode(e->paths[i], val, e->restore_mode);
        }
    } else {
        sysfs_write_fallback(e->paths, val);
    }
}

/* Paired min/max writes: lower the floor before raising the ceiling, or raise
 * the ceiling before lowering the floor, so the kernel never sees min > max. */
static void restore_policy_pair(const char *policy_dir, int min_f, int max_f) {
    if (min_f <= 0 || max_f <= 0) return;
    char min_path[256], max_path[256], buf[32];
    snprintf(min_path, sizeof(min_path), "%s/scaling_min_freq", policy_dir);
    snprintf(max_path, sizeof(max_path), "%s/scaling_max_freq", policy_dir);

    int cur_min = sysfs_read_int(min_path);
    if (cur_min > max_f) {
        snprintf(buf, sizeof(buf), "%d", min_f);
        sysfs_write(min_path, buf);
        snprintf(buf, sizeof(buf), "%d", max_f);
        sysfs_write(max_path, buf);
    } else {
        snprintf(buf, sizeof(buf), "%d", max_f);
        sysfs_write(max_path, buf);
        snprintf(buf, sizeof(buf), "%d", min_f);
        sysfs_write(min_path, buf);
    }
}

static void baseline_capture(void) {
    memset(&g_stock_baseline, 0, sizeof(g_stock_baseline));

    for (size_t i = 0; i < BL_COUNT; i++) bl_read_field(&g_bl[i]);

    /* Vendor ROMs report 10/-1 for "no thermal override" on some builds. */
    if (g_stock_baseline.sconfig[0] == '\0' ||
        strcmp(g_stock_baseline.sconfig, "10") == 0 ||
        strcmp(g_stock_baseline.sconfig, "-1") == 0) {
        snprintf(g_stock_baseline.sconfig, sizeof(g_stock_baseline.sconfig), "%s", "0");
    }
    if (g_stock_baseline.mali_max_freq[0] == '\0' ||
        strcmp(g_stock_baseline.mali_max_freq, "0") == 0) {
        snprintf(g_stock_baseline.mali_max_freq, sizeof(g_stock_baseline.mali_max_freq),
                 "%s", get_max_gpu_freq_hz());
    }

    g_stock_baseline.has_baseline = 1;
    baseline_save();
    log_info("Baseline", "Untouched factory baseline captured and saved to %s/stock_state.conf",
             g_nodes.data_dir);
}

void baseline_save(void) {
    char path[300], tmp[310];
    snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.data_dir);
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "# HyperCore Stock Factory Baseline\n# Captured automatically on first install\n");
    fprintf(f, "has_baseline=1\n");

    for (size_t i = 0; i < BL_COUNT; i++) {
        if (g_bl[i].is_int) fprintf(f, "%s=%d\n", g_bl[i].key, *(int *)g_bl[i].field);
        else fprintf(f, "%s=%s\n", g_bl[i].key, (char *)g_bl[i].field);
    }

    if (fclose(f) != 0) {
        unlink(tmp);
        return;
    }
    rename(tmp, path);
    chmod(path, 0600);
}

static int baseline_load(void) {
    char path[300];
    snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.data_dir);
    FILE *f = fopen(path, "r");
    if (!f && g_nodes.mod_dir[0] && strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.mod_dir);
        f = fopen(path, "r");
    }
    if (!f) return 0;

    memset(&g_stock_baseline, 0, sizeof(g_stock_baseline));

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        char *nl = strpbrk(val, "\r\n");
        if (nl) *nl = '\0';

        if (strcmp(key, "has_baseline") == 0) {
            g_stock_baseline.has_baseline = atoi(val);
            continue;
        }
        for (size_t i = 0; i < BL_COUNT; i++) {
            if (strcmp(key, g_bl[i].key) != 0) continue;
            if (g_bl[i].is_int) *(int *)g_bl[i].field = atoi(val);
            else snprintf((char *)g_bl[i].field, g_bl[i].cap, "%s", val);
            break;
        }
    }
    fclose(f);
    g_stock_baseline.has_baseline = 1;
    return 1;
}

void baseline_init(void) {
    if (g_stock_baseline.has_baseline) return;
    if (baseline_load()) {
        log_info("Baseline", "Loaded existing factory stock baseline from stock_state.conf");
        return;
    }
    log_info("Baseline", "No saved stock baseline found. Capturing untouched factory baseline...");
    baseline_capture();
}

void baseline_restore(void) {
    if (!g_stock_baseline.has_baseline) return;

    for (size_t i = 0; i < BL_COUNT; i++) bl_write_field(&g_bl[i]);

    restore_policy_pair("/sys/devices/system/cpu/cpufreq/policy0",
                        g_stock_baseline.lit_min_freq, g_stock_baseline.lit_max_freq);
    restore_policy_pair("/sys/devices/system/cpu/cpufreq/policy6",
                        g_stock_baseline.big_min_freq, g_stock_baseline.big_max_freq);

    if (g_stock_baseline.io_read_ahead[0]) set_read_ahead(g_stock_baseline.io_read_ahead);
    if (g_stock_baseline.io_nr_requests[0]) set_io_nr_requests(g_stock_baseline.io_nr_requests);

    /* Rate-limit nodes are chmod-guarded on write; leave them 0644 so the next
     * boot's vendor init sees the mode it shipped with. */
    const char *rate_nodes[] = {
        "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
        NULL
    };
    for (int i = 0; rate_nodes[i]; i++) {
        if (access(rate_nodes[i], F_OK) == 0) chmod(rate_nodes[i], 0644);
    }

    /* Reset the two runtime properties that are intentionally not persistent. */
    async_resetprop("debug.sf.latch_unsignaled", "0", 0);
    async_resetprop("persist.sys.wifi.low_latency", NULL, 1);
}
