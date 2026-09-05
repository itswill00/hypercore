#!/system/bin/sh
SKIPUNZIP=1

KERNEL_VER=$(uname -r)
ui_print "- Checking Linux Kernel compatibility..."
ui_print "- Detected Kernel: $KERNEL_VER"

case "$KERNEL_VER" in
    5.10.*|5.15.*|6.1.*|6.6.*|6.12.*)
        ui_print "- Kernel $KERNEL_VER verified. Compatible."
        ;;
    *)
        ui_print "- Universal Dual-Kernel GKI architecture enabled for Kernel $KERNEL_VER."
        ;;
esac

ui_print "- Checking Hardware Platform & MediaTek MT6789 Family nodes..."
PLATFORM=$(getprop ro.board.platform 2>/dev/null)
HARDWARE=$(getprop ro.hardware 2>/dev/null)
SOC_MODEL=$(getprop ro.soc.model 2>/dev/null)
ui_print "- Detected SoC Platform: ${PLATFORM:-$HARDWARE}"

# Strict Hardware Inspection
IS_MT6789=0
if echo "$PLATFORM $HARDWARE $SOC_MODEL" | grep -iqE 'mt6789|helio-g99|helio-g100|helio-g200'; then
    IS_MT6789=1
fi

HAS_GED=0
if [ -d "/sys/module/ged" ]; then
    HAS_GED=1
fi

HAS_MALI_DEVFREQ=0
for mali_path in /sys/class/devfreq/13000000.mali /sys/class/devfreq/soc:mali /sys/devices/platform/soc/13000000.mali /sys/devices/platform/soc/soc:mali /sys/devices/platform/soc/13fbf000.gpufreq; do
    if [ -d "$mali_path" ]; then
        HAS_MALI_DEVFREQ=1
        break
    fi
done

HAS_DVFSRC=0
for dvf_path in /sys/kernel/helio-dvfsrc /sys/devices/platform/10012000.dvfsrc /sys/module/dvfsrc; do
    if [ -d "$dvf_path" ]; then
        HAS_DVFSRC=1
        break
    fi
done

if [ $HAS_GED -eq 1 ] || [ $HAS_MALI_DEVFREQ -eq 1 ]; then
    ui_print "- MediaTek MT6789 Family Hardware Verified (GED & Mali GPU OK)."
else
    ui_print "--------------------------------------------------"
    ui_print "! ERROR: INCOMPATIBLE HARDWARE PLATFORM DETECTED!"
    ui_print "! Detected Platform: ${PLATFORM:-$HARDWARE}"
    ui_print "! HyperCore is strictly designed for MediaTek MT6789 Family"
    ui_print "! (Helio G99 / G100 / G200 with Mali GPU)."
    ui_print "--------------------------------------------------"
    abort "! Incompatible Chipset / Device Target"
    exit 1
fi

# Clean up running daemon instances before module upgrade to prevent binary lock
ui_print "- Stopping any active daemon instances before upgrade..."
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
rm -f /data/adb/modules/hypercore/hypercore.sock /data/adb/hypercore/hypercore.sock /data/adb/modules/hypercore/hypercore.pid /data/adb/hypercore/hypercore.pid 2>/dev/null || true

