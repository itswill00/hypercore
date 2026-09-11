#!/system/bin/sh

pkill -9 -x hypercore_daemon >/dev/null 2>&1 || true
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
pkill -9 -x hypermoon_daemon >/dev/null 2>&1 || true
pkill -9 -f com.hypermoon.HyperMoonOverlay >/dev/null 2>&1 || true

# Restore factory stock baseline from stock_state.conf if available
STOCK_CONF="/data/adb/hypercore/stock_state.conf"
STOCK_GOV0=""
STOCK_GOV6=""
STOCK_LIT_MIN=""
STOCK_BIG_MIN=""
STOCK_MALI_POL=""
STOCK_MALI_GOV=""
STOCK_MALI_POLL=""
STOCK_SCONFIG=""

if [ -f "$STOCK_CONF" ]; then
    STOCK_GOV0=$(grep '^gov0=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_GOV6=$(grep '^gov6=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_LIT_MIN=$(grep '^lit_min_freq=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_BIG_MIN=$(grep '^big_min_freq=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_MALI_POL=$(grep '^mali_policy=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_MALI_GOV=$(grep '^mali_gpu_gov=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_MALI_POLL=$(grep '^mali_poll_int=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
    STOCK_SCONFIG=$(grep '^sconfig=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2)
fi

# Reset CPU scaling governor & frequencies
[ -z "$STOCK_GOV0" ] && STOCK_GOV0="schedutil"
[ -z "$STOCK_GOV6" ] && STOCK_GOV6="schedutil"
[ -f /sys/devices/system/cpu/cpufreq/policy0/scaling_governor ] && echo "$STOCK_GOV0" > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor 2>/dev/null
[ -f /sys/devices/system/cpu/cpufreq/policy6/scaling_governor ] && echo "$STOCK_GOV6" > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor 2>/dev/null

[ -z "$STOCK_LIT_MIN" ] && STOCK_LIT_MIN="500000"
for i in 0 1 2 3 4 5; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo "$STOCK_LIT_MIN" > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done

[ -z "$STOCK_BIG_MIN" ] && STOCK_BIG_MIN="725000"
for i in 6 7; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo "$STOCK_BIG_MIN" > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done

# Restore CPU governor rate limit permissions
chmod 644 /sys/devices/system/cpu/cpufreq/policy*/sugov_ext/*rate_limit_us \
          /sys/devices/system/cpu/cpufreq/policy*/schedutil/*rate_limit_us \
          /sys/devices/system/cpu/cpufreq/policy*/rate_limit_us 2>/dev/null || true

# Reset Mali GPU power policy & devfreq governor
[ -z "$STOCK_MALI_POL" ] && STOCK_MALI_POL="coarse_demand"
for p in /sys/devices/platform/soc/*.mali/power_policy /sys/class/devfreq/*.mali/power_policy; do
    [ -f "$p" ] && echo "$STOCK_MALI_POL" > "$p" 2>/dev/null
done
[ -z "$STOCK_MALI_GOV" ] && STOCK_MALI_GOV="simple_ondemand"
for g in /sys/class/devfreq/*.mali/governor /sys/devices/platform/soc/*.mali/devfreq/*.mali/governor; do
    [ -f "$g" ] && echo "$STOCK_MALI_GOV" > "$g" 2>/dev/null
done
[ -z "$STOCK_MALI_POLL" ] && STOCK_MALI_POLL="50"
for i in /sys/class/devfreq/*.mali/polling_interval /sys/devices/platform/soc/*.mali/devfreq/*.mali/polling_interval; do
    [ -f "$i" ] && echo "$STOCK_MALI_POLL" > "$i" 2>/dev/null
done

# Reset GED & FPSGO thermal drivers
[ -f "/sys/module/ged/parameters/boost_gpu_enable" ] && echo 0 > /sys/module/ged/parameters/boost_gpu_enable 2>/dev/null
[ -f "/sys/module/ged/parameters/enable_gpu_boost" ] && echo 0 > /sys/module/ged/parameters/enable_gpu_boost 2>/dev/null
[ -f "/sys/kernel/fpsgo/fbt/thrm_enable" ] && echo 1 > /sys/kernel/fpsgo/fbt/thrm_enable 2>/dev/null
[ -f "/sys/kernel/fpsgo/fbt/ultra_rescue" ] && echo 0 > /sys/kernel/fpsgo/fbt/ultra_rescue 2>/dev/null
[ -z "$STOCK_SCONFIG" ] && STOCK_SCONFIG="0"
chmod 664 /sys/class/thermal/thermal_message/sconfig /sys/devices/virtual/thermal/thermal_message/sconfig 2>/dev/null || true
[ -f "/sys/class/thermal/thermal_message/sconfig" ] && echo "$STOCK_SCONFIG" > /sys/class/thermal/thermal_message/sconfig 2>/dev/null
[ -f "/sys/devices/virtual/thermal/thermal_message/sconfig" ] && echo "$STOCK_SCONFIG" > /sys/devices/virtual/thermal/thermal_message/sconfig 2>/dev/null

# Reset charging control
[ -f "/sys/class/power_supply/battery/input_suspend" ] && echo 0 > /sys/class/power_supply/battery/input_suspend 2>/dev/null
[ -f "/sys/class/power_supply/battery/charge_control_limit" ] && echo 0 > /sys/class/power_supply/battery/charge_control_limit 2>/dev/null

# Clean up symlinks & lock files
rm -f /data/adb/ap/bin/libhypercore.so /data/adb/ksu/bin/libhypercore.so /data/adb/modules/bin/libhypercore.so 2>/dev/null
rm -f /data/adb/ap/bin/hypercore-bugreport /data/adb/ksu/bin/hypercore-bugreport /data/adb/modules/bin/hypercore-bugreport 2>/dev/null
rm -f /data/adb/ap/bin/hypermoon_daemon /data/adb/ksu/bin/hypermoon_daemon /data/adb/modules/bin/hypermoon_daemon 2>/dev/null
rm -f /data/adb/modules/hypercore/.hypercore_lock /data/adb/modules/hypercore/hypercore.sock /data/adb/modules/hypercore/hypercore.pid 2>/dev/null
rm -f /data/adb/modules/hypercore/status.json /data/adb/modules/hypercore/*.conf /data/adb/modules/hypercore/gamelist.txt 2>/dev/null
rm -rf /data/adb/hypercore 2>/dev/null || true
rm -f /data/local/tmp/.hypercore_lock /data/local/tmp/hypercore.sock /data/local/tmp/hypercore.pid 2>/dev/null
rm -rf /data/local/tmp/hypercore 2>/dev/null || true
rm -f /dev/hypercore.sock /dev/hypercore_status.json 2>/dev/null
rm -f /sdcard/Android/.hypercore_lock 2>/dev/null
rm -f /sdcard/Android/hypercore.log 2>/dev/null
