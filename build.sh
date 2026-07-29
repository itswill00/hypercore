#!/system/bin/sh
# Build script for Tanzanite HyperCore

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"
ZIP_OUT="Tanzanite-HyperCore-v3.1.zip"

set -e

mkdir -p "$RELEASE_DIR"
cd "$PROJECT_DIR"

echo "-> Compiling hypercore daemon..."
clang -O3 -flto -s -I"$PROJECT_DIR/src/include" "$PROJECT_DIR"/src/*.c -o "$PROJECT_DIR/hypercore"

echo "-> Setting executable bits..."
chmod +x hypercore customize.sh post-fs-data.sh service.sh uninstall.sh

echo "-> Generating SHA-256 integrity checksums..."
sha256sum hypercore customize.sh post-fs-data.sh service.sh system.prop module.prop gamelist.txt uninstall.sh webroot/index.html > checksums.sha256

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
    uninstall.sh \
    checksums.sha256 \
    hypercore >/dev/null

cp "$RELEASE_DIR/$ZIP_OUT" "/data/data/com.termux/files/home/$ZIP_OUT"

echo "Done! Built: $RELEASE_DIR/$ZIP_OUT"
