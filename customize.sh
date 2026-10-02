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

HAS_FPSGO=0
if [ -d "/sys/kernel/fpsgo" ]; then
    HAS_FPSGO=1
fi

# ponytail: strictly GED + Mali + FPSGO — matches daemon validate_hardware_target() so install never lies
if [ $HAS_GED -eq 1 ] && [ $HAS_MALI_DEVFREQ -eq 1 ] && [ $HAS_FPSGO -eq 1 ]; then
    ui_print "- MediaTek MT6789 Family Hardware Verified (GED + Mali GPU + FPSGO OK)."
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

# Graceful stop before upgrade: SIGTERM lets the old daemon run
# restore_baseline_nodes() (~60 sysfs nodes) before it exits. SIGKILL here
# used to leave the device stranded on tuned nodes until the next reboot.
ui_print "- Stopping any active daemon instances before upgrade..."
pkill -15 -x libhypercore.so >/dev/null 2>&1 || true
for _i in 1 2 3 4 5 6 7 8 9 10; do
    pidof libhypercore.so >/dev/null 2>&1 || break
    sleep 1
done
pkill -9 -x libhypercore.so >/dev/null 2>&1 || true
rm -f /data/adb/modules/hypercore/hypercore.sock /data/adb/hypercore/hypercore.sock /data/adb/modules/hypercore/hypercore.pid /data/adb/hypercore/hypercore.pid /dev/hypercore.sock 2>/dev/null || true

ui_print "- Preserving user configurations in /data/adb/hypercore..."
mkdir -p /data/adb/hypercore
for conf in charge_mode.conf custom_charge_limit.conf night_charging.conf smart_chg.conf protect_80.conf battery_cycle.conf stock_state.conf zram.conf stock_zram.conf; do
    if [ -f "/data/adb/modules/hypercore/$conf" ] && [ ! -f "/data/adb/hypercore/$conf" ]; then
        cp -f "/data/adb/modules/hypercore/$conf" "/data/adb/hypercore/$conf" 2>/dev/null || true
    fi
done

# Stock baseline capture happens via scripts/stock_baseline.sh, but only AFTER
# the payload integrity check below — it is sourced as root, so it must not run
# on an unverified file.

