#!/system/bin/sh
SKIPUNZIP=1

ui_print "--------------------------------------"
ui_print "        Tanzanite HyperCore v3.0      "
ui_print "             by @itswill00            "
ui_print "--------------------------------------"

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
        ui_print "! Tanzanite HyperCore is strictly designed for Linux 5.10.x."
        ui_print "! Installation aborted."
        ui_print "--------------------------------------"
        abort "! Incompatible kernel version: $KERNEL_VER"
        exit 1
        ;;
    *)
        ui_print "--------------------------------------"
        ui_print "! ERROR: Unsupported Kernel version ($KERNEL_VER)!"
        ui_print "! Tanzanite HyperCore only supports Linux kernel 5.10.x."
        ui_print "! Installation aborted."
        ui_print "--------------------------------------"
        abort "! Incompatible kernel version: $KERNEL_VER"
        exit 1
        ;;
esac

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

ui_print "- Setting permissions..."
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
[ -f "$MODPATH/hypercore" ] && set_perm "$MODPATH/hypercore" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/system.prop" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644

ui_print "- Native daemon installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch."
ui_print "- Installation complete. Reboot to apply changes."
ui_print "--------------------------------------"
