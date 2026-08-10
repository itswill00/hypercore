<p align="center">
  <img src="banner.png" alt="HyperCore Banner" width="100%">
</p>

<h1 align="center">HyperCore</h1>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL_v3-blue.svg" alt="License: GPL v3"></a>
  <img src="https://img.shields.io/badge/SoC-MediaTek_MT6789_Family-orange.svg" alt="Platform">
  <img src="https://img.shields.io/badge/Chipset-G99%20%7C%20G100%20%7C%20G200-red.svg" alt="Chipset">
  <img src="https://img.shields.io/badge/Kernel-Linux_5.10.x-green.svg" alt="Kernel">
  <img src="https://img.shields.io/badge/Arch-ARM64-lightgrey.svg" alt="Architecture">
  <img src="https://img.shields.io/badge/Root-KernelSU%20%7C%20APatch%20%7C%20Magisk-brightgreen.svg" alt="Root">
  <img src="https://img.shields.io/badge/Language-C99%20%7C%20Vue%203-blueviolet.svg" alt="Language">
  <img src="https://img.shields.io/badge/Release-v4.5-purple.svg" alt="Release">
</p>

<p align="center">
  <i>A lightweight background daemon and kernel tuning module tailored for MediaTek MT6789 Family devices.</i><br>
  <i>Inspired by encore @Rem01Gaming</i>
</p>

---


## Overview

Android devices often experience frame drops or scrolling stutters due to conservative CPU/GPU scaling and governor delays. HyperCore runs a native C daemon in the background to dynamically tune CPU governor parameters, GPU frequency limits, memory interconnect bandwidth, storage read-ahead queues, and thermal limits based on workload state.

---

## Key Features

<details>
<summary><b>✨ Click to expand / collapse Key Features list</b></summary>

<br>

- **Read-Before-Write Sysfs Cache**: Prevents unnecessary sysfs file writes by verifying existing values before writing.
- **Event-Driven Screen & Battery Watcher (`inotify` & `netlink`)**: Monitors display state via `inotify` on backlight nodes and battery events via kernel `netlink` sockets for immediate state transitions.
- **Non-Blocking Execution Engine**: Uses POSIX process management (`async_system_cmd`) to keep the main daemon loop non-blocking.
- **Automated Profile Switcher**: Evaluates foreground applications, screen state, and system load automatically without manual intervention.
- **PID Caching Game Detector**: Fast package lookup and PID caching with sub-process matching (`com.package:child`).
- **UNIX Domain Socket Telemetry (IPC)**: Provides a non-blocking domain socket server (`hypercore.sock`) serving JSON telemetry (`status`, `cpu_temp`, `bat_temp`, `gpu_temp`, `chg_temp`, `battery_cycles`) to the WebUI.
- **Mali GPU Devfreq Management**: Sets GPU devfreq governor (`simple_ondemand` / `performance`) and frequency floors according to the active profile.
- **MediaTek DVFSRC Bandwidth Optimization**: Adjusts interconnect memory bandwidth limits (`dvfsrc_qos_mode`) during gaming workloads.
- **Thermal Charging Protection**: Adjusts battery charging current limits based on real-time temperature thresholds to prevent overheating while charging.
- **Display State Sleep Integration**: Detects display sleep state to lower scaling limits when the device is locked.
- **MediaTek FPSGO & GED Integration**: Configures FPSGO and GED boost parameters for frame stabilization during heavy graphics rendering.
- **Configuration Re-enforcement**: Periodically verifies and restores intended governor settings if external scripts modify them.
- **Foreground App Burst**: Applies short frequency and read-ahead boosts during application launch and switching.
- **Governor Rate Tuning**: Configures `sugov_ext` / `schedutil` up and down rate limits for responsive frequency scaling.
- **Interrupt Pinning**: Directs GPU, display, and touch interrupts to Cortex-A76 Big Cores (CPU 6–7) for lower latency.
- **SHA-256 Integrity Verification**: Self-verifies daemon binary and module files on startup (`libhypercore.so --verify-integrity`).
- **Material Design 3 WebUI**: Integrated WebUI for KernelSU / APatch / Magisk providing status monitoring, profile selection, log viewing, and RAM cleanup tools.

</details>

---

## Operational Profiles

The daemon evaluates system state periodically (500 ms – 8000 ms) and applies one of three operational profiles:

| Profile | Target Workload | Little Cores (0–5) | Big Cores (6–7) | GPU Devfreq Gov | GPU Floor | Power Policy | Interconnect Bandwidth |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen locked | 500 – 1200 MHz | 725 – 1000 MHz | `simple_ondemand` | 390 MHz | `coarse_demand` | Dynamic (Low) |
| **Interactive** | Daily use / Browsing | 600 – 2000 MHz | 1000 – 2200 MHz | `simple_ondemand` | 390 MHz | `coarse_demand` | Dynamic |
| **Gaming** | Active foreground game | 1400 – 2000 MHz | 1800 – 2200 MHz | `performance` | 674 MHz | `always_on` | Unlocked (4.266 GHz) |

---

## Thermal Management & Hardware Coexistence

HyperCore works alongside native kernel hardware thermal drivers. The daemon manages battery thermal charging protection while allowing hardware HALs and vendor thermal policies to enforce thermal safety limits.

---

## Codebase Architecture

```text
src/
├── include/
│   ├── common.hpp             # Data structures and profile definitions
│   ├── cpu.hpp                # CPU scaling and governor rate limits
│   ├── embedded_checksums.hpp # Generated SHA-256 checksum definitions
│   ├── gamelist.hpp           # Game package detector and PID cache
│   ├── gpu.hpp                # MediaTek GED, FPSGO, and Mali GPU settings
│   ├── integrity.hpp          # Binary integrity check header
│   ├── io.hpp                 # Storage queue and IRQ affinity tuning
│   ├── ipc.hpp                # UNIX domain socket IPC header
│   ├── log.hpp                # Logging macros and functions
│   ├── memory.hpp             # Memory and ZRAM tuning
│   ├── sha256.hpp             # SHA-256 calculation header
│   ├── sysfs.hpp              # Non-blocking sysfs file I/O helpers
│   └── thermal.hpp            # Thermal zone scanning and tier detection
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

You can manage configured games via the WebUI or by editing `/data/adb/modules/hypercore/gamelist.txt`.

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

To compile the native C daemon, generate embedded checksums, build the WebUI frontend, and create the release package:

```bash
./build.sh
```

Output files:
- `HyperCore_Release/HyperCore-v4.5.zip`
- `/sdcard/HyperCore_Releases/HyperCore-v4.5.zip`

---

## Requirements

- **Platform**: MediaTek MT6789 Family
- **Kernel**: Linux 5.10.x
- **Environment**: Rooted via KernelSU, APatch, or Magisk

---

## License & Attribution

This project is licensed under the **GNU General Public License v3.0**. See `LICENSE` and `NOTICE.md` for details.

Developed by **[@itswill00](https://github.com/itswill00)**.  
Inspired by encore **[@Rem01Gaming](https://github.com/Rem01Gaming)**.
