<p align="center">
  <img src="banner.jpg" alt="HyperCore Banner" width="100%">
</p>

<h1 align="center">HyperCore</h1>

<p align="center">
  <strong>Universal Dual-Kernel performance optimizer for MediaTek MT6789 Family</strong><br>
  <i>A lightweight native daemon + Material Design 3 WebUI for Helio G99 / G100 / G200 devices.</i><br>
  <i>Inspired by encore <a href="https://github.com/Rem01Gaming">@Rem01Gaming</a></i>
</p>

<p align="center">
  <a href="https://github.com/itswill00/hypercore/releases"><img src="https://img.shields.io/badge/Release-v6.12.0-purple.svg" alt="Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL_v3-blue.svg" alt="License: GPL v3"></a>
  <img src="https://img.shields.io/badge/SoC-MediaTek_MT6789_Family-orange.svg" alt="Platform">
  <img src="https://img.shields.io/badge/Chipset-G99%20%7C%20G100%20%7C%20G200-red.svg" alt="Chipset">
  <img src="https://img.shields.io/badge/Kernel-Linux_5.10_--_6.12_GKI-green.svg" alt="Kernel">
  <img src="https://img.shields.io/badge/Arch-ARM64-lightgrey.svg" alt="Architecture">
  <img src="https://img.shields.io/badge/Root-KernelSU%20%7C%20APatch%20%7C%20Magisk-brightgreen.svg" alt="Root">
  <img src="https://img.shields.io/badge/Language-C17%20%7C%20Vue%203-blueviolet.svg" alt="Language">
</p>

<p align="center">
  <a href="#-installation">Installation</a> •
  <a href="#-operational-profiles">Profiles</a> •
  <a href="#-thermal-policy">Thermal</a> •
  <a href="#-hypermoon-hud">HUD</a> •
  <a href="#-webui">WebUI</a> •
  <a href="#-security-model">Security</a> •
  <a href="#-troubleshooting">Troubleshooting</a>
</p>

---

