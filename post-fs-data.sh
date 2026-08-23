#!/system/bin/sh
# HyperCore post-fs-data initialization script
# Direct procfs writes are used to ensure reliable early-boot execution
# without depending on the availability of the sysctl binary in PATH.

write_vm() {
    node="/proc/sys/vm/$1"
    val="$2"
    if [ -f "$node" ]; then
        echo "$val" > "$node" 2>/dev/null || sysctl -w "vm.$1=$val" >/dev/null 2>&1 || true
    fi
}

write_vm "swappiness" 60
write_vm "dirty_ratio" 15
write_vm "dirty_background_ratio" 5
write_vm "vfs_cache_pressure" 100
write_vm "page-cluster" 0
