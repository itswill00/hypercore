# Changelog

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
