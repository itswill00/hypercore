#!/system/bin/sh
SKIPUNZIP=1

ui_print "--------------------------------------"
ui_print "        Tanzanite HyperCore v3.0      "
ui_print "             by @itswill00            "
ui_print "--------------------------------------"
ui_print "- Extracting module files..."

unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

ui_print "- Setting permissions..."
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
[ -f "$MODPATH/hypercore_engine" ] && set_perm "$MODPATH/hypercore_engine" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/system.prop" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644

ui_print "- Native engine installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch."
ui_print "- Installation complete. Reboot to apply changes."
ui_print "--------------------------------------"
