#!/system/bin/sh
SKIPUNZIP=1

KERNEL_VER=$(uname -r)
ui_print "- Checking Linux Kernel compatibility..."
ui_print "- Detected Kernel: $KERNEL_VER"

case "$KERNEL_VER" in
    5.10.*|5.15.*|6.1.*|6.6.*|6.12.*)
        ui_print "- Kernel $KERNEL_VER verified. Compatible."
        ;;
    *)
        ui_print "- Universal Dual-Kernel GKI architecture enabled for Kernel $KERNEL_VER."
        ;;
esac

ui_print "- Checking Hardware Platform & MediaTek MT6789 Family nodes..."
PLATFORM=$(getprop ro.board.platform 2>/dev/null)
HARDWARE=$(getprop ro.hardware 2>/dev/null)
SOC_MODEL=$(getprop ro.soc.model 2>/dev/null)
ui_print "- Detected SoC Platform: ${PLATFORM:-$HARDWARE}"

# Strict Hardware Inspection
IS_MT6789=0
if echo "$PLATFORM $HARDWARE $SOC_MODEL" | grep -iqE 'mt6789|helio-g99|helio-g100|helio-g200'; then
    IS_MT6789=1
fi

HAS_GED=0
if [ -d "/sys/module/ged" ]; then
    HAS_GED=1
fi

HAS_MALI_DEVFREQ=0
for mali_path in /sys/class/devfreq/13000000.mali /sys/class/devfreq/soc:mali /sys/devices/platform/soc/13000000.mali /sys/devices/platform/soc/soc:mali /sys/devices/platform/soc/13fbf000.gpufreq; do
    if [ -d "$mali_path" ]; then
        HAS_MALI_DEVFREQ=1
        break
    fi
done

HAS_DVFSRC=0
for dvf_path in /sys/kernel/helio-dvfsrc /sys/devices/platform/10012000.dvfsrc /sys/module/dvfsrc; do
    if [ -d "$dvf_path" ]; then
        HAS_DVFSRC=1
        break
    fi
done

if [ $HAS_GED -eq 1 ] || [ $HAS_MALI_DEVFREQ -eq 1 ]; then
    ui_print "- MediaTek MT6789 Family Hardware Verified (GED & Mali GPU OK)."
else
    ui_print "--------------------------------------------------"
    ui_print "! ERROR: INCOMPATIBLE HARDWARE PLATFORM DETECTED!"
    ui_print "! Detected Platform: ${PLATFORM:-$HARDWARE}"
    ui_print "! HyperCore is strictly designed for MediaTek MT6789 Family"
    ui_print "! (Helio G99 / G100 / G200 with Mali GPU)."
    ui_print "--------------------------------------------------"
    abort "! Incompatible Chipset / Device Target"
    exit 1
fi

# Clean up running daemon instances before module upgrade to prevent binary lock
ui_print "- Stopping any active daemon instances before upgrade..."
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
rm -f /data/adb/modules/hypercore/hypercore.sock /data/adb/hypercore/hypercore.sock /data/adb/modules/hypercore/hypercore.pid /data/adb/hypercore/hypercore.pid /dev/hypercore.sock 2>/dev/null || true

ui_print "- Preserving user configurations in /data/adb/hypercore..."
mkdir -p /data/adb/hypercore
for conf in charge_mode.conf custom_charge_limit.conf night_charging.conf smart_chg.conf protect_80.conf battery_cycle.conf stock_state.conf; do
    if [ -f "/data/adb/modules/hypercore/$conf" ] && [ ! -f "/data/adb/hypercore/$conf" ]; then
        cp -f "/data/adb/modules/hypercore/$conf" "/data/adb/hypercore/$conf" 2>/dev/null || true
    fi
done

if [ -f "/data/adb/hypercore/stock_state.conf" ]; then
    ui_print "- Preserving existing factory hardware baseline..."
