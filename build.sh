#!/system/bin/sh

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
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
# ponytail: single source of truth = module.prop, ceiling sed is best-effort, no VERSION file dup
sed -i "s/badge\/Release-v[^-\"]*/badge\/Release-${VERSION}/" README.md 2>/dev/null || true
sed -i "s/HyperCore-v[^-\"]*-b[0-9][0-9]*/HyperCore-${VERSION}-b${VERSION_CODE}/" README.md 2>/dev/null || true
sed -i "s/Version.*: v.*/Version        : ${VERSION} (Build ${VERSION_CODE})/" docs/DOCUMENTATION.txt 2>/dev/null || true
sed -i "s/Release ZIP.*: HyperCore.*/Release ZIP    : HyperCore-${VERSION}-b${VERSION_CODE}-Unified.zip/" docs/DOCUMENTATION.txt 2>/dev/null || true
sed -i "s/VERSION_NAME=\"v.*\"/VERSION_NAME=\"${VERSION}\"/" customize.sh 2>/dev/null || true

echo "building hypercore ${VERSION} (${VERSION_CODE})"

for tool in clang zip node npm; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool is not installed"
        exit 1
    fi
done

# The kernel truncates a process name to 15 characters (TASK_COMM_LEN - 1) in
# /proc/PID/comm, which is what `pkill -x` and `pidof` match against. A longer
# name is silently invisible to them, so a long-running daemon can never be
# stopped and never be found again. `hypermoon_daemon` was 16 chars and leaked
# one process per HUD toggle, then survived uninstall entirely.
#
# This applies to daemons only. One-shot helpers such as hypercore-bugreport are
# invoked synchronously and, if they ever hang, `pkill -f <path>` matches the
# full command line and is unaffected by the truncation.
for daemon in libhypercore.so hypermoon_d; do
    name=$(basename "$daemon")
    if [ "${#name}" -gt 15 ]; then
        echo "error: daemon binary name '$name' is ${#name} chars; must be <= 15 to stay visible to pkill -x / pidof"
        exit 1
    fi
done
# ecj/dx optional for hypermoon dex (skipped if missing, ponytail: no hard fail for HUD)

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
# Shipped payload, verified at install time by customize.sh (sha256sum -c) and
# re-checked from inside the daemon. This MUST cover the executables — a manifest
# of non-executable files verifies nothing, since the files that can actually
# compromise the device are the ones it skipped.
#
# libhypercore.so is the one unavoidable exclusion: a binary cannot contain its
# own hash. It is covered by checksums.txt (generated after the final link) and
# verified by customize.sh before the daemon is ever started.
CHECKSUM_FILES="system.prop service.sh post-fs-data.sh customize.sh uninstall.sh changelog.md banner.jpg webroot/index.html scripts/stock_baseline.sh system/bin/hypermoon_d system/bin/hypermoon.dex system/bin/hypercore-bugreport"

write_embedded_table() {
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
    done < "$1"
    cat << 'EOF' >> src/include/embedded_checksums.hpp
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
EOF
}

# Pass 1: a stub table so the pre-hash link has something valid to include.
rm -f checksums.txt
write_embedded_table /dev/null

echo "compiling hypermoon c daemon..."
clang -O3 -Wall -Wextra \
    src/hud/hypermoon_daemon.c \
    -o system/bin/hypermoon_d
chmod 755 system/bin/hypermoon_d

echo "compiling hypermoon java overlay dex..."
if command -v ecj >/dev/null 2>&1 && [ -f "src/hud/HyperMoonOverlay.java" ]; then
ANDROID_JAR="/data/data/com.termux/files/usr/share/java/android.jar"
if [ ! -f "$ANDROID_JAR" ]; then
    ANDROID_JAR=$(find /data/data/com.termux/files/ -name "android.jar" 2>/dev/null | head -n 1)
fi
rm -rf build/classes
mkdir -p build/classes
if ecj -cp "$ANDROID_JAR" -d build/classes src/hud/HyperMoonOverlay.java 2>/dev/null || javac -cp "$ANDROID_JAR" -d build/classes src/hud/HyperMoonOverlay.java 2>/dev/null; then
if command -v dx >/dev/null 2>&1; then
    dx --dex --output=system/bin/hypermoon.dex build/classes
elif command -v d8 >/dev/null 2>&1; then
    d8 --output system/bin/ build/classes/com/hypermoon/HyperMoonOverlay*.class 2>/dev/null
    mv system/bin/classes.dex system/bin/hypermoon.dex 2>/dev/null || true
fi
fi
rm -rf build/classes
[ -f system/bin/hypermoon.dex ] && chmod 644 system/bin/hypermoon.dex || echo "warning: hypermoon.dex not built (ecj/dx missing), continuing"
else
    echo "warning: ecj not found, skipping hypermoon dex build"
    [ -f system/bin/hypermoon.dex ] || touch system/bin/hypermoon.dex 2>/dev/null || true
fi

# All non-daemon payloads now exist, so the manifest can be hashed for real.
for file in $CHECKSUM_FILES; do
    if [ ! -f "$file" ]; then
        echo "error: expected payload missing, cannot build a trustworthy manifest: $file"
        exit 1
    fi
    sha256sum "$file" >> checksums.txt
done
write_embedded_table checksums.txt

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
chmod 755 system/bin/libhypercore.so

# The daemon cannot embed its own hash, so append it to the shipped manifest
# only after the final link. customize.sh verifies this before starting it.
sha256sum system/bin/libhypercore.so >> checksums.txt

# Fail loudly rather than shipping a manifest that does not match the payload.
if ! sha256sum -c checksums.txt >/dev/null 2>&1; then
    echo "error: checksums.txt does not match the built payload"
    sha256sum -c checksums.txt || true
    exit 1
fi
echo "manifest verified: $(wc -l < checksums.txt) files"

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR/HyperCore-${VERSION}-b${VERSION_CODE}"*.zip

echo "packaging zip package..."
if ! zip -r "$OUTPUT_DIR/$ZIP_OUT" \
    module.prop \
    system.prop \
    service.sh \
    post-fs-data.sh \
    customize.sh \
    checksums.txt \
    scripts/stock_baseline.sh \
    system/bin/libhypercore.so \
    system/bin/hypercore-bugreport \
    system/bin/hypermoon_d \
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
        pkill -9 -x hypercore_daemon 2>/dev/null || true
        pkill -9 -x libhypercore.so 2>/dev/null || true
        pkill -9 -x hypermoon_d 2>/dev/null || true
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
            rm -f \$MOD_TARGET/system/bin/libhypercore.so \$MOD_TARGET/system/bin/hypermoon_d \$MOD_TARGET/system/bin/hypermoon.dex
            cp system/bin/libhypercore.so \$MOD_TARGET/system/bin/libhypercore.so
            [ -f system/bin/hypercore-bugreport ] && cp system/bin/hypercore-bugreport \$MOD_TARGET/system/bin/hypercore-bugreport
            [ -f system/bin/hypermoon_d ] && cp system/bin/hypermoon_d \$MOD_TARGET/system/bin/hypermoon_d
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
