#!/system/bin/sh

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
OUTPUT_DIR="/sdcard/HyperCore_Releases"

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

for tool in clang zip node npm; do
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
    webroot/index.html \
    gamelist.txt \
    changelog.md \
    uninstall.sh \
    banner.jpg >/dev/null; then
    echo "error: packaging failed"
    exit 1
fi

am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE -d "file://$OUTPUT_DIR/$ZIP_OUT" >/dev/null 2>&1 || true

echo "build finished: ${OUTPUT_DIR}/${ZIP_OUT}"

if [ "$1" = "--deploy" ] || [ "$1" = "-d" ]; then
    echo "deploying to live device modules..."
    if su -c "
        pkill -9 -x libhypercore.so 2>/dev/null || true
        MOD_TARGET=\"/data/adb/modules/hypercore\"
        DATA_TARGET=\"/data/adb/hypercore\"
        mkdir -p \"\$DATA_TARGET\"
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
            rm -f \$MOD_TARGET/system/bin/libhypercore.so
            cp system/bin/libhypercore.so \$MOD_TARGET/system/bin/libhypercore.so
            [ -f system/bin/hypercore-bugreport ] && cp system/bin/hypercore-bugreport \$MOD_TARGET/system/bin/hypercore-bugreport
            cp webroot/index.html \$MOD_TARGET/webroot/index.html
            cp banner.jpg \$MOD_TARGET/banner.jpg
            cp module.prop \$MOD_TARGET/module.prop
            cp system.prop \$MOD_TARGET/system.prop
            cp service.sh \$MOD_TARGET/service.sh
            cp post-fs-data.sh \$MOD_TARGET/post-fs-data.sh
            cp uninstall.sh \$MOD_TARGET/uninstall.sh
            cp changelog.md \$MOD_TARGET/changelog.md
            chmod 755 \$MOD_TARGET/system/bin/*
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
