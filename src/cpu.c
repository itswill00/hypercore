#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "log.hpp"

/* Forward declaration — defined later in this file as a static function */
static const char *get_best_governor(profile_t prof);

void detect_cpu_hardware_limits(void) {
    if (g_nodes.boot_cpu_gov[0] == '\0') {
        if (!sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", g_nodes.boot_cpu_gov, sizeof(g_nodes.boot_cpu_gov))) {
            strcpy(g_nodes.boot_cpu_gov, "schedutil");
        }
        log_info("Hardware", "Boot CPU Scaling Governor captured: '%s'", g_nodes.boot_cpu_gov);
    }

    int lit_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq");
    int lit_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq");
    g_nodes.lit_hw_min_freq = (lit_min > 0) ? lit_min : 500000;
    g_nodes.lit_hw_max_freq = (lit_max > 0) ? lit_max : FREQ_LITTLE_MAX;

    int big_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_min_freq");
    if (big_min <= 0) big_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_min_freq");
    if (big_min <= 0) big_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy7/cpuinfo_min_freq");

    int big_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_max_freq");
    if (big_max <= 0) big_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_max_freq");
    if (big_max <= 0) big_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy7/cpuinfo_max_freq");

    g_nodes.big_hw_min_freq = (big_min > 0) ? big_min : 725000;
    g_nodes.big_hw_max_freq = (big_max > 0) ? big_max : FREQ_BIG_MAX;

    log_info("Hardware", "CPU Freq Bounds - Little: [%d - %d kHz], Big: [%d - %d kHz]",
             g_nodes.lit_hw_min_freq, g_nodes.lit_hw_max_freq,
             g_nodes.big_hw_min_freq, g_nodes.big_hw_max_freq);

    /* Pre-cache best governor strings for each profile class so get_best_governor()
     * doesn't re-read scaling_available_governors on every profile transition. */
    if (g_nodes.cached_gov_perf[0] == '\0') {
        const char *gp = get_best_governor(PROFILE_Gaming);
        strncpy(g_nodes.cached_gov_perf, gp, sizeof(g_nodes.cached_gov_perf) - 1);
        g_nodes.cached_gov_perf[sizeof(g_nodes.cached_gov_perf) - 1] = '\0';
        const char *gb = get_best_governor(PROFILE_Interactive);
        strncpy(g_nodes.cached_gov_balanced, gb, sizeof(g_nodes.cached_gov_balanced) - 1);
        g_nodes.cached_gov_balanced[sizeof(g_nodes.cached_gov_balanced) - 1] = '\0';
        log_info("Hardware", "CPU Governor cache: perf='%s' balanced='%s'",
                 g_nodes.cached_gov_perf, g_nodes.cached_gov_balanced);
    }
}

static const char *s_policy0_up_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
    NULL
};
static const char *s_policy0_down_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
    NULL
};
static const char *s_policy4_up_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy4/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy4/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy4/rate_limit_us",
    NULL
};
static const char *s_policy4_down_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy4/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy4/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy4/rate_limit_us",
    NULL
};
static const char *s_policy6_up_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
    NULL
};
static const char *s_policy6_down_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
    NULL
};
static const char *s_policy7_up_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy7/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy7/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy7/rate_limit_us",
    NULL
};
static const char *s_policy7_down_rate_nodes[] = {
    "/sys/devices/system/cpu/cpufreq/policy7/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy7/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy7/rate_limit_us",
    NULL
};

static void write_rate_limit_fallback(const char *paths[], const char *val) {
    if (!paths || !val) return;
    for (int i = 0; paths[i]; i++) {
        if (!paths[i] || paths[i][0] == '\0') continue;
        if (access(paths[i], F_OK) != 0) continue;
        chmod(paths[i], 0644);
        sysfs_write(paths[i], val);
        chmod(paths[i], 0444);
    }
}

void set_rate_limits(const char *up, const char *down) {
    write_rate_limit_fallback(s_policy0_up_rate_nodes, up);
    write_rate_limit_fallback(s_policy0_down_rate_nodes, down);
    write_rate_limit_fallback(s_policy4_up_rate_nodes, up);
    write_rate_limit_fallback(s_policy4_down_rate_nodes, down);
    write_rate_limit_fallback(s_policy6_up_rate_nodes, up);
    write_rate_limit_fallback(s_policy6_down_rate_nodes, down);
    write_rate_limit_fallback(s_policy7_up_rate_nodes, up);
    write_rate_limit_fallback(s_policy7_down_rate_nodes, down);
}

