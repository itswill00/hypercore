# HyperCore

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/SoC-MediaTek_MT6789_Family-orange.svg)]()
[![Kernel](https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-ARM64-red.svg)]()
[![Release](https://img.shields.io/badge/Release-v4.4-purple.svg)]()

HyperCore is a lightweight, zero-overhead background daemon and kernel tuning module tailored specifically for MediaTek MT6789 Family devices running Linux kernel 5.10.x.

It is designed to eliminate micro-stutters, optimize daily touch response, manage thermal limits intelligently, and deliver high performance during gaming without unnecessary battery drain.

*Inspired By encore @Rem01Gaming*

---

## Overview

Android smartphones often suffer from aggressive CPU downclocking between frame renders, leading to dropped frames during gaming or UI micro-stutters while scrolling. HyperCore runs a lightweight native C daemon in the background that dynamically adjusts CPU governor rate limits, GPU frequency floors, interconnect memory bandwidth, storage read-ahead queues, and thermal limits based on real-time workloads.

---

## Key Features

- **Universal Read-Before-Write Guard**: Eliminates sysfs I/O thrashing with 0.0001ms fast-path checks on all CPU, GPU, Memory, IO, Cgroup, and Thermal sysfs writes.
- **Event-Driven Screen & Battery Watcher (`inotify` & `netlink`)**: Integrates Linux `inotify` on screen backlight and Linux `NETLINK_KOBJECT_UEVENT` kernel socket for instant 0ms latency transitions with 0% CPU wakeups during sleep.
- **Non-Blocking Asynchronous Command Engine (`async_system_cmd`)**: Executes system shell calls via POSIX double-forking, ensuring the main daemon loop thread never stalls (0ms interruption).
- **Smart Automated Adaptive Engine**: Zero-intervention background daemon that automatically evaluates active workloads, screen lock states, and foreground apps without requiring manual mode toggles.
- **Fast PID Caching**: High-efficiency game detection path with 0.001ms latency and sub-process prefix matching (`com.package:child`), inspired by `encore`.
- **Anti-Root & Anti-Cheat Stealth Binary (`libhypercore.so`)**: Disguised native daemon executable to prevent detection by banking apps and game anti-cheats (Tencent ACE / MTP).
- **UNIX Domain Socket IPC (0ms Latency)**: Non-blocking IPC socket server (`hypercore.sock`) for instant JSON telemetry updates (`status`, `cpu_temp`, `bat_temp`, `gpu_temp`, `chg_temp`, `battery_cycles`, `bat_health`) to the WebUI.
- **Mali GPU Devfreq Governor & 674 MHz Floor**: Automatically switches GPU Devfreq governor (`/sys/class/devfreq/13000000.mali/governor` or `/sys/class/devfreq/soc:mali/governor`) to `performance` and locks a 674 MHz minimum frequency floor during active gaming sessions, eliminating GPU frequency drops under heavy loads.
- **MediaTek DVFSRC Interconnect Boost**: Unlocks LPDDR4X RAM throughput up to 4.266 GHz during active gaming sessions (`dvfsrc_qos_mode = 1`) to eliminate memory bandwidth bottlenecks.
- **Dynamic Thermal Charging Guard**: 3-stage thermal charging current guard (`charge_control` = 2, 4, 8) based on battery temperature (≥42°C restricted to 2A, ≥37°C restricted to 4A, <37°C full 8A fast charging).
- **MediaTek DRM Screen Off Detection**: Accurate display state monitoring via MediaTek DRM backlight drivers, ensuring the device enters deep sleep (`PROFILE_SLEEP`) immediately when locked.
- **MediaTek FPSGO `ultra_rescue` & `boost_ta`**: Instant 0ms touch acceleration and frame-drop rescue bursts during heavy 3D combat.
- **Kernel Auto Self-Recovery Guard**: Detects unauthorized sysfs mutations by third-party kernel tweak apps or rogue scripts (e.g., CPU governor, Mali GPU power policy, or GPU devfreq governor overrides) and automatically re-enforces HyperCore's optimal configuration within 1-2 seconds.
- **Foreground App Transition & Gesture Boost Engine**: Detects active app switches (e.g. TikTok to Instagram or MIUI Launcher) and injects a 1.5s burst (1.6 GHz Big core floor, 35% uclamp, 384KB read-ahead) for 120 FPS stutter-free app switching animations.
- **Calibrated `sugov_ext` / `reflex` Governor Scaling**: Auto-detects custom kernel `reflex` governor or sets `sugov_ext` up-rate limit (`200 µs`) for 0ms touch response and down-rate limit (`20,000 µs`) for 60/90/120 Hz smooth scrolling.
- **Hardware IRQ Subdirectory Pinning**: Scans Linux kernel `/proc/irq/<irq>/` subdirectories to pin Mali GPU, DSI Display, and FocalTech Touchscreen interrupts directly to Cortex-A76 Big Cores (CPU 6–7) for low-latency interrupt handling.
- **Embedded Binary SHA-256 Self-Integrity Check**: Verifies binary and module file checksums on startup to detect tampering or corruption (`libhypercore.so --verify-integrity`).
- **Human-Centric Material Design 3 WebUI**: Integrated KernelSU / APatch / Magisk WebUI featuring active profile hero badge, natural English titles, ultra-compact micro-tile status cards, instant 120 FPS accordion transitions, real-time RAM cache flushing, Base64 data URL inlining, and native Telegram intent launcher bridge.

---

## Operational Profiles

The daemon evaluates device metrics using an adaptive loop (500 ms – 8000 ms) and applies one of three consolidated core operational profiles:

| Profile | Target Workload | Little Cores (0–5) | Big Cores (6–7) | GPU Devfreq Gov | GPU Floor | Power Policy | Interconnect Bandwidth |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen locked | 500 – 1200 MHz | 725 – 1000 MHz | `simple_ondemand` | 390 MHz | `coarse_demand` | Dynamic (Low) |
| **Interactive** | Daily use / Browsing | 600 – 2000 MHz | 1000 – 2200 MHz | `simple_ondemand` | 390 MHz | `coarse_demand` | Dynamic |
| **Gaming** | Active foreground game | 1400 – 2000 MHz | 1800 – 2200 MHz | `performance` | 674 MHz | `always_on` | Unlocked (4.266 GHz) |

---

## Non-Interfering Thermal Hardware Coexistence

HyperCore relies on native kernel hardware thermal drivers and Xiaomi `sconfig = 10` Game Turbo thermal mode for throttling. The daemon dynamically adjusts 3-stage thermal charging current limits while allowing hardware HALs and vendor thermal drivers to maintain safe thermal limits.

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

You can manage your game list directly through the WebUI or by editing `/data/adb/modules/hypercore/gamelist.txt`. Installed games are automatically auto-detected during installation.

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
- `HyperCore_Release/HyperCore-v4.4.zip`
- `/sdcard/HyperCore_Releases/HyperCore-v4.4.zip`

---

## Requirements

- **Target Hardware**: MediaTek MT6789 Family
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License & Attribution

This project is licensed under the **GNU General Public License v3.0**. See `LICENSE` and `NOTICE.md` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.  
Inspired By encore **[@Rem01Gaming](https://github.com/Rem01Gaming)**.
