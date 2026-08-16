#!/system/bin/sh
MODDIR="${0%/*}"

# Wait for boot completion
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 2

mkdir -p /data/adb/hypercore 2>/dev/null || true

# Kill previous running daemon instances cleanly
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
pkill -9 -x hypercore >/dev/null 2>&1 || true
rm -f "$MODDIR/hypercore.sock" "$MODDIR/hypercore.pid" /data/adb/hypercore/hypercore.sock /data/adb/hypercore/hypercore.pid 2>/dev/null || true

# Universal ROM Shield: scan and suppress conflicting legacy boot scripts in service.d or post-fs-data.d
for script in /data/adb/service.d/* /data/adb/post-fs-data.d/*; do
    if [ -f "$script" ] && echo "$script" | grep -v "hypercore" | grep -iE 'thermal|tweak|encore|ktweak' >/dev/null 2>&1; then
        chmod 644 "$script" 2>/dev/null || true
    fi
done

BIN="$MODDIR/system/bin/libhypercore.so"
if [ -f "$BIN" ]; then
    chmod 755 "$BIN"
    "$BIN" >/dev/null 2>&1 &
    sleep 1
    if ! pidof libhypercore.so >/dev/null 2>&1 && ! pidof hypercore >/dev/null 2>&1; then
        nohup "$BIN" >/dev/null 2>&1 &
    fi
fi