static const char *s_cpuset_bg_cpus_nodes[] = {
    "/dev/cpuset/background/cpus",
    "/sys/fs/cgroup/background/cpuset.cpus",
    "/sys/fs/cgroup/background/cpus",
    "/sys/fs/cgroup/system/background/cpuset.cpus",
    "/sys/fs/cgroup/system/background/cpus",
    NULL
};

static const char *s_cpuset_sys_bg_cpus_nodes[] = {
    "/dev/cpuset/system-background/cpus",
    "/sys/fs/cgroup/system-background/cpuset.cpus",
    "/sys/fs/cgroup/system-background/cpus",
    "/sys/fs/cgroup/system/sys-background/cpuset.cpus",
    "/sys/fs/cgroup/system/sys-background/cpus",
    NULL
};

static const char *s_cpuset_top_app_cpus_nodes[] = {
    "/dev/cpuset/top-app/cpus",
    "/sys/fs/cgroup/top-app/cpuset.cpus",
    "/sys/fs/cgroup/top-app/cpus",
    "/sys/fs/cgroup/top-app.slice/cpuset.cpus",
    "/sys/fs/cgroup/top-app.slice/cpus",
    NULL
};

static const char *s_cpuset_fg_cpus_nodes[] = {
    "/dev/cpuset/foreground/cpus",
    "/sys/fs/cgroup/foreground/cpuset.cpus",
    "/sys/fs/cgroup/foreground/cpus",
    "/sys/fs/cgroup/app.slice/cpuset.cpus",
    "/sys/fs/cgroup/app.slice/cpus",
    NULL
};

static const char *s_cpuctl_bg_shares_nodes[] = {
    "/dev/cpuctl/background/cpu.shares",
    "/sys/fs/cgroup/background/cpu.shares",
    "/sys/fs/cgroup/background/cpu.weight",
    "/sys/fs/cgroup/system/background/cpu.weight",
    "/sys/fs/cgroup/system/background/cpu.shares",
    NULL
};

static const char *s_cpuctl_bg_uclamp_min_nodes[] = {
    "/dev/cpuctl/background/cpu.uclamp.min",
    "/sys/fs/cgroup/background/cpu.uclamp.min",
    "/sys/fs/cgroup/system/background/cpu.uclamp.min",
    NULL
};

static const char *s_cpuctl_bg_uclamp_max_nodes[] = {
    "/dev/cpuctl/background/cpu.uclamp.max",
    "/sys/fs/cgroup/background/cpu.uclamp.max",
    "/sys/fs/cgroup/system/background/cpu.uclamp.max",
    NULL
};

static const char *s_cpuctl_sys_bg_uclamp_max_nodes[] = {
    "/dev/cpuctl/system-background/cpu.uclamp.max",
    "/sys/fs/cgroup/system-background/cpu.uclamp.max",
    "/sys/fs/cgroup/system/sys-background/cpu.uclamp.max",
    NULL
};

static const char *s_cpuctl_top_app_shares_nodes[] = {
    "/dev/cpuctl/top-app/cpu.shares",
    "/sys/fs/cgroup/top-app/cpu.shares",
    "/sys/fs/cgroup/top-app/cpu.weight",
    "/sys/fs/cgroup/top-app.slice/cpu.weight",
    "/sys/fs/cgroup/top-app.slice/cpu.shares",
    NULL
};

static const char *s_cpuctl_top_app_uclamp_min_nodes[] = {
    "/dev/cpuctl/top-app/cpu.uclamp.min",
    "/sys/fs/cgroup/top-app/cpu.uclamp.min",
    "/sys/fs/cgroup/top-app.slice/cpu.uclamp.min",
    NULL
};

static const char *s_cpuctl_top_app_uclamp_max_nodes[] = {
    "/dev/cpuctl/top-app/cpu.uclamp.max",
    "/sys/fs/cgroup/top-app/cpu.uclamp.max",
    "/sys/fs/cgroup/top-app.slice/cpu.uclamp.max",
    NULL
};

