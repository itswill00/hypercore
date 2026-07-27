# Tanzanite HyperFlow

Kernel and system performance tuner for MediaTek Helio G99 Ultra (MT6789) running HyperOS, MIUI, or AOSP.

Author: **@itswill00**

---

## What it does

Tanzanite HyperFlow runs a lightweight native C daemon (`hyperflow_engine`) that dynamically tunes CPU scaling, GPU frequencies, storage queues, and ZRAM based on current load, thermal state, and active apps.

- **Native C Engine:** Compiles to a 22 KB ARM64 binary with zero subshell overhead and low RAM usage (<500 KB).
- **Dynamic Profiles:** Automatically switches between `Sleep`, `Interactive`, `Touch Boost`, `Gaming`, and `Thermal Protection`.
- **UI Smoothness:** Sets CPU frequency ramp rate limits (`up_rate_limit_us`) to `0us` for instant response on touch.
- **Storage Queues:** Configures UFS storage queues to `none` scheduler with 128 KB read-ahead.
- **Memory Tuning:** Sets `vm.page-cluster = 0` for direct ZRAM swap I/O.
- **Network Tuning:** Sets `cubic` TCP congestion control and enables Wi-Fi low-latency mode.
- **Native WebUI Dashboard:** Built-in web dashboard for KernelSU, APatch, and MMRL Manager.

## Installation

1. Open **KernelSU Manager**, **APatch**, or **Magisk**.
2. Flash `Tanzanite-HyperFlow-v2.5.zip`.
3. Reboot your device.

## WebUI Management

Open the WebUI tab inside **KernelSU Manager**, **APatch**, or **MMRL** to view real-time hardware metrics, toggle Gaming Lock mode, flush RAM, or restart the engine daemon.

## License

MIT License.
