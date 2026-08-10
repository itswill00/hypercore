#!/system/bin/sh
SKIPUNZIP=1

KERNEL_VER=$(uname -r)
ui_print "- Checking Linux Kernel compatibility..."
ui_print "- Detected Kernel: $KERNEL_VER"

case "$KERNEL_VER" in
    5.10.*)
        ui_print "- Kernel 5.10.x verified. Compatible."
        ;;
    *)
        ui_print "--------------------------------------"
        ui_print "! ERROR: Unsupported Kernel version ($KERNEL_VER)!"
        ui_print "! HyperCore strictly requires Linux Kernel 5.10.x."
        ui_print "! Installation aborted."
        ui_print "--------------------------------------"
        abort "! Incompatible Kernel Version: $KERNEL_VER"
        exit 1
        ;;
esac

ui_print "- Checking Hardware Platform & MediaTek MT6789 Family nodes..."
PLATFORM=$(getprop ro.board.platform 2>/dev/null)
HARDWARE=$(getprop ro.hardware 2>/dev/null)
SOC_MODEL=$(getprop ro.soc.model 2>/dev/null)
ui_print "- Detected SoC Platform: ${PLATFORM:-$HARDWARE}"

# Strict Hardware Inspection
IS_MT6789=0

# 1. Check getprop platform / hardware / soc_model
if echo "$PLATFORM $HARDWARE $SOC_MODEL" | grep -iqE 'mt6789|helio-g99|helio-g100|helio-g200'; then
    IS_MT6789=1
fi

# 2. Check MediaTek GED GPU Driver parameter directory
HAS_GED=0
if [ -d "/sys/module/ged" ]; then
    HAS_GED=1
fi

# 3. Check ARM Mali-G57 devfreq hardware nodes
HAS_MALI_DEVFREQ=0
for mali_path in /sys/class/devfreq/13000000.mali /sys/class/devfreq/soc:mali /sys/devices/platform/soc/13000000.mali /sys/devices/platform/soc/soc:mali; do
    if [ -d "$mali_path" ]; then
        HAS_MALI_DEVFREQ=1
        break
    fi
done

# 4. Check MediaTek Helio DVFSRC interconnect hardware nodes
HAS_DVFSRC=0
for dvf_path in /sys/kernel/helio-dvfsrc /sys/devices/platform/10012000.dvfsrc /sys/module/dvfsrc; do
    if [ -d "$dvf_path" ]; then
        HAS_DVFSRC=1
        break
    fi
done

# Enforce Strict Validation: Device MUST pass at least GED + Mali Devfreq checks
if [ $HAS_GED -eq 1 ] && [ $HAS_MALI_DEVFREQ -eq 1 ]; then
    ui_print "- MediaTek MT6789 Family Hardware Verified (GED & Mali-G57 OK)."
else
    ui_print "--------------------------------------------------"
    ui_print "! ERROR: INCOMPATIBLE HARDWARE PLATFORM DETECTED!"
    ui_print "! Detected Platform: ${PLATFORM:-$HARDWARE}"
    ui_print "! HyperCore is strictly designed for MediaTek MT6789 Family"
    ui_print "! (Helio G99 / G100 / G200 with Mali-G57 GPU)."
    ui_print "!"
    ui_print "! Missing required hardware nodes:"
    [ $HAS_GED -eq 0 ] && ui_print "  - Missing MediaTek GED GPU Driver (/sys/module/ged)"
    [ $HAS_MALI_DEVFREQ -eq 0 ] && ui_print "  - Missing Mali GPU Devfreq Node (13000000.mali / soc:mali)"
    [ $HAS_DVFSRC -eq 0 ] && ui_print "  - Missing Helio DVFSRC Interconnect (/sys/kernel/helio-dvfsrc)"
    ui_print "!"
    ui_print "! Installation ABORTED to protect your device."
    ui_print "--------------------------------------------------"
    abort "! Incompatible Chipset / Device Target"
    exit 1
fi

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

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
        echo "$pkg" >> "$MODPATH/gamelist.txt"
        ui_print "  + Auto-added game: $pkg"
    done
else
    ui_print "  (No installed games auto-detected, gamelist ready for manual entries)"
fi
set_perm "$MODPATH/gamelist.txt" 0 0 0644

VERSION_NAME=$(grep '^version=' "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
[ -z "$VERSION_NAME" ] && VERSION_NAME="v4.5.0"
ui_print "- Daemon $VERSION_NAME installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch / Magisk."
ui_print "- Installation complete! REBOOT YOUR DEVICE to apply update."
