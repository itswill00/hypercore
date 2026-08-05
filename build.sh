#!/system/bin/sh

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/HyperCore_Release"

set -e

cd "$PROJECT_DIR"

if [ ! -f "module.prop" ]; then
    echo "error: module.prop not found"
    exit 1
fi
VERSION=$(grep '^version=' module.prop | cut -d= -f2)
VERSION_CODE=$(grep '^versionCode=' module.prop | cut -d= -f2)
ZIP_OUT="HyperCore-${VERSION}.zip"

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
fi

echo "generating sha256 checksums..."
find . -maxdepth 2 -type f \
    ! -path "./.*" \
    ! -path "./webui/*" \
    ! -path "./src/*" \
    ! -path "./system/bin/*" \
    ! -name "*.zip" \
    ! -name "*.o" \
    ! -name "build.sh" \
    ! -name "checksums.txt" \
    -exec sha256sum {} + | sort -k2 > checksums.txt

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
    -o system/bin/libhypercore.so

mkdir -p "$RELEASE_DIR"
rm -f "$RELEASE_DIR/$ZIP_OUT"

echo "packaging zip package..."
if ! zip -r "$RELEASE_DIR/$ZIP_OUT" \
    module.prop \
    system.prop \
    service.sh \
    post-fs-data.sh \
    customize.sh \
    system/bin/libhypercore.so \
    webroot/index.html \
    gamelist.txt \
    update.json \
    changelog.md \
    NOTICE.md \
    uninstall.sh \
    banner.jpg >/dev/null; then
    echo "error: packaging failed"
    exit 1
fi

INTERNAL_DIR="/sdcard/HyperCore_Releases"
mkdir -p "$INTERNAL_DIR"
cp "$RELEASE_DIR/$ZIP_OUT" "/data/data/com.termux/files/home/$ZIP_OUT"
cp "$RELEASE_DIR/$ZIP_OUT" "$INTERNAL_DIR/$ZIP_OUT"

echo "build finished: ${RELEASE_DIR}/${ZIP_OUT}"

if [ "$1" = "--deploy" ] || [ "$1" = "-d" ]; then
    echo "deploying to live device modules..."
    if su -c "
        pkill -9 -x libhypercore.so 2>/dev/null || true
        MOD_TARGET=\"/data/adb/modules/hypercore\"
        if [ ! -d \"\$MOD_TARGET\" ]; then
            MOD_TARGET=\"/data/adb/modules/tanzanite_hypercore\"
        fi
        if [ -d \"\$MOD_TARGET\" ]; then
            cp system/bin/libhypercore.so \$MOD_TARGET/system/bin/libhypercore.so
            cp webroot/index.html \$MOD_TARGET/webroot/index.html
            cp module.prop \$MOD_TARGET/module.prop
            cp update.json \$MOD_TARGET/update.json
            cp changelog.md \$MOD_TARGET/changelog.md
            chmod 755 \$MOD_TARGET/system/bin/libhypercore.so
            exec \$MOD_TARGET/system/bin/libhypercore.so
        fi
    "; then
        echo "daemon restarted successfully"
    else
        echo "error: deploy failed"
        exit 1
    fi
fi
