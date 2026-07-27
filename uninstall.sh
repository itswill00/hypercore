#!/system/bin/sh

pkill -f "hyperflow_engine.sh" >/dev/null 2>&1
pkill -f "webui_server.py" >/dev/null 2>&1

for i in 0 1 2 3 4 5; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo 500000 > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done
for i in 6 7; do
    [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" ] && echo 725000 > "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 2>/dev/null
done

rm -f /data/adb/modules/tanzanite_hyperflow/.hyperflow_lock 2>/dev/null
rm -f /data/local/tmp/.hyperflow_lock 2>/dev/null
rm -f /sdcard/Android/.hyperflow_lock 2>/dev/null
rm -f /sdcard/Android/tanzanite_hyperflow.log 2>/dev/null
rm -f /sdcard/Android/hyperflow_engine.log 2>/dev/null
