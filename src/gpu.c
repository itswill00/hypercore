/*
 * HyperCore - GPU DVFS & FPSGO Tuning Implementation
 * Author: @itswill00
 */

#include "gpu.hpp"

void apply_gpu_tuning(void) {
    // FPSGO Governor & Frame Rescue Policy
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");

    // MediaTek GED GPU Governor Policy
    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
    sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
    sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "40");

    const char *game_mode_nodes[] = {
        "/sys/module/ged/parameters/gx_game_mode",
        "/sys/module/ged/parameters/gx_boost_on",
        NULL
    };
    sysfs_write_fallback(game_mode_nodes, "1");

    // ARM Mali-G57 MC2 Coarse Demand Power Policy
    sysfs_write("/sys/devices/platform/soc/13000000.mali/power_policy", "coarse_demand");
}
