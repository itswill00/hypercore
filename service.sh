#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

pkill -f "hypercore" >/dev/null 2>&1

chmod 755 "$MODDIR/hypercore"
exec "$MODDIR/hypercore"
