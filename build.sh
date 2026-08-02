#!/system/bin/sh
# Build script for Tanzanite HyperCore

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"
ZIP_OUT="Tanzanite-HyperCore-v4.2.zip"

set -e

mkdir -p "$RELEASE_DIR"
cd "$PROJECT_DIR"

if [ -d "$PROJECT_DIR/webui" ]; then
    echo "-> Compiling Vue 3 WebUI..."
    (cd "$PROJECT_DIR/webui" && node ./node_modules/vite/bin/vite.js build >/dev/null 2>&1)
    cp "$PROJECT_DIR/webui/dist/index.html" "$PROJECT_DIR/webroot/index.html"
fi

rm -f "$PROJECT_DIR/hypercore" "$PROJECT_DIR/libhypercore.so"
rm -rf "$PROJECT_DIR/system/bin"
mkdir -p "$PROJECT_DIR/system/bin"

echo "-> Embedding SHA-256 checksums into C binary header..."
EMBED_HEADER="$PROJECT_DIR/src/include/embedded_checksums.hpp"
cat << 'EOF' > "$EMBED_HEADER"
#ifndef EMBEDDED_CHECKSUMS_HPP
#define EMBEDDED_CHECKSUMS_HPP

typedef struct {
    const char *rel_path;
    const char *expected_sha256;
} file_checksum_t;

static const file_checksum_t g_embedded_checksums[] = {
EOF

for f in customize.sh post-fs-data.sh service.sh system.prop module.prop gamelist.txt update.json changelog.md uninstall.sh webroot/index.html; do
    if [ -f "$f" ]; then
        hash=$(sha256sum "$f" | awk '{print $1}')
        echo "    { \"$f\", \"$hash\" }," >> "$EMBED_HEADER"
    fi
done

cat << 'EOF' >> "$EMBED_HEADER"
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
EOF

echo "-> Compiling single hypercore daemon into system/bin/libhypercore.so..."
clang -O3 -flto -s -fvisibility=hidden -Wl,--gc-sections -Wall -Wextra -Werror -I"$PROJECT_DIR/src/include" "$PROJECT_DIR"/src/*.c -o "$PROJECT_DIR/system/bin/libhypercore.so"
echo "-> Setting executable bits..."
chmod +x system/bin/libhypercore.so customize.sh post-fs-data.sh service.sh uninstall.sh

echo "-> Generating SHA-256 integrity checksums..."
sha256sum system/bin/libhypercore.so customize.sh post-fs-data.sh service.sh system.prop module.prop gamelist.txt update.json changelog.md uninstall.sh webroot/index.html > checksums.sha256

echo "-> Packaging module zip..."
rm -f "$RELEASE_DIR/$ZIP_OUT"
zip -r9 "$RELEASE_DIR/$ZIP_OUT" \
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
    uninstall.sh \
    checksums.sha256 >/dev/null

INTERNAL_DIR="/sdcard/HyperCore_Releases"
mkdir -p "$INTERNAL_DIR"
cp "$RELEASE_DIR/$ZIP_OUT" "/data/data/com.termux/files/home/$ZIP_OUT"
cp "$RELEASE_DIR/$ZIP_OUT" "$INTERNAL_DIR/$ZIP_OUT"

echo "Done! Built: $RELEASE_DIR/$ZIP_OUT"
echo "Internal: $INTERNAL_DIR/$ZIP_OUT"
