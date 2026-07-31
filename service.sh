#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

pkill -f "libhypercore.so" >/dev/null 2>&1
pkill -f "hypercore" >/dev/null 2>&1

if [ -f "$MODDIR/libhypercore.so" ]; then
    chmod 755 "$MODDIR/libhypercore.so"
    exec "$MODDIR/libhypercore.so"
else
    chmod 755 "$MODDIR/hypercore"
    exec "$MODDIR/hypercore"
fi
