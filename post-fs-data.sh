#!/system/bin/sh
# Tanzanite HyperFlow post-fs-data initialization script

sysctl -w vm.swappiness=65 >/dev/null 2>&1
sysctl -w vm.dirty_ratio=15 >/dev/null 2>&1
sysctl -w vm.dirty_background_ratio=5 >/dev/null 2>&1
sysctl -w vm.vfs_cache_pressure=90 >/dev/null 2>&1
sysctl -w vm.page-cluster=0 >/dev/null 2>&1
sysctl -w vm.compaction_proactiveness=20 >/dev/null 2>&1

sysctl -w net.ipv4.tcp_fastopen=3 >/dev/null 2>&1
sysctl -w net.ipv4.tcp_syncookies=1 >/dev/null 2>&1
sysctl -w net.ipv4.tcp_low_latency=1 >/dev/null 2>&1

# Storage I/O Optimization (UFS & eMMC)
for b in /sys/block/sd*/queue /sys/block/mmcblk*/queue; do
    [ -f "$b/scheduler" ] && echo "none" > "$b/scheduler" 2>/dev/null || echo "mq-deadline" > "$b/scheduler" 2>/dev/null
    [ -f "$b/read_ahead_kb" ] && echo "128" > "$b/read_ahead_kb" 2>/dev/null
    [ -f "$b/iostats" ] && echo "0" > "$b/iostats" 2>/dev/null
    [ -f "$b/nr_requests" ] && echo "64" > "$b/nr_requests" 2>/dev/null
done
