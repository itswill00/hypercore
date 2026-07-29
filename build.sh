#!/system/bin/sh
# Build script for Tanzanite HyperCore

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"
ZIP_OUT="Tanzanite-HyperCore-v3.3.zip"

set -e

mkdir -p "$RELEASE_DIR"
cd "$PROJECT_DIR"

echo "-> Compiling hypercore daemon..."
clang -O3 -flto -s -Wall -Wextra -Werror -I"$PROJECT_DIR/src/include" "$PROJECT_DIR"/src/*.c -o "$PROJECT_DIR/hypercore"

echo "-> Setting executable bits..."
chmod +x hypercore customize.sh post-fs-data.sh service.sh uninstall.sh

echo "-> Generating SHA-256 integrity checksums..."
sha256sum hypercore customize.sh post-fs-data.sh service.sh system.prop module.prop gamelist.txt update.json changelog.md uninstall.sh webroot/index.html > checksums.sha256

echo "-> Packaging module zip..."
rm -f "$RELEASE_DIR/$ZIP_OUT"
zip -r9 "$RELEASE_DIR/$ZIP_OUT" \
    META-INF \
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
    checksums.sha256 \
    hypercore >/dev/null

INTERNAL_DIR="/sdcard/HyperCore_Releases"
mkdir -p "$INTERNAL_DIR"
cp "$RELEASE_DIR/$ZIP_OUT" "/data/data/com.termux/files/home/$ZIP_OUT"
cp "$RELEASE_DIR/$ZIP_OUT" "$INTERNAL_DIR/$ZIP_OUT"

echo "Done! Built: $RELEASE_DIR/$ZIP_OUT"
echo "Internal: $INTERNAL_DIR/$ZIP_OUT"
