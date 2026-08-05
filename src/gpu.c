#include "gpu.hpp"

static const char *s_mali_poll_nodes[] = {
    "/sys/class/devfreq/13000000.mali/polling_interval",
    "/sys/class/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/polling_interval",
    NULL
};

static const char *s_mali_gov_nodes[] = {
    "/sys/class/devfreq/13000000.mali/governor",
    "/sys/class/devfreq/soc:mali/governor",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
    NULL
};

static const char *s_mali_upthreshold_nodes[] = {
    "/sys/class/devfreq/13000000.mali/simple_ondemand/upthreshold",
    "/sys/class/devfreq/soc:mali/simple_ondemand/upthreshold",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/upthreshold",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/upthreshold",
    NULL
};

static const char *s_mali_downdiff_nodes[] = {
    "/sys/class/devfreq/13000000.mali/simple_ondemand/downdifferential",
    "/sys/class/devfreq/soc:mali/simple_ondemand/downdifferential",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/downdifferential",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/downdifferential",
    NULL
};

static const char *s_mali_min_freq_nodes[] = {
    "/sys/class/devfreq/13000000.mali/min_freq",
    "/sys/class/devfreq/soc:mali/min_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
    NULL
};

static const char *s_mali_max_freq_nodes[] = {
    "/sys/class/devfreq/13000000.mali/max_freq",
    "/sys/class/devfreq/soc:mali/max_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
    NULL
};

static const char *s_mali_power_policy_nodes[] = {
    "/sys/devices/platform/soc/13000000.mali/power_policy",
    "/sys/devices/platform/soc/soc:mali/power_policy",
    "/sys/class/devfreq/13000000.mali/power_policy",
    "/sys/class/devfreq/soc:mali/power_policy",
    NULL
};

void enforce_interactive_gpu_polling(int target_poll_ms) {
    char poll_buf[16];
    snprintf(poll_buf, sizeof(poll_buf), "%d", target_poll_ms);

    // 1. Ensure GPU governor is simple_ondemand
    sysfs_write_fallback(s_mali_gov_nodes, "simple_ondemand");

    // 2. Enforce polling_interval across all multi-OEM candidate nodes
    for (int i = 0; s_mali_poll_nodes[i]; i++) {
        if (access(s_mali_poll_nodes[i], W_OK) == 0) {
            int current_poll = sysfs_read_int(s_mali_poll_nodes[i]);
            if (current_poll != target_poll_ms) {
                sysfs_write(s_mali_poll_nodes[i], poll_buf);
            }
        }
    }

    // 3. Configure aggressive upthreshold (50%) and downdifferential (10%)
    sysfs_write_fallback(s_mali_upthreshold_nodes, "50");
    sysfs_write_fallback(s_mali_downdiff_nodes, "10");

    // 4. Ensure frequency bounds are fully open (min 390MHz, max 1.003GHz)
    sysfs_write_fallback(s_mali_min_freq_nodes, "390000000");
    sysfs_write_fallback(s_mali_max_freq_nodes, "1003000000");

    // 5. Ensure power_policy is coarse_demand
    sysfs_write_fallback(s_mali_power_policy_nodes, "coarse_demand");

    // 6. Unlock MediaTek GED frequency bounds so GPU scales freely up to 1003 MHz
    sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "1003000");
    sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
    sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
}

void apply_gpu_tuning(void) {
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");

    const char *game_mode_nodes[] = {
        "/sys/module/ged/parameters/gx_game_mode",
        "/sys/module/ged/parameters/gx_boost_on",
        NULL
    };
    sysfs_write_fallback(game_mode_nodes, "0");

    enforce_interactive_gpu_polling(20);
}
