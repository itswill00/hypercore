#!/system/bin/sh
# Tanzanite HyperFlow post-fs-data initialization script

# Apply core sysctl VM tunables
sysctl -w vm.swappiness=65 >/dev/null 2>&1
sysctl -w vm.dirty_ratio=15 >/dev/null 2>&1
sysctl -w vm.dirty_background_ratio=5 >/dev/null 2>&1
sysctl -w vm.vfs_cache_pressure=90 >/dev/null 2>&1
sysctl -w vm.page-cluster=0 >/dev/null 2>&1
