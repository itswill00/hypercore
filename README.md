# Tanzanite HyperCore

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/SoC-MediaTek_Helio_G99_Ultra-orange.svg)]()
[![Kernel](https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-ARM64-red.svg)]()
[![Release](https://img.shields.io/badge/Release-v4.3-purple.svg)]()

Tanzanite HyperCore is a lightweight, zero-overhead background daemon and kernel tuning module tailored specifically for MediaTek Helio G99 Ultra (MT6789 / MT6877) devices running Linux kernel 5.10.

It is designed to eliminate micro-stutters, optimize daily touch response, manage thermal limits intelligently, and deliver high performance during gaming without unnecessary battery drain.

*Inspired By encore @Rem01Gaming*

---

## Overview

Android smartphones often suffer from aggressive CPU downclocking between frame renders, leading to dropped frames during gaming or UI micro-stutters while scrolling. HyperCore runs a lightweight native C daemon in the background that dynamically adjusts CPU governor rate limits, GPU frequency floors, interconnect memory bandwidth, storage read-ahead queues, and thermal limits based on real-time workloads.

---

## Key Features

- **Smart Automated Adaptive Engine**: Zero-intervention background daemon that automatically evaluates active workloads, screen lock states, and foreground apps without requiring manual mode toggles.
- **Fast PID Caching**: High-efficiency game detection path with 0.001ms latency and sub-process prefix matching (`com.package:child`), inspired by `encore`.
- **Anti-Root & Anti-Cheat Stealth Binary (`libhypercore.so`)**: Disguised native daemon executable to prevent detection by banking apps and game anti-cheats (Tencent ACE / MTP).
- **UNIX Domain Socket IPC (0ms Latency)**: Non-blocking IPC socket server (`hypercore.sock`) for instant JSON telemetry updates to the WebUI.
- **MediaTek DVFSRC Interconnect Boost**: Unlocks LPDDR RAM throughput up to 4.266 GHz during active gaming sessions (`dvfsrc_qos_mode = 1`) to eliminate memory bandwidth bottlenecks.
- **Dynamic Thermal Charging Guard**: 3-stage charging current scaling during gaming based on battery temperature (<37°C 2.5A fast charge, 37°C–41°C 1.8A safety balance, ≥42°C 1.0A trickle guard), restoring full fast charging upon game exit.
- **MediaTek DRM Screen Off Detection**: Accurate display state monitoring via MediaTek DRM backlight drivers, ensuring the device enters deep sleep (`PROFILE_SLEEP`) immediately when locked.
- **Load-Aware GPU Floor & DVFS Margin**: Dynamically scales GPU bottom frequency (300 MHz vs 600 MHz) and DVFS margin based on real-time GPU load (`gpu_loading`), saving energy during loading screens while boosting full throttle during heavy 3D fights.
- **MediaTek FPSGO `ultra_rescue` & `boost_ta`**: Instant 0ms touch acceleration and frame-drop rescue bursts during heavy 3D combat.
- **Calibrated `sugov_ext` / `reflex` Governor Scaling**: Auto-detects custom kernel `reflex` governor or sets `sugov_ext` up-rate limit (`500 µs`) for 0ms touch response and down-rate limit (`20,000 µs`) for 90/120 Hz smooth scrolling.
- **Enterprise Structured Logging**: Millisecond-precision timestamped logger with structured severity levels (`INFO`, `STATE`, `WARN`, `ERROR`), using a static ring buffer to eliminate heap memory fragmentation.
- **Hardware IRQ Affinity Pinning**: Directs Mali GPU, GED, DSI Display, and storage interrupts to Cortex-A76 Big Cores (CPU 6–7) for low-latency interrupt processing.
- **Embedded Binary SHA-256 Self-Integrity Check**: Verifies binary and module file checksums on startup to detect tampering or corruption (`libhypercore.so --verify-integrity`).
- **Streamlined Material Design 3 WebUI**: Integrated KernelSU / APatch / Magisk WebUI featuring status banner, smart engine indicator, game manager, live log console, and quick actions.

---

## Operational Profiles

The daemon evaluates device metrics using an adaptive loop (500 ms – 8000 ms) and applies one of three consolidated core operational profiles:

| Profile | Target Workload | Little Cores (0–5) | Big Cores (6–7) | GPU Floor | Power Policy | Interconnect Bandwidth |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen locked | 500 – 1200 MHz | 725 – 1000 MHz | 300 MHz | `coarse_demand` | Dynamic (Low) |
| **Interactive** | Daily use / Browsing | 600 – 2000 MHz | 1000 – 2200 MHz | 300 MHz | `coarse_demand` | Dynamic |
| **Gaming** | Active foreground game | 1400 – 2000 MHz | 1800 – 2200 MHz | 300 / 600 MHz | `coarse_demand` | Unlocked (4.266 GHz) |

---

## Non-Interfering Thermal Hardware Coexistence

HyperCore relies on native kernel hardware thermal drivers and user-installed thermal mods for throttling. The daemon dynamically adjusts 3-stage thermal charging current limits (2.5A / 1.8A / 1.0A) while allowing hardware HALs and thermal mods to maintain optimal thermal limits.

---

## Codebase Architecture

```text
src/
├── include/
│   ├── common.hpp             # Common data structures and profile definitions
│   ├── cpu.hpp                # Governor rate limits and CPU scaling
│   ├── embedded_checksums.hpp # Embedded SHA-256 checksum definitions (generated)
│   ├── gamelist.hpp           # Top-app game detector and PID cache manager
│   ├── gpu.hpp                # MediaTek GED, FPSGO, and Mali power policy tuning
│   ├── integrity.hpp          # Binary self-integrity verification header
│   ├── io.hpp                 # Storage queue and IRQ affinity tuning
│   ├── ipc.hpp                # Non-blocking UNIX domain socket IPC header
│   ├── log.hpp                # Structured logging header & level macros
│   ├── memory.hpp             # Virtual memory and ZRAM parameters
│   ├── sha256.hpp             # Lightweight SHA-256 calculation header
│   ├── sysfs.hpp              # Non-blocking sysfs file I/O helpers
│   └── thermal.hpp            # Thermal zone scanning and tier calculations
├── cpu.c
├── gamelist.c
├── gpu.c
├── integrity.c
├── io.c
├── ipc.c
├── log.c
├── main.c
├── memory.c
├── sha256.c
├── sysfs.c
└── thermal.c
```

---

## Game List Configuration (`gamelist.txt`)

You can manage your game list directly through the WebUI or by editing `/data/adb/modules/tanzanite_hypercore/gamelist.txt`. Installed games are automatically auto-detected during installation.

Example `gamelist.txt`:

```text
# Configured Games
com.mobile.legends:GAMING
com.tencent.ig:GAMING
com.miHoYo.GenshinImpact:GAMING
com.dts.freefireth:GAMING
```

---

## Building from Source

To compile the native C daemon, generate embedded checksums, build the WebUI frontend, and package the release ZIP module:

```bash
./build.sh
```

The script compiles the daemon using `clang -O3 -flto -s -fvisibility=hidden -Wl,--gc-sections -Wall -Wextra -Werror` and outputs the final package to:
- `Tanzanite_HyperCore_Release/Tanzanite-HyperCore-v4.3.zip`
- `/sdcard/HyperCore_Releases/Tanzanite-HyperCore-v4.3.zip`

---

## Requirements

- **Target Hardware**: MediaTek Helio G99 Ultra (MT6789 / MT6877)
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License & Attribution

This project is licensed under the **GNU General Public License v3.0**. See `LICENSE` and `NOTICE.md` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.  
Inspired By encore **[@Rem01Gaming](https://github.com/Rem01Gaming)**.
