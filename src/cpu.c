/*
 * HyperCore - CPU Governor & Scaling Tuning Implementation
 * Author: @itswill00
 */

#include "cpu.hpp"

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
}

void set_cpu_freqs(int min_lit, int max_lit, int min_big, int max_big, const char *up_rate, const char *down_rate) {
    char path[256], buf[32];

    for (int i = 0; i <= 5; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        snprintf(buf, sizeof(buf), "%d", min_lit);
        sysfs_write(path, buf);

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        snprintf(buf, sizeof(buf), "%d", max_lit);
        sysfs_write(path, buf);

        set_rate_limits(i, up_rate, down_rate);
    }

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

void apply_profile(profile_t prof, int tier) {
    switch (prof) {
        case PROFILE_SLEEP:
            set_cpu_freqs(500000, 1450000, 725000, 1300000, "2000", "1000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
            break;

        case PROFILE_INTERACTIVE: {
            int big_max = (tier >= 3) ? 2000000 : FREQ_BIG_MAX;
            set_cpu_freqs(900000, FREQ_LITTLE_MAX, 1100000, big_max, "0", "10000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "550000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "12");
            break;
        }

        case PROFILE_TOUCH:
            set_cpu_freqs(1200000, FREQ_LITTLE_MAX, 1500000, FREQ_BIG_MAX, "0", "15000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "650000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
            break;

        case PROFILE_GAMING: {
            const char *gpu_freq = (tier >= 3) ? "700000" : "800000";
            int big_min = (tier >= 3) ? 1600000 : 1800000;
            set_cpu_freqs(1400000, FREQ_LITTLE_MAX, big_min, FREQ_BIG_MAX, "0", "30000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", gpu_freq);
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "25");
            break;
        }

        case PROFILE_THERMAL:
            set_cpu_freqs(900000, FREQ_LITTLE_MAX, 1200000, 1800000, "2000", "5000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "500000");
            break;
    }
}