else
    ui_print "- Capturing pristine factory hardware baseline from live system..."
    read_stock_node() {
        local file="$1"
        local default_val="$2"
        if [ -f "$file" ]; then
            local val=$(head -n 1 "$file" 2>/dev/null | tr -d '\r\n ')
            if [ -n "$val" ]; then
                echo "$val"
                return
            fi
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
mali_policy=coarse_demand
mali_gpu_gov=$(read_stock_node /sys/class/devfreq/13000000.mali/governor dummy)
mali_poll_int=$(read_stock_node /sys/class/devfreq/13000000.mali/polling_interval 0)
mali_upthresh=80
mali_downdiff=20
mali_min_freq=$(read_stock_node /sys/class/devfreq/13000000.mali/min_freq 390000000)
mali_max_freq=$(read_stock_node /sys/class/devfreq/13000000.mali/max_freq 1003000000)
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
fi

PRESERVE_GL="/tmp/hypercore_gamelist_bak.txt"
rm -f "$PRESERVE_GL"
if [ -f "/data/adb/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/hypercore/gamelist.txt" "$PRESERVE_GL"
elif [ -f "/data/adb/modules/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/modules/hypercore/gamelist.txt" "$PRESERVE_GL"
fi

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

if [ -f "$PRESERVE_GL" ]; then
    ui_print "- Merging preserved user gamelist entries..."
    cat "$PRESERVE_GL" "$MODPATH/gamelist.txt" 2>/dev/null | awk -F: '!seen[$1]++' > /data/adb/hypercore/gamelist.txt
    rm -f "$PRESERVE_GL"
elif [ -f "$MODPATH/gamelist.txt" ]; then
    cp -f "$MODPATH/gamelist.txt" /data/adb/hypercore/gamelist.txt 2>/dev/null || touch /data/adb/hypercore/gamelist.txt
fi

# Clean up non-module files, temporary configs, and duplicates from installed module root
rm -f "$MODPATH/gamelist.txt"
rm -f "$MODPATH/NOTICE.md"
rm -f "$MODPATH/update.json"
rm -f "$MODPATH/webroot/banner.jpg"
rm -f "$MODPATH"/*.conf
rm -f "$MODPATH/hypercore" "$MODPATH/libhypercore.so"
rm -f "$MODPATH/hypercore.sock" "$MODPATH/hypercore.pid" "$MODPATH/status.json"

ui_print "- Verifying embedded binary SHA-256 integrity..."
chmod 755 "$MODPATH/system/bin/libhypercore.so" 2>/dev/null || true
if [ -f "$MODPATH/system/bin/libhypercore.so" ]; then
    INTEGRITY_OUT=$("$MODPATH/system/bin/libhypercore.so" --verify-integrity "$MODPATH" 2>&1)
    if [ $? -eq 0 ]; then
        ui_print "- Embedded binary SHA-256 integrity verified successfully."
    else
        ui_print "--------------------------------------"
        ui_print "! ERROR: File tampering or corruption detected!"
        ui_print "$INTEGRITY_OUT"
        ui_print "! Installation aborted for security."
        ui_print "--------------------------------------"
        rm -rf "$MODPATH"
        abort "! Embedded Binary Checksum Mismatch"
        exit 1
    fi
fi

ui_print "- Setting permissions & PATH symlinks..."
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
[ -d "$MODPATH/system/bin" ] && set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/system.prop" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644
[ -f "$MODPATH/banner.jpg" ] && set_perm "$MODPATH/banner.jpg" 0 0 0644
[ -f "$MODPATH/changelog.md" ] && set_perm "$MODPATH/changelog.md" 0 0 0644

# Create symlinks in root manager PATH for KSU / APatch / Magisk
for manager_dir in /data/adb/ap/bin /data/adb/ksu/bin /data/adb/modules/bin; do
    if [ -d "$manager_dir" ]; then
        ui_print "- Creating PATH symlink in $manager_dir"
        ln -sf "$MODPATH/system/bin/libhypercore.so" "$manager_dir/libhypercore.so" 2>/dev/null || true
        [ -f "$MODPATH/system/bin/hypercore-bugreport" ] && ln -sf "$MODPATH/system/bin/hypercore-bugreport" "$manager_dir/hypercore-bugreport" 2>/dev/null || true
        [ -f "$MODPATH/system/bin/hypermoon_daemon" ] && ln -sf "$MODPATH/system/bin/hypermoon_daemon" "$manager_dir/hypermoon_daemon" 2>/dev/null || true
    fi
done

# HyperMoon HUD State directory & FPS Moon migration
mkdir -p /data/adb/hypercore/hud
if [ -d "/data/adb/modules/fps_moon/state" ] && [ ! -f "/data/adb/hypercore/hud/config.json" ]; then
    ui_print "- Migrating existing FPS Moon preferences to HyperMoon..."
    cp -rf /data/adb/modules/fps_moon/state/* /data/adb/hypercore/hud/ 2>/dev/null || true
fi

if [ ! -f "/data/adb/hypercore/hud/config.json" ]; then
    cat << 'EOF' > /data/adb/hypercore/hud/config.json
{
  "visible": false,
  "auto_gaming": false,
  "show_fps": true,
  "show_cpu": true,
  "show_cpu_freq": true,
  "show_gov": false,
  "show_gpu": true,
  "show_gpu_freq": true,
  "show_gpu_gov": false,
  "show_ram": true,
  "show_zram": false,
  "show_battery": true,
  "show_net": true,
  "is_horizontal": false,
  "align": "left",
  "theme": "cyber_neon",
  "custom_color": "#6366F1",
  "opacity": 0.7,
  "scale": 0.65,
  "font_size": 14,
  "corner_radius": 14,
  "bg_width": 150,
  "bg_height": 160,
  "refresh_interval": 1500,
  "target_fps": 60
}
EOF
    chmod 666 /data/adb/hypercore/hud/config.json 2>/dev/null || true
fi

if [ ! -f "/data/adb/hypercore/hud/position.json" ]; then
    cat << 'EOF' > /data/adb/hypercore/hud/position.json
{
  "x": 697,
  "y": 411
}
EOF
    chmod 666 /data/adb/hypercore/hud/position.json 2>/dev/null || true
fi
chmod 777 /data/adb/hypercore/hud 2>/dev/null || true

ui_print "- Auto-detecting installed games on your device..."
AUTO_GAMES=$(pm list packages -3 2>/dev/null | cut -d: -f2 | grep -iE 'game|legend|pubg|mihoyo|genshin|honkai|freefire|roblox|activision|shooter|mojang|minecraft|supercell|brawl|clash|garena|stumble|pokemon|wanda|maleo|konami|krafton|netmarble|nexon|ea\.gp|riotgames|square_enix|bandainamco|gameloft|zynga|rovio|miniclip|yostar|ubisoft|subway|bussimulator|carx|slither|angrybirds|asphalt|shadowfight|realracing|needforspeed|efootball|pes20|fifa|tft|nintendo|sega|squareenix|capcom' 2>/dev/null)

if [ -n "$AUTO_GAMES" ]; then
    for pkg in $AUTO_GAMES; do
        if ! grep -q -E "^${pkg}(:|$)" /data/adb/hypercore/gamelist.txt 2>/dev/null; then
            echo "$pkg" >> /data/adb/hypercore/gamelist.txt
            ui_print "  + Auto-added game: $pkg"
        fi
    done
else
    ui_print "  (No installed games auto-detected, gamelist ready for manual entries)"
fi
rm -f "$MODPATH/gamelist.txt"
set_perm /data/adb/hypercore/gamelist.txt 0 0 0644
for c in /data/adb/hypercore/*.conf; do
    [ -f "$c" ] && set_perm "$c" 0 0 0644
done

VERSION_NAME=$(grep '^version=' "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
[ -z "$VERSION_NAME" ] && VERSION_NAME="v6.8.0"
ui_print "- Daemon $VERSION_NAME installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch / Magisk."
ui_print "- Installation complete! REBOOT YOUR DEVICE to apply update."
