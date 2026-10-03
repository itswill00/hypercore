#!/system/bin/sh

# Restore factory stock baseline from stock_state.conf if available

# SIGTERM first so the daemon can run restore_baseline_nodes() and put the
# sysfs nodes back the way it found them. Only then escalate. The wait matches
# service.sh (10 s): the restore rewrites ~60 nodes and a short grace used to
# SIGKILL it mid-restore, leaving nodes stranded after uninstall.
pkill -15 -x libhypercore.so >/dev/null 2>&1 || true
for _i in 1 2 3 4 5 6 7 8 9 10; do
    pidof libhypercore.so >/dev/null 2>&1 || break
    sleep 1
done
# `pkill -x` matches the kernel's 15-char comm field, so it cannot see a longer
# name. hypermoon_d is 11 chars and matches fine; the -f form is belt-and-braces
# in case the binary is ever renamed again.
pkill -9 -x hypermoon_d >/dev/null 2>&1 || true
pkill -9 -f 'hypermoon_d' >/dev/null 2>&1 || true
pkill -9 -f 'HyperMoonOverlay' >/dev/null 2>&1 || true
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true

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

# Restore full stock baseline via restore_baseline_nodes equivalent (fallback if daemon already dead)
for _node in "/sys/module/ged/parameters/boost_gpu_enable:0" "/sys/module/ged/parameters/ged_smart_boost:0" "/sys/module/ged/parameters/ged_boost_enable:1" "/sys/module/ged/parameters/enable_gpu_boost:1" "/sys/kernel/fpsgo/common/force_onoff:0" "/sys/kernel/fpsgo/fbt/boost_ta:0" "/sys/kernel/fpsgo/fbt/ultra_rescue:0" "/sys/kernel/fpsgo/fbt/switch_idleprefer:0" "/sys/kernel/fpsgo/fbt/thrm_enable:1"; do
  _p=${_node%%:*}; _v=${_node##*:}; [ -f "$_p" ] && echo "$_v" > "$_p" 2>/dev/null
done
[ -z "$STOCK_SCONFIG" ] && STOCK_SCONFIG="0"
chmod 664 /sys/class/thermal/thermal_message/sconfig /sys/devices/virtual/thermal/thermal_message/sconfig 2>/dev/null || true
# cpu_limits got the same 0666 treatment at runtime (service.sh + thermal
# bypass), so its mode is restored too instead of staying world-writable.
chmod 664 /sys/class/thermal/thermal_message/cpu_limits /sys/devices/virtual/thermal/thermal_message/cpu_limits 2>/dev/null || true
[ -f "/sys/class/thermal/thermal_message/sconfig" ] && echo "$STOCK_SCONFIG" > /sys/class/thermal/thermal_message/sconfig 2>/dev/null
[ -f "/sys/devices/virtual/thermal/thermal_message/sconfig" ] && echo "$STOCK_SCONFIG" > /sys/devices/virtual/thermal/thermal_message/sconfig 2>/dev/null
# Clear runtime props that would survive uninstall without daemon restore
resetprop debug.sf.latch_unsignaled 0 2>/dev/null || true
resetprop --delete persist.sys.wifi.low_latency 2>/dev/null || true
# Restore VM nodes from stock_state.conf when daemon restore_baseline_nodes never ran
if [ -f "$STOCK_CONF" ]; then
  _sw=$(grep '^vm_swappiness=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2); [ -n "$_sw" ] && [ -f /proc/sys/vm/swappiness ] && echo "$_sw" > /proc/sys/vm/swappiness 2>/dev/null
  _dr=$(grep '^vm_dirty_ratio=' "$STOCK_CONF" 2>/dev/null | cut -d= -f2); [ -n "$_dr" ] && [ -f /proc/sys/vm/dirty_ratio ] && echo "$_dr" > /proc/sys/vm/dirty_ratio 2>/dev/null
fi

# Restore stock ZRAM pool when the user had resized it (best-effort, never aborts).
# Next reboot would heal it anyway via the ROM init.rc, this just avoids
# leaving a custom pool behind for the current session.
if [ -f "/data/adb/hypercore/stock_zram.conf" ] && [ -f /sys/block/zram0/disksize ]; then
  _zstock=$(grep '^stock_disksize=' /data/adb/hypercore/stock_zram.conf 2>/dev/null | cut -d= -f2 | tr -d ' \r\n')
  _zcur=$(cat /sys/block/zram0/disksize 2>/dev/null | tr -d ' \r\n')
  if [ -n "$_zstock" ] && [ -n "$_zcur" ] && [ "$_zstock" != "$_zcur" ]; then
    # Same low-memory guard as service.sh: swapoff forces every compressed
    # page back into RAM at once, which freezes or OOMs under pressure.
    # 1 GB headroom — SwapUsed is the compressed size, decompressed costs more.
    _zswap_kb=$(awk '/^SwapTotal:/{t=$2} /^SwapFree:/{f=$2} END{print (t-f)}' /proc/meminfo 2>/dev/null)
    _zavail_kb=$(awk '/^MemAvailable:/{print $2}' /proc/meminfo 2>/dev/null)
    if [ -n "$_zswap_kb" ] && [ -n "$_zavail_kb" ] && [ "$_zavail_kb" -lt $((_zswap_kb + 1048576)) ]; then
      echo "HyperCore uninstall: low memory, keeping live ZRAM size (ROM init.rc heals it next boot)" >> /data/adb/hypercore/hypercore.log 2>/dev/null || true
    else
      swapoff /dev/block/zram0 2>/dev/null || true
      echo 1 > /sys/block/zram0/reset 2>/dev/null || true
      [ "$_zstock" != "0" ] && {
        echo "$_zstock" > /sys/block/zram0/disksize 2>/dev/null || true
        mkswap /dev/block/zram0 >/dev/null 2>&1 || true
        swapon /dev/block/zram0 2>/dev/null || true
      }
    fi
  fi
fi

# Reset charging control
[ -f "/sys/class/power_supply/battery/input_suspend" ] && echo 0 > /sys/class/power_supply/battery/input_suspend 2>/dev/null
[ -f "/sys/class/power_supply/battery/charge_control_limit" ] && echo 0 > /sys/class/power_supply/battery/charge_control_limit 2>/dev/null

# Clean up symlinks, lock files & runtime state
rm -f /data/adb/ap/bin/libhypercore.so /data/adb/ksu/bin/libhypercore.so /data/adb/modules/bin/libhypercore.so 2>/dev/null
rm -f /data/adb/ap/bin/hypercore-bugreport /data/adb/ksu/bin/hypercore-bugreport /data/adb/modules/bin/hypercore-bugreport 2>/dev/null
rm -f /data/adb/ap/bin/hypermoon_d /data/adb/ksu/bin/hypermoon_d /data/adb/modules/bin/hypermoon_d 2>/dev/null
rm -f /data/adb/modules/hypercore/.hypercore_lock /data/adb/modules/hypercore/hypercore.sock /data/adb/modules/hypercore/hypercore.pid 2>/dev/null
rm -f /data/adb/modules/hypercore/status.json /data/adb/modules/hypercore/*.conf /data/adb/modules/hypercore/gamelist.txt 2>/dev/null
# Legacy log location from before the daemon moved it under /data/adb
rm -f /sdcard/Android/hypercore.log 2>/dev/null
# Revoke what service.sh granted at install time. These appops and pm grants
# outlive the module — without this, com.android.shell and the whole android
# package keep SYSTEM_ALERT_WINDOW permanently after the overlay is gone.
cmd appops set --uid 0     SYSTEM_ALERT_WINDOW default 2>/dev/null || true
cmd appops set --uid 1000  SYSTEM_ALERT_WINDOW default 2>/dev/null || true
cmd appops set --uid 2000  SYSTEM_ALERT_WINDOW default 2>/dev/null || true
cmd appops set android     SYSTEM_ALERT_WINDOW default 2>/dev/null || true
pm revoke com.android.shell android.permission.SYSTEM_ALERT_WINDOW 2>/dev/null || true

# Preserve user configuration instead of deleting it with the runtime state.
# The previous `rm -rf /data/adb/hypercore` took the charge-mode and limit
# settings, the hand-built gamelist, the HyperMoon HUD config and every log with
# it, so uninstalling to "reset" the module silently cost the user all of it and
# a reinstall started from nothing. Runtime and scratch state still go; only the
# settings the user actually configured survive, in a timestamped sidecar.
PRESERVE_DIR="/data/adb/hypercore_removed"
PRESERVE_LIST="charge_mode.conf custom_charge_limit.conf night_charging.conf smart_chg.conf protect_80.conf battery_cycle.conf stock_state.conf zram.conf stock_zram.conf gamelist.txt"
if [ -d /data/adb/hypercore ]; then
    kept=""
    for f in $PRESERVE_LIST; do
        [ -f "/data/adb/hypercore/$f" ] && kept="$kept $f"
    done
    # HUD layout is user configuration too — losing it while charger and
    # gamelist settings survive made no sense.
    _hud_kept=""
    for hf in config.json position.json; do
        [ -f "/data/adb/hypercore/hud/$hf" ] && _hud_kept="$_hud_kept $hf"
    done
    [ -n "$_hud_kept" ] && kept="$kept hud/"
    if [ -n "$kept" ]; then
        stamp=$(date +%Y%m%d-%H%M%S 2>/dev/null || echo manual)
        if mkdir -p "$PRESERVE_DIR/$stamp" 2>/dev/null; then
            # One cp per file on purpose. Building the sources as
            # "/data/adb/hypercore/$kept" word-splits into the bare directory
            # plus one argument per name, which cp rejects, and the whole
            # preserve step then silently copies nothing.
            copied=""
            for f in $kept; do
                if [ "$f" = "hud/" ]; then
                    if mkdir -p "$PRESERVE_DIR/$stamp/hud" 2>/dev/null; then
                        for hf in $_hud_kept; do
                            if cp -f "/data/adb/hypercore/hud/$hf" "$PRESERVE_DIR/$stamp/hud/$hf" 2>/dev/null; then
                                copied="$copied hud/$hf"
                            fi
                        done
                    fi
                elif cp -f "/data/adb/hypercore/$f" "$PRESERVE_DIR/$stamp/$f" 2>/dev/null; then
                    copied="$copied $f"
                fi
            done
            if [ -n "$copied" ]; then
                echo "HyperCore: user configuration saved to $PRESERVE_DIR/$stamp ($copied )" > /sdcard/hypercore_uninstall_note.txt 2>/dev/null
            else
                echo "HyperCore: could not preserve user configuration" > /sdcard/hypercore_uninstall_note.txt 2>/dev/null
            fi
        fi
    fi
fi
rm -rf /data/adb/hypercore 2>/dev/null || true
rm -f /data/local/tmp/.hypercore_lock /data/local/tmp/hypercore.sock /data/local/tmp/hypercore.pid 2>/dev/null
rm -rf /data/local/tmp/hypercore 2>/dev/null || true
rm -f /dev/hypercore.sock /dev/hypercore_status.json 2>/dev/null
