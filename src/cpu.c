#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "io.h"
#include "nodes.h"
#include "sysfs.h"
#include "util.h"
#include "log.h"

/* -------------------------------------------------------------------------
 * Hardware detection
 * ---------------------------------------------------------------------- */

static const char *get_best_governor(profile_t prof) {
    if (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) {
        if (g_nodes.cached_gov_perf[0] != '\0') return g_nodes.cached_gov_perf;
    } else {
        if (g_nodes.cached_gov_balanced[0] != '\0') return g_nodes.cached_gov_balanced;
    }

    char avail[256] = "";
    if (!sysfs_read_str(N_CPUAVAIL_GOV[0], avail, sizeof(avail))) {
        return (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) ? "performance" : "schedutil";
    }

    if (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) {
        if (strstr(avail, "performance")) return "performance";
        if (strstr(avail, "prowess")) return "prowess";
        if (strstr(avail, "impulse")) return "impulse";
        if (strstr(avail, "blu_schedutil")) return "blu_schedutil";
    } else {
        if (g_nodes.boot_cpu_gov[0] != '\0' &&
            strcmp(g_nodes.boot_cpu_gov, "performance") != 0 &&
            strcmp(g_nodes.boot_cpu_gov, "powersave") != 0 &&
            strstr(avail, g_nodes.boot_cpu_gov)) {
            return g_nodes.boot_cpu_gov;
        }
        if (strstr(avail, "helix_schedutil")) return "helix_schedutil";
        if (strstr(avail, "electroutil")) return "electroutil";
        if (strstr(avail, "reflex")) return "reflex";
        if (strstr(avail, "sugov_ext")) return "sugov_ext";
    }

    return "schedutil";
}

void detect_cpu_hardware_limits(void) {
    if (g_nodes.boot_cpu_gov[0] == '\0') {
        if (!sysfs_read_str(N_CPU_GOV0[0], g_nodes.boot_cpu_gov, sizeof(g_nodes.boot_cpu_gov))) {
            snprintf(g_nodes.boot_cpu_gov, sizeof(g_nodes.boot_cpu_gov), "%s", "schedutil");
        }
        log_info("Hardware", "Boot CPU scaling governor captured: '%s'", g_nodes.boot_cpu_gov);
    }

    int lit_min = node_read_int(N_CPU_POL0_MIN);
    int lit_max = node_read_int(N_CPU_POL0_MAX);
    g_nodes.lit_hw_min_freq = (lit_min > 0) ? lit_min : 500000;
    g_nodes.lit_hw_max_freq = (lit_max > 0) ? lit_max : FREQ_LITTLE_MAX;

    int big_min = node_read_int(N_CPU_POL6_MIN);
    int big_max = node_read_int(N_CPU_POL6_MAX);
    g_nodes.big_hw_min_freq = (big_min > 0) ? big_min : 725000;
    g_nodes.big_hw_max_freq = (big_max > 0) ? big_max : FREQ_BIG_MAX;

    log_info("Hardware", "CPU freq bounds - Little: [%d - %d kHz], Big: [%d - %d kHz]",
             g_nodes.lit_hw_min_freq, g_nodes.lit_hw_max_freq,
             g_nodes.big_hw_min_freq, g_nodes.big_hw_max_freq);

    if (g_nodes.cached_gov_perf[0] == '\0') {
        snprintf(g_nodes.cached_gov_perf, sizeof(g_nodes.cached_gov_perf), "%s",
                 get_best_governor(PROFILE_Gaming));
        snprintf(g_nodes.cached_gov_balanced, sizeof(g_nodes.cached_gov_balanced), "%s",
                 get_best_governor(PROFILE_Interactive));
        log_info("Hardware", "CPU governor cache: perf='%s' balanced='%s'",
                 g_nodes.cached_gov_perf, g_nodes.cached_gov_balanced);
    }
}

/* -------------------------------------------------------------------------
 * CPU writers
 *
 * MT6789 is a strict bi-cluster part. Per-CPU scaling_* nodes are symlinks to
 * their policy node on this kernel, so writing policy0/policy6 once applies to
 * every core in the cluster; the old per-CPU loops issued the same write six
 * to eight times per transition for no effect.
 * ---------------------------------------------------------------------- */

