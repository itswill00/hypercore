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

#include "log.hpp"

static char s_max_gpu_freq_hz[32]  = "1003000000";
static char s_max_gpu_freq_khz[32] = "1003000";
static int  s_gpu_freq_detected     = 0;

void detect_max_gpu_freq(void) {
    if (s_gpu_freq_detected) return;

    const char *avail_nodes[] = {
        "/sys/class/devfreq/13000000.mali/available_frequencies",
        "/sys/class/devfreq/soc:mali/available_frequencies",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/available_frequencies",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/available_frequencies",
        NULL
    };

    long max_freq = 0;

    for (int i = 0; avail_nodes[i]; i++) {
        FILE *f = fopen(avail_nodes[i], "r");
        if (f) {
            long freq;
            while (fscanf(f, "%ld", &freq) == 1) {
                if (freq > max_freq) {
                    max_freq = freq;
                }
            }
            fclose(f);
            if (max_freq > 0) break;
        }
    }

    if (max_freq <= 0) {
        for (int i = 0; s_mali_max_freq_nodes[i]; i++) {
            long val = (long)sysfs_read_int(s_mali_max_freq_nodes[i]);
            if (val > max_freq) max_freq = val;
        }
    }

    if (max_freq >= 800000000L && max_freq <= 2500000000L) {
        snprintf(s_max_gpu_freq_hz, sizeof(s_max_gpu_freq_hz), "%ld", max_freq);
        snprintf(s_max_gpu_freq_khz, sizeof(s_max_gpu_freq_khz), "%ld", max_freq / 1000L);
        log_info("GPU", "Detected Hardware Max GPU Frequency: %ld Hz (%ld MHz)", max_freq, max_freq / 1000000L);
    } else if (max_freq >= 800000L && max_freq <= 2500000L) {
        snprintf(s_max_gpu_freq_hz, sizeof(s_max_gpu_freq_hz), "%ld", max_freq * 1000L);
        snprintf(s_max_gpu_freq_khz, sizeof(s_max_gpu_freq_khz), "%ld", max_freq);
        log_info("GPU", "Detected Hardware Max GPU Frequency: %ld kHz (%ld MHz)", max_freq, max_freq / 1000L);
    } else {
        log_info("GPU", "Defaulting Max GPU Frequency to 1003000000 Hz (1003 MHz)");
    }

    s_gpu_freq_detected = 1;
}

const char *get_max_gpu_freq_hz(void) {
    if (!s_gpu_freq_detected) detect_max_gpu_freq();
    return s_max_gpu_freq_hz;
}

const char *get_max_gpu_freq_khz(void) {
    if (!s_gpu_freq_detected) detect_max_gpu_freq();
    return s_max_gpu_freq_khz;
}

void enforce_interactive_gpu_polling(int target_poll_ms) {
    detect_max_gpu_freq();

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

    // 2. Check if GED frequency cap was imposed below detected hardware max
    int cust_upbound = sysfs_read_int("/sys/module/ged/parameters/gpu_cust_upbound_freq");
    int max_khz = atoi(get_max_gpu_freq_khz());
    if (cust_upbound > 0 && cust_upbound < max_khz) {
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
    sysfs_write_fallback(s_mali_max_freq_nodes, get_max_gpu_freq_hz());
    sysfs_write_fallback(s_mali_power_policy_nodes, "coarse_demand");

    sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", get_max_gpu_freq_khz());
    sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", "0");
    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "0");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "0");
    sysfs_write("/sys/module/ged/parameters/ged_boost_enable", "0");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "0");
}

int read_mali_power_policy(char *out_buf, size_t max_len) {
    return sysfs_read_str_fallback(s_mali_power_policy_nodes, out_buf, max_len);
}

int read_mali_governor(char *out_buf, size_t max_len) {
    return sysfs_read_str_fallback(s_mali_gov_nodes, out_buf, max_len);
}

void apply_gpu_tuning(void) {
    detect_max_gpu_freq();

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
