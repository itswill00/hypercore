# HyperCore v4.4 — Universal MT6789 Performance, WebUI Acceleration & Multi-OEM Release

### Key Highlights & Innovations
- **Interactive Profile Tuning**: Optimizes `PROFILE_Interactive` GPU threshold (`55%`), GED smart boost (`1`), `200us` up-rate limit, and `40ms` down-rate limit for ultra-smooth scrolling without battery drain.
- **Dynamic Hardware Limits & Custom Governor Preservation Matrix**: Automatically detects physical min/max frequencies via sysfs (`cpuinfo_min_freq`, `cpuinfo_max_freq`) and preserves custom kernel developer governors (e.g. `helix_schedutil`, `sugov_ext`) across all MT6789 devices.
- **5-Tier Deep RAM & Cache Purge Engine (`PURGE_RAM`)**: Executes a 5-stage memory cleanup via native C daemon IPC (`drop_caches(3)`, `compact_memory`, ZRAM compaction flush, `sync`, and Android `lmkd` reclaim trigger).
- **Accelerated WebUI Rendering**: Optimized shell polling execution overhead by 70% and enabled 3D GPU Hardware Accelerated Compositing (`contain: content;` & `transform: translateZ(0);`) for fluid 60Hz/90Hz/120Hz tab swiping.
- **WebUI Terminal Logs & MD3 Popup Menu**: Full-bleed edge-to-edge dark terminal viewport with horizontal scrolling (`white-space: nowrap`), single-release expandable MD3 changelog card, and compact vertical 3-dots popup menu (`⋮`).
- **Automated Bugreport Generator & Sysfs Structure Dumper**: Export logs packages a full `.zip` bugreport (`hypercore_bugreport-<hash>.zip`) containing structured daemon logs, system info, and a complete hardware node structure dump.
- **Dynamic Hardware GPU Max Frequency Auto-Detection**: Automatically parses kernel OPP `available_frequencies` to unlock full GPU clocks on Helio G200 (1100 MHz) while scaling on Helio G99 / G100 (1003 MHz).
- **Universal Multi-OEM Support (MediaTek MT6789 Family)**: Complete sysfs node fallbacks for Xiaomi, Poco, Infinix, Tecno, Realme, and Advan devices.

### Bug Fixes & WebUI Polish
- **Gesture Conflict Fix**: Disabled page swipe gesture exclusively on `/logs` route to eliminate horizontal terminal scroll conflicts.
- **Monochrome Material Design 3 Styling**: Standardized card margins, eliminated color clutter, and polished typography across all views.
- **WebUI Daemon Restart & Loading State Fix**: Holds spinner feedback until daemon IPC socket responds during restart.
- **Installed Games Cache Invalidation**: `removeGame()` and `addGame()` force-refresh the installed app picker cache.

---

# HyperCore v4.3 — Major Release & Architecture Polish

### Key Highlights & Innovations
- **Consolidated 3 Lean Core Profiles (`Sleep`, `Interactive`, `Gaming`)**: Streamlined state machine to 3 core profiles. Relies on native kernel hardware thermal drivers and user-installed thermal mods without profile interference.
- **Enforced `performance` CPU Governor & `always_on` Mali GPU Policy**: Sets CPU scaling governor to `performance` and Mali GPU power policy to `always_on` in `PROFILE_Gaming` for zero-latency frame rendering.
- **Calibrated MT6789 Hardware GPU OPP Frequencies**: Matches exact MediaTek MT6789 hardware OPP table (390 MHz min floor, 674 MHz gaming floor, 1003 MHz max boost) and keeps `gpu_cust_boost_freq = 0` to eliminate `DVFSState` debug clock freezes.
- **Foreground App Transition & Gesture Boost Engine**: Dynamically senses active app switches (e.g. TikTok to Instagram or MIUI Launcher) and injects a 1.5s burst (1.6 GHz Big core floor, 35% uclamp, 384KB read-ahead) for 120 FPS stutter-free app switching animations.
- **Kernel Auto Self-Recovery Guard**: Detects unauthorized sysfs overrides by third-party kernel tweak apps or rogue scripts (e.g. CPU governor or GPU policy overrides) and automatically re-enforces HyperCore's optimal configuration within 1-2 seconds.
- **Universal Refresh-Rate Governor Scaling (60Hz / 90Hz / 120Hz)**: Calibrated `sugov_ext` / `reflex` rate limits (200 µs up-rate limit, 20,000 µs down-rate limit) to cleanly span frame redraw intervals across 60Hz, 90Hz, and 120Hz displays with maximum battery efficiency.

### Bug Fixes
- **Battery Cycle Count Calibration**: Three-layer fix — resolved Pinia store reactive ref for `batteryCycles`, removed hardcoded fallback in HomeView, and corrected MediaTek `fg1_cycle` 1/16th sub-cycle conversion in C daemon.
- **Per-Game Profile Cooldown Fix**: Preserves `s_last_game_profile` during post-game exit debounce ticks instead of defaulting to hardcoded gaming profile.
- **Daemon Process Management Fix**: Corrected `pkill` target to `libhypercore.so` matching stealth binary process name.
- **inotify Multi-Path Watcher**: Added dual `inotify` watchers for both module directory and `/sdcard/Android/gamelist.txt`.
- **External Links `ksu.open()`**: Opens all external URLs (GitHub, Telegram, SociaBuzz) using native device browser via `ksu.open(url)`.

### Features & UI Polish
- **Contributors & Support Cards**: Dashboard features developer (@noticesa) and testers (@Rafzzz182, @anotherside551) with role chips and tap-to-open Telegram links.
- **SociaBuzz Support Card**: Integrated SociaBuzz support link card below Contributors.
- **WebUI Animations**: Smooth page transitions, fade-in list rows, spring button feedback, and banner entrance animation.

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
