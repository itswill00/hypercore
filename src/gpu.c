/*
 * HyperCore - GPU DVFS & FPSGO Tuning Implementation
 * Author: @itswill00
 */

#include "gpu.hpp"

void apply_gpu_tuning(void) {
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");

    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
    sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "12");
    sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "40");
}
