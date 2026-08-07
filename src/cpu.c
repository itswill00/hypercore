#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "log.hpp"

void detect_cpu_hardware_limits(void) {
    /* 1. Capture boot scaling governor to preserve custom kernel developer tweaks */
    if (g_nodes.boot_cpu_gov[0] == '\0') {
        if (!sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", g_nodes.boot_cpu_gov, sizeof(g_nodes.boot_cpu_gov))) {
            strcpy(g_nodes.boot_cpu_gov, "schedutil");
        }
        log_info("Hardware", "Boot CPU Scaling Governor captured: '%s'", g_nodes.boot_cpu_gov);
    }

    /* 2. Read hardware min/max freq for policy0 (Little Cores) */
    int lit_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq");
    int lit_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq");
    g_nodes.lit_hw_min_freq = (lit_min > 0) ? lit_min : 500000;
    g_nodes.lit_hw_max_freq = (lit_max > 0) ? lit_max : FREQ_LITTLE_MAX;

    /* 3. Read hardware min/max freq for policy6 (Big Cores) */
    int big_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_min_freq");
    int big_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_max_freq");
    g_nodes.big_hw_min_freq = (big_min > 0) ? big_min : 725000;
    g_nodes.big_hw_max_freq = (big_max > 0) ? big_max : FREQ_BIG_MAX;

    log_info("Hardware", "CPU Freq Bounds - Little: [%d - %d kHz], Big: [%d - %d kHz]",
             g_nodes.lit_hw_min_freq, g_nodes.lit_hw_max_freq,
             g_nodes.big_hw_min_freq, g_nodes.big_hw_max_freq);
}

void set_rate_limits(const char *up, const char *down) {
    if (g_nodes.has_sugov_ext) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us", down);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us", down);
    }
    if (g_nodes.has_schedutil) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us", down);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us", down);
    }
}

void apply_cpuset(void) {
    if (access("/dev/cpuset", F_OK) == 0) {
        sysfs_write("/dev/cpuset/top-app/cpus", "0-7");
        sysfs_write("/dev/cpuset/foreground/cpus", "0-7");
        sysfs_write("/dev/cpuset/background/cpus", "0-3");
        sysfs_write("/dev/cpuset/system-background/cpus", "0-3");
    }

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
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_min_freq", buf);
    snprintf(buf, sizeof(buf), "%d", max_big);
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq", buf);

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
    sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", gov);

    for (int i = 0; i <= 7; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        sysfs_write(path, gov);
    }
}

void apply_cgroup_gaming_policy(int enable) {
    if (enable) {
        sysfs_write("/dev/cpuset/background/cpus", "0-3");
        sysfs_write("/dev/cpuset/system-background/cpus", "0-3");
        sysfs_write("/dev/cpuctl/background/cpu.shares", "256");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.min", "0");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "40");
        sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "max");

        sysfs_write("/dev/cpuset/top-app/cpus", "0-7");
        sysfs_write("/dev/cpuctl/top-app/cpu.shares", "2048");
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", "50");
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
    } else {
        sysfs_write("/dev/cpuset/background/cpus", "0-3");
        sysfs_write("/dev/cpuset/system-background/cpus", "0-3");
        sysfs_write("/dev/cpuctl/background/cpu.shares", "1024");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.min", "0");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "max");
        sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "max");

        sysfs_write("/dev/cpuset/top-app/cpus", "0-7");
        sysfs_write("/dev/cpuctl/top-app/cpu.shares", "1024");
        const char *uclamp_min = (g_state.app_boost_ticks > 0) ? "25" : "0";
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", uclamp_min);
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
    }
}

