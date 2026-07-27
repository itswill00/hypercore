#!/system/bin/sh
MODDIR="${0%/*}"

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 3
done

sleep 5

pkill -f "hyperflow_engine" >/dev/null 2>&1
if [ -f "$MODDIR/hyperflow_engine" ]; then
    chmod 755 "$MODDIR/hyperflow_engine"
    nohup "$MODDIR/hyperflow_engine" >/dev/null 2>&1 &
elif [ -f "$MODDIR/hyperflow_engine.sh" ]; then
    chmod 755 "$MODDIR/hyperflow_engine.sh"
    nohup "$MODDIR/hyperflow_engine.sh" >/dev/null 2>&1 &
fi
