
#include "cpu.hpp"
#include "memory.hpp"
#include "io.hpp"

void set_rate_limits(int cpu, const char *up, const char *down) {
    char path[256];
    if (g_nodes.has_sugov_ext) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/up_rate_limit_us", cpu);
        sysfs_write(path, up);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/down_rate_limit_us", cpu);
        sysfs_write(path, down);
    }
    if (g_nodes.has_schedutil) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us", down);
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

        set_rate_limits(i, up_rate, down_rate);
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

        set_rate_limits(i, up_rate, down_rate);
    }
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
        sysfs_write("/dev/cpuctl/background/cpu.shares", "512");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.min", "0");
        sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "40");
        sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "40");

        sysfs_write("/dev/cpuset/top-app/cpus", "0-7");
        sysfs_write("/dev/cpuctl/top-app/cpu.shares", "1024");
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", "30");
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
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", "0");
        sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
    }
}

static const char *get_best_governor(void) {
    char avail[256] = "";
    if (sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors", avail, sizeof(avail))) {
        if (strstr(avail, "reflex")) return "reflex";
        if (strstr(avail, "sugov_ext")) return "sugov_ext";
    }
    return "schedutil";
}

void apply_profile(profile_t prof, int tier, int gpu_load) {
    const char *gov = get_best_governor();

    apply_cgroup_gaming_policy(prof == PROFILE_Gaming);

    switch (prof) {
        case PROFILE_Sleep: {
            set_cpu_governor(gov);
            set_cpu_freqs(500000, 1200000, 725000, 1000000, "5000", "500");
            set_io_nr_requests("32");
            sysfs_write("/dev/cpuset/background/cpus", "0-3");
            sysfs_write("/dev/cpuset/system-background/cpus", "0-3");
            sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "20");
            sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "20");
            sysfs_write("/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode", "0");
            sysfs_write("/sys/devices/platform/soc/13000000.mali/power_policy", "coarse_demand");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "400000");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
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
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "50");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write("/sys/class/thermal/thermal_message/sconfig", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");
            sysfs_write(g_nodes.touch_thp_smooth, "0");
            sysfs_write(g_nodes.touch_edge, "0");
            sysfs_write(g_nodes.charge_control, "8");
            break;
        }

        case PROFILE_Interactive: {
            int big_max = (tier >= 3) ? 2000000 : FREQ_BIG_MAX;
            set_cpu_governor(gov);
            set_cpu_freqs(600000, FREQ_LITTLE_MAX, 1000000, big_max, "500", "20000");
            set_io_nr_requests("64");
            sysfs_write("/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode", "0");
            sysfs_write("/sys/devices/platform/soc/13000000.mali/power_policy", "coarse_demand");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "0");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "0");
            const char *interactive_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(interactive_game_mode_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write("/sys/class/thermal/thermal_message/sconfig", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");
            sysfs_write(g_nodes.touch_thp_smooth, "0");
            sysfs_write(g_nodes.touch_edge, "0");
            sysfs_write(g_nodes.charge_control, "8");
            break;
        }

        case PROFILE_Gaming: {
            int is_heavy_gpu = (gpu_load >= 20);
            const char *gpu_freq = is_heavy_gpu ? ((tier >= 3) ? "500000" : "600000") : "300000";
            const char *dvfs_margin = is_heavy_gpu ? "40" : "20";
            int big_min = (tier >= 3) ? 1600000 : 1800000;
            set_cpu_governor(gov);
            set_cpu_freqs(1400000, FREQ_LITTLE_MAX, big_min, FREQ_BIG_MAX, "0", "30000");
            set_io_nr_requests("128");
            sysfs_write("/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode", "1");
            sysfs_write("/sys/devices/platform/soc/13000000.mali/power_policy", "coarse_demand");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", gpu_freq);
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "25");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", dvfs_margin);
            const char *gaming_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(gaming_game_mode_nodes, "1");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write("/sys/class/thermal/thermal_message/sconfig", "10");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");
            sysfs_write(g_nodes.touch_thp_smooth, "1");
            sysfs_write(g_nodes.touch_edge, "1");

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

        case PROFILE_Thermal: {
            set_cpu_governor(gov);
            set_cpu_freqs(600000, FREQ_LITTLE_MAX, 900000, 1600000, "2000", "5000");
            set_io_nr_requests("64");
            sysfs_write("/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode", "0");
            sysfs_write("/sys/devices/platform/soc/13000000.mali/power_policy", "coarse_demand");
            sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
            sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "500000");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
            sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "0");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "0");
            const char *thermal_game_mode_nodes[] = {
                "/sys/module/ged/parameters/gx_game_mode",
                "/sys/module/ged/parameters/gx_boost_on",
                NULL
            };
            sysfs_write_fallback(thermal_game_mode_nodes, "0");
            sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "2");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "20");
            sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
            sysfs_write("/sys/class/thermal/thermal_message/sconfig", "0");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");
            sysfs_write(g_nodes.touch_thp_smooth, "0");
            sysfs_write(g_nodes.touch_edge, "0");
            sysfs_write(g_nodes.charge_control, "8");
            break;
        }
    }
}
