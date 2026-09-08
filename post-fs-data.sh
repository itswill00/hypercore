#!/system/bin/sh
# HyperCore post-fs-data initialization script
# Ensures runtime state directory is ready before early daemon launch

mkdir -p /data/adb/hypercore 2>/dev/null || true
chmod 755 /data/adb/hypercore 2>/dev/null || true

