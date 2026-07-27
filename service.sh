#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

pkill -f "hypercore" >/dev/null 2>&1
pkill -f "hypercore.sh" >/dev/null 2>&1

if [ -f "$MODDIR/hypercore" ]; then
    chmod 755 "$MODDIR/hypercore"
    "$MODDIR/hypercore"
elif [ -f "$MODDIR/hypercore.sh" ]; then
    chmod 755 "$MODDIR/hypercore.sh"
    nohup "$MODDIR/hypercore.sh" >/dev/null 2>&1 &
fi
