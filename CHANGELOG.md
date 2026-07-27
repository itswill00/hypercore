# Changelog

## [v3.0] - HyperCore Edition

- **Engine Rebrand**: Renamed engine and module to **HyperCore**.
- **Refactored Architecture**: Replaced legacy codebase with clean, modular C code structure in `src/hypercore.c`.
- **Thermal Hysteresis & Cooldown**: Added thermal hold timers and hysteresis thresholds to eliminate profile thrashing.
- **Daemon Robustness**: Implemented `daemon(1, 1)` auto-detaching and `SIGHUP` signal handling.
- **WebUI Update**: Updated dashboard and control routines for HyperCore v3.0 compatibility.
