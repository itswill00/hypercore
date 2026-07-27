#!/system/bin/sh
# Tanzanite HyperCore Automated Build & Packaging Script
# Author: @itswill00

MOD_DIR="/data/data/com.termux/files/home/HyperFlow_Module"
OUT_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"
ZIP_NAME="Tanzanite-HyperCore-v3.0.zip"

mkdir -p "$OUT_DIR"

echo "======================================"
echo " Building Tanzanite HyperCore v3.0   "
echo "======================================"

echo "[1/4] Compiling native C engine..."
clang -O3 -flto -s "$MOD_DIR/src/hypercore.c" -o "$MOD_DIR/hypercore_engine"
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed!"
    exit 1
fi

echo "[2/4] Setting file permissions..."
chmod 755 "$MOD_DIR/hypercore_engine"
chmod 755 "$MOD_DIR/customize.sh"
chmod 755 "$MOD_DIR/post-fs-data.sh"
chmod 755 "$MOD_DIR/service.sh"
chmod 755 "$MOD_DIR/uninstall.sh"

echo "[3/4] Packing clean release zip archive..."
cd "$MOD_DIR" || exit 1
rm -f "$OUT_DIR/$ZIP_NAME"

zip -r "$OUT_DIR/$ZIP_NAME" \
    META-INF \
    webroot \
    customize.sh \
    module.prop \
    post-fs-data.sh \
    service.sh \
    system.prop \
    uninstall.sh \
    hypercore_engine

cp "$OUT_DIR/$ZIP_NAME" "/data/data/com.termux/files/home/$ZIP_NAME"
cp "$MOD_DIR/LICENSE" "$OUT_DIR/LICENSE" 2>/dev/null

echo "[4/4] Verification..."
ls -lh "$OUT_DIR/$ZIP_NAME"
echo "======================================"
echo " Build Completed Successfully!        "
echo " Output: $OUT_DIR/$ZIP_NAME           "
echo "======================================"
