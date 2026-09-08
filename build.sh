#!/system/bin/sh

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
OUTPUT_DIR="/data/data/com.termux/files/home/HyperCore_Releases"

set -e

cd "$PROJECT_DIR"

if [ ! -f "module.prop" ]; then
    echo "error: module.prop not found"
    exit 1
fi
VERSION=$(grep '^version=' module.prop | cut -d= -f2)
VERSION_CODE=$(grep '^versionCode=' module.prop | cut -d= -f2)
ZIP_OUT="HyperCore-${VERSION}-b${VERSION_CODE}-Unified.zip"

echo "building hypercore ${VERSION} (${VERSION_CODE})"

for tool in clang zip node npm ecj; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool is not installed"
        exit 1
    fi
done

if [ -d "webui" ]; then
    if [ ! -d "webui/node_modules" ]; then
        echo "installing webui dependencies..."
        (cd webui && npm install --no-audit --no-fund)
    fi
    
    echo "compiling webui..."
    if ! (cd webui && node ./node_modules/vite/bin/vite.js build); then
        echo "error: vite build failed"
        exit 1
    fi
    
    if [ ! -f "webui/dist/index.html" ]; then
        echo "error: webui output not found"
        exit 1
    fi
    
    mkdir -p webroot
    cp webui/dist/index.html webroot/index.html
    rm -f webroot/banner.jpg
fi

echo "generating sha256 checksums..."
rm -f checksums.txt
for file in system.prop service.sh post-fs-data.sh webroot/index.html changelog.md uninstall.sh banner.jpg; do
    if [ -f "$file" ]; then
        sha256sum "$file" >> checksums.txt
    fi
done

cat << 'EOF' > src/include/embedded_checksums.hpp
#ifndef EMBEDDED_CHECKSUMS_HPP
#define EMBEDDED_CHECKSUMS_HPP

typedef struct {
    const char *rel_path;
    const char *expected_sha256;
} file_checksum_t;

