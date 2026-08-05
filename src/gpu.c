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
    // 1. Read-Before-Write State Guard: Check if polling_interval matches target
    int needs_enforce = 0;
    for (int i = 0; s_mali_poll_nodes[i]; i++) {
        if (access(s_mali_poll_nodes[i], R_OK) == 0) {
            int current_poll = sysfs_read_int(s_mali_poll_nodes[i]);
            if (current_poll != target_poll_ms) {
                needs_enforce = 1;
            }
            break;
        }
    }

    // 2. Check if GED frequency cap was imposed
    int cust_upbound = sysfs_read_int("/sys/module/ged/parameters/gpu_cust_upbound_freq");
    if (cust_upbound > 0 && cust_upbound < 1003000) {
        needs_enforce = 1;
    }

    // Fast-path return: 0ms overhead if hardware parameters are already optimal
    if (!needs_enforce) return;

    // Apply enforcement writes only on state mismatch
    char poll_buf[16];
    snprintf(poll_buf, sizeof(poll_buf), "%d", target_poll_ms);

    sysfs_write_fallback(s_mali_gov_nodes, "simple_ondemand");
    sysfs_write_fallback(s_mali_poll_nodes, poll_buf);
    sysfs_write_fallback(s_mali_upthreshold_nodes, "50");
    sysfs_write_fallback(s_mali_downdiff_nodes, "10");
    sysfs_write_fallback(s_mali_min_freq_nodes, "390000000");
    sysfs_write_fallback(s_mali_max_freq_nodes, "1003000000");
    sysfs_write_fallback(s_mali_power_policy_nodes, "coarse_demand");

    sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", "1003000");
    sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
    sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
}

int read_mali_power_policy(char *out_buf, size_t max_len) {
    return sysfs_read_str_fallback(s_mali_power_policy_nodes, out_buf, max_len);
}

int read_mali_governor(char *out_buf, size_t max_len) {
    return sysfs_read_str_fallback(s_mali_gov_nodes, out_buf, max_len);
}

void apply_gpu_tuning(void) {
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");

    const char *game_mode_nodes[] = {
        "/sys/module/ged/parameters/gx_game_mode",
        "/sys/module/ged/parameters/gx_boost_on",
        NULL
    };
    sysfs_write_fallback(game_mode_nodes, "0");

    enforce_interactive_gpu_polling(20);
}
