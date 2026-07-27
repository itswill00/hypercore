#!/system/bin/sh

LOG="/sdcard/Android/tanzanite_hyperflow.log"
LOCK_FILE="/data/adb/modules/tanzanite_hyperflow/.hyperflow_lock"
[ ! -d "/data/adb/modules/tanzanite_hyperflow" ] && LOCK_FILE="/sdcard/Android/.hyperflow_lock"

log_msg() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" >> "$LOG" 2>/dev/null
}

rotate_log() {
    if [ -f "$LOG" ]; then
        sz=$(wc -c < "$LOG" 2>/dev/null)
        if [ -n "$sz" ] && [ "$sz" -gt 102400 ]; then
            tail -n 200 "$LOG" > "${LOG}.tmp" 2>/dev/null
            mv -f "${LOG}.tmp" "$LOG" 2>/dev/null
        fi
    fi
}

log_msg "Tanzanite HyperFlow v2.5 (Zero-Overhead Engine) started."

LITTLE_MAX=2000000
BIG_MAX=2200000

# Cache path locations at startup to avoid repeated filesystem checks
CPU_TZ_PATH=""
BAT_TZ_PATH=""

find_thermal_nodes() {
    for tz in /sys/class/thermal/thermal_zone*; do
        if [ -f "$tz/type" ]; then
            tz_type=$(cat "$tz/type" 2>/dev/null)
            case "$tz_type" in
                *"cpu"*|*"mtktscpu"*|*"soc"*|*"tzts"*|*"cpu_0"*)
                    [ -z "$CPU_TZ_PATH" ] && CPU_TZ_PATH="$tz/temp"
                    ;;
                *"bat"*|*"battery"*)
                    [ -z "$BAT_TZ_PATH" ] && BAT_TZ_PATH="$tz/temp"
                    ;;
            esac
        fi
    done
    [ -z "$CPU_TZ_PATH" ] && [ -f /sys/class/thermal/thermal_zone16/temp ] && CPU_TZ_PATH="/sys/class/thermal/thermal_zone16/temp"
    [ -z "$CPU_TZ_PATH" ] && CPU_TZ_PATH="/sys/class/thermal/thermal_zone0/temp"
    [ -z "$BAT_TZ_PATH" ] && [ -f /sys/class/thermal/thermal_zone25/temp ] && BAT_TZ_PATH="/sys/class/thermal/thermal_zone25/temp"
    [ -z "$BAT_TZ_PATH" ] && BAT_TZ_PATH="/sys/class/thermal/thermal_zone1/temp"
}
find_thermal_nodes

# Cache CPU governor rate limit node type
HAS_SUGOV_EXT=0
HAS_SCHEDUTIL=0
[ -d "/sys/devices/system/cpu/cpu0/cpufreq/sugov_ext" ] && HAS_SUGOV_EXT=1
[ -d "/sys/devices/system/cpu/cpufreq/schedutil" ] && HAS_SCHEDUTIL=1

# Cache Backlight Path
BL_PATH=""
for p in /sys/class/backlight/panel0-backlight/brightness \
         /sys/class/backlight/lcd-backlight/brightness \
         /sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness \
         /sys/class/leds/lcd-backlight/brightness; do
    if [ -f "$p" ]; then
        BL_PATH="$p"
        break
    fi
done

write_node() {
    [ -f "$1" ] && echo "$2" > "$1" 2>/dev/null
}

set_cpu_rate_limits() {
    cpu=$1
    up=$2
    down=$3
    if [ "$HAS_SUGOV_EXT" -eq 1 ]; then
        write_node "/sys/devices/system/cpu/cpu$cpu/cpufreq/sugov_ext/up_rate_limit_us" "$up"
        write_node "/sys/devices/system/cpu/cpu$cpu/cpufreq/sugov_ext/down_rate_limit_us" "$down"
    fi
    if [ "$HAS_SCHEDUTIL" -eq 1 ]; then
        write_node "/sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us" "$up"
        write_node "/sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us" "$down"
    fi
}

