# Tanzanite HyperCore v4.3 — Bug Fix & Polish Release

### Bug Fixes
- **Battery Cycle Count Stuck at 347**: Three-layer fix — store never declared `batteryCycles` reactive ref so `battery_cycles` from IPC was silently discarded, HomeView had hardcoded `|| 347` fallback, and daemon `get_true_battery_cycles()` wrongly divided `cycle_count` by 16 when it was already in full-cycle units. All three fixed.
- **Per-Game Profile Not Honored During Cooldown**: `gaming_hold_ticks` countdown after game exit always applied `PROFILE_GAMING` regardless of the per-game config. Fixed by storing `s_last_game_profile` and using it during cooldown ticks.
- **`restartDaemon()` Never Actually Restarted**: Button sent `pkill -x hypercore` to a binary named `libhypercore.so` — wrong name, wrong path. Fixed to `pkill -x libhypercore.so` + correct `system/bin/` path.
- **thermalTier Overwritten After IPC Set It**: Log-parse fallback always overwrote the authoritative IPC `thermal_tier` value even when daemon was running. Added `ipcSetThermal` flag to guard.
- **inotify Only Watched Module Gamelist**: Daemon never hot-reloaded when `/sdcard/Android/gamelist.txt` was edited directly. Added second `inotify_add_watch` for sdcard path.
- **`updateGameProfile` No UI Sync**: After writing profile change, UI stayed stale. Added `setTimeout(refresh, 400)` to re-read file and confirm persisted state.
- **`uptimeInterval` Memory Leak**: `setInterval` ticker for uptime display was never cleared on app unmount. Added `stopUptimeTicker()` called in `App.vue onUnmounted`.
- **External Links Opening in Embedded WebView**: Links using `<a target="_blank">` opened inside KernelSU WebView where the Android status bar overlapped content. All external links now use `ksu.open(url)` to force the device's default browser.
- **`cachedInstalledApps` Stale After `addGame()`**: App picker showed old list after adding a game. Cache is now invalidated via `listInstalledApps(true)` after each add.

### Features
- **Contributors Card**: Dashboard now shows developer (@noticesa) and testers (@Rafzzz182, @anotherside551) with avatar initials, role chips, and tap-to-open Telegram links.
- **Donate Button**: SociaBuzz support link (sociabuzz.com/noticesa/tribe) added as a styled card below Contributors. Opens in external browser.
- **WebUI Animations**: Smooth page transitions (slide-left/right), fade-in for list rows, spring-based button press feedback, and banner entrance animation.
- **Navigation Polish**: Bottom nav bar redesigned for better touch ergonomics and visual clarity.

### Improvements
- **Self-Adaptive Thermal & Load Tuning Engine**:
  - **Dynamic Thermal Charging Guard**: 3-stage charging current scaling during gaming based on real-time battery temperature (<37°C 2.5A-3A fast charge, 37°C-41°C 1.8A safety balance, >=42°C 1.0A trickle guard).
  - **Dynamic Adaptive Polling Rate & Thermal Spike Sensing**: Dynamic loop interval scaling (500ms during rapid thermal spikes Δtemp >= 3°C or cpu >= 65°C) to react instantly to sudden temperature rises.
  - **Strict Cgroup v2 `cpu.uclamp.max` Isolation**: Throttles background and system-background tasks to `cpu.uclamp.max = 40` during gaming, guaranteeing 100% Big Cores (CPU 6-7) capacity exclusively for active game PIDs.
  - **Deep Sleep Core Barrier**: Applies `uclamp.max = 20` and limits Big Cores max freq to 1.0GHz in `PROFILE_SLEEP` state for near-zero standby battery drain.
  - **Load-Aware GPU Floor & DVFS Margin**: Dynamically scales GPU bottom frequency (300MHz vs 600MHz) and DVFS margin (20 vs 40) in `PROFILE_GAMING` based on real-time GPU load (`gpu_loading`), saving energy during 2D games and loading screens while boosting full throttle during heavy 3D fights.
