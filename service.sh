#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 2

# Kill previous running daemon instances cleanly using exact process name matching (-x)
# DO NOT use 'pkill -f hypercore' because it matches 'tanzanite_hypercore' in script path!
pkill -x libhypercore.so >/dev/null 2>&1
pkill -x hypercore >/dev/null 2>&1

# Universal ROM Shield: scan and suppress conflicting legacy boot scripts in service.d or post-fs-data.d
for script in /data/adb/service.d/* /data/adb/post-fs-data.d/*; do
    if [ -f "$script" ] && echo "$script" | grep -v "hypercore" | grep -iE 'thermal|tweak|encore|ktweak' >/dev/null 2>&1; then
        chmod 644 "$script" 2>/dev/null || true
    fi
done

BIN=""
if [ -f "$MODDIR/system/bin/libhypercore.so" ]; then
    BIN="$MODDIR/system/bin/libhypercore.so"
elif [ -f "$MODDIR/system/bin/hypercore" ]; then
    BIN="$MODDIR/system/bin/hypercore"
elif [ -f "$MODDIR/libhypercore.so" ]; then
    BIN="$MODDIR/libhypercore.so"
elif [ -f "$MODDIR/hypercore" ]; then
    BIN="$MODDIR/hypercore"
fi

if [ -n "$BIN" ]; then
    chmod 755 "$BIN"
    exec "$BIN"
fi
