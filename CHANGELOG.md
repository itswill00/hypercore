# Changelog

## [v4.0] - UNIX Socket IPC & Dynamic Efficiency Release

- **UNIX Domain Socket IPC (0ms Latency)**: Non-blocking IPC socket server (`hypercore.sock`) for instant JSON telemetry and zero WebUI latency.
- **Cgroup v2 & Process Priority Isolation**: Restricted background cpusets to cores 0-1 and throttled cpu.shares to 10% during gaming; active game PID set to `priority -10`.
- **Dynamic Poll Scaling**: Intelligent loop timing (8000ms sleep, 1000ms gaming, 2000ms interactive).
- **Proactive Memory Compaction on Sleep**: Automatic RAM defragmentation (`compact_memory`) upon screen lock.
- **Zero-Overhead UFS 2.2 Storage Queue Tuning**: Disabled `nomerges` and dynamically scaled `nr_requests` (32/64/128).
- **Sleep Ramp-Up Barrier**: Applied 4000us `up_rate_limit_us` delay during sleep to eliminate battery drain from background notification clock spikes.

## [v3.6] - Fast Charging Restoration

- **Unrestricted Fast Charging**: Removed daemon current limit caps (`apply_battery_thermal_guard`), restoring 100% native fast charging speeds (33W/67W/120W) during and after gaming.
- **Fixed Post-Gaming Charge Lock**: Solved kernel charge current lock issues after exiting games.
- **Installed App Filtering**: WebUI `My games` now displays real App Labels and icons for installed games only.
- **Kernel String Overflow Protection**: Added truncation and tap-to-expand toggle for long kernel version strings.

## [v3.5] - Major Update

- **Material Design 3 WebUI Overhaul**: Rebuilt interface with modern MD3 dark-theme design tokens, floating navigation bar, and clean sentence-case typography.
- **Native System Font Integration**: Switched to native system-ui font stack for zero-delay offline rendering using the user's default phone font.
- **Interactive Game Mode Cycler**: Tap-to-cycle profile modes directly on game tags in `My games`.
- **Live Terminal Syntax Highlighting**: Colorized event log line parser for real-time daemon event monitoring.
- **Visual Memory Progress Bars**: Added RAM and ZRAM percentage progress bars to System Status dashboard.
- **Thermal Hysteresis & Debouncing**: Added 4°C hysteresis margin and 3-tick game exit debounce to prevent profile flapping.
- **Dynamic CPU Governors**: Enhanced governor scaling using `sugov_ext` and `schedutil`.
- **Flash Protection**: Throttled `module.prop` status disk writes to extend flash memory lifespan.

## [v3.0] - HyperCore Edition

- **Project Rebrand**: Renamed system and module to **HyperCore**.
- **Refactored Architecture**: Replaced legacy codebase with clean, modular C code structure in `src/hypercore.c`.
- **Thermal Hysteresis & Cooldown**: Added thermal hold timers and hysteresis thresholds to eliminate profile thrashing.
- **Daemon Robustness**: Implemented `daemon(0, 0)` auto-detaching and `SIGHUP` signal handling.
- **WebUI Update**: Updated dashboard and control routines for HyperCore v3.0 compatibility.