static const file_checksum_t g_embedded_checksums[] = {
EOF

while read -r hash path; do
    rel_path=$(echo "$path" | sed 's|^\./||')
    echo "    { \"$rel_path\", \"$hash\" }," >> src/include/embedded_checksums.hpp
done < checksums.txt

cat << 'EOF' >> src/include/embedded_checksums.hpp
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
EOF

echo "compiling c daemon..."
clang -O3 -Wall -Werror \
    -DVERSION=\"${VERSION}\" \
    -I./src/include \
    src/main.c \
    src/cpu.c \
    src/gpu.c \
    src/thermal.c \
    src/memory.c \
    src/io.c \
    src/gamelist.c \
    src/ipc.c \
    src/log.c \
    src/sysfs.c \
    src/integrity.c \
    src/sha256.c \
    src/charger.c \
    -o system/bin/libhypercore.so

echo "compiling hypermoon c daemon..."
clang -O3 -Wall -Wextra \
    src/hud/hypermoon_daemon.c \
    -o system/bin/hypermoon_daemon
chmod 755 system/bin/hypermoon_daemon

echo "compiling hypermoon java overlay dex..."
ANDROID_JAR="/data/data/com.termux/files/usr/share/java/android.jar"
if [ ! -f "$ANDROID_JAR" ]; then
    ANDROID_JAR=$(find /data/data/com.termux/files/ -name "android.jar" 2>/dev/null | head -n 1)
fi
rm -rf build/classes
mkdir -p build/classes
ecj -cp "$ANDROID_JAR" -d build/classes src/hud/HyperMoonOverlay.java
if command -v dx >/dev/null 2>&1; then
    dx --dex --output=system/bin/hypermoon.dex build/classes
elif command -v d8 >/dev/null 2>&1; then
    d8 --output system/bin/ build/classes/com/hypermoon/HyperMoonOverlay*.class
    mv system/bin/classes.dex system/bin/hypermoon.dex
fi
rm -rf build/classes
chmod 644 system/bin/hypermoon.dex

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR/HyperCore-${VERSION}-b${VERSION_CODE}"*.zip

echo "packaging zip package..."
if ! zip -r "$OUTPUT_DIR/$ZIP_OUT" \
    module.prop \
    system.prop \
    service.sh \
    post-fs-data.sh \
    customize.sh \
    system/bin/libhypercore.so \
    system/bin/hypercore-bugreport \
    system/bin/hypermoon_daemon \
    system/bin/hypermoon.dex \
    webroot/index.html \
    gamelist.txt \
    changelog.md \
    uninstall.sh \
    banner.jpg >/dev/null; then
    echo "error: packaging failed"
    exit 1
fi

echo "build finished: ${OUTPUT_DIR}/${ZIP_OUT}"

# Also sync copy to /sdcard/HyperCore_Releases for external root file managers
su -c "mkdir -p /sdcard/HyperCore_Releases && cp -f '$OUTPUT_DIR/$ZIP_OUT' /sdcard/HyperCore_Releases/ && chmod 666 '/sdcard/HyperCore_Releases/$ZIP_OUT'" 2>/dev/null || true
am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE -d "file:///sdcard/HyperCore_Releases/$ZIP_OUT" >/dev/null 2>&1 || true

if [ "$1" = "--deploy" ] || [ "$1" = "-d" ]; then
    echo "deploying to live device modules..."
    if su -c "
        pkill -9 -x libhypercore.so 2>/dev/null || true
        kill -9 $(pidof hypermoon_daemon 2>/dev/null) 2>/dev/null || true
        for p in \$(pgrep -f '[H]yperMoonOverlay' 2>/dev/null); do [ \"\$p\" != \"\$\$\" ] && kill -9 \"\$p\" 2>/dev/null || true; done
        MOD_TARGET=\"/data/adb/modules/hypercore\"
        DATA_TARGET=\"/data/adb/hypercore\"
        mkdir -p \"\$DATA_TARGET\"
        mkdir -p \"\$DATA_TARGET/hud\"
        if [ -d \"\$MOD_TARGET\" ]; then
            # Migrate existing user configs from module root to persistent data dir
            for f in charge_mode.conf custom_charge_limit.conf night_charging.conf smart_chg.conf protect_80.conf battery_cycle.conf; do
                if [ -f \"\$MOD_TARGET/\$f\" ] && [ ! -f \"\$DATA_TARGET/\$f\" ]; then
                    cp -f \"\$MOD_TARGET/\$f\" \"\$DATA_TARGET/\$f\" 2>/dev/null || true
                fi
                rm -f \"\$MOD_TARGET/\$f\"
            done
            if [ -f \"\$MOD_TARGET/gamelist.txt\" ] && [ ! -f \"\$DATA_TARGET/gamelist.txt\" ]; then
                cp -f \"\$MOD_TARGET/gamelist.txt\" \"\$DATA_TARGET/gamelist.txt\" 2>/dev/null || true
            fi
            rm -f \"\$MOD_TARGET/gamelist.txt\" \"\$MOD_TARGET/NOTICE.md\" \"\$MOD_TARGET/update.json\" \"\$MOD_TARGET/webroot/banner.jpg\"
            rm -f \"\$MOD_TARGET/hypercore.sock\" \"\$MOD_TARGET/hypercore.pid\" \"\$MOD_TARGET/status.json\"

            mkdir -p \$MOD_TARGET/system/bin
            mkdir -p \$MOD_TARGET/webroot
            rm -f \$MOD_TARGET/system/bin/libhypercore.so \$MOD_TARGET/system/bin/hypermoon_daemon \$MOD_TARGET/system/bin/hypermoon.dex
            cp system/bin/libhypercore.so \$MOD_TARGET/system/bin/libhypercore.so
            [ -f system/bin/hypercore-bugreport ] && cp system/bin/hypercore-bugreport \$MOD_TARGET/system/bin/hypercore-bugreport
            [ -f system/bin/hypermoon_daemon ] && cp system/bin/hypermoon_daemon \$MOD_TARGET/system/bin/hypermoon_daemon
            [ -f system/bin/hypermoon.dex ] && cp system/bin/hypermoon.dex \$MOD_TARGET/system/bin/hypermoon.dex
            cp webroot/index.html \$MOD_TARGET/webroot/index.html
            cp banner.jpg \$MOD_TARGET/banner.jpg
            cp module.prop \$MOD_TARGET/module.prop
            cp system.prop \$MOD_TARGET/system.prop
            cp service.sh \$MOD_TARGET/service.sh
            cp post-fs-data.sh \$MOD_TARGET/post-fs-data.sh
            cp uninstall.sh \$MOD_TARGET/uninstall.sh
            cp changelog.md \$MOD_TARGET/changelog.md
            chmod 755 \$MOD_TARGET/system/bin/*
            [ -f \$MOD_TARGET/system/bin/hypermoon.dex ] && chmod 644 \$MOD_TARGET/system/bin/hypermoon.dex
            chmod 755 \$MOD_TARGET/service.sh \$MOD_TARGET/post-fs-data.sh \$MOD_TARGET/uninstall.sh
            chmod 644 \$MOD_TARGET/module.prop \$MOD_TARGET/system.prop \$MOD_TARGET/banner.jpg \$MOD_TARGET/changelog.md \$MOD_TARGET/webroot/index.html
            rm -f /dev/hypercore.sock \$DATA_TARGET/hypercore.sock \$DATA_TARGET/hypercore.pid 2>/dev/null || true
            exec \$MOD_TARGET/system/bin/libhypercore.so
        fi
    "; then
        echo "daemon restarted successfully"
    else
        echo "error: deploy failed"
        exit 1
    fi
fi
