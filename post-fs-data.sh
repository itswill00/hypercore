#!/system/bin/sh
# HyperCore post-fs-data initialization script

sysctl -w vm.swappiness=60 >/dev/null 2>&1
sysctl -w vm.dirty_ratio=15 >/dev/null 2>&1
sysctl -w vm.dirty_background_ratio=5 >/dev/null 2>&1
sysctl -w vm.vfs_cache_pressure=100 >/dev/null 2>&1
sysctl -w vm.page-cluster=0 >/dev/null 2>&1