PRESERVE_GL="/data/local/tmp/hypercore/hypercore_gamelist_bak.txt"
mkdir -p /data/local/tmp/hypercore 2>/dev/null || true
rm -f "$PRESERVE_GL"
if [ -f "/data/adb/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/hypercore/gamelist.txt" "$PRESERVE_GL"
elif [ -f "/data/adb/modules/hypercore/gamelist.txt" ]; then
    cp -f "/data/adb/modules/hypercore/gamelist.txt" "$PRESERVE_GL"
fi

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH"

ui_print "- Verifying SHA-256 integrity of the extracted payload..."
# This runs immediately after extraction BEFORE any file is modified or executed.
# It ensures everything in $MODPATH is verified against the signed manifest first.
chmod 755 "$MODPATH/system/bin/libhypercore.so" 2>/dev/null || true

INTEGRITY_OK=1
INTEGRITY_OUT=""

# Layer 1: the shipped manifest, which covers every payload file including
# libhypercore.so and gamelist.txt as extracted.
if [ -f "$MODPATH/checksums.txt" ] && command -v sha256sum >/dev/null 2>&1; then
    INTEGRITY_OUT=$(cd "$MODPATH" && sha256sum -c checksums.txt 2>&1)
    if [ $? -ne 0 ]; then
        INTEGRITY_OK=0
    fi
elif [ ! -f "$MODPATH/checksums.txt" ]; then
    ui_print "! WARNING: checksums.txt missing from package — cannot verify binaries."
    INTEGRITY_OK=0
else
    ui_print "! WARNING: sha256sum unavailable — binary manifest cannot be verified."
    INTEGRITY_OK=0
fi

# Layer 2: the table compiled into the daemon, which re-checks every script
# independently of the shell tooling.
#
# Only reached once layer 1 passed. Layer 2 is the one place the installer
# actually executes payload code, so it must never run against a binary that
# already failed the manifest.
if [ $INTEGRITY_OK -eq 1 ] && [ -f "$MODPATH/system/bin/libhypercore.so" ]; then
    EMBEDDED_OUT=$("$MODPATH/system/bin/libhypercore.so" --verify-integrity "$MODPATH" 2>&1)
    if [ $? -ne 0 ]; then
        INTEGRITY_OK=0
        INTEGRITY_OUT="$INTEGRITY_OUT
$EMBEDDED_OUT"
    fi
fi

if [ $INTEGRITY_OK -eq 1 ]; then
    ui_print "- Payload SHA-256 integrity verified successfully."
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

# Verified — now it is safe to run payload code.
if [ -f "$MODPATH/scripts/stock_baseline.sh" ]; then
    . "$MODPATH/scripts/stock_baseline.sh"
    capture_stock_baseline
fi

# Merge / preserve user gamelist
if [ -f "$PRESERVE_GL" ]; then
    ui_print "- Merging preserved user gamelist entries..."
    _GLTMP="/data/adb/hypercore/.gamelist.merge.tmp"
    if cat "$PRESERVE_GL" "$MODPATH/gamelist.txt" 2>/dev/null | awk -F: 'BEGIN { OFS=":" } /^[ \t\r]*#/ { if (!seen[$0]++) print; next } { key=$1; gsub(/^[ \t\r]+|[ \t\r]+$/, "", key); if (key == "") next; if (!seen[key]++) { $1=key; print } }' > "$_GLTMP" 2>/dev/null; then
        mv -f "$_GLTMP" /data/adb/hypercore/gamelist.txt
    else
        rm -f "$_GLTMP"
    fi
    rm -f "$PRESERVE_GL"
elif [ -f "/data/adb/hypercore/gamelist.txt" ]; then
    ui_print "- Keeping existing user gamelist in /data/adb/hypercore..."
elif [ -f "$MODPATH/gamelist.txt" ]; then
    cp -f "$MODPATH/gamelist.txt" /data/adb/hypercore/gamelist.txt 2>/dev/null || touch /data/adb/hypercore/gamelist.txt
fi

# Clean up non-module files, temporary configs, and duplicates from installed module root
rm -f "$MODPATH/gamelist.txt"
rm -f "$MODPATH/NOTICE.md"
rm -f "$MODPATH/update.json"
rm -f "$MODPATH/webroot/banner.jpg"
rm -f "$MODPATH"/*.conf
rm -f "$MODPATH/hypercore" "$MODPATH/libhypercore.so"
rm -f "$MODPATH/hypercore.sock" "$MODPATH/hypercore.pid" "$MODPATH/status.json"

ui_print "- Setting permissions & PATH symlinks..."
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
[ -d "$MODPATH/system/bin" ] && set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/system.prop" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644
set_perm "$MODPATH/checksums.txt" 0 0 0644
[ -f "$MODPATH/banner.jpg" ] && set_perm "$MODPATH/banner.jpg" 0 0 0644
[ -f "$MODPATH/changelog.md" ] && set_perm "$MODPATH/changelog.md" 0 0 0644

# Create symlinks in root manager PATH for KSU / APatch / Magisk
for manager_dir in /data/adb/ap/bin /data/adb/ksu/bin /data/adb/modules/bin; do
    if [ -d "$manager_dir" ]; then
        ui_print "- Creating PATH symlink in $manager_dir"
        ln -sf "$MODPATH/system/bin/libhypercore.so" "$manager_dir/libhypercore.so" 2>/dev/null || true
        [ -f "$MODPATH/system/bin/hypercore-bugreport" ] && ln -sf "$MODPATH/system/bin/hypercore-bugreport" "$manager_dir/hypercore-bugreport" 2>/dev/null || true
        [ -f "$MODPATH/system/bin/hypermoon_d" ] && ln -sf "$MODPATH/system/bin/hypermoon_d" "$manager_dir/hypermoon_d" 2>/dev/null || true
    fi
done

# HyperMoon HUD State directory & FPS Moon migration
mkdir -p /data/adb/hypercore/hud
if [ -d "/data/adb/modules/fps_moon/state" ] && [ ! -f "/data/adb/hypercore/hud/config.json" ]; then
    ui_print "- Migrating existing FPS Moon preferences to HyperMoon..."
    cp -rf /data/adb/modules/fps_moon/state/* /data/adb/hypercore/hud/ 2>/dev/null || true
fi

if [ ! -f "/data/adb/hypercore/hud/config.json" ]; then
    cat << 'EOF' > /data/adb/hypercore/hud/config.json
{
  "visible": false,
  "auto_gaming": false,
  "show_fps": true,
  "show_cpu": true,
  "show_cpu_freq": true,
  "show_gov": false,
  "show_gpu": true,
  "show_gpu_freq": true,
  "show_gpu_gov": false,
  "show_ram": true,
  "show_zram": false,
  "show_battery": true,
  "show_net": true,
  "is_horizontal": false,
  "align": "left",
  "theme": "cyber_neon",
  "custom_color": "#6366F1",
  "opacity": 0.7,
  "scale": 0.65,
  "font_size": 14,
  "corner_radius": 14,
  "bg_width": 150,
  "bg_height": 160,
  "refresh_interval": 1500,
  "target_fps": 60
}
EOF
    chmod 644 /data/adb/hypercore/hud/config.json 2>/dev/null || true
fi

if [ ! -f "/data/adb/hypercore/hud/position.json" ]; then
    cat << 'EOF' > /data/adb/hypercore/hud/position.json
{
  "x": 697,
  "y": 411
}
EOF
    chmod 644 /data/adb/hypercore/hud/position.json 2>/dev/null || true
fi
chmod 755 /data/adb/hypercore/hud 2>/dev/null || true

if [ ! -f "/data/adb/hypercore/.gamelist_autodetected" ]; then
    ui_print "- Auto-detecting installed games on your device (first install)..."
    AUTO_GAMES=$(pm list packages -3 2>/dev/null | cut -d: -f2 | grep -iE 'game|legend|pubg|mihoyo|genshin|honkai|freefire|roblox|activision|shooter|mojang|minecraft|supercell|brawl|clash|garena|stumble|pokemon|wanda|maleo|konami|krafton|netmarble|nexon|ea\.gp|riotgames|square_enix|bandainamco|gameloft|zynga|rovio|miniclip|yostar|ubisoft|subwaysurf|bussimulator|carxtech|slither|angrybirds|asphalt|shadowfight|realracing|needforspeed|efootball|nintendo|sega|squareenix|capcom|kiloo|innersloth|levelinfinite' 2>/dev/null)

    if [ -n "$AUTO_GAMES" ]; then
        for pkg in $AUTO_GAMES; do
            # Escape regex dots: an unescaped "com.foo" pattern also matches
            # "comXfoo" and could skip a legitimate auto-add.
            _pkg_esc=$(printf '%s' "$pkg" | sed 's/\./\\./g')
            if ! grep -q -E "^${_pkg_esc}(:|$)" /data/adb/hypercore/gamelist.txt 2>/dev/null; then
                echo "${pkg}:GAMING" >> /data/adb/hypercore/gamelist.txt
                ui_print "  + Auto-added game: $pkg"
            fi
        done
    else
        ui_print "  (No installed games auto-detected, gamelist ready for manual entries)"
    fi
    touch /data/adb/hypercore/.gamelist_autodetected 2>/dev/null || true
else
    ui_print "- Preserved user-managed gamelist preferences."
fi
rm -f "$MODPATH/gamelist.txt"
set_perm /data/adb/hypercore/gamelist.txt 0 0 0644
for c in /data/adb/hypercore/*.conf; do
    [ -f "$c" ] && set_perm "$c" 0 0 0644
done

VERSION_NAME=$(grep '^version=' "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
[ -z "$VERSION_NAME" ] && VERSION_NAME="v6.11.3"
ui_print "- Daemon $VERSION_NAME installed successfully."
ui_print "- WebUI Dashboard enabled for KernelSU / APatch / Magisk."
ui_print "- Installation complete! REBOOT YOUR DEVICE to apply update."