## 📑 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Operational Profiles](#-operational-profiles)
- [Thermal Policy](#-thermal-policy)
- [HyperMoon HUD](#-hypermoon-hud)
- [WebUI](#-webui)
- [Security Model](#-security-model)
- [IPC Protocol](#-ipc-protocol)
- [Daemon Lifecycle](#-daemon-lifecycle)
- [Codebase Architecture](#-codebase-architecture)
- [Installation](#-installation)
- [Game List Configuration](#-game-list-configuration)
- [Building from Source](#-building-from-source)
- [Requirements](#-requirements)
- [Troubleshooting](#-troubleshooting)
- [License & Attribution](#-license--attribution)

---

## ✨ Overview

Android devices often suffer frame drops and scrolling stutters from conservative CPU/GPU scaling and governor delays. **HyperCore** runs a native C daemon (`libhypercore.so`) in the background that dynamically tunes — based on real-time workload state:

> CPU governors • uclamp targets • Mali GPU devfreq scaling • MediaTek GED / FPSGO drivers • DVFSRC bandwidth • storage read-ahead

| | |
| :--- | :--- |
| **What it does** | Kills stutter, stabilizes gaming FPS, cools sustained load, protects the battery — automatically, no manual toggling |
| **How it decides** | Foreground-app scan + screen state + system load → one of 4 profiles, re-evaluated every tick |
| **How it coexists** | Never touches the kernel thermal stack — `mi_thermald` and the vendor drivers remain the sole thermal authority |

---

## 🚀 Key Features

<details open>
<summary><b>Core Engine</b></summary>

<br>

| Feature | Description |
| :--- | :--- |
| **Universal Dual-Kernel** | One binary across Linux 5.10 → 6.12+ GKI (Android 12–16), with Cgroup v1 (`/dev/cpuset/`) + v2 (`/sys/fs/cgroup/`) sysfs fallback drivers |
| **Non-blocking loop** | POSIX process management, zero heap allocation in the hot loop, adaptive poll intervals (0.5 s gaming → 8 s sleep) |
| **Read-before-write sysfs** | Every node is read first — no redundant kernel writes, no wakeup spam |
| **Event-driven watchers** | `inotify` on backlight nodes + `netlink` uevents wake the loop early on screen/battery changes |
| **Boot settle (v6.9.4)** | Re-applies Interactive at ticks 5/15/30 to cover late-probing drivers and vendor overwrites, then verifies the result in the log |

</details>

<details>
<summary><b>Performance</b></summary>

<br>

| Feature | Description |
| :--- | :--- |
| **Automated profile switcher** | Sleep / Interactive / Gaming / Gaming MOBA from foreground app + screen + load — fully automatic |
| **MOBA low-latency mode** | Separate frame-pacing tuning (`g_fb_dvfs` / margin) for MLBB-style titles |
| **Mali + GED + FPSGO** | Devfreq governor, polling, GED boost params and FPSGO frame-stabilization per active profile |
| **DVFSRC bandwidth unlock** | Raises interconnect QoS during gaming to remove RAM-bus bottlenecks |
| **Launch boost** | Fresh game launches get temporary read-ahead + notification toast |

</details>

<details>
<summary><b>Memory</b></summary>

<br>

| Feature | Description |
| :--- | :--- |
| **ZRAM pool sizing** | Stock / 4 GB / 2 GB / Off compressed swap presets, applied once per boot with a low-memory guard |

</details>

<details>
<summary><b>Experience</b></summary>

<br>

| Feature | Description |
| :--- | :--- |
| **HyperMoon HUD** | Dual-engine overlay (C daemon + Java DEX): FPS, frametime, CPU/GPU, battery watts, network — pill or card, auto-shows in games |
| **Material Design 3 WebUI** | Status, games, HUD designer, logs, RAM + ZRAM tools — single-file app, no server needed |
| **Zero footprint exit** | Uninstall / daemon stop restores ~60 factory nodes from the captured baseline |

</details>

---

## 🎯 Operational Profiles

The daemon re-evaluates state every tick and applies exactly one profile:

| Profile | Target workload | Little 0–5 | Big 6–7 | GPU gov | GPU range | Touch |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sleep** | Screen off | 500 – 1000 MHz | 725 – 850 MHz | `simple_ondemand` | locked 390 MHz | off |
| **Interactive** | Daily use | stock¹ | stock¹ | stock¹ | stock¹ | smoothing + noise filter |
| **Gaming** | 3D / AAA | 500 – 2000 MHz | 725 – 2200 MHz | `performance` (20 ms) | 390 – 1003 MHz | game + edge |
| **Gaming MOBA** | Low-latency MOBA | 500 – 2000 MHz | 725 – 2200 MHz | `performance` (20 ms) | 390 – 1003 MHz | game, no edge |

> ¹ **Interactive = factory stock + touch.** Every CPU/GPU/GED/FPSGO/VM/cgroup node is restored to the captured `stock_state.conf` — the *only* enhancement is touchscreen smoothing + noise filtering for silky 90/120 Hz scrolling.

```mermaid
flowchart TD
    A[Screen off?] -->|yes| S[Sleep]
    A -->|no| B[Game in foreground?]
    B -->|MOBA title| M[Gaming MOBA]
    B -->|3D / AAA| G[Gaming]
    B -->|no| C{Hold / manual lock?}
    C -->|yes| H[Keep held profile]
    C -->|no| I[Interactive]
```

---

## 🌡️ Thermal Policy

HyperCore does **not** manage thermal policy or charging. Frequency ceilings,
throttling and battery safety are handled exclusively by the vendor thermal
stack (`mi_thermald`, FPSGO thermal, GPU devfreq cooler) and the stock OEM
hardware drivers. The daemon never writes to thermal policy nodes —
`fpsgo_thrm_enable` and Xiaomi `sconfig` always keep their stock values in every profile.
The only exception outside thermal/charging control is a boot-time battery-data
sanity sync: when the vendor auth-chip counter is clearly impossible, the daemon
copies the BMS gauge value into `battery/cycle_count` (see `src/battery.c`).

### ZRAM Sizing

HyperOS hands the device a 6 GB compressed swap pool. Capacity past what you
actually swap is not free — it raises the ceiling for compression work and keeps
`kswapd` busy. **WebUI → Home → Memory** resizes it:

| Preset | Pool | Best for |
| :--- | :--- | :--- |
| **Stock** | ROM default (6 GB) | Untouched behaviour |
| **4 GB** | 4 GB | Daily use, cooler idle |
| **2 GB** | 2 GB | Gaming, least swap churn |
| **Off** | none | No swap at all; swappiness drops to 20 |

> [!IMPORTANT]
> The choice is applied **once per boot**, not instantly. `swapoff` has to pull
> every compressed page back into RAM in one go, which is how a mid-game freeze
> happens — so the card saves your pick, then offers a Reboot button. The resize
> is also skipped when `MemAvailable` can't absorb the swap in use, and a failed
> write re-enables the device's original pool rather than leaving you swapless.
>
> Shrinking does not hand you free RAM instantly. What it buys is a smaller
> worst-case compression ceiling and calmer swap latency under load.

---

## 🌕 HyperMoon HUD

Native dual-engine overlay — near-zero overhead, no extra module needed:

```text
hypermoon_d (C daemon, 11 chars for pkill -x)  →  collects FPS / CPU / GPU / batt / net
HyperMoonOverlay (Java DEX via app_process)    →  draws floating pill / card
```

- **Metrics:** FPS, frametime, CPU load + freq + governor, GPU load + freq + governor, RAM, ZRAM, battery W/temp, network throughput
- **Layouts:** horizontal pill ↔ vertical card • Minimal / Compact / Detailed one-tap presets • Cyber Neon, AMOLED Dark, Matrix Green, Crimson Red + custom hex
- **Auto-gaming:** appears in games, hides in daily use • sub-ms config sync via kernel `inotify`
- **Positioning:** drag with a finger on Android ≤14, or WebUI D-pad + X/Y sliders on any version — required on Android 14 QPR3+/15 where the overlay runs on a hardware surface with no input channel
- **Geometry:** width, height, scale, font, radius, opacity, refresh interval — all live sliders

---

## 🖥️ WebUI

Single-file Vue 3 + Pinia app (`webroot/index.html`) served by your root manager — AMOLED MD3 interface, no backend server:

| Tab | What you get |
| :--- | :--- |
| **Home** | Live daemon status, profile, temps, HUD toggle, ZRAM pool sizing, RAM purge |
| **Games** | Auto-detect scanner, per-title Gaming/MOBA mapping, 2-tap delete guard |
| **HUD** | Full HyperMoon designer with live preview |
| **Logs** | Daemon log viewer + one-tap bugreport export (`hypercore-bugreport`) |
| **About** | Version, baseline info, links |

> [!TIP]
> Telemetry is written atomically (`tmp` + `rename`) so the dashboard never flickers mid-update.

---

## 🔒 Security Model

> [!WARNING]
> A kernel module runs as root. HyperCore treats every install like hostile input:

1. **Two-layer integrity** — `customize.sh` runs `sha256sum -c checksums.txt` over the *executable* payload **before sourcing or executing anything**, then the daemon re-verifies from an embedded checksum table (`--verify-integrity`). `libhypercore.so` can't embed its own hash, so it's covered by the shipped manifest. Runtime-mutated files (`module.prop`, `gamelist.txt`) and `customize.sh` are excluded so the manifest stays valid on a running install.
2. **IPC authorization** — `/dev/hypercore.sock` checks every peer with `SO_PEERCRED`: only **uid 0** (root bridges, bugreport) and **uid 2000** (adb shell). Strangers are dropped *before* the daemon reads a byte.
3. **Single instance** — advisory `flock` on `<data_dir>/.hypercore_lock`; the kernel releases it even on `SIGKILL`.
4. **Hygiene** — logs/telemetry live in `/data/adb/hypercore/` (never world-readable `/sdcard`), shell calls are quoted against injection, socket reads loop to newline under a 25 ms timeout.

---

## 🔌 IPC Protocol

Talk to the live daemon straight from adb shell:

```bash
echo GET_STATUS | nc -U /dev/hypercore.sock
echo SET_PROFILE:GAMING | nc -U /dev/hypercore.sock
```

| Command | Effect |
| :--- | :--- |
| `GET_STATUS` / `STATUS` | JSON telemetry (profile, gpu load, temps, uptime, DDR clock) |
| `SET_PROFILE:AUTO` | Back to autonomous profiling |
| `SET_PROFILE:SLEEP` / `INTERACTIVE` / `GAMING` / `MOBA` | Manual lock (persists until `AUTO`) |
| `PURGE_RAM` / `CLEAR_CACHE` | `drop_caches` + memory compaction |
| `PING` | → `PONG` (liveness) |

Live mirror files: `/dev/hypercore.sock` • `/dev/hypercore_status.json` • `/data/adb/hypercore/status.json`

---

## 🔄 Daemon Lifecycle

```mermaid
flowchart TD
    P[post-fs-data.sh<br/>mkdir /data/adb/hypercore] --> C[customize.sh<br/>verify then snapshot baseline then perms]
    C --> S[service.sh<br/>wait boot_completed then graceful restart]
    S --> D[libhypercore.so<br/>flock then integrity then INIT to Interactive]
    D --> T5[tick 5/15/30<br/>boot settle re-apply + verify]
    T5 --> L{profile change?}
    L -->|yes| A[apply_profile + status sync]
    L -->|no| K[keep + telemetry]
    A --> L
    K --> L
```

- **Install** (`customize.sh`): strict MT6789 check (GED + Mali + FPSGO) → kill old daemon → preserve `*.conf` + merge gamelist → `unzip` → **verify first** → capture `stock_state.conf` + `stock_zram.conf` → permissions + PATH symlinks → game auto-detect.
- **Boot** (`service.sh`): waits `sys.boot_completed` → apply saved ZRAM pool → `SIGTERM` + 10 s grace (lets ~60 nodes restore) → `SIGKILL` fallback → start daemon (+ `nohup` retry) → optional HUD autostart.
- **Uninstall** (`uninstall.sh`): `SIGTERM` + 5 s grace → restore factory nodes and the stock ZRAM pool from baseline → remove symlinks, socket, data.

---

## 🏗️ Codebase Architecture

```text
src/
├── include/
│   ├── common.h  # hw_nodes / core_state / stock_baseline structs
│   ├── baseline.h  # Stock capture / restore API
│   ├── battery.h  # Battery cycle sanity-sync API
│   ├── cpu.h  # Profile matrix + governor/freq API
│   ├── embedded_checksums.h  # GENERATED — SHA-256 table (do not hand-edit)
│   ├── gamelist.h  # Foreground game detector + PID cache
│   ├── gpu.h  # GED / FPSGO / Mali devfreq API
│   ├── integrity.h  # Embedded integrity-check API
│   ├── io.h  # Read-ahead, nr_requests, IRQ affinity
│   ├── ipc.h  # UNIX socket protocol
│   ├── log.h  # Leveled logging + rotation
│   ├── memory.h  # Swappiness, VM, swap-pool awareness, purge
│   ├── module.h  # module.prop status writer
│   ├── nodes.h  # Device path registry + thermal-zone candidates
│   ├── sha256.h  # SHA-256 implementation
│   ├── sysfs.h  # Sysfs/procfs I/O primitives
│   └── util.h  # Async resetprop / command helpers
├── hud/
│   ├── hypermoon_daemon.c       # C telemetry collector (hypermoon_d)
│   └── HyperMoonOverlay.java    # DEX overlay renderer
├── main.c                       # init, boot settle, adaptive main loop
├── nodes.c                      # Path discovery + thermal-zone resolution
├── baseline.c                   # One-table stock capture / save / load / restore
├── battery.c                    # Battery cycle sanity sync against BMS gauge
├── cpu.c                        # build_profile_matrix() + apply_profile()
├── gamelist.c                   # cgroup top-app scan, hourly pm fallback
├── ipc.c                        # SO_PEERCRED socket + atomic status.json
├── sysfs.c                      # Read-before-write primitives + fallback arrays
├── module.c / util.c            # module.prop status + async helpers
├── integrity.c / sha256.c       # Manifest verification primitives
├── io.c / memory.c / gpu.c      # Storage, VM and GPU tuning
└── log.c                        # Persistent FD, 512 KB / 250-line rotation
webui/  →  Vue 3 + Pinia SPA  →  webroot/index.html (single file)
```

---

## 📦 Installation

1. Download **`HyperCore-v6.12.0-b6120-Unified.zip`** from the [Releases page](https://github.com/itswill00/hypercore/releases)
2. Flash it in **KernelSU / APatch / Magisk**
3. **Reboot** — the daemon starts automatically and settles into Interactive
4. Open the manager's **WebUI** to monitor, add games, tune per-app profiles and design your HUD

Verify a running install any time (uid 0 / adb shell):

```bash
cd /data/adb/modules/hypercore && sha256sum -c checksums.txt
/data/adb/modules/hypercore/system/bin/libhypercore.so --verify-integrity /data/adb/modules/hypercore
```

---

## 🎮 Game List Configuration

Manage via **WebUI → Games**, or edit `/data/adb/hypercore/gamelist.txt` (survives updates — entries are merged, never overwritten):

```text
# Configured Games
com.mobile.legends:Gaming MOBA
com.tencent.ig:Gaming
com.miHoYo.GenshinImpact:Gaming
com.dts.freefireth:Gaming
```

- Flash-time auto-detect scans `pm list packages -3` for known titles
- Runtime detection prefers cgroup `top-app` scan (up to 64 PIDs), `dumpsys window` as 10 s rate-limited fallback
- New launches trigger a temporary boost + toast

---

## 🔨 Building from Source

Compiles the C daemon, rebuilds the WebUI, regenerates checksums (two-pass: stub → hash → embed → relink → manifest gate) and packages the release ZIP:

```bash
./build.sh
```

Output:

```text
out/HyperCore-v6.12.0-b6120-Unified.zip
```

| Tool | Needed for |
| :--- | :--- |
| `clang` | `libhypercore.so` + `hypermoon_d` (`-O3 -Werror`) |
| `node` / `npm` | WebUI (`vite build` → single file) |
| `zip` | Release packaging |
| `ecj`/`javac` + `dx`/`d8` | *Optional* — HyperMoon DEX (built when present, cleanly skipped when absent) |

Deploy straight to the live device (root) after building:

```bash
./build.sh --deploy
```

---

## ✅ Requirements

- [x] **Platform** — MediaTek MT6789 Family (Helio G99 / G100 / G200, Mali-G57 + GED + FPSGO)
- [x] **Kernel** — GKI Linux 5.10 → 6.12+ (Android 12 → 16), ARM64
- [x] **Root** — KernelSU, APatch, or Magisk
- [ ] Everything else — Samsung / Snapdragon / Unisoc / Tensor are **not supported** (installer aborts)

---

## 🛠️ Troubleshooting

<details>
<summary><b>Flash aborts with "File tampering or corruption detected"</b></summary>

<br>

- Flash the **release ZIP**, not the source-code ZIP — build artifacts (`libhypercore.so`, `hypermoon_d`, `webroot/index.html`) are git-ignored, so a raw checkout can never pass the manifest.
- Always package with `./build.sh` — hand-zipped archives miss `checksums.txt` entries.
- Re-download if the ZIP was edited or partially copied.

</details>

<details>
<summary><b>After reboot it says Interactive but doesn't feel applied</b></summary>

<br>

Fixed in **v6.9.4**: the daemon now re-applies Interactive at boot-settle ticks and verifies the result. Check the proof in the log:

```bash
grep -i "boot settle" /data/adb/hypercore/hypercore.log
# Boot settle re-apply Interactive (1/3) … (2/3) … verified: Interactive fully applied
```

</details>

<details>
<summary><b>Frequencies stuck below stock while "Interactive"</b></summary>

<br>

- HyperCore never caps frequencies — throttling is entirely `mi_thermald`/vendor territory, and vendor clamps are expected under load.
- If the device is cool and still capped, another module may be fighting over `scaling_max_freq`.

</details>

<details>
<summary><b>WebUI shows stale / flickering data</b></summary>

<br>

- The WebUI reads `status.json` first, then the IPC socket, then sysfs directly. Restart the daemon via WebUI (**About → Restart**) or re-flash.
- `echo PING | nc -U /dev/hypercore.sock` should answer `PONG` — silence means the daemon isn't running (`pidof libhypercore.so` to confirm).

</details>

---

## 📜 License & Attribution

This project is licensed under the **GNU General Public License v3.0** — see [`LICENSE`](LICENSE) and [`NOTICE.md`](NOTICE.md) for details.

Developed by **[@itswill00](https://github.com/itswill00)** · Inspired by encore **[@Rem01Gaming](https://github.com/Rem01Gaming)**

<p align="center">
  <a href="#-hypercore">Back to top</a>
</p>
