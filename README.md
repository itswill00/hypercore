# Tanzanite HyperCore

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/SoC-MediaTek_Helio_G99_Ultra-orange.svg)]()
[![Kernel](https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-ARM64-red.svg)]()

Tanzanite HyperCore is a lightweight background daemon and kernel tuning module for MediaTek Helio G99 Ultra (MT6789 / MT6877) devices running Linux 5.10.

It is designed to eliminate micro-stutters in games, smooth out daily UI navigation, and manage thermal limits intelligently without unnecessary battery drain.

---

## Overview

Modern Android smartphones often suffer from aggressive CPU downclocking between frame renders, leading to dropped frames during gaming or social media scrolling. HyperCore runs a native C daemon in the background to dynamically adjust CPU governor rate limits, GPU frequency floors, storage read-ahead queues, and kernel thermal thresholds based on active workloads.

---

## Key Features

- **Smooth UI Navigation & App Switching**: Extends CPU governor down-rate limits (`sugov_ext` / `schedutil`) to 20ms–25ms, preventing CPU frequencies from dropping during brief touch pauses while scrolling or switching apps.
- **Automated Game Detection**: Scans `/dev/cpuset/top-app/tasks` in real time to identify active games without background CPU overhead. Automatically applies the `Gaming` profile when a recognized game is in the foreground.
- **Custom Per-App Profiles**: Supports custom per-app mode definitions in `gamelist.txt` (for example, setting games to `GAMING` or social media apps to `TOUCH`).
- **Plugged-In Gaming Thermal Guard**: Limits battery charging current to 2.0A (`charge_control_limit`) while gaming on a charger, preventing battery temperatures from exceeding 45°C.
- **Hardware IRQ Pinning**: Binds Mali GPU and MTK Display interrupt requests directly to Cortex-A76 Big Cores (CPU 6–7), ensuring high-priority display rendering interrupts are processed without delay.
- **Storage Read-Ahead Boost**: Temporarily boosts UFS read-ahead queue size to 512 KB when a game launches to speed up DEX loading and asset initialization, then relaxes to a 256 KB baseline.
- **Modern WebUI Dashboard**: Integrated KernelSU / APatch / Magisk WebUI with interactive profile selection, real-time system metrics, gamelist editor, and 1-click online updates.

---

## System Profiles

The daemon evaluates hardware load every 2 seconds and applies one of five operational profiles:

| Profile | Target Workload | Little Cores (0–5) | Big Cores (6–7) | GPU Floor | Down-Rate Limit |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen off | 500 – 1450 MHz | 725 – 1300 MHz | 300 MHz | 1000 µs |
| **Interactive** | Daily usage / browsing | 1200 – 2000 MHz | 1400 – 2200 MHz | 600 MHz | 20000 µs |
| **Touch** | Gestures & UI transitions | 1400 – 2000 MHz | 1700 – 2200 MHz | 700 MHz | 25000 µs |
| **Gaming** | Active foreground game | 1400 – 2000 MHz | 1800 – 2200 MHz | 800 MHz | 30000 µs |
| **Thermal** | CPU ≥ 75°C or Battery ≥ 53°C | 900 – 2000 MHz | 1200 – 1800 MHz | 500 MHz | 5000 µs |

---

## Thermal Thresholds

HyperCore uses a 4-tier thermal state machine with hysteresis to maintain performance while keeping temperatures under control:

- **Tier 0 (Optimal)**: CPU < 58°C, Battery < 44°C. Full unthrottled frequencies.
- **Tier 1 (Normal Load)**: CPU 58°C–64°C, Battery 44°C–47°C. Standard operating range.
- **Tier 2 (Warm Load)**: CPU 65°C–74°C, Battery 48°C–52°C. Sustained heavy usage.
- **Tier 3 (Thermal Protection)**: CPU ≥ 75°C, Battery ≥ 53°C (72°C / 50°C during charging). Holds protective throttle state for 6 seconds before relaxing.

---

## Codebase Architecture

```
src/
├── include/
│   ├── common.hpp     # Common data structures and profile definitions
│   ├── sysfs.hpp      # Sysfs file I/O helpers
│   ├── log.hpp        # Logging and buffer rotation
│   ├── cpu.hpp        # Governor rate limits and CPU scaling
│   ├── gpu.hpp        # MediaTek GED and FPSGO tuning
│   ├── memory.hpp     # Virtual memory and ZRAM parameters
│   ├── io.hpp         # Storage queue and IRQ affinity tuning
│   ├── thermal.hpp    # Thermal zone scanning and tier calculations
│   └── gamelist.hpp   # Top-app game detector and gamelist loader
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

## Game List Setup (`gamelist.txt`)

You can edit `gamelist.txt` via the WebUI or directly at `/data/adb/modules/tanzanite_hypercore/gamelist.txt`. Package names listed in this file automatically trigger the **Gaming** profile upon launch.

You can also specify custom per-app profiles using the `:MODE` suffix:

```text
# Default Gaming trigger
com.tencent.ig
com.mobile.legends

# Custom profile overrides
com.instagram.android:TOUCH
com.ss.android.ugc.trill:TOUCH
```

---

## Building from Source

To compile the native binary and build the release ZIP package:

```bash
./build.sh
```

The script compiles the daemon using `-O3 -flto -s -Wall -Wextra -Werror` and outputs the final package to:
- `Tanzanite_HyperCore_Release/Tanzanite-HyperCore-v3.2.zip`
- `/sdcard/HyperCore_Releases/Tanzanite-HyperCore-v3.2.zip`

---

## Requirements

- **Device**: MediaTek Helio G99 Ultra (MT6789 / MT6877)
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License

This project is licensed under the **GNU General Public License v3.0**. See `LICENSE` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.