void apply_cpuset(void) {
    sysfs_write_fallback(s_cpuset_top_app_cpus_nodes, "0-7");
    sysfs_write_fallback(s_cpuset_fg_cpus_nodes, "0-7");
    sysfs_write_fallback(s_cpuset_bg_cpus_nodes, "0-3");
    sysfs_write_fallback(s_cpuset_sys_bg_cpus_nodes, "0-3");

    sysfs_write("/proc/sys/kernel/sched_migration_cost_ns", "200000");
    sysfs_write("/proc/sys/kernel/sched_latency_ns", "10000000");
    sysfs_write("/proc/sys/kernel/sched_nr_migrate", "32");
}

void set_cpu_freqs(int min_lit, int max_lit, int min_big, int max_big, const char *up_rate, const char *down_rate) {
    char path[256], buf[32];

    snprintf(buf, sizeof(buf), "%d", min_lit);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", buf);
    snprintf(buf, sizeof(buf), "%d", max_lit);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", buf);

    for (int i = 0; i <= 5; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        snprintf(buf, sizeof(buf), "%d", min_lit);
        sysfs_write(path, buf);

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        snprintf(buf, sizeof(buf), "%d", max_lit);
        sysfs_write(path, buf);
    }

    snprintf(buf, sizeof(buf), "%d", min_big);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy4/scaling_min_freq", buf);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_min_freq", buf);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy7/scaling_min_freq", buf);

    snprintf(buf, sizeof(buf), "%d", max_big);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq", buf);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq", buf);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq", buf);

    for (int i = 6; i <= 7; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        snprintf(buf, sizeof(buf), "%d", min_big);
        sysfs_write(path, buf);

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        snprintf(buf, sizeof(buf), "%d", max_big);
        sysfs_write(path, buf);
    }

    set_rate_limits(up_rate, down_rate);
}

void set_cpu_governor(const char *gov) {
    char path[256];
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", gov);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", gov);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", gov);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy7/scaling_governor", gov);

    for (int i = 0; i <= 7; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        sysfs_write(path, gov);
    }
}

void apply_cgroup_gaming_policy(int enable) {
    if (enable) {
        sysfs_write_fallback(s_cpuset_bg_cpus_nodes, "0-3");
        sysfs_write_fallback(s_cpuset_sys_bg_cpus_nodes, "0-3");
        sysfs_write_fallback(s_cpuctl_bg_shares_nodes, "256");
        sysfs_write_fallback(s_cpuctl_bg_uclamp_min_nodes, "0");
        sysfs_write_fallback(s_cpuctl_bg_uclamp_max_nodes, "40");
        sysfs_write_fallback(s_cpuctl_sys_bg_uclamp_max_nodes, "max");

        sysfs_write_fallback(s_cpuset_top_app_cpus_nodes, "0-7");
        sysfs_write_fallback(s_cpuctl_top_app_shares_nodes, "2048");
        sysfs_write_fallback(s_cpuctl_top_app_uclamp_min_nodes, "50");
        sysfs_write_fallback(s_cpuctl_top_app_uclamp_max_nodes, "max");
    } else {
        sysfs_write_fallback(s_cpuset_bg_cpus_nodes, "0-3");
        sysfs_write_fallback(s_cpuset_sys_bg_cpus_nodes, "0-3");
        sysfs_write_fallback(s_cpuctl_bg_shares_nodes, "1024");
        sysfs_write_fallback(s_cpuctl_bg_uclamp_min_nodes, "0");
        sysfs_write_fallback(s_cpuctl_bg_uclamp_max_nodes, "max");
        sysfs_write_fallback(s_cpuctl_sys_bg_uclamp_max_nodes, "max");

        sysfs_write_fallback(s_cpuset_top_app_cpus_nodes, "0-7");
        sysfs_write_fallback(s_cpuctl_top_app_shares_nodes, "1024");
        const char *uclamp_min = (g_state.app_boost_ticks > 0) ? "25" : "0";
        sysfs_write_fallback(s_cpuctl_top_app_uclamp_min_nodes, uclamp_min);
        sysfs_write_fallback(s_cpuctl_top_app_uclamp_max_nodes, "max");
    }
}

