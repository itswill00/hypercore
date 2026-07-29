# Tanzanite HyperCore

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/SoC-MediaTek_Helio_G99_Ultra-orange.svg)]()
[![Kernel](https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-ARM64_%2F_aarch64-red.svg)]()
[![Manager](https://img.shields.io/badge/Manager-KernelSU_%2F_APatch_%2F_Magisk-purple.svg)]()

**Tanzanite HyperCore** is a lightweight, low-overhead native C daemon and kernel optimization engine tailored specifically for MediaTek Helio G99 Ultra (MT6789 / MT6877) devices running Linux kernel 5.10.x.

Built from the ground up to address real-world frame drops, frequency ramp delays, and ZRAM compression spikes, HyperCore directly interfaces with kernel sysfs nodes, hardware IRQ affinities, and MediaTek GED / FPSGO frameworks.

---

## Technical Highlights

### ⚡ Governor Rate-Limit & Sustained Clocks
Eliminates micro-stutters during 60–120 FPS gameplay and social media feed scrolling by extending CPU down-rate limits (`sugov_ext` / `schedutil`) to 20ms–30ms. This prevents CPU frequencies from collapsing between frame render cycles.

### 🎮 Zero-Overhead Foreground Game Detector
Monitors active tasks in `/dev/cpuset/top-app/tasks` and cross-references `/proc/<pid>/cmdline`. Automatically switches to the `Gaming` profile when a recognized package is active, with zero background CPU polling overhead. Supports per-app profile overrides via `gamelist.txt` (e.g. `com.instagram.android:TOUCH`).

### 🔋 Battery Charge Thermal Guard
When gaming while plugged in, HyperCore dynamically caps charging current to 2.0A (`charge_control_limit`), preventing battery heat spikes over 45°C, preserving battery health, and avoiding premature thermal throttling.

### 🚀 Game Launch I/O Burst
Triggers a dynamic 512 KB UFS storage read-ahead burst upon game detection for 6 seconds to accelerate DEX loading and asset initialization, before relaxing to a 256 KB baseline.

### 🎯 Hardware IRQ Affinity Pinning
Binds Mali GPU (`mali_jd`), Graphics Engine Driver (`ged`), and Display (`mtk_dsi`) interrupt vectors directly to Cortex-A76 Big Cores (CPU 6–7, mask `0xc0`), ensuring high-priority display interrupts never get delayed on Little cores.

### 🖥️ Developer WebUI Dashboard
Integrated KernelSU / APatch / Magisk WebUI featuring a clean developer dark theme in sentence case, interactive profile override pills (`Auto`, `Gaming`, `Balanced`, `Touch`, `Saver`), dynamic `gamelist.txt` management, and 1-Click online module updates via `updateJson`.

---

## Engine Architecture

```
src/
├── include/
│   ├── common.hpp     # Shared data structures, constants, and profile enums
│   ├── sysfs.hpp      # Fast sysfs file descriptor read/write helpers
│   ├── log.hpp        # Circular log buffer & file rotation
│   ├── cpu.hpp        # Governor rate limits, cpuset, & frequency scaling
│   ├── gpu.hpp        # MediaTek GED & FPSGO FSTB synchronizer
│   ├── memory.hpp     # Virtual memory & ZRAM compaction tunables
│   ├── io.hpp         # Storage queue read-ahead & IRQ affinity pinning
│   ├── thermal.hpp    # Thermal zone scanning & hysteresis evaluation
│   └── gamelist.hpp   # cgroup top-app game detector & gamelist loader
├── cpu.c
├── gpu.c
├── io.c
├── log.c
├── main.c
├── memory.c
├── sysfs.c
├── thermal.c
└── gamelist.c
```

---

## Engine Profiles & Scaling Matrix

HyperCore continuously evaluates hardware state every 2 seconds:

| Profile | Conditions | Little Cores (CPU 0–5) | Big Cores (CPU 6–7) | GPU Floor | Down-Rate Limit |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen off | 500 – 1450 MHz | 725 – 1300 MHz | 300 MHz | 1000 µs |
| **Interactive** | Daily usage | 1200 – 2000 MHz | 1400 – 2200 MHz | 600 MHz | 20000 µs |
| **Touch** | Gestures / UI load spikes | 1400 – 2000 MHz | 1700 – 2200 MHz | 700 MHz | 25000 µs |
| **Gaming** | Foreground game active | 1400 – 2000 MHz | 1800 – 2200 MHz | 800 MHz | 30000 µs |
| **Thermal** | CPU ≥ 75°C or Bat ≥ 53°C | 900 – 2000 MHz | 1200 – 1800 MHz | 500 MHz | 5000 µs |

---

## Thermal Hysteresis Logic

HyperCore uses a 4-tier thermal evaluation model to prevent rapid frequency oscillation:

- **Tier 0 (Optimal)**: CPU < 58°C, Battery < 44°C. Full unthrottled performance.
- **Tier 1 (Normal Load)**: CPU 58°C–64°C, Battery 44°C–47°C. Standard operating range.
- **Tier 2 (Warm Load)**: CPU 65°C–74°C, Battery 48°C–52°C. Sustained heavy workload.
- **Tier 3 (Protection Hold)**: CPU ≥ 75°C, Battery ≥ 53°C (72°C/50°C during charging). Holds protective throttle state for 6 seconds (3 ticks) before relaxing.

---

## Game List Configuration (`gamelist.txt`)

Package names added to `gamelist.txt` automatically trigger the **Gaming** profile upon detection. You can also specify per-app profile overrides using the `:MODE` suffix:

```text
# Default Gaming trigger
com.tencent.ig
com.mobile.legends

# Custom profile overrides
com.instagram.android:TOUCH
com.ss.android.ugc.trill:TOUCH
```

`gamelist.txt` can be edited directly via the WebUI dashboard or located at `/data/adb/modules/tanzanite_hypercore/gamelist.txt`.

---

## Compilation

To compile the native binary with strict compiler flags (`-O3 -flto -s -Wall -Wextra -Werror`) and package the release ZIP:

```bash
./build.sh
```

Built packages are automatically placed in:
- `Tanzanite_HyperCore_Release/Tanzanite-HyperCore-v3.2.zip`
- `/sdcard/HyperCore_Releases/Tanzanite-HyperCore-v3.2.zip`

---

## Requirements

- **Device**: MediaTek Helio G99 Ultra (MT6789 / MT6877)
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License

Distributed under the **GNU General Public License v3.0**. See `LICENSE` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.
