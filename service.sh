#!/system/bin/sh
MODDIR="${0%/*}"

# Wait for boot completion, but never forever: if the property stalls the
# daemon would never start and the device would sit untuned with no log.
_boot_wait=0
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
    _boot_wait=$((_boot_wait + 1))
    [ "$_boot_wait" -ge 60 ] && break
done

sleep 2

mkdir -p /data/adb/hypercore 2>/dev/null || true

# ZRAM pool sizing, applied once per boot so it wins over the ROM init.rc
# (which writes comp_algorithm then swapon_all from fstab.enableswap).
# Reboot-required by design: swapoff forces every compressed page back into
# RAM in one go, and doing that mid-game is how you get a freeze.
apply_zram_config() {
    CONF="/data/adb/hypercore/zram.conf"
    ZNODE="/sys/block/zram0/disksize"
    [ -f "$CONF" ] || return 0
    [ -f "$ZNODE" ] || return 0
    WANT_MB=$(grep '^size_mb=' "$CONF" 2>/dev/null | cut -d= -f2 | tr -d ' \r\n')
    case "$WANT_MB" in ''|*[!0-9]*) return 0 ;; esac

    LOG="/data/adb/hypercore/hypercore.log"
    if [ "$WANT_MB" = "0" ]; then
        if grep -q '/dev/block/zram0' /proc/swaps 2>/dev/null; then
            swapoff /dev/block/zram0 2>/dev/null || return 0
            echo 1 > /sys/block/zram0/reset 2>/dev/null || true
            echo "HyperCore ZRAM disabled by user config (swapoff)" >> "$LOG" 2>/dev/null || true
        fi
        return 0
    fi

    # Fixed presets only, mapped as strings: /system/bin/sh does 32-bit
    # arithmetic, so MB*1024*1024 overflows past 2GB.
    case "$WANT_MB" in
        0) ;;
        2048) WANT_BYTES="2147483648" ;;
        4096) WANT_BYTES="4294967296" ;;
        *) return 0 ;;
    esac
    CUR_BYTES=$(cat "$ZNODE" 2>/dev/null | tr -d ' \r\n')
    [ "$CUR_BYTES" = "$WANT_BYTES" ] && return 0

    # Refuse the swapoff when RAM is too tight to take the pages back.
    SWAP_KB=$(awk '/^SwapTotal:/{t=$2} /^SwapFree:/{f=$2} END{print (t-f)}' /proc/meminfo 2>/dev/null)
    AVAIL_KB=$(awk '/^MemAvailable:/{print $2}' /proc/meminfo 2>/dev/null)
    [ -n "$SWAP_KB" ] && [ -n "$AVAIL_KB" ] && [ "$AVAIL_KB" -lt $((SWAP_KB + 524288)) ] && {
        echo "HyperCore ZRAM resize skipped: low memory, keeping live size" >> "$LOG" 2>/dev/null || true
        return 0
    }

    swapoff /dev/block/zram0 2>/dev/null || return 0
    echo 1 > /sys/block/zram0/reset 2>/dev/null || true
    echo "$WANT_BYTES" > "$ZNODE" 2>/dev/null || {
        echo "HyperCore ZRAM resize failed, restoring swap" >> "$LOG" 2>/dev/null || true
        mkswap /dev/block/zram0 >/dev/null 2>&1 || true
        swapon /dev/block/zram0 2>/dev/null || true
        return 0
    }
    mkswap /dev/block/zram0 >/dev/null 2>&1 || true
    swapon /dev/block/zram0 2>/dev/null || true
    echo "HyperCore ZRAM pool set to ${WANT_MB}MB" >> "$LOG" 2>/dev/null || true
}

apply_zram_config

# Graceful stop old daemon so it restores baseline nodes before new one takes over.
# Give restore_baseline_nodes() (it rewrites ~60 sysfs nodes) room to finish
# before escalating to SIGKILL, otherwise a killed daemon leaves nodes stranded.
pkill -15 -x libhypercore.so >/dev/null 2>&1 || true
for _i in 1 2 3 4 5 6 7 8 9 10; do
    pidof libhypercore.so >/dev/null 2>&1 || break
    sleep 1
done
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
rm -f "$MODDIR/hypercore.sock" "$MODDIR/hypercore.pid" "$MODDIR/status.json" /data/adb/hypercore/hypercore.sock /data/adb/hypercore/hypercore.pid /data/adb/hypercore/.hypercore_lock /data/adb/hypercore/status.json /dev/hypercore.sock /dev/hypercore_status.json 2>/dev/null || true

# NOTE: HyperCore deliberately does NOT touch other modules' scripts.
# An earlier version chmod'd any /data/adb/service.d/* or post-fs-data.d/*
# entry matching 'thermal|tweak|encore|ktweak' to 0644 to "win" conflicts.
# That silently disabled third-party modules, recorded no original mode, and
# was never undone on uninstall. Documented conflicts in docs/DOCUMENTATION.txt
# instead.

# Ensure Xiaomi thermal control nodes are writable
chmod 666 /sys/class/thermal/thermal_message/sconfig \
          /sys/devices/virtual/thermal/thermal_message/sconfig \
          /sys/class/thermal/thermal_message/cpu_limits \
          /sys/devices/virtual/thermal/thermal_message/cpu_limits 2>/dev/null || true

BIN="$MODDIR/system/bin/libhypercore.so"
if [ -f "$BIN" ]; then
    chmod 755 "$BIN"
    "$BIN" >/dev/null 2>&1 &
    # Wait for the daemon to register before assuming it died. The old
    # `sleep 1 || nohup` fallback double-launched whenever startup took
    # longer than a second (saved only by the in-daemon flock).
    _started=0
    for _i in 1 2 3 4 5 6 7 8 9 10; do
        if pidof libhypercore.so >/dev/null 2>&1; then
            _started=1
            break
        fi
        sleep 1
    done
    if [ "$_started" -eq 0 ]; then
        nohup "$BIN" >/dev/null 2>&1 &
    fi
fi

# HyperMoon HUD Service Setup & Auto-Start
cmd appops set --uid 0 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true
cmd appops set --uid 1000 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true
cmd appops set --uid 2000 SYSTEM_ALERT_WINDOW allow 2>/dev/null || true
cmd appops set android SYSTEM_ALERT_WINDOW allow 2>/dev/null || true
cmd appops set com.android.shell SYSTEM_ALERT_WINDOW allow 2>/dev/null || true
pm grant com.android.shell android.permission.SYSTEM_ALERT_WINDOW 2>/dev/null || true

HUD_STATE="/data/adb/hypercore/hud"
mkdir -p "$HUD_STATE" 2>/dev/null
chmod 755 "$HUD_STATE" 2>/dev/null || true

if [ -f "$HUD_STATE/config.json" ] && grep -q '"visible"[[:space:]]*:[[:space:]]*true' "$HUD_STATE/config.json" 2>/dev/null; then
    if [ -f "$MODDIR/system/bin/hypermoon_d" ]; then
        export HYPERMOON_STATE_DIR="$HUD_STATE"
        nohup "$MODDIR/system/bin/hypermoon_d" > "$HUD_STATE/daemon.log" 2>&1 &
    fi
    if [ -f "$MODDIR/system/bin/hypermoon.dex" ]; then
        export HYPERMOON_STATE_DIR="$HUD_STATE"
        CLASSPATH="$MODDIR/system/bin/hypermoon.dex" nohup /system/bin/app_process /system/bin com.hypermoon.HyperMoonOverlay "$HUD_STATE" > "$HUD_STATE/overlay.log" 2>&1 &
    fi
fi
