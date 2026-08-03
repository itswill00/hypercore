#!/system/bin/sh
# Modernized Build Script for Tanzanite HyperCore

PROJECT_DIR="/data/data/com.termux/files/home/HyperCore_Module"
RELEASE_DIR="/data/data/com.termux/files/home/Tanzanite_HyperCore_Release"

set -e

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Tanzanite HyperCore Build System ===${NC}"

cd "$PROJECT_DIR"

# 1. Parse version from module.prop
if [ ! -f "module.prop" ]; then
    echo -e "${RED}[ERROR] module.prop not found!${NC}"
    exit 1
fi
VERSION=$(grep '^version=' module.prop | cut -d= -f2)
VERSION_CODE=$(grep '^versionCode=' module.prop | cut -d= -f2)
ZIP_OUT="Tanzanite-HyperCore-${VERSION}.zip"
echo -e "${GREEN}[INFO] Detected Version: ${VERSION} (Code: ${VERSION_CODE})${NC}"

# 2. Check for required tools
for tool in clang zip node npm; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo -e "${RED}[ERROR] Required tool '$tool' is not installed!${NC}"
        exit 1
    fi
done

# 3. Handle WebUI Compilation
if [ -d "webui" ]; then
    echo -e "${YELLOW}[BUILD] Processing Vue 3 WebUI...${NC}"
    
    # Check if node_modules is missing
    if [ ! -d "webui/node_modules" ]; then
        echo -e "${YELLOW}[DEPS] node_modules not found. Installing webui dependencies...${NC}"
        (cd webui && npm install --no-audit --no-fund)
    fi
    
    echo -e "${YELLOW}[BUILD] Compiling WebUI production bundle...${NC}"
    if ! (cd webui && node ./node_modules/vite/bin/vite.js build); then
        echo -e "${RED}[ERROR] Vite compilation failed!${NC}"
        exit 1
    fi
    
    # Verify build output
    if [ ! -f "webui/dist/index.html" ]; then
        echo -e "${RED}[ERROR] webui/dist/index.html was not generated!${NC}"
        exit 1
    fi
    
    mkdir -p webroot
    cp "webui/dist/index.html" "webroot/index.html"
    echo -e "${GREEN}[SUCCESS] WebUI compiled and copied to webroot/index.html${NC}"
fi

# 4. Generate embedded checksums
echo -e "${YELLOW}[BUILD] Generating SHA-256 checksums...${NC}"
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

for f in customize.sh post-fs-data.sh service.sh system.prop module.prop gamelist.txt update.json changelog.md uninstall.sh webroot/index.html banner.jpg; do
    if [ -f "$f" ]; then
        hash=$(sha256sum "$f" | awk '{print $1}')
        echo "    { \"$f\", \"$hash\" }," >> "$EMBED_HEADER"
    fi
done

cat << 'EOF' >> "$EMBED_HEADER"
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
EOF
echo -e "${GREEN}[SUCCESS] Checksums embedded in C header.${NC}"

# 5. Compile C Daemon
echo -e "${YELLOW}[BUILD] Compiling C daemon binary...${NC}"
rm -f hypercore libhypercore.so
rm -rf system/bin
mkdir -p system/bin

if ! clang -O3 -flto -s -fvisibility=hidden -Wl,--gc-sections -Wall -Wextra -Werror -I"$PROJECT_DIR/src/include" src/*.c -o system/bin/libhypercore.so; then
    echo -e "${RED}[ERROR] C Daemon compilation failed!${NC}"
    exit 1
fi
chmod +x system/bin/libhypercore.so customize.sh post-fs-data.sh service.sh uninstall.sh
echo -e "${GREEN}[SUCCESS] Daemon compiled to system/bin/libhypercore.so${NC}"

# 6. Package Module Zip
echo -e "${YELLOW}[BUILD] Packaging release ZIP: ${ZIP_OUT}...${NC}"
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
    uninstall.sh \
    banner.jpg >/dev/null; then
    echo -e "${RED}[ERROR] Zip packaging failed!${NC}"
    exit 1
fi

INTERNAL_DIR="/sdcard/HyperCore_Releases"
mkdir -p "$INTERNAL_DIR"
cp "$RELEASE_DIR/$ZIP_OUT" "/data/data/com.termux/files/home/$ZIP_OUT"
cp "$RELEASE_DIR/$ZIP_OUT" "$INTERNAL_DIR/$ZIP_OUT"

echo -e "${GREEN}[SUCCESS] Build completed!${NC}"
echo -e "Release Output: ${RELEASE_DIR}/${ZIP_OUT}"
echo -e "Internal Storage: ${INTERNAL_DIR}/${ZIP_OUT}"

# 7. Check for --deploy flag
if [ "$1" = "--deploy" ] || [ "$1" = "-d" ]; then
    echo -e "${YELLOW}[DEPLOY] Deploying to live modules path...${NC}"
    if su -c "
        pkill -9 -x libhypercore.so 2>/dev/null || true
        cp system/bin/libhypercore.so /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so
        cp webroot/index.html /data/adb/modules/tanzanite_hypercore/webroot/index.html
        cp module.prop /data/adb/modules/tanzanite_hypercore/module.prop
        cp update.json /data/adb/modules/tanzanite_hypercore/update.json
        cp changelog.md /data/adb/modules/tanzanite_hypercore/changelog.md
        chmod 755 /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so
        /data/adb/modules/tanzanite_hypercore/system/bin/libhypercore.so &
    "; then
        echo -e "${GREEN}[SUCCESS] Successfully deployed to device and restarted daemon!${NC}"
    else
        echo -e "${RED}[ERROR] Live deployment failed!${NC}"
        exit 1
    fi
fi
