#!/system/bin/sh

pkill -9 -x libhypercore.so >/dev/null 2>&1

# Reset CPU scaling governor & frequencies
for p in /sys/devices/system/cpu/cpufreq/policy*/scaling_governor; do
    [ -f "$p" ] && echo schedutil > "$p" 2>/dev/null
done
for i in 0 1 2 3 4 5; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo 500000 > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done
for i in 6 7; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo 725000 > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done

# Reset Mali GPU power policy & devfreq governor
for p in /sys/devices/platform/soc/*.mali/power_policy /sys/class/devfreq/*.mali/power_policy; do
    [ -f "$p" ] && echo coarse_demand > "$p" 2>/dev/null
done
for g in /sys/class/devfreq/*.mali/governor /sys/devices/platform/soc/*.mali/devfreq/*.mali/governor; do
    [ -f "$g" ] && echo simple_ondemand > "$g" 2>/dev/null
done
for i in /sys/class/devfreq/*.mali/polling_interval /sys/devices/platform/soc/*.mali/devfreq/*.mali/polling_interval; do
    [ -f "$i" ] && echo 50 > "$i" 2>/dev/null
done

# Reset GED & FPSGO thermal drivers
[ -f "/sys/module/ged/parameters/boost_gpu_enable" ] && echo 0 > /sys/module/ged/parameters/boost_gpu_enable 2>/dev/null
[ -f "/sys/module/ged/parameters/enable_gpu_boost" ] && echo 0 > /sys/module/ged/parameters/enable_gpu_boost 2>/dev/null
[ -f "/sys/kernel/fpsgo/fbt/thrm_enable" ] && echo 1 > /sys/kernel/fpsgo/fbt/thrm_enable 2>/dev/null
[ -f "/sys/kernel/fpsgo/fbt/ultra_rescue" ] && echo 0 > /sys/kernel/fpsgo/fbt/ultra_rescue 2>/dev/null
[ -f "/sys/class/thermal/thermal_message/sconfig" ] && echo 0 > /sys/class/thermal/thermal_message/sconfig 2>/dev/null

# Clean up symlinks & lock files
rm -f /data/adb/ap/bin/libhypercore.so /data/adb/ksu/bin/libhypercore.so /data/adb/modules/bin/libhypercore.so 2>/dev/null
rm -f /data/adb/modules/hypercore/.hypercore_lock /data/adb/hypercore/hypercore.sock /data/adb/hypercore/hypercore.pid 2>/dev/null
rm -f /data/local/tmp/.hypercore_lock /data/local/tmp/hypercore.sock /data/local/tmp/hypercore.pid 2>/dev/null
rm -f /dev/hypercore.sock 2>/dev/null
rm -f /sdcard/Android/.hypercore_lock 2>/dev/null
rm -f /sdcard/Android/hypercore.log /data/adb/hypercore/hypercore.log 2>/dev/null
