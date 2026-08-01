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
