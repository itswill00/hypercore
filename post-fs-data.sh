#!/system/bin/sh
# HyperCore post-fs-data initialization script
# Ensures runtime state directory is ready before early daemon launch

mkdir -p /data/adb/hypercore 2>/dev/null || true
# 0700: telemetry + charge configs + gamelist live here — root-only, never
# world-readable. 0755 used to expose battery/thermal history to any app.
chmod 700 /data/adb/hypercore 2>/dev/null || true