apply_base_tuning() {
    # VM & ZRAM Tuning
    write_node /proc/sys/vm/swappiness 65
    write_node /proc/sys/vm/dirty_ratio 15
    write_node /proc/sys/vm/dirty_background_ratio 5
    write_node /proc/sys/vm/vfs_cache_pressure 90
    write_node /proc/sys/vm/stat_interval 2
    write_node /proc/sys/vm/compaction_proactiveness 20
    write_node /proc/sys/vm/page-cluster 0
    write_node /proc/sys/vm/extfrag_threshold 750

    # Network TCP Low Latency & Congestion
    sysctl -w net.ipv4.tcp_congestion_control=cubic >/dev/null 2>&1 || sysctl -w net.ipv4.tcp_congestion_control=bbr >/dev/null 2>&1
    sysctl -w net.core.default_qdisc=fq_codel >/dev/null 2>&1 || sysctl -w net.core.default_qdisc=fq >/dev/null 2>&1
    sysctl -w net.ipv4.tcp_low_latency=1 >/dev/null 2>&1
    sysctl -w net.ipv4.tcp_fastopen=3 >/dev/null 2>&1

    # UFS Storage Queue Scheduler
    for block in /sys/block/sd*/queue /sys/block/mmcblk*/queue; do
        [ -f "$block/scheduler" ] && echo "none" > "$block/scheduler" 2>/dev/null || echo "mq-deadline" > "$block/scheduler" 2>/dev/null
        [ -f "$block/read_ahead_kb" ] && echo "128" > "$block/read_ahead_kb" 2>/dev/null
        [ -f "$block/iostats" ] && echo "0" > "$block/iostats" 2>/dev/null
        [ -f "$block/nr_requests" ] && echo "64" > "$block/nr_requests" 2>/dev/null
    done

    # MediaTek FPSGO & GED Parameters
    write_node /sys/kernel/fpsgo/fbt/switch_idleprefer 1
    write_node /sys/kernel/fpsgo/fbt/ultra_rescue 1
    write_node /sys/kernel/fpsgo/fbt/light_loading_policy 10

    write_node /sys/module/ged/parameters/boost_gpu_enable 1
    write_node /sys/module/ged/parameters/enable_gpu_boost 1
    write_node /sys/module/ged/parameters/ged_smart_boost 1
    write_node /sys/module/ged/parameters/g_fb_dvfs_threshold 25
    write_node /sys/module/ged/parameters/gx_fb_dvfs_margin 50

    for mali_gov in /sys/class/misc/mali0/device/devfreq/*/governor; do
        write_node "$mali_gov" "bitorq" 2>/dev/null || write_node "$mali_gov" "coarse_demand"
    done

    # System Props Overrides
    setprop persist.sys.smartpower.display_camera_fps_enable false 2>/dev/null
    setprop persist.sys.smartpower.display.enable false 2>/dev/null
    setprop persist.vendor.fps.switch.thermal false 2>/dev/null
    setprop persist.vendor.display.touch.idle.enable false 2>/dev/null
    setprop persist.sys.joyose.fps_boost true 2>/dev/null
    setprop persist.sys.smartpower.game.enable true 2>/dev/null
    setprop persist.sys.wifi.low_latency 1 2>/dev/null
}

apply_cpuset() {
    if [ -d /dev/cpuset ]; then
        write_node /dev/cpuset/top-app/cpus "0-7"
        write_node /dev/cpuset/foreground/cpus "0-7"
        write_node /dev/cpuset/background/cpus "0-3"
        write_node /dev/cpuset/system-background/cpus "0-3"
    fi
}

apply_profile_nodes() {
    prof=$1
    ttier=$2

    case "$prof" in
        "Sleep")
            for i in 0 1 2 3 4 5; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 500000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" 1450000
                set_cpu_rate_limits "$i" 2000 500
            done
            for i in 6 7; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 725000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" 1300000
                set_cpu_rate_limits "$i" 2000 500
            done
            write_node /sys/kernel/fpsgo/fbt/boost_ta 0
            write_node /sys/module/ged/parameters/gpu_bottom_freq 300000
            ;;

        "Interactive")
            max_big=$BIG_MAX
            [ "$ttier" -ge 2 ] && max_big=2000000

            for i in 0 1 2 3 4 5; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 650000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$LITTLE_MAX"
                set_cpu_rate_limits "$i" 500 1000
            done
            for i in 6 7; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 800000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$max_big"
                set_cpu_rate_limits "$i" 500 1000
            done
            write_node /sys/kernel/fpsgo/fbt/boost_ta 1
            write_node /sys/module/ged/parameters/gpu_bottom_freq 550000
            ;;

        "Touch")
            for i in 0 1 2 3 4 5; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 1000000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$LITTLE_MAX"
                set_cpu_rate_limits "$i" 0 1500
            done
            for i in 6 7; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 1400000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$BIG_MAX"
                set_cpu_rate_limits "$i" 0 1500
            done
            write_node /sys/kernel/fpsgo/fbt/boost_ta 1
            write_node /sys/module/ged/parameters/gpu_bottom_freq 700000
            ;;

        "Gaming")
            gpu_target_freq=800000
            [ "$ttier" -eq 2 ] && gpu_target_freq=700000

            for i in 0 1 2 3 4 5; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 1200000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$LITTLE_MAX"
                set_cpu_rate_limits "$i" 0 2500
            done
            for i in 6 7; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 1600000
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" "$BIG_MAX"
                set_cpu_rate_limits "$i" 0 2500
            done
            write_node /sys/kernel/fpsgo/fbt/boost_ta 1
            write_node /sys/module/ged/parameters/gpu_bottom_freq "$gpu_target_freq"
            ;;

        "Thermal")
            for i in 0 1 2 3 4 5; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 500000
            done
            for i in 6 7; do
                write_node "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" 725000
            done
            write_node /sys/kernel/fpsgo/fbt/boost_ta 0
            write_node /sys/module/ged/parameters/gpu_bottom_freq 400000
            ;;
    esac
}

apply_base_tuning
apply_cpuset

current_profile="INIT"
current_thermal_tier=0
iteration=0
prev_load=0
touch_boost_ticks=0

while true; do
    iteration=$((iteration + 1))

    # Fast backlight check without subshell
    screen_on=0
    if [ -n "$BL_PATH" ]; then
        b=$(cat "$BL_PATH" 2>/dev/null)
        [ -n "$b" ] && [ "$b" -gt 0 ] && screen_on=1
    else
        dumpsys display 2>/dev/null | grep -q "mScreenState=ON" && screen_on=1
    fi

    # Temperature checks
    raw_cpu=$(cat "$CPU_TZ_PATH" 2>/dev/null)
    cpu_temp=$(( ${raw_cpu:-0} / 1000 ))
    [ "$cpu_temp" -eq 0 ] && cpu_temp=40

    raw_bat=$(cat "$BAT_TZ_PATH" 2>/dev/null)
    bat_temp=$(( ${raw_bat:-0} / 1000 ))
    [ "$bat_temp" -gt 100 ] && bat_temp=$((bat_temp / 10))

    # Thermal tiering
    thermal_tier=0
    if [ "$cpu_temp" -ge 55 ] || [ "$bat_temp" -ge 45 ]; then
        thermal_tier=3
    elif [ "$cpu_temp" -ge 49 ] || [ "$bat_temp" -ge 43 ]; then
        thermal_tier=2
    elif [ "$cpu_temp" -ge 42 ] || [ "$bat_temp" -ge 39 ]; then
        thermal_tier=1
    fi

    # GPU Loading & Load average
    gpu_load=$(cat /sys/module/ged/parameters/gpu_loading 2>/dev/null)
    gpu_load=${gpu_load:-0}

    load_val=0
    if read -r load_str _ < /proc/loadavg 2>/dev/null; then
        l_int=${load_str%%.*}
        l_dec=${load_str#*.}
        l_dec=${l_dec%% *}
        load_val=$((l_int * 100 + l_dec))
    fi

    manual_lock=""
    if [ -f "$LOCK_FILE" ]; then
        manual_lock=$(cat "$LOCK_FILE" 2>/dev/null)
    fi

    # Lightweight game detection: ONLY check dumpsys if screen is ON, NOT locked, and GPU load < 35%
    game_detected=0
    if [ "$screen_on" -eq 1 ] && [ "$manual_lock" != "GAMING" ] && [ "$gpu_load" -lt 35 ]; then
        if [ $((iteration % 5)) -eq 0 ]; then
            focused=$(dumpsys window 2>/dev/null | grep -E 'mCurrentFocus|mFocusedApp' | head -n 1)
            case "$focused" in
                *"mobile.legends"*|*"pubg"*|*"codm"*|*"garena"*|*"genshin"*|*"mihoyo"*|*"honkai"*|*"roblox"*|*"eafootball"*|*"wuthering"*|*"minecraft"*|*"freefire"*)
                    game_detected=1
                    ;;
            esac
        fi
    fi

    next_profile="Interactive"

    if [ "$screen_on" -eq 0 ]; then
        next_profile="Sleep"
        touch_boost_ticks=0
    elif [ "$thermal_tier" -eq 3 ]; then
        next_profile="Thermal"
        touch_boost_ticks=0
    elif [ "$manual_lock" = "GAMING" ] || [ "$game_detected" -eq 1 ]; then
        next_profile="Gaming"
        touch_boost_ticks=0
    elif [ "$gpu_load" -ge 35 ] || { [ "$gpu_load" -ge 15 ] && [ "$load_val" -ge 1200 ]; }; then
        next_profile="Gaming"
        touch_boost_ticks=0
    elif [ $((load_val - prev_load)) -gt 250 ] || [ "$touch_boost_ticks" -gt 0 ]; then
        if [ "$current_profile" != "Touch" ] && [ "$touch_boost_ticks" -eq 0 ]; then
            touch_boost_ticks=2
        else
            touch_boost_ticks=$((touch_boost_ticks - 1))
        fi
        if [ "$touch_boost_ticks" -gt 0 ]; then
            next_profile="Touch"
        else
            next_profile="Interactive"
        fi
    else
        next_profile="Interactive"
        touch_boost_ticks=0
    fi

    prev_load=$load_val

    # CRITICAL OPTIMIZATION: Only execute sysfs write operations when profile or thermal tier CHANGES!
    if [ "$next_profile" != "$current_profile" ] || [ "$thermal_tier" -ne "$current_thermal_tier" ]; then
        log_msg "Profile: $current_profile -> $next_profile (CPU: ${cpu_temp}°C, Bat: ${bat_temp}°C, GPU: ${gpu_load}%, Load: ${load_str})"
        apply_profile_nodes "$next_profile" "$thermal_tier"
        current_profile="$next_profile"
        current_thermal_tier="$thermal_tier"
    fi

    # Housekeeping every 20 iterations (~40-60 sec)
    if [ $((iteration % 20)) -eq 0 ]; then
        apply_cpuset
        rotate_log
        write_node /sys/module/ged/parameters/g_fb_dvfs_threshold 25
        write_node /sys/module/ged/parameters/gx_fb_dvfs_margin 50
        if [ -f /proc/pressure/memory ]; then
            psi_some=$(grep "some" /proc/pressure/memory 2>/dev/null)
            psi_some=${psi_some#*avg10=}
            psi_some=${psi_some%% *}
            psi_int=${psi_some%%.*}
            if [ -n "$psi_int" ] && [ "$psi_int" -gt 5 ]; then
                log_msg "Memory PSI stall ($psi_some). Trimming cache."
                write_node /proc/sys/vm/drop_caches 3
                write_node /proc/sys/vm/compact_memory 1
            fi
        fi
    fi

    sleep 2
done
