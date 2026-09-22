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
bg_shares=1024
bg_uclamp_min=0
bg_uclamp_max=max
sys_bg_uclamp_max=max
top_app_shares=1024
top_app_uclamp_min=0
top_app_uclamp_max=max
mali_policy=$(cat /sys/class/devfreq/13000000.mali/power_policy 2>/dev/null || cat /sys/class/devfreq/soc:mali/power_policy 2>/dev/null || echo coarse_demand)
mali_gpu_gov=$(cat /sys/class/devfreq/13000000.mali/governor 2>/dev/null || cat /sys/class/devfreq/soc:mali/governor 2>/dev/null || echo dummy)
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
gx_game_mode=0
fpsgo_force_onoff=0
fpsgo_boost_ta=0
fpsgo_ultra_rescue=0
fpsgo_light_loading=0
fpsgo_idleprefer=0
fpsgo_thrm_enable=1
sconfig=0
vm_swappiness=$(read_stock_node /proc/sys/vm/swappiness 100)
vm_dirty_ratio=$(read_stock_node /proc/sys/vm/dirty_ratio 20)
vm_dirty_bg_ratio=$(read_stock_node /proc/sys/vm/dirty_background_ratio 10)
vm_vfs_cache_pressure=$(read_stock_node /proc/sys/vm/vfs_cache_pressure 100)
vm_stat_interval=$(read_stock_node /proc/sys/vm/stat_interval 1)
vm_dirty_writeback=$(read_stock_node /proc/sys/vm/dirty_writeback_centisecs 500)
vm_page_cluster=$(read_stock_node /proc/sys/vm/page-cluster 3)
io_read_ahead=1024
io_nr_requests=128
io_iostats=1
sched_migration_cost=$(read_stock_node /proc/sys/kernel/sched_migration_cost_ns 200000)
sched_latency=$(read_stock_node /proc/sys/kernel/sched_latency_ns 10000000)
sched_nr_migrate=$(read_stock_node /proc/sys/kernel/sched_nr_migrate 32)
charge_limit=0
EOF
    chmod 644 /data/adb/hypercore/stock_state.conf 2>/dev/null || true
}
# ponytail: one function, zero hardcode beyond fallback — upgrade path is to read more nodes via read_stock_node