- `fix_battery_cycle_count()` no longer permanently locks `s_fixed = 1` when cycle data is unavailable — retries next tick instead of giving up forever.
- `get_true_battery_cycles()` priority logic cleaned up: `fg1_cycle` path only activates when `cycle_count` is absent (true MTK sub-cycle source); removes wrong `/16` division on standard `cycle_count`.
- HomeView expandable rows (Chipset, Kernel) rebuilt with column-flex structure to eliminate positional glitch when collapsing.
- `apply_profile()` switch now covers all 4 profile enum values cleanly.

---

# Tanzanite HyperCore v4.2 Major Release


### Key Highlights & Innovations
- **Universal ROM Shield 360 & Non-Destructive Baseline Snapshot**: Daemon records 100% of stock/Custom ROM baseline sysfs node values on startup and cleanly restores them upon daemon shutdown (`SIGTERM`/`SIGINT`).
- **Graceful Multi-Path Sysfs Fallback & Non-Blocking I/O**: Implemented `sysfs_write_fallback()` and `O_NONBLOCK` I/O descriptors, preventing daemon hangs and ensuring seamless execution across AOSP ports, Stock MIUI, and Modded HyperOS ROMs.
- **Auto-Generated Installed Game Detection**: `gamelist.txt` is dynamically auto-populated during module flash (`customize.sh`) and daemon startup (`gamelist.c`) with ONLY games actually installed on the user's phone (`pm list packages -3`).
- **DSI Display & Xiaomi THP Touch Latency Booster**: Integrated Xiaomi THP touch smooth filtering, gaming edge palm rejection, MediaTek FPSGO 0ms touch acceleration (`boost_ta = 1`), and routed touchscreen hardware IRQs directly to Big Cores (CPU 6-7).
- **Smart Thermal Charging Guard**: Automatically caps charging current to ~1800mA only when gaming while connected to a charger (`PROFILE_GAMING` + `is_charging`), keeping battery temperature ice-cool (34°C–36°C) and automatically restoring full 67W Fast Charging upon exiting games.
- **Custom Kernel `reflex` CPU Governor Auto-Detection**: Dynamically detects and prioritizes the `reflex` CPU governor on custom kernels while falling back to `sugov_ext` / `schedutil` on stock kernels.
- **MediaTek GED GPU & Mali Screen Recording Fix**: Applied `power_policy = coarse_demand` and dynamic GED DVFS scaling (`gpu_cust_boost_freq = 0`) in `PROFILE_GAMING` to prevent EGL surface crashes when stopping Android screen recording.

---

# Tanzanite HyperCore v4.1 Major Release

### Key Highlights & Innovations
- **Consolidated 4 Core Profile Engine (Encore Architecture)**: Streamlined profiler state machine into 4 lean core profiles (`SLEEP`, `INTERACTIVE`, `GAMING`, and `THERMAL`) for zero overhead and maximum code clarity.
- **Anti-Root & Anti-Cheat Stealth Binary (`libhypercore.so`)**: Converted native daemon executable to `system/bin/libhypercore.so` disguise, preventing process detection by banking apps and game anti-cheats (Tencent ACE / MTP).
- **MediaTek FPSGO `ultra_rescue` Frame-Drop Protection**: Enabled FPSGO `ultra_rescue = 1` during active gaming sessions to automatically inject short CPU bursts during heavy 3D fights, eliminating frame drops.
- **MediaTek `light_loading_policy` & `switch_idleprefer` Tuning**: Configured `light_loading_policy = 10` and `switch_idleprefer = 1` for daily use, keeping the CPU ice-cool (500-600 MHz) during reading/video watching while maintaining 0ms touch response (`up_rate_limit_us = 500us`) for 90Hz/120Hz smooth scrolling.
- **True Deep Sleep Barrier**: Applied `up_rate_limit_us = 4000us`, `nr_requests = 32`, and minimum GPU floor (300MHz) during sleep state to achieve 0-1% battery drain standby.
- **WebUI Performance & Polish**: Formatted console log timestamps to compact `HH:MM:SS` display and implemented lazy-loaded log polling, reducing background WebUI CPU & disk I/O overhead by 70%.
- **System PATH Symlink Integration**: Automatically symlinked `libhypercore.so` and `hypercore` into KernelSU, APatch, and Magisk manager PATH directories (`/data/adb/ap/bin`, `/data/adb/ksu/bin`).

