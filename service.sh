#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

pkill -f "hypercore_engine" >/dev/null 2>&1
pkill -f "hypercore.sh" >/dev/null 2>&1

if [ -f "$MODDIR/hypercore_engine" ]; then
    chmod 755 "$MODDIR/hypercore_engine"
    "$MODDIR/hypercore_engine"
elif [ -f "$MODDIR/hypercore.sh" ]; then
    chmod 755 "$MODDIR/hypercore.sh"
    nohup "$MODDIR/hypercore.sh" >/dev/null 2>&1 &
fi