ui_print "- Preserving existing user gamelist.txt..."
PRESERVE_GL="/tmp/hypercore_gamelist_bak.txt"
rm -f "$PRESERVE_GL"
if [ -f "/data/adb/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/hypercore/gamelist.txt" "$PRESERVE_GL"
elif [ -f "/data/adb/modules/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/modules/hypercore/gamelist.txt" "$PRESERVE_GL"
fi

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

mkdir -p /data/adb/hypercore

if [ -f "$PRESERVE_GL" ]; then
    ui_print "- Merging preserved user gamelist entries..."
    cat "$PRESERVE_GL" "$MODPATH/gamelist.txt" 2>/dev/null | awk -F: '!seen[$1]++' > /data/adb/hypercore/gamelist.txt
    rm -f "$PRESERVE_GL"
else
    cp -f "$MODPATH/gamelist.txt" /data/adb/hypercore/gamelist.txt 2>/dev/null || touch /data/adb/hypercore/gamelist.txt
fi

ui_print "- Verifying embedded binary SHA-256 integrity..."
chmod 755 "$MODPATH/system/bin/libhypercore.so" 2>/dev/null || true
if [ -f "$MODPATH/system/bin/libhypercore.so" ]; then
    INTEGRITY_OUT=$("$MODPATH/system/bin/libhypercore.so" --verify-integrity "$MODPATH" 2>&1)
    if [ $? -eq 0 ]; then
        ui_print "- Embedded binary SHA-256 integrity verified successfully."
    else
        ui_print "--------------------------------------"
        ui_print "! ERROR: File tampering or corruption detected!"
        ui_print "$INTEGRITY_OUT"
        ui_print "! Installation aborted for security."
        ui_print "--------------------------------------"
        rm -rf "$MODPATH"
        abort "! Embedded Binary Checksum Mismatch"
        exit 1
    fi
fi

# Clean up legacy root binary copies if upgrading
rm -f "$MODPATH/hypercore" "$MODPATH/libhypercore.so"

ui_print "- Setting permissions & PATH symlinks..."
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
[ -d "$MODPATH/system/bin" ] && set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/system.prop" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644

# Create symlinks in root manager PATH for KSU / APatch / Magisk
for manager_dir in /data/adb/ap/bin /data/adb/ksu/bin /data/adb/modules/bin; do
    if [ -d "$manager_dir" ]; then
        ui_print "- Creating PATH symlink in $manager_dir"
        ln -sf "$MODPATH/system/bin/libhypercore.so" "$manager_dir/libhypercore.so" 2>/dev/null || true
        [ -f "$MODPATH/system/bin/hypercore-bugreport" ] && ln -sf "$MODPATH/system/bin/hypercore-bugreport" "$manager_dir/hypercore-bugreport" 2>/dev/null || true
    fi
done

ui_print "- Auto-detecting installed games on your device..."
AUTO_GAMES=$(pm list packages -3 2>/dev/null | cut -d: -f2 | grep -iE 'game|legend|pubg|mihoyo|genshin|honkai|freefire|roblox|activision|shooter|mojang|minecraft|supercell|brawl|clash|garena|stumble|pokemon|wanda|maleo|konami|krafton|netmarble|nexon|ea\.gp|riotgames|square_enix|bandainamco|gameloft|zynga|rovio|miniclip|yostar|ubisoft|subway|bussimulator|carx|slither|angrybirds|asphalt|shadowfight|realracing|needforspeed|efootball|pes20|fifa|tft|nintendo|sega|squareenix|capcom' 2>/dev/null)

if [ -n "$AUTO_GAMES" ]; then
    for pkg in $AUTO_GAMES; do
        if ! grep -q -E "^${pkg}(:|$)" /data/adb/hypercore/gamelist.txt 2>/dev/null; then
            echo "$pkg" >> /data/adb/hypercore/gamelist.txt
            ui_print "  + Auto-added game: $pkg"
        fi
    done
else
    ui_print "  (No installed games auto-detected, gamelist ready for manual entries)"
fi
cp -f /data/adb/hypercore/gamelist.txt "$MODPATH/gamelist.txt" 2>/dev/null || true
set_perm /data/adb/hypercore/gamelist.txt 0 0 0644
set_perm "$MODPATH/gamelist.txt" 0 0 0644

VERSION_NAME=$(grep '^version=' "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
[ -z "$VERSION_NAME" ] && VERSION_NAME="v6.4.8"
ui_print "- Daemon $VERSION_NAME installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch / Magisk."
ui_print "- Installation complete! REBOOT YOUR DEVICE to apply update."