static const char *get_best_governor(profile_t prof) {
    /* Use cached value if available (populated at startup by detect_cpu_hardware_limits) */
    if (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) {
        if (g_nodes.cached_gov_perf[0] != '\0') return g_nodes.cached_gov_perf;
    } else {
        if (g_nodes.cached_gov_balanced[0] != '\0') return g_nodes.cached_gov_balanced;
    }

    /* Fallback: read sysfs (only happens before detect_cpu_hardware_limits completes) */
    char avail[256] = "";
    if (!sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors", avail, sizeof(avail))) {
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

static const char *s_devfreq_gov_nodes[] = {
    "/sys/class/devfreq/13000000.mali/governor",
    "/sys/class/devfreq/soc:mali/governor",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
    "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/governor",
    "/sys/devices/platform/soc/13fbf000.gpufreq/governor",
    NULL
};

static const char *s_devfreq_min_nodes[] = {
    "/sys/class/devfreq/13000000.mali/min_freq",
    "/sys/class/devfreq/soc:mali/min_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
    "/sys/devices/platform/soc/13fbf000.gpufreq/min_freq",
    NULL
};

static const char *s_devfreq_max_nodes[] = {
    "/sys/class/devfreq/13000000.mali/max_freq",
    "/sys/class/devfreq/soc:mali/max_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
    "/sys/devices/platform/soc/13fbf000.gpufreq/max_freq",
    NULL
};

static const char *s_devfreq_poll_nodes[] = {
    "/sys/class/devfreq/13000000.mali/polling_interval",
    "/sys/class/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/soc/13fbf000.gpufreq/polling_interval",
    NULL
};

static const char *s_power_policy_nodes[] = {
    "/sys/devices/platform/soc/13000000.mali/power_policy",
    "/sys/devices/platform/soc/soc:mali/power_policy",
    "/sys/class/devfreq/13000000.mali/power_policy",
    "/sys/class/devfreq/soc:mali/power_policy",
    "/sys/devices/platform/13000000.mali/power_policy",
    "/sys/devices/platform/soc/13fbf000.gpufreq/power_policy",
    NULL
};

static const char *s_sconfig_nodes[] = {
    "/sys/class/thermal/thermal_message/sconfig",
    "/sys/devices/virtual/thermal/thermal_message/sconfig",
    "/sys/class/thermal/thermal_message/sport_mode",
    NULL
};

static const char *s_dvfsrc_nodes[] = {
    "/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode",
    "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
    "/sys/module/dvfsrc/parameters/dvfsrc_qos_mode",
    "/sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
    NULL
};

static const char *s_game_mode_nodes[] = {
    "/sys/module/ged/parameters/gx_game_mode",
    "/sys/module/ged/parameters/gx_boost_on",
    NULL
};

typedef struct {
    profile_t target_profile;
    int thermal_tier;

    const char *cpu_gov;
    int lit_min_freq;
    int lit_max_freq;
    int big_min_freq;
    int big_max_freq;
    const char *up_rate_limit;
    const char *down_rate_limit;

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
    const char *fpsgo_ultra_rescue;
    const char *fpsgo_light_loading;
    const char *fpsgo_idleprefer;
    const char *fpsgo_thrm_enable;

    const char *sconfig;
    const char *touch_thp_smooth;
    const char *touch_game_mode;
    const char *touch_sensitivity;
    const char *touch_edge;
} profile_matrix_t;

static void build_profile_matrix(profile_t prof, int tier, profile_matrix_t *m) {
    memset(m, 0, sizeof(*m));
    m->target_profile = prof;
    m->thermal_tier = tier;
    m->cpu_gov = get_best_governor(prof);

    detect_max_gpu_freq();
    const char *max_gpu_hz  = get_max_gpu_freq_hz();
    const char *max_gpu_khz = get_max_gpu_freq_khz();

    m->bg_cpus = "0-3";
    m->sys_bg_cpus = "0-3";
    m->top_app_cpus = "0-7";
    m->sys_bg_uclamp_max = "max";
    m->devfreq_gov = "simple_ondemand";
    m->power_policy = "coarse_demand";
    m->gpu_cust_boost_freq = "0";
    m->fpsgo_force_onoff = "2";
    m->fpsgo_idleprefer = "1";

    switch (prof) {
        case PROFILE_Sleep:
            m->lit_min_freq = g_nodes.lit_hw_min_freq;
            m->lit_max_freq = 1000000;
            m->big_min_freq = g_nodes.big_hw_min_freq;
            m->big_max_freq = 850000;
            m->up_rate_limit = "20000";
            m->down_rate_limit = "50000";

            m->nr_requests = "32";
            m->read_ahead = "128";

            m->bg_shares = "1024";
            m->bg_uclamp_min = "0";
            m->bg_uclamp_max = "30";
            m->top_app_shares = "1024";
            m->top_app_uclamp_min = "0";
            m->top_app_uclamp_max = "40";

            m->devfreq_poll_ms = "100";
            m->devfreq_upthresh = "75";
            m->devfreq_downdiff = "15";
            m->devfreq_min_freq = "390000000";
            m->devfreq_max_freq = "390000000";
            m->power_policy = "coarse_demand";

            m->dvfsrc_qos = "0";

            m->boost_gpu_enable = "0";
            m->ged_smart_boost = "0";
            m->ged_boost_enable = "0";
            m->enable_gpu_boost = "0";
            m->gpu_cust_upbound_freq = "390000";
            m->gpu_bottom_freq = "390000";
            m->g_fb_dvfs_threshold = "15";
            m->gx_fb_dvfs_margin = "0";
            m->gx_game_mode = "0";

            m->fpsgo_boost_ta = "0";
            m->fpsgo_ultra_rescue = "0";
            m->fpsgo_light_loading = "50";
            m->fpsgo_thrm_enable = "1";

            m->sconfig = "0";
            m->touch_thp_smooth = "0";
            m->touch_game_mode = "0";
            m->touch_sensitivity = "0";
            m->touch_edge = "0";
            break;

        case PROFILE_Interactive:
            m->lit_min_freq = g_nodes.lit_hw_min_freq;
            m->lit_max_freq = 1600000;    /* 1.6 GHz Little core ceiling — optimal efficiency on A55 cluster */
            m->big_min_freq = 725000;     /* Minimum 725MHz idle floor */
            /* Cap Big cores at 1.8GHz for daily UI & social media.
             * 2.0-2.2GHz is reserved for Gaming/MOBA to avoid heavy A76 voltage heat and power drain */
            if (tier >= 2) {
                m->big_max_freq = 1400000;
            } else if (tier == 1) {
                m->big_max_freq = 1600000;
            } else {
                m->big_max_freq = 1800000;
            }

            m->up_rate_limit = "4000";    /* 4ms filter — prevents jumping to high clocks on transient micro-spikes */
            m->down_rate_limit = "3000";  /* 3ms ramp-down — drops CPU immediately to idle between frames to dissipate heat */

            m->nr_requests = "128";
            m->read_ahead = "256";

            m->bg_shares = "1024";
            m->bg_uclamp_min = "0";
            m->bg_uclamp_max = "50";      /* Clamp background tasks to 50% max capacity — prevents background drain */
            m->sys_bg_uclamp_max = "50";
            m->top_app_shares = "1024";
            m->top_app_uclamp_min = "0";
            m->top_app_uclamp_max = "max";

            m->devfreq_poll_ms = "50";
            m->devfreq_upthresh = "65";   /* Responsive 65% load threshold — ramps up GPU swiftly for smooth UI rendering */
            m->devfreq_downdiff = "20";
            m->devfreq_min_freq = "390000000";
            m->devfreq_max_freq = max_gpu_hz;   /* Uncapped to hardware max frequency for fluid UI */

            m->dvfsrc_qos = "0";

            m->boost_gpu_enable = "0";
            m->ged_smart_boost = "0";
            m->ged_boost_enable = "0";
            m->enable_gpu_boost = "0";
            m->gpu_cust_upbound_freq = max_gpu_khz; /* Hardware max kHz upbound */
            m->gpu_bottom_freq = "390000";
            m->g_fb_dvfs_threshold = "15";
            m->gx_fb_dvfs_margin = "10";
            m->gx_game_mode = "0";

            m->fpsgo_boost_ta = "0";
            m->fpsgo_ultra_rescue = "0";
            m->fpsgo_light_loading = "50";
            m->fpsgo_thrm_enable = "1";

            m->sconfig = "0";
            m->touch_thp_smooth = "0";     /* Reserve high touch sampling IC power for PROFILE_Gaming */
            m->touch_game_mode = "0";
            m->touch_sensitivity = "0";
            m->touch_edge = "0";
            break;

        case PROFILE_Gaming_MOBA:
        case PROFILE_Gaming: {
            m->devfreq_gov = "performance";
            m->lit_min_freq = g_nodes.lit_hw_min_freq;
            m->lit_max_freq = g_nodes.lit_hw_max_freq;
            m->big_min_freq = g_nodes.big_hw_min_freq;
            m->big_max_freq = g_nodes.big_hw_max_freq;
            m->up_rate_limit = "0";
            m->down_rate_limit = "30000";

            m->nr_requests = "256";
            m->read_ahead = (g_state.launch_boost_ticks > 0) ? "512" : "256";

            m->bg_shares = "256";
            m->bg_uclamp_min = "0";
            m->bg_uclamp_max = "40";
            m->top_app_shares = "2048";
            m->top_app_uclamp_min = "50";
            m->top_app_uclamp_max = "max";

            m->devfreq_poll_ms = (tier == 0) ? "20" : (tier == 1 ? "30" : "45");
            m->devfreq_upthresh = "50";
            m->devfreq_downdiff = "20";
            m->devfreq_min_freq = "390000000";
            m->devfreq_max_freq = max_gpu_hz;

            m->dvfsrc_qos = "1";

            m->boost_gpu_enable = "1";
            m->ged_smart_boost = "1";
            m->ged_boost_enable = "1";
            m->enable_gpu_boost = "1";
            m->gpu_cust_upbound_freq = max_gpu_khz;
            m->gpu_bottom_freq = "390000";

            if (prof == PROFILE_Gaming_MOBA) {
                m->g_fb_dvfs_threshold = (tier == 0) ? "25" : (tier == 1 ? "30" : "35");
                m->gx_fb_dvfs_margin    = (tier == 0) ? "30" : (tier == 1 ? "35" : "40");
            } else {
                m->g_fb_dvfs_threshold = (tier == 0) ? "30" : (tier == 1 ? "35" : "40");
                m->gx_fb_dvfs_margin    = (tier == 0) ? "50" : (tier == 1 ? "40" : "30");
            }

            m->gx_game_mode = "1";

            m->fpsgo_boost_ta = "1";
            m->fpsgo_ultra_rescue = "1";
            m->fpsgo_light_loading = "0";
            m->fpsgo_thrm_enable = "0";

            m->sconfig = "10";
            m->touch_thp_smooth = "1";
            m->touch_game_mode = "1";
            m->touch_sensitivity = "1";
            m->touch_edge = (prof == PROFILE_Gaming_MOBA) ? "0" : "1";

            break;
        }
    }
}

void apply_profile(profile_t prof, int tier, int gpu_load) {
    (void)gpu_load;
    profile_matrix_t m;
    build_profile_matrix(prof, tier, &m);

    set_cpu_governor(m.cpu_gov);
    set_cpu_freqs(m.lit_min_freq, m.lit_max_freq, m.big_min_freq, m.big_max_freq,
                  m.up_rate_limit, m.down_rate_limit);

    set_io_nr_requests(m.nr_requests);
    set_read_ahead(m.read_ahead);

    sysfs_write_fallback(s_cpuset_bg_cpus_nodes, m.bg_cpus);
    sysfs_write_fallback(s_cpuset_sys_bg_cpus_nodes, m.sys_bg_cpus);
    sysfs_write_fallback(s_cpuset_top_app_cpus_nodes, m.top_app_cpus);

    sysfs_write_fallback(s_cpuctl_bg_shares_nodes, m.bg_shares);
    sysfs_write_fallback(s_cpuctl_bg_uclamp_min_nodes, m.bg_uclamp_min);
    sysfs_write_fallback(s_cpuctl_bg_uclamp_max_nodes, m.bg_uclamp_max);
    sysfs_write_fallback(s_cpuctl_sys_bg_uclamp_max_nodes, m.sys_bg_uclamp_max);
    sysfs_write_fallback(s_cpuctl_top_app_shares_nodes, m.top_app_shares);
    sysfs_write_fallback(s_cpuctl_top_app_uclamp_min_nodes, m.top_app_uclamp_min);
    sysfs_write_fallback(s_cpuctl_top_app_uclamp_max_nodes, m.top_app_uclamp_max);

    sysfs_write_fallback(s_devfreq_gov_nodes, m.devfreq_gov);
    sysfs_write_fallback(s_devfreq_poll_nodes, m.devfreq_poll_ms);
    sysfs_write_fallback(s_devfreq_min_nodes, m.devfreq_min_freq);
    sysfs_write_fallback(s_devfreq_max_nodes, m.devfreq_max_freq);
    sysfs_write_fallback(s_power_policy_nodes, m.power_policy);
    set_mali_upthreshold(m.devfreq_upthresh);
    set_mali_downdifferential(m.devfreq_downdiff);

    sysfs_write_fallback(s_dvfsrc_nodes, m.dvfsrc_qos);

    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", m.boost_gpu_enable);
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", m.ged_smart_boost);
    sysfs_write("/sys/module/ged/parameters/ged_boost_enable", m.ged_boost_enable);
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", m.enable_gpu_boost);
    sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", m.gpu_cust_boost_freq);
    sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", m.gpu_cust_upbound_freq);
    sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", m.gpu_bottom_freq);
    sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", m.g_fb_dvfs_threshold);
    sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", m.gx_fb_dvfs_margin);
    sysfs_write_fallback(s_game_mode_nodes, m.gx_game_mode);

    sysfs_write("/sys/kernel/fpsgo/common/force_onoff", m.fpsgo_force_onoff);
    sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", m.fpsgo_boost_ta);
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", m.fpsgo_ultra_rescue);
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", m.fpsgo_light_loading);
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", m.fpsgo_idleprefer);
    sysfs_write_fallback(s_sconfig_nodes, m.sconfig);
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", m.fpsgo_thrm_enable);

    sysfs_write(g_nodes.touch_thp_smooth, m.touch_thp_smooth);
    sysfs_write(g_nodes.touch_game_mode, m.touch_game_mode);
    sysfs_write(g_nodes.touch_sensitivity, m.touch_sensitivity);
    sysfs_write(g_nodes.touch_edge, m.touch_edge);

    /* Re-apply IRQ affinity on every profile transition:
     * Gaming/MOBA -> big cores only (0xc0), others -> all cores (0xff) */
    apply_irq_tuning(prof);

    /* Toggle VM stat calculation frequency & dirty writeback interval for Sleep profile */
    set_memory_sleep_mode(prof == PROFILE_Sleep);

    /* Apply SurfaceFlinger latch_unsignaled at runtime via resetprop so it is NOT
     * persistent in system.prop (which triggers mBanking integrity scanners).
     * Enabled only during Gaming/MOBA for smoother frame delivery; disabled on exit. */
    static int s_prev_gaming_state = -1;
    int is_gaming = (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA);
    if (is_gaming != s_prev_gaming_state) {
        s_prev_gaming_state = is_gaming;
        if (is_gaming) {
            system("resetprop debug.sf.latch_unsignaled 1 2>/dev/null || true");
            system("resetprop persist.sys.wifi.low_latency 1 2>/dev/null || true");
        } else {
            system("resetprop debug.sf.latch_unsignaled 0 2>/dev/null || true");
            system("resetprop --delete persist.sys.wifi.low_latency 2>/dev/null || true");
        }
    }
}

int audit_active_profile_state(profile_t active_prof, int tier) {
    if ((int)active_prof < 0 || (int)active_prof >= 4) return 0;

    profile_matrix_t m;
    build_profile_matrix(active_prof, tier, &m);

    int tampered = 0;

    char curr_gov[64] = "";
    if (sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", curr_gov, sizeof(curr_gov))) {
        if (strcmp(curr_gov, m.cpu_gov) != 0) {
            log_warn("Guard", "Governor mutation detected [%s -> %s]! Re-enforcing...", curr_gov, m.cpu_gov);
            tampered = 1;
        }
    }

    char curr_mali[64] = "";
    if (read_mali_power_policy(curr_mali, sizeof(curr_mali))) {
        if (strstr(curr_mali, m.power_policy) == NULL) {
            log_warn("Guard", "GPU power_policy mutation detected! Re-enforcing...");
            tampered = 1;
        }
    }

    char curr_gpu_gov[64] = "";
    if (read_mali_governor(curr_gpu_gov, sizeof(curr_gpu_gov))) {
        if (strstr(curr_gpu_gov, m.devfreq_gov) == NULL) {
            log_warn("Guard", "GPU governor mutation detected! Re-enforcing...");
            tampered = 1;
        }
    }

    if (tampered) {
        apply_profile(active_prof, tier, 0);
    }

    return tampered;
}

void reset_to_interactive_baseline(void) {
    apply_profile(PROFILE_Interactive, 0, 0);
}
