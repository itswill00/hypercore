#!/system/bin/sh

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"

set -e

cd "$PROJECT_DIR"

if [ ! -f "module.prop" ]; then
    echo "error: module.prop not found"
    exit 1
fi
VERSION=$(grep '^version=' module.prop | cut -d= -f2)
VERSION_CODE=$(grep '^versionCode=' module.prop | cut -d= -f2)
ZIP_OUT="Tanzanite-HyperCore-${VERSION}.zip"

echo "building tanzanite hypercore ${VERSION} (${VERSION_CODE})"

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
    cp "webui/dist/index.html" "webroot/index.html"
fi

echo "generating sha256 checksums..."
EMBED_HEADER="src/include/embedded_checksums.hpp"
cat << 'EOF' > "$EMBED_HEADER"
#ifndef EMBEDDED_CHECKSUMS_HPP
#define EMBEDDED_CHECKSUMS_HPP

typedef struct {
    const char *rel_path;
    const char *expected_sha256;
} file_checksum_t;

static const file_checksum_t g_embedded_checksums[] = {
EOF

for f in post-fs-data.sh service.sh system.prop module.prop update.json changelog.md NOTICE.md uninstall.sh webroot/index.html banner.jpg; do
    if [ -f "$f" ]; then
        hash=$(sha256sum "$f" | awk '{print $1}')
        echo "    { \"$f\", \"$hash\" }," >> "$EMBED_HEADER"
    fi
done

cat << 'EOF' >> "$EMBED_HEADER"
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
EOF

echo "compiling c daemon..."
rm -f hypercore libhypercore.so
rm -rf system/bin
mkdir -p system/bin

if ! clang -O3 -flto -s -fvisibility=hidden -Wl,--gc-sections -Wall -Wextra -Werror -I"$PROJECT_DIR/src/include" src/*.c -o system/bin/libhypercore.so; then
    echo "error: daemon build failed"
    exit 1
fi
chmod +x system/bin/libhypercore.so customize.sh post-fs-data.sh service.sh uninstall.sh

echo "packaging zip package..."
mkdir -p "$RELEASE_DIR"
rm -f "$RELEASE_DIR/$ZIP_OUT"

if ! zip -r9 "$RELEASE_DIR/$ZIP_OUT" \
    META-INF \
    system \
    webroot \
    customize.sh \
    module.prop \
    post-fs-data.sh \
    service.sh \
    system.prop \
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
        cp system/bin/libhypercore.so /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so
        cp webroot/index.html /data/adb/modules/tanzanite_hypercore/webroot/index.html
        cp module.prop /data/adb/modules/tanzanite_hypercore/module.prop
        cp update.json /data/adb/modules/tanzanite_hypercore/update.json
        cp changelog.md /data/adb/modules/tanzanite_hypercore/changelog.md
        chmod 755 /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so
        exec /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so
    "; then
        echo "daemon restarted successfully"
    else
        echo "error: deploy failed"
        exit 1
    fi
fi
