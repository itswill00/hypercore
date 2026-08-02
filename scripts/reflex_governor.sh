#!/system/bin/sh
# Minimalist Reflex CPU Governor Detector & Tuner for Custom Kernels

AVAIL0=$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors 2>/dev/null)
AVAIL6=$(cat /sys/devices/system/cpu/cpufreq/policy6/scaling_available_governors 2>/dev/null)

if echo "$AVAIL0 $AVAIL6" | grep -q "reflex"; then
    echo "[+] Reflex governor detected on custom kernel!"
    echo "reflex" > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor 2>/dev/null || true
    echo "reflex" > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor 2>/dev/null || true
else
    echo "[-] Standard kernel detected (reflex not present)."
fi