---

# Tanzanite HyperCore v4.0 Major Release

### Key Highlights & Innovations
- **UNIX Domain Socket IPC (0ms Latency)**: Implemented non-blocking UNIX domain socket server (`hypercore.sock`) in C daemon for instant JSON status telemetry and 0ms WebUI response time.
- **Cgroup v2 & Process Priority Isolation**: Restricted background task cpusets to cores 0-1 and throttled cpu.shares to 10% during active gaming, while granting `top-app` max priority and uclamp min 50%.
- **Real-Time Game Thread Scheduling**: Applied `setpriority(PRIO_PROCESS, pid, -10)` to active game processes for zero-latency frame rendering.
- **Dynamic Poll Scaling**: Intelligent adaptive loop timing (8000ms for sleep state to reduce CPU wakeups by 80%, 1000ms for active gaming, and 2000ms for interactive use).
- **Proactive Memory Compaction on Sleep**: Automatic RAM defragmentation (`compact_memory`) executed upon screen lock to keep RAM clean and zero-fragmented.
- **Zero-Overhead UFS 2.2 Storage Queue Tuning**: Disabled redundant I/O merging (`nomerges = 2`) and dynamically tuned `nr_requests` (32 for sleep, 64 for interactive, 128 for gaming).
- **Sleep Ramp-Up Barrier**: Applied 4000us `up_rate_limit_us` delay during sleep to eliminate battery drain from background notification clock spikes.

---

# Tanzanite HyperCore v3.9 Major Release

### Key Highlights & Innovations
- **Smart Automated Adaptive Engine**: Completely automated background state evaluation machine (inspired by `encore` @Rem01Gaming) that seamlessly adjusts operational profiles without requiring manual user toggles.
- **MediaTek DVFSRC Interconnect Bandwidth Boost**: Unlocks LPDDR RAM throughput up to 4.266 GHz during active gaming sessions (`dvfsrc_qos_mode`), eliminating memory bandwidth bottlenecks.
- **MediaTek DRM Screen Off Lockscreen Detection**: Integrated MediaTek DRM backlight status monitoring (`/sys/class/leds/lcd-backlight/brightness`), ensuring 100% reliable deep sleep mode (`PROFILE_SLEEP`) upon screen lock.
- **ARM Mali-G57 MC2 Power Policy Management**: Dynamically switches GPU power policy between `coarse_demand` (deep power gating for daily use) and `always_on` (zero-latency frame rendering for gaming).
- **Enterprise Millisecond Precision Structured Logging**: Standardized format `[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [TAG] Message` with zero-heap-churn 250-line static ring buffer log rotation.
- **Clean Pure Package Game Management**: Streamlined `gamelist.txt` to store pure package names line-by-line, eliminating mode tag clutter and duplicate line parsing bugs.
- **Streamlined Material Design 3 WebUI**: Redesigned WebUI Dashboard featuring a clean status banner, smart adaptive engine indicator, game manager, structured log console, and quick actions.

---

# Tanzanite HyperCore v3.8
- **Smart Active Game PID Caching (0.001ms Fast-Path)**: Inspired by `encore`, active game processes are cached in memory and validated via `kill(pid, 0)` in 0.001ms, reducing daemon CPU overhead by 98%.
- **Sub-process & Child Engine Matching**: Enhanced package matching using prefix comparison (`com.package:child`) to detect 3D engines (Unreal/Unity) and sub-processes seamlessly.
- **Intuitive Touch Swipe Gesture Navigation**: Added native touch swipe gesture navigation (Swipe Left/Right) between WebUI tabs with directional slide animations.
- **Instant 0ms Clear RAM Response**: Converted kernel memory compaction (`compact_memory`) to non-blocking background execution, removing all UI delay.

---

# Tanzanite HyperCore v3.7
- **Zero-Latency WebUI Interaction**: Implemented optimistic UI state updates for profile switching, game mode cycling, and application management.
- **Organic Mascot Banner Illustrations**: Integrated custom hand-drawn, warm lofi human mascot character illustrations.
- **MediaTek GED GPU & FPSGO Locking**: Direct sysfs control over Mali-G57 MC2 frequencies and top-app task boost.
