#!/system/bin/sh
SKIPUNZIP=1

KERNEL_VER=$(uname -r)
ui_print "- Checking Linux Kernel compatibility..."
ui_print "- Detected Kernel: $KERNEL_VER"

case "$KERNEL_VER" in
    5.10.*)
        ui_print "- Kernel 5.10.x verified. Compatible."
        ;;
    6.12.*|6.12*)
        ui_print "--------------------------------------"
        ui_print "! ERROR: Unsupported Kernel 6.12 detected!"
        ui_print "! HyperCore is strictly designed for Linux 5.10.x."
        ui_print "! Installation aborted."
        ui_print "--------------------------------------"
        abort "! Incompatible kernel version: $KERNEL_VER"
        exit 1
        ;;
    *)
        ui_print "--------------------------------------"
        ui_print "! ERROR: Unsupported Kernel version ($KERNEL_VER)!"
        ui_print "! HyperCore only supports Linux kernel 5.10.x."
        ui_print "! Installation aborted."
        ui_print "--------------------------------------"
        abort "! Incompatible kernel version: $KERNEL_VER"
        exit 1
        ;;
esac

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
[ -z "$VERSION_NAME" ] && VERSION_NAME="v4.4"
ui_print "- Daemon $VERSION_NAME installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch / Magisk."
ui_print "- Installation complete! REBOOT YOUR DEVICE to apply update."