static void write_policy_pair(const char *policy_dir, int min_f, int max_f) {
    if (min_f <= 0 || max_f <= 0) return;
    if (min_f > max_f) max_f = min_f;

    char min_path[256], max_path[256], buf[32];
    snprintf(min_path, sizeof(min_path), "%s/scaling_min_freq", policy_dir);
    snprintf(max_path, sizeof(max_path), "%s/scaling_max_freq", policy_dir);

    int cur_min = sysfs_read_int(min_path);
    if (cur_min > max_f) {
        /* Lower the floor before raising the ceiling, or the kernel rejects it. */
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

static void set_cpu_governor(const char *lit_gov, const char *big_gov) {
    sysfs_write_fallback(N_CPU_GOV0, lit_gov);
    sysfs_write_fallback(N_CPU_GOV6, big_gov);
}

/* Rate-limit nodes are 0644 on a stock ROM but some vendor kernels ship them
 * 0444; widen only for the duration of the write. */
static void write_rate_nodes(const char *const paths[], const char *val) {
    if (!val || val[0] == '\0') return;
    for (int i = 0; paths[i]; i++) {
        if (access(paths[i], F_OK) != 0) continue;
        sysfs_write_temp_mode(paths[i], val, 0644);
    }
}

static void set_rate_limits(const char *lit_up, const char *lit_down,
                            const char *big_up, const char *big_down) {
    write_rate_nodes(N_RATE_P0_UP, lit_up);
    write_rate_nodes(N_RATE_P0_DOWN, lit_down);
    write_rate_nodes(N_RATE_P6_UP, big_up);
    write_rate_nodes(N_RATE_P6_DOWN, big_down);
}

/* -------------------------------------------------------------------------
 * Profile matrix
 *
 * A profile is a sparse override of the captured stock baseline. NULL means
 * "restore what the device shipped", which is exactly what Interactive is.
 * This replaces the 230-line switch that repeated all 50 fields per profile
 * and silently let one profile inherit another's leftovers (Gaming's
 * swappiness=60 used to survive into Sleep).
 * ---------------------------------------------------------------------- */

typedef enum {
    GOV_STOCK,      /* profile0 <- stock gov0, profile6 <- stock gov6 */
    GOV_BALANCED,   /* best balanced governor on both clusters */
    GOV_PERF        /* best performance governor on both clusters */
} gov_class_t;

typedef enum {
    FREQ_STOCK,     /* captured factory min/max */
    FREQ_HW         /* hardware bounds, with optional per-cluster ceiling */
} freq_mode_t;

typedef struct {
    gov_class_t gov;
    freq_mode_t freq_mode;
    int lit_max_override;   /* 0 = hardware maximum */
    int big_max_override;   /* 0 = hardware maximum */
    int use_hw_gpu_max;     /* devfreq ceiling and cust upbound = detected max */

    /* Resolved at build time from gov_class_t; never NULL after build. */
    const char *cpu_gov_lit;
    const char *cpu_gov_big_hint;

    const char *up_rate;
    const char *down_rate;
    const char *nr_requests;
    const char *read_ahead;

    const char *bg_cpus;
    const char *sys_bg_cpus;
    const char *top_app_cpus;
    const char *bg_shares;
    const char *bg_uclamp_min;
    const char *bg_uclamp_max;
    const char *sys_bg_uclamp_max;
    const char *top_app_shares;
    const char *top_app_uclamp_min;
    const char *top_app_uclamp_max;

    const char *devfreq_gov;
    const char *devfreq_poll_ms;
    const char *devfreq_upthresh;
    const char *devfreq_downdiff;
    const char *devfreq_min_freq;
    const char *devfreq_max_freq;
    const char *power_policy;
    const char *dvfsrc_qos;

    const char *boost_gpu_enable;
    const char *ged_smart_boost;
    const char *ged_boost_enable;
    const char *enable_gpu_boost;
    const char *gpu_cust_boost_freq;
    const char *gpu_cust_upbound_freq;
    const char *gpu_bottom_freq;
    const char *g_fb_dvfs_threshold;
    const char *gx_fb_dvfs_margin;
    const char *gx_game_mode;

    const char *fpsgo_force_onoff;
    const char *fpsgo_boost_ta;
    const char *fpsgo_light_loading;
    const char *fpsgo_idleprefer;
    const char *fpsgo_thrm_enable;

    const char *sconfig;

    const char *touch_smooth;
    const char *touch_noise;
    const char *touch_game;
    const char *touch_sens;
    const char *touch_edge;

    const char *vm_swappiness;
    const char *vm_dirty_ratio;
    const char *vm_dirty_bg_ratio;
    const char *vm_vfs_cache_pressure;
    const char *vm_stat_interval;
    const char *vm_dirty_writeback;
    const char *vm_compaction;
} profile_matrix_t;

static const profile_matrix_t k_profiles[PROFILE_COUNT] = {
    [PROFILE_Sleep] = {
        .gov = GOV_BALANCED,
        .freq_mode = FREQ_HW,
        .lit_max_override = 1000000,
        .big_max_override = 850000,
        .up_rate = "20000",
        .down_rate = "50000",
        .nr_requests = "32",
        .read_ahead = "128",
        .bg_cpus = "0-3",
        .sys_bg_cpus = "0-3",
        .top_app_cpus = "0-7",
        .bg_shares = "1024",
        .bg_uclamp_min = "0",
        .bg_uclamp_max = "30",
        .sys_bg_uclamp_max = "max",
        .top_app_shares = "1024",
        .top_app_uclamp_min = "0",
        .top_app_uclamp_max = "40",
        .devfreq_gov = "simple_ondemand",
        .devfreq_poll_ms = "100",
        .devfreq_upthresh = "75",
        .devfreq_downdiff = "15",
        .devfreq_min_freq = "390000000",
        .devfreq_max_freq = "390000000",
        .power_policy = "coarse_demand",
        .dvfsrc_qos = "0",
        .boost_gpu_enable = "0",
        .ged_smart_boost = "0",
        .ged_boost_enable = "0",
        .enable_gpu_boost = "0",
        .gpu_cust_boost_freq = "0",
        .gpu_cust_upbound_freq = "390000",
        .gpu_bottom_freq = "390000",
        .g_fb_dvfs_threshold = "15",
        .gx_fb_dvfs_margin = "0",
        .gx_game_mode = "0",
        .fpsgo_force_onoff = "2",
        .fpsgo_boost_ta = "0",
        .fpsgo_light_loading = "50",
        .fpsgo_idleprefer = "1",
        .fpsgo_thrm_enable = "1",
        .touch_smooth = "0",
        .touch_noise = "0",
        .touch_game = "0",
        .touch_sens = "0",
        .touch_edge = "0",
        .vm_stat_interval = "60",
        .vm_dirty_writeback = "3000",
    },

    [PROFILE_Interactive] = {
        /* Factory stock is the whole profile; the only intentional change is
         * touch smoothing. Everything else resolves from stock_state.conf. */
        .gov = GOV_STOCK,
        .freq_mode = FREQ_STOCK,
        .dvfsrc_qos = "0",
        .touch_smooth = "1",
        .touch_noise = "1",
        .touch_game = "0",
        .touch_sens = "0",
        .touch_edge = "0",
    },

    [PROFILE_Gaming] = {
        .gov = GOV_PERF,
        .freq_mode = FREQ_HW,
        .use_hw_gpu_max = 1,
        .up_rate = "0",
        .down_rate = "30000",
        .nr_requests = "256",
        .read_ahead = "256",
        .bg_cpus = "0-3",
        .sys_bg_cpus = "0-3",
        .top_app_cpus = "0-7",
        .bg_shares = "256",
        .bg_uclamp_min = "0",
        .bg_uclamp_max = "40",
        .sys_bg_uclamp_max = "max",
        .top_app_shares = "2048",
        .top_app_uclamp_min = "50",
        .top_app_uclamp_max = "max",
        .devfreq_gov = "performance",
        .devfreq_poll_ms = "20",
        .devfreq_upthresh = "50",
        .devfreq_downdiff = "20",
        .devfreq_min_freq = "390000000",
        .power_policy = "coarse_demand",
        .dvfsrc_qos = "1",
        .boost_gpu_enable = "1",
        .ged_smart_boost = "1",
        .ged_boost_enable = "1",
        .enable_gpu_boost = "1",
        .gpu_cust_boost_freq = "0",
        .gpu_bottom_freq = "390000",
        .g_fb_dvfs_threshold = "25",
        .gx_fb_dvfs_margin = "50",
        .gx_game_mode = "1",
        .fpsgo_force_onoff = "2",
        .fpsgo_boost_ta = "1",
        .fpsgo_light_loading = "0",
        .fpsgo_idleprefer = "1",
        .touch_smooth = "1",
        .touch_noise = "1",
        .touch_game = "1",
        .touch_sens = "1",
        .touch_edge = "1",
        .vm_swappiness = "60",
        .vm_dirty_ratio = "15",
        .vm_dirty_bg_ratio = "5",
        .vm_vfs_cache_pressure = "100",
        .vm_stat_interval = "20",
        .vm_dirty_writeback = "1500",
    },

    [PROFILE_Gaming_MOBA] = {
        .gov = GOV_PERF,
        .freq_mode = FREQ_HW,
        .use_hw_gpu_max = 1,
        .up_rate = "0",
        .down_rate = "30000",
        .nr_requests = "256",
        .read_ahead = "256",
        .bg_cpus = "0-3",
        .sys_bg_cpus = "0-3",
        .top_app_cpus = "0-7",
        .bg_shares = "256",
        .bg_uclamp_min = "0",
        .bg_uclamp_max = "40",
        .sys_bg_uclamp_max = "max",
        .top_app_shares = "2048",
        .top_app_uclamp_min = "50",
        .top_app_uclamp_max = "max",
        .devfreq_gov = "performance",
        .devfreq_poll_ms = "20",
        .devfreq_upthresh = "50",
        .devfreq_downdiff = "20",
        .devfreq_min_freq = "390000000",
        .power_policy = "coarse_demand",
        .dvfsrc_qos = "1",
        .boost_gpu_enable = "1",
        .ged_smart_boost = "1",
        .ged_boost_enable = "1",
        .enable_gpu_boost = "1",
        .gpu_cust_boost_freq = "0",
        .gpu_bottom_freq = "390000",
        .g_fb_dvfs_threshold = "25",
        .gx_fb_dvfs_margin = "30",
        .gx_game_mode = "1",
        .fpsgo_force_onoff = "2",
        .fpsgo_boost_ta = "1",
        .fpsgo_light_loading = "0",
        .fpsgo_idleprefer = "1",
        .touch_smooth = "1",
        .touch_noise = "1",
        .touch_game = "1",
        .touch_sens = "1",
        .touch_edge = "0",
        .vm_swappiness = "60",
        .vm_dirty_ratio = "15",
        .vm_dirty_bg_ratio = "5",
        .vm_vfs_cache_pressure = "100",
        .vm_stat_interval = "20",
        .vm_dirty_writeback = "1500",
    },
};

/* Fields whose NULL value means "what the device shipped". */
#define STOCK_FALLBACK_FIELDS(X) \
    X(nr_requests, io_nr_requests) \
    X(read_ahead, io_read_ahead) \
    X(bg_cpus, bg_cpus) \
    X(sys_bg_cpus, sys_bg_cpus) \
    X(top_app_cpus, top_app_cpus) \
    X(bg_shares, bg_shares) \
    X(bg_uclamp_min, bg_uclamp_min) \
    X(bg_uclamp_max, bg_uclamp_max) \
    X(sys_bg_uclamp_max, sys_bg_uclamp_max) \
    X(top_app_shares, top_app_shares) \
    X(top_app_uclamp_min, top_app_uclamp_min) \
    X(top_app_uclamp_max, top_app_uclamp_max) \
    X(devfreq_gov, mali_gpu_gov) \
    X(devfreq_poll_ms, mali_poll_int) \
    X(devfreq_upthresh, mali_upthresh) \
    X(devfreq_downdiff, mali_downdiff) \
    X(devfreq_min_freq, mali_min_freq) \
    X(devfreq_max_freq, mali_max_freq) \
    X(power_policy, mali_policy) \
    X(boost_gpu_enable, boost_gpu_enable) \
    X(ged_smart_boost, ged_smart_boost) \
    X(ged_boost_enable, ged_boost_enable) \
    X(enable_gpu_boost, enable_gpu_boost) \
    X(gpu_cust_boost_freq, gpu_cust_boost_freq) \
    X(gpu_cust_upbound_freq, gpu_cust_upbound_freq) \
    X(gpu_bottom_freq, gpu_bottom_freq) \
    X(g_fb_dvfs_threshold, g_fb_dvfs_threshold) \
    X(gx_fb_dvfs_margin, gx_fb_dvfs_margin) \
    X(gx_game_mode, gx_game_mode) \
    X(fpsgo_force_onoff, fpsgo_force_onoff) \
    X(fpsgo_boost_ta, fpsgo_boost_ta) \
    X(fpsgo_light_loading, fpsgo_light_loading) \
    X(fpsgo_idleprefer, fpsgo_idleprefer) \
    X(fpsgo_thrm_enable, fpsgo_thrm_enable) \
    X(sconfig, sconfig) \
    X(vm_swappiness, vm_swappiness) \
    X(vm_dirty_ratio, vm_dirty_ratio) \
    X(vm_dirty_bg_ratio, vm_dirty_bg_ratio) \
    X(vm_vfs_cache_pressure, vm_vfs_cache_pressure) \
    X(vm_stat_interval, vm_stat_interval) \
    X(vm_dirty_writeback, vm_dirty_writeback) \
    X(vm_compaction, vm_compaction)

static void build_profile_matrix(profile_t prof, profile_matrix_t *m) {
    *m = k_profiles[prof];

    if (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) {
        if (!m->devfreq_max_freq) m->devfreq_max_freq = get_max_gpu_freq_hz();
        if (!m->gpu_cust_upbound_freq) m->gpu_cust_upbound_freq = get_max_gpu_freq_khz();
    }

#define FILL_STOCK(field, stock_field) \
    if (!m->field) { \
        if (g_stock_baseline.stock_field[0]) m->field = g_stock_baseline.stock_field; \
    }
    STOCK_FALLBACK_FIELDS(FILL_STOCK)
#undef FILL_STOCK

    /* Governor class is resolved here, not baked into the table, so the
     * "best available governor" probe stays in one place. */
    switch (m->gov) {
        case GOV_STOCK:
            m->cpu_gov_lit = g_stock_baseline.gov0[0] ? g_stock_baseline.gov0 : get_best_governor(PROFILE_Interactive);
            m->cpu_gov_big_hint = g_stock_baseline.gov6[0] ? g_stock_baseline.gov6 : m->cpu_gov_lit;
            break;
        case GOV_PERF:
            m->cpu_gov_lit = get_best_governor(PROFILE_Gaming);
            m->cpu_gov_big_hint = m->cpu_gov_lit;
            break;
        case GOV_BALANCED:
        default:
            m->cpu_gov_lit = get_best_governor(PROFILE_Interactive);
            m->cpu_gov_big_hint = m->cpu_gov_lit;
            break;
    }
}

/* -------------------------------------------------------------------------
 * Apply
 * ---------------------------------------------------------------------- */

static void apply_cpu_freqs(const profile_matrix_t *m) {
    int lit_min, lit_max, big_min, big_max;

    if (m->freq_mode == FREQ_STOCK) {
        lit_min = g_stock_baseline.lit_min_freq > 0 ? g_stock_baseline.lit_min_freq : g_nodes.lit_hw_min_freq;
        lit_max = g_stock_baseline.lit_max_freq > 0 ? g_stock_baseline.lit_max_freq : g_nodes.lit_hw_max_freq;
        big_min = g_stock_baseline.big_min_freq > 0 ? g_stock_baseline.big_min_freq : g_nodes.big_hw_min_freq;
        big_max = g_stock_baseline.big_max_freq > 0 ? g_stock_baseline.big_max_freq : g_nodes.big_hw_max_freq;
    } else {
        lit_min = g_nodes.lit_hw_min_freq;
        lit_max = m->lit_max_override > 0 ? m->lit_max_override : g_nodes.lit_hw_max_freq;
        big_min = g_nodes.big_hw_min_freq;
        big_max = m->big_max_override > 0 ? m->big_max_override : g_nodes.big_hw_max_freq;
    }

    write_policy_pair("/sys/devices/system/cpu/cpufreq/policy0", lit_min, lit_max);
    write_policy_pair("/sys/devices/system/cpu/cpufreq/policy6", big_min, big_max);
}

static void apply_rates(const profile_matrix_t *m) {
    const char *lit_up = m->up_rate ? m->up_rate
                        : (g_stock_baseline.pol0_up_rate[0] ? g_stock_baseline.pol0_up_rate : "1000");
    const char *lit_down = m->down_rate ? m->down_rate
                          : (g_stock_baseline.pol0_down_rate[0] ? g_stock_baseline.pol0_down_rate : "1000");
    const char *big_up = m->up_rate ? m->up_rate
                        : (g_stock_baseline.pol6_up_rate[0] ? g_stock_baseline.pol6_up_rate : "1000");
    const char *big_down = m->down_rate ? m->down_rate
                          : (g_stock_baseline.pol6_down_rate[0] ? g_stock_baseline.pol6_down_rate : "1000");
    set_rate_limits(lit_up, lit_down, big_up, big_down);
}

static void apply_cgroups(const profile_matrix_t *m) {
    sysfs_write_fallback(N_CPUSET_BG, m->bg_cpus);
    sysfs_write_fallback(N_CPUSET_SYS_BG, m->sys_bg_cpus);
    sysfs_write_fallback(N_CPUSET_TOP_APP, m->top_app_cpus);

    sysfs_write_fallback(N_CPUCTL_BG_SHARES, m->bg_shares);
    sysfs_write_fallback(N_CPUCTL_BG_MIN, m->bg_uclamp_min);
    sysfs_write_fallback(N_CPUCTL_BG_MAX, m->bg_uclamp_max);
    sysfs_write_fallback(N_CPUCTL_SYS_BG_MAX, m->sys_bg_uclamp_max);
    sysfs_write_fallback(N_CPUCTL_TOP_SHARES, m->top_app_shares);
    sysfs_write_fallback(N_CPUCTL_TOP_MIN, m->top_app_uclamp_min);
    sysfs_write_fallback(N_CPUCTL_TOP_MAX, m->top_app_uclamp_max);
}

static void apply_gpu(const profile_matrix_t *m) {
    sysfs_write_fallback(N_MALI_GOV, m->devfreq_gov);

    /* polling_interval only exists for simple_ondemand; with performance
     * selected the node may be absent, and writing an empty value would
     * clobber the stock interval. */
    if (m->devfreq_poll_ms && m->devfreq_poll_ms[0]) {
        sysfs_write_fallback(N_MALI_POLL, m->devfreq_poll_ms);
    }

    /* Devfreq rejects min > max with -EINVAL: ceiling first, then floor. */
    sysfs_write_fallback(N_MALI_MAX, m->devfreq_max_freq);
    sysfs_write_fallback(N_MALI_MIN, m->devfreq_min_freq);
    sysfs_write_fallback(N_MALI_POLICY, m->power_policy);
    set_mali_upthreshold(m->devfreq_upthresh);
    set_mali_downdifferential(m->devfreq_downdiff);
    sysfs_write_fallback(N_DVFSRC_QOS, m->dvfsrc_qos);
}

static void apply_ged(const profile_matrix_t *m) {
    sysfs_write_fallback(N_GED_BOOST_GPU, m->boost_gpu_enable);
    sysfs_write_fallback(N_GED_SMART_BOOST, m->ged_smart_boost);
    sysfs_write_fallback(N_GED_BOOST_ENABLE, m->ged_boost_enable);
    sysfs_write_fallback(N_GED_ENABLE_GPU_BOOST, m->enable_gpu_boost);
    sysfs_write_fallback(N_GED_CUST_BOOST_FREQ, m->gpu_cust_boost_freq);
    sysfs_write_fallback(N_GED_CUST_UPBOUND, m->gpu_cust_upbound_freq);
    sysfs_write_fallback(N_GED_GPU_BOTTOM, m->gpu_bottom_freq);
    sysfs_write_fallback(N_GED_FB_DVFS_THRESHOLD, m->g_fb_dvfs_threshold);
    sysfs_write_fallback(N_GED_GX_MARGIN, m->gx_fb_dvfs_margin);
    sysfs_write_fallback(N_GED_GX_GAME_MODE, m->gx_game_mode);
}

static void apply_fpsgo(const profile_matrix_t *m) {
    /* ultra_rescue is deliberately absent: it is a transient burst owned by the
     * jitter-rescue path in main.c. The profile used to pin it to 1 and the
     * burst code then wrote 0, leaving the two fighting over one node. */
    sysfs_write_fallback(N_FPSGO_FORCE, m->fpsgo_force_onoff);
    sysfs_write_fallback(N_FPSGO_BOOST_TA, m->fpsgo_boost_ta);
    sysfs_write_fallback(N_FPSGO_LIGHT, m->fpsgo_light_loading);
    sysfs_write_fallback(N_FPSGO_IDLEPREFER, m->fpsgo_idleprefer);
    sysfs_write_fallback(N_FPSGO_THRM, m->fpsgo_thrm_enable);
}

static void apply_touch(const profile_matrix_t *m) {
    sysfs_write_fallback(N_TOUCH_SMOOTH, m->touch_smooth);
    sysfs_write_fallback(N_TOUCH_NOISE, m->touch_noise);
    sysfs_write_fallback(N_TOUCH_GAME, m->touch_game);
    sysfs_write_fallback(N_TOUCH_SENS, m->touch_sens);
    sysfs_write_fallback(N_TOUCH_EDGE, m->touch_edge);
}

static void apply_vm(const profile_matrix_t *m) {
    sysfs_write_fallback(N_VM_SWAPPINESS, m->vm_swappiness);
    sysfs_write_fallback(N_VM_DIRTY_RATIO, m->vm_dirty_ratio);
    sysfs_write_fallback(N_VM_DIRTY_BG_RATIO, m->vm_dirty_bg_ratio);
    sysfs_write_fallback(N_VM_VFS_CACHE, m->vm_vfs_cache_pressure);
    sysfs_write_fallback(N_VM_STAT_INTERVAL, m->vm_stat_interval);
    sysfs_write_fallback(N_VM_DIRTY_WRITEBACK, m->vm_dirty_writeback);
    sysfs_write_fallback(N_VM_COMPACTION, m->vm_compaction);
}

void apply_profile(profile_t prof) {
    if (prof < 0 || prof >= PROFILE_COUNT) return;

    profile_matrix_t m;
    build_profile_matrix(prof, &m);

    /* Touch IC often probes after daemon start. */
    nodes_rediscover_touch();

    set_cpu_governor(m.cpu_gov_lit, m.cpu_gov_big_hint);
    apply_cpu_freqs(&m);
    apply_rates(&m);

    set_io_nr_requests(m.nr_requests);
    set_read_ahead(m.read_ahead);

    apply_cgroups(&m);
    apply_gpu(&m);
    apply_ged(&m);
    apply_fpsgo(&m);
    apply_touch(&m);
    apply_vm(&m);

    sysfs_write_temp_mode(N_SCONFIG[0], m.sconfig, 0666);

    apply_irq_tuning(prof);

    static int s_prev_gaming_state = -1;
    int is_gaming = (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA);
    if (is_gaming != s_prev_gaming_state) {
        s_prev_gaming_state = is_gaming;
        if (is_gaming) {
            async_resetprop("debug.sf.latch_unsignaled", "1", 0);
            async_resetprop("persist.sys.wifi.low_latency", "1", 0);
        } else {
            async_resetprop("debug.sf.latch_unsignaled", "0", 0);
            async_resetprop("persist.sys.wifi.low_latency", NULL, 1);
        }
    }
}

const char *profile_read_ahead(profile_t prof) {
    if (prof < 0 || prof >= PROFILE_COUNT) return "256";
    const char *ra = k_profiles[prof].read_ahead;
    if (ra) return ra;
    if (g_stock_baseline.io_read_ahead[0]) return g_stock_baseline.io_read_ahead;
    return "1024";
}
