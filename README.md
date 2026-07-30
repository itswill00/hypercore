# Tanzanite HyperCore

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/SoC-MediaTek_Helio_G99_Ultra-orange.svg)]()
[![Kernel](https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-ARM64-red.svg)]()
[![Release](https://img.shields.io/badge/Release-v3.8-purple.svg)]()

Tanzanite HyperCore is a lightweight background daemon and kernel tuning module tailored for MediaTek Helio G99 Ultra (MT6789 / MT6877) devices running Linux 5.10.

It is designed to eliminate micro-stutters, optimize daily touch response, manage thermal limits intelligently, and deliver high performance during gaming without unnecessary battery drain.

*Inspired By encore @Rem01Gaming*

---

## Overview

Android smartphones often suffer from aggressive CPU downclocking between frame renders, leading to dropped frames during gaming or social media scrolling. HyperCore runs a lightweight native C daemon in the background that dynamically adjusts CPU governor rate limits, GPU frequency floors, interconnect memory bandwidth, storage read-ahead queues, and thermal limits based on real-time workloads.

---

## Key Features

- **Smart Automated Adaptive Engine**: Zero-intervention background daemon that automatically evaluates active workloads, screen lock states, and foreground apps without requiring manual mode toggles.
- **Fast PID Caching**: High-efficiency game detection path with 0.001ms latency and sub-process prefix matching, inspired by `encore`.
- **MediaTek DVFSRC Interconnect Boost**: Unlocks LPDDR RAM throughput up to 4.266 GHz during active gaming sessions (`dvfsrc_qos_mode`) to eliminate memory bandwidth bottlenecks.
- **MediaTek DRM Screen Off Detection**: Accurate display state monitoring via MediaTek DRM backlight drivers (`/sys/class/leds/lcd-backlight/brightness`), ensuring the device enters deep sleep (`PROFILE_SLEEP`) immediately when locked.
- **ARM Mali-G57 Power Policy Management**: Dynamically switches GPU core power policy between `coarse_demand` (deep power gating for daily use) and `always_on` (zero-latency frame rendering for gaming).
- **Calibrated `sugov_ext` Rate Limits**: Configured up-rate limit (`500 µs`) for instant touch response and down-rate limit (`20,000 µs`) to eliminate micro-stutters while scrolling social media.
- **Enterprise Structured Logging**: Millisecond-precision timestamped logger with structured severity levels (`INFO`, `STATE`, `WARN`, `ERROR`) and component tags (`DAEMON`, `PROFILER`, `GAME`, `THERMAL`), using a static ring buffer to eliminate heap memory fragmentation.
- **Hardware IRQ Affinity Pinning**: Directs Mali GPU, GED, DSI Display, and storage interrupts to Cortex-A76 Big Cores (CPU 6–7) for low-latency interrupt processing.
- **Storage Read-Ahead Boost**: Temporarily boosts UFS read-ahead queue size to 512 KB when a game launches to accelerate asset loading, then relaxes to a 256 KB baseline.
- **Streamlined Material Design 3 WebUI**: Integrated KernelSU / APatch / Magisk WebUI featuring a clean status banner, smart engine indicator, game manager, structured log console, and one-click quick actions.

---

## Operational Profiles

The daemon evaluates device metrics every 2 seconds and applies one of five tuned operational profiles:

| Profile | Target Workload | Little Cores (0–5) | Big Cores (6–7) | GPU Floor | Power Policy | Interconnect Bandwidth |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen locked | 500 – 1200 MHz | 725 – 1200 MHz | 300 MHz | `coarse_demand` | Dynamic (Low) |
| **Interactive** | Daily use / Browsing | 600 – 2000 MHz | 800 – 2200 MHz | 300 MHz | `coarse_demand` | Dynamic |
| **Touch** | UI Gestures & Animations | 1000 – 2000 MHz | 1200 – 2200 MHz | 500 MHz | `coarse_demand` | Dynamic |
| **Gaming** | Active foreground game | 1400 – 2000 MHz | 1800 – 2200 MHz | 800 MHz | `always_on` | Unlocked (4.266 GHz) |
| **Thermal** | CPU ≥ 75°C or Battery ≥ 53°C | 600 – 2000 MHz | 900 – 1600 MHz | 300 MHz | `coarse_demand` | Dynamic |

---

## Thermal Protection State Machine

HyperCore uses a 4-tier thermal state machine with hysteresis to maintain performance while keeping temperatures within safe boundaries:

- **Tier 0 (Optimal)**: CPU < 58°C, Battery < 44°C. Standard unthrottled profile limits.
- **Tier 1 (Normal Load)**: CPU 58°C–64°C, Battery 44°C–47°C. Sustained daily operation.
- **Tier 2 (Warm Load)**: CPU 65°C–74°C, Battery 48°C–52°C. Elevated workload adjustment.
- **Tier 3 (Thermal Protection)**: CPU ≥ 75°C or Battery ≥ 53°C. Enforces protective frequency capping.

---

## Codebase Architecture

```text
src/
├── include/
│   ├── common.hpp     # Common data structures and profile definitions
│   ├── sysfs.hpp      # Sysfs file I/O helpers
│   ├── log.hpp        # Structured logging header & level macros
│   ├── cpu.hpp        # Governor rate limits and CPU scaling
│   ├── gpu.hpp        # MediaTek GED, FPSGO, and Mali power policy tuning
│   ├── memory.hpp     # Virtual memory and ZRAM parameters
│   ├── io.hpp         # Storage queue and IRQ affinity tuning
│   ├── thermal.hpp    # Thermal zone scanning and tier calculations
│   └── gamelist.hpp   # Top-app game detector and PID cache manager
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

## Game List Configuration (`gamelist.txt`)

You can manage your game list directly through the WebUI or by editing `/data/adb/modules/tanzanite_hypercore/gamelist.txt`. Package names listed in this file automatically trigger the **Gaming** profile upon launch.

Example `gamelist.txt`:

```text
# Configured Games
com.mobile.legends
com.tencent.ig
com.miHoYo.GenshinImpact
com.dts.freefireth
```

---

## Building from Source

To compile the native C daemon and package the release ZIP module:

```bash
./build.sh
```

The script compiles the daemon using `-O3 -flto -s -Wall -Wextra -Werror` and outputs the final package to:
- `Tanzanite_HyperCore_Release/Tanzanite-HyperCore-v3.8.zip`
- `/sdcard/HyperCore_Releases/Tanzanite-HyperCore-v3.8.zip`

---

## Requirements

- **Target Hardware**: MediaTek Helio G99 Ultra (MT6789 / MT6877)
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License & Credits

This project is licensed under the **GNU General Public License v3.0**. See `LICENSE` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.  
Inspired By encore **[@Rem01Gaming](https://github.com/Rem01Gaming)**.
