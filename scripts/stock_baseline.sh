#!/system/bin/sh
# HyperCore stock baseline capture — single source of truth for factory nodes
# Sourced/executed from customize.sh; also usable standalone

capture_stock_baseline() {
    if [ -f "/data/adb/hypercore/stock_state.conf" ]; then
        ui_print "- Preserving existing factory hardware baseline..."
        return 0
    fi
    ui_print "- Capturing pristine factory hardware baseline from live system..."
    read_stock_node() {
        local file="$1" default_val="$2"
        if [ -f "$file" ]; then
            local val=$(head -n 1 "$file" 2>/dev/null | tr -d '\r\n ')
            if [ -n "$val" ]; then echo "$val"; return; fi
        fi
        echo "$default_val"
    }
    sconfig_raw=$(read_stock_node /sys/class/thermal/thermal_message/sconfig 0)
    case "$sconfig_raw" in 10|-1) sconfig_val=0 ;; *) sconfig_val="$sconfig_raw" ;; esac
    cat << EOF > /data/adb/hypercore/stock_state.conf
# HyperCore Stock Factory Baseline
# Captured automatically on initial module installation
has_baseline=1
gov0=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy0/scaling_governor sugov_ext)
gov6=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy6/scaling_governor sugov_ext)
lit_min_freq=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq 500000)
lit_max_freq=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq 2000000)
big_min_freq=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy6/cpuinfo_min_freq 725000)
big_max_freq=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy6/cpuinfo_max_freq 2200000)
pol0_up_rate=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us 1000)
pol0_down_rate=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us 1000)
pol6_up_rate=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us 1000)
pol6_down_rate=$(read_stock_node /sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us 1000)
bg_cpus=$(read_stock_node /dev/cpuset/background/cpus 0-3)
sys_bg_cpus=$(read_stock_node /dev/cpuset/system-background/cpus 0-5)
top_app_cpus=$(read_stock_node /dev/cpuset/top-app/cpus 0-7)
bg_shares=$(read_stock_node /dev/cpuctl/background/cpu.shares 1024)
bg_uclamp_min=$(read_stock_node /dev/cpuctl/background/cpu.uclamp.min 0)
bg_uclamp_max=$(read_stock_node /dev/cpuctl/background/cpu.uclamp.max max)
sys_bg_uclamp_max=$(read_stock_node /dev/cpuctl/system-background/cpu.uclamp.max max)
top_app_shares=$(read_stock_node /dev/cpuctl/top-app/cpu.shares 1024)
top_app_uclamp_min=$(read_stock_node /dev/cpuctl/top-app/cpu.uclamp.min 0)
top_app_uclamp_max=$(read_stock_node /dev/cpuctl/top-app/cpu.uclamp.max max)
mali_policy=$(cat /sys/class/devfreq/13000000.mali/power_policy 2>/dev/null || cat /sys/class/devfreq/soc:mali/power_policy 2>/dev/null || echo coarse_demand)
mali_gpu_gov=$(cat /sys/class/devfreq/13000000.mali/governor 2>/dev/null || cat /sys/class/devfreq/soc:mali/governor 2>/dev/null || true)
mali_poll_int=$(cat /sys/class/devfreq/13000000.mali/polling_interval 2>/dev/null || cat /sys/class/devfreq/soc:mali/polling_interval 2>/dev/null || echo 0)
mali_upthresh=80
mali_downdiff=20
mali_min_freq=$(cat /sys/class/devfreq/13000000.mali/min_freq 2>/dev/null || cat /sys/class/devfreq/soc:mali/min_freq 2>/dev/null || echo 390000000)
mali_max_freq=$(cat /sys/class/devfreq/13000000.mali/max_freq 2>/dev/null || cat /sys/class/devfreq/soc:mali/max_freq 2>/dev/null || echo 1003000000)
boost_gpu_enable=$(read_stock_node /sys/module/ged/parameters/boost_gpu_enable 0)
ged_smart_boost=$(read_stock_node /sys/module/ged/parameters/ged_smart_boost 0)
ged_boost_enable=$(read_stock_node /sys/module/ged/parameters/ged_boost_enable 1)
enable_gpu_boost=$(read_stock_node /sys/module/ged/parameters/enable_gpu_boost 1)
gpu_cust_boost_freq=$(read_stock_node /sys/module/ged/parameters/gpu_cust_boost_freq 390000)
gpu_cust_upbound_freq=$(read_stock_node /sys/module/ged/parameters/gpu_cust_upbound_freq 1003000)
gpu_bottom_freq=$(read_stock_node /sys/module/ged/parameters/gpu_bottom_freq 390000)
g_fb_dvfs_threshold=$(read_stock_node /sys/module/ged/parameters/g_fb_dvfs_threshold 80)
gx_fb_dvfs_margin=$(read_stock_node /sys/module/ged/parameters/gx_fb_dvfs_margin 40)
gx_game_mode=$(read_stock_node /sys/module/ged/parameters/gx_game_mode 0)
fpsgo_force_onoff=$(read_stock_node /sys/kernel/fpsgo/common/force_onoff 0)
fpsgo_boost_ta=$(read_stock_node /sys/kernel/fpsgo/fbt/boost_ta 0)
fpsgo_ultra_rescue=$(read_stock_node /sys/kernel/fpsgo/fbt/ultra_rescue 0)
fpsgo_light_loading=$(read_stock_node /sys/kernel/fpsgo/fbt/light_loading_policy 0)
fpsgo_idleprefer=$(read_stock_node /sys/kernel/fpsgo/fbt/switch_idleprefer 0)
fpsgo_thrm_enable=$(read_stock_node /sys/kernel/fpsgo/fbt/thrm_enable 1)
sconfig=$sconfig_val
touch_smooth=$(read_stock_node /sys/class/touch/touch_dev/touch_thp_smooth 0)
touch_noise=$(read_stock_node /sys/class/touch/touch_dev/touch_thp_noisefilter 0)
touch_game=$(read_stock_node /sys/class/touch/touch_dev/touch_thp_game 0)
touch_sens=$(read_stock_node /sys/class/touch/touch_dev/touch_sensitivity 0)
touch_edge=$(read_stock_node /sys/class/touch/touch_dev/touch_edge 0)
vm_swappiness=$(read_stock_node /proc/sys/vm/swappiness 100)
vm_dirty_ratio=$(read_stock_node /proc/sys/vm/dirty_ratio 20)
vm_dirty_bg_ratio=$(read_stock_node /proc/sys/vm/dirty_background_ratio 10)
vm_vfs_cache_pressure=$(read_stock_node /proc/sys/vm/vfs_cache_pressure 100)
vm_stat_interval=$(read_stock_node /proc/sys/vm/stat_interval 1)
vm_dirty_writeback=$(read_stock_node /proc/sys/vm/dirty_writeback_centisecs 500)
vm_page_cluster=$(read_stock_node /proc/sys/vm/page-cluster 3)
vm_compaction=$(read_stock_node /proc/sys/vm/compaction_proactiveness 20)
io_read_ahead=$(cat /sys/block/mmcblk0/queue/read_ahead_kb 2>/dev/null || cat /sys/block/sda/queue/read_ahead_kb 2>/dev/null || cat /sys/block/dm-0/queue/read_ahead_kb 2>/dev/null || echo 1024)
io_nr_requests=$(cat /sys/block/mmcblk0/queue/nr_requests 2>/dev/null || cat /sys/block/sda/queue/nr_requests 2>/dev/null || cat /sys/block/dm-0/queue/nr_requests 2>/dev/null || echo 128)
io_iostats=$(cat /sys/block/mmcblk0/queue/iostats 2>/dev/null || cat /sys/block/sda/queue/iostats 2>/dev/null || echo 1)
sched_migration_cost=$(read_stock_node /proc/sys/kernel/sched_migration_cost_ns 200000)
sched_latency=$(read_stock_node /proc/sys/kernel/sched_latency_ns 10000000)
sched_nr_migrate=$(read_stock_node /proc/sys/kernel/sched_nr_migrate 32)
charge_limit=$(read_stock_node /sys/class/power_supply/battery/constant_charge_current_max 0)
chg_smart=$(read_stock_node /sys/class/power_supply/battery/smart_chg 0)
chg_night=$(read_stock_node /sys/class/power_supply/battery/night_charging 0)
EOF
    chmod 644 /data/adb/hypercore/stock_state.conf 2>/dev/null || true

    # Stock ZRAM size lives in its own file so the daemon never rewrites it
    # when it round-trips stock_state.conf through its C baseline tables.
    if [ ! -f "/data/adb/hypercore/stock_zram.conf" ]; then
        _zd=$(cat /sys/block/zram0/disksize 2>/dev/null | tr -d '\r\n ')
        _zc=$(tr ' ' '\n' < /sys/block/zram0/comp_algorithm 2>/dev/null | grep '^\[' | tr -d '[]')
        printf '# Stock ZRAM pool captured at install time\nstock_disksize=%s\nstock_comp=%s\n' "${_zd:-}" "${_zc:-}" > /data/adb/hypercore/stock_zram.conf 2>/dev/null || true
        chmod 644 /data/adb/hypercore/stock_zram.conf 2>/dev/null || true
    fi
}
# ponytail: one function, zero hardcode beyond fallback — upgrade path is to read more nodes via read_stock_node