static const char *get_best_governor(profile_t prof) {
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
        /* For Interactive/Sleep: Preserve custom kernel boot governor if it's non-standard */
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

void apply_profile(profile_t prof, int tier, int gpu_load) {
    (void)gpu_load;
    const char *gov = get_best_governor(prof);

    apply_cgroup_gaming_policy(prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA);

    switch (prof) {
        case PROFILE_Sleep: {
            g_state.app_boost_ticks = 0;
            set_cpu_governor(gov);
            set_cpu_freqs(g_nodes.lit_hw_min_freq, 1000000, g_nodes.big_hw_min_freq, 850000, "10000", "500");
            set_io_nr_requests("32");
            set_read_ahead("128");
            sysfs_write("/dev/cpuset/background/cpus", "0-1");
            sysfs_write("/dev/cpuset/system-background/cpus", "0-1");
            sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "20");
            sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "30");
            sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", "0");
            sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.max", "40");
            const char *devfreq_gov_nodes[] = {
                "/sys/class/devfreq/13000000.mali/governor",
                "/sys/class/devfreq/soc:mali/governor",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
                "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/governor",
                NULL
            };
            const char *devfreq_min_nodes[] = {
                "/sys/class/devfreq/13000000.mali/min_freq",
                "/sys/class/devfreq/soc:mali/min_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
                NULL
            };
            const char *devfreq_max_nodes[] = {
                "/sys/class/devfreq/13000000.mali/max_freq",
                "/sys/class/devfreq/soc:mali/max_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
                NULL
            };
            const char *devfreq_poll_nodes[] = {
                "/sys/class/devfreq/13000000.mali/polling_interval",
                "/sys/class/devfreq/soc:mali/polling_interval",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
                NULL
            };
            const char *power_policy_nodes[] = {
                "/sys/devices/platform/soc/13000000.mali/power_policy",
                "/sys/devices/platform/soc/soc:mali/power_policy",
                "/sys/class/devfreq/13000000.mali/power_policy",
                "/sys/class/devfreq/soc:mali/power_policy",
                NULL
            };
            const char *sconfig_nodes[] = {
                "/sys/class/thermal/thermal_message/sconfig",
                "/sys/devices/virtual/thermal/thermal_message/sconfig",
                "/sys/class/thermal/thermal_message/sport_mode",
                NULL
            };

            const char *dvfsrc_nodes[] = {
                "/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/module/dvfsrc/parameters/dvfsrc_qos_mode",
                "/sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                NULL
            };

            sysfs_write_fallback(devfreq_gov_nodes, "simple_ondemand");
            sysfs_write_fallback(devfreq_poll_nodes, "100");
            sysfs_write_fallback(devfreq_min_nodes, "390000000");
            sysfs_write_fallback(devfreq_max_nodes, "390000000");
            sysfs_write_fallback(power_policy_nodes, "coarse_demand");
            sysfs_write_fallback(dvfsrc_nodes, "0");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "0");
            sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "390000");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "390000");
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "0");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "0");
            const char *sleep_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(sleep_game_mode_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "50");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write_fallback(sconfig_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");
            sysfs_write(g_nodes.touch_thp_smooth, "0");
            sysfs_write(g_nodes.touch_edge, "0");
            sysfs_write(g_nodes.charge_control, "8");
            break;
        }

        case PROFILE_Interactive: {
            int big_max = (tier >= 3) ? 2000000 : g_nodes.big_hw_max_freq;
            int big_min = (g_state.app_boost_ticks > 0) ? 1400000 : 725000;
            set_cpu_governor(gov);
            set_cpu_freqs(g_nodes.lit_hw_min_freq, g_nodes.lit_hw_max_freq, big_min, big_max, "200", "40000");
            set_io_nr_requests("128");
            const char *devfreq_gov_nodes[] = {
                "/sys/class/devfreq/13000000.mali/governor",
                "/sys/class/devfreq/soc:mali/governor",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
                "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/governor",
                NULL
            };
            const char *devfreq_min_nodes[] = {
                "/sys/class/devfreq/13000000.mali/min_freq",
                "/sys/class/devfreq/soc:mali/min_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
                NULL
            };
            const char *devfreq_max_nodes[] = {
                "/sys/class/devfreq/13000000.mali/max_freq",
                "/sys/class/devfreq/soc:mali/max_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
                NULL
            };
            const char *devfreq_poll_nodes[] = {
                "/sys/class/devfreq/13000000.mali/polling_interval",
                "/sys/class/devfreq/soc:mali/polling_interval",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
                NULL
            };
            const char *power_policy_nodes[] = {
                "/sys/devices/platform/soc/13000000.mali/power_policy",
                "/sys/devices/platform/soc/soc:mali/power_policy",
                "/sys/class/devfreq/13000000.mali/power_policy",
                "/sys/class/devfreq/soc:mali/power_policy",
                NULL
            };
            const char *sconfig_nodes[] = {
                "/sys/class/thermal/thermal_message/sconfig",
                "/sys/devices/virtual/thermal/thermal_message/sconfig",
                "/sys/class/thermal/thermal_message/sport_mode",
                NULL
            };

            const char *devfreq_upthreshold_nodes[] = {
                "/sys/class/devfreq/13000000.mali/simple_ondemand/upthreshold",
                "/sys/class/devfreq/soc:mali/simple_ondemand/upthreshold",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/upthreshold",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/upthreshold",
                NULL
            };
            const char *devfreq_downdifferential_nodes[] = {
                "/sys/class/devfreq/13000000.mali/simple_ondemand/downdifferential",
                "/sys/class/devfreq/soc:mali/simple_ondemand/downdifferential",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/downdifferential",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/downdifferential",
                NULL
            };

            const char *dvfsrc_nodes[] = {
                "/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/module/dvfsrc/parameters/dvfsrc_qos_mode",
                "/sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                NULL
            };

            sysfs_write_fallback(devfreq_gov_nodes, "simple_ondemand");
            sysfs_write_fallback(devfreq_poll_nodes, "50");
            sysfs_write_fallback(devfreq_upthreshold_nodes, "65");
            sysfs_write_fallback(devfreq_downdifferential_nodes, "20");
            sysfs_write_fallback(devfreq_min_nodes, "390000000");
            sysfs_write_fallback(devfreq_max_nodes, get_max_gpu_freq_hz());
            sysfs_write_fallback(power_policy_nodes, "coarse_demand");
            sysfs_write_fallback(dvfsrc_nodes, "0");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
            sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "0");
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", get_max_gpu_freq_khz());
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "390000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "10");
            const char *interactive_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(interactive_game_mode_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "50");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write_fallback(sconfig_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");
            sysfs_write(g_nodes.touch_thp_smooth, "0");
            sysfs_write(g_nodes.touch_edge, "0");
            sysfs_write(g_nodes.charge_control, "8");
            break;
        }

        case PROFILE_Gaming_MOBA:
        case PROFILE_Gaming: {
            set_cpu_governor(gov);
            set_cpu_freqs(1400000, g_nodes.lit_hw_max_freq, 1800000, g_nodes.big_hw_max_freq, "200", "30000");
            set_io_nr_requests("256");
            const char *devfreq_gov_nodes[] = {
                "/sys/class/devfreq/13000000.mali/governor",
                "/sys/class/devfreq/soc:mali/governor",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
                "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/governor",
                NULL
            };
            const char *devfreq_min_nodes[] = {
                "/sys/class/devfreq/13000000.mali/min_freq",
                "/sys/class/devfreq/soc:mali/min_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
                NULL
            };
            const char *devfreq_max_nodes[] = {
                "/sys/class/devfreq/13000000.mali/max_freq",
                "/sys/class/devfreq/soc:mali/max_freq",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
                NULL
            };
            const char *devfreq_poll_nodes[] = {
                "/sys/class/devfreq/13000000.mali/polling_interval",
                "/sys/class/devfreq/soc:mali/polling_interval",
                "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
                "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
                NULL
            };
            const char *power_policy_nodes[] = {
                "/sys/devices/platform/soc/13000000.mali/power_policy",
                "/sys/devices/platform/soc/soc:mali/power_policy",
                "/sys/class/devfreq/13000000.mali/power_policy",
                "/sys/class/devfreq/soc:mali/power_policy",
                NULL
            };
            const char *sconfig_nodes[] = {
                "/sys/class/thermal/thermal_message/sconfig",
                "/sys/devices/virtual/thermal/thermal_message/sconfig",
                "/sys/class/thermal/thermal_message/sport_mode",
                NULL
            };
            const char *dvfsrc_nodes[] = {
                "/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                "/sys/module/dvfsrc/parameters/dvfsrc_qos_mode",
                "/sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
                NULL
            };

            sysfs_write_fallback(devfreq_gov_nodes, "simple_ondemand");
            sysfs_write_fallback(devfreq_poll_nodes, "20");
            sysfs_write_fallback(devfreq_min_nodes, "674000000");
            sysfs_write_fallback(devfreq_max_nodes, get_max_gpu_freq_hz());
            sysfs_write_fallback(power_policy_nodes, "always_on");
            sysfs_write_fallback(dvfsrc_nodes, "1");

            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
            sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "1");
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", get_max_gpu_freq_khz());
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "674000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", (prof == PROFILE_Gaming_MOBA) ? "25" : "30");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", (prof == PROFILE_Gaming_MOBA) ? "30" : "50");
            const char *gaming_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(gaming_game_mode_nodes, "1");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write_fallback(sconfig_nodes, "10");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");
            sysfs_write(g_nodes.touch_thp_smooth, "1");
            sysfs_write(g_nodes.touch_edge, (prof == PROFILE_Gaming_MOBA) ? "0" : "1");

            if (g_state.is_charging) {
                int bat_temp = sysfs_read_int(g_nodes.bat_temp);
                if (bat_temp > 1000) bat_temp /= 1000;
                else if (bat_temp > 100) bat_temp /= 10;

                if (bat_temp >= 42) {
                    sysfs_write(g_nodes.charge_control, "2");
                } else if (bat_temp >= 37) {
                    sysfs_write(g_nodes.charge_control, "4");
                } else {
                    sysfs_write(g_nodes.charge_control, "8");
                }
            } else {
                sysfs_write(g_nodes.charge_control, "8");
            }
            break;
        }
    }
}
