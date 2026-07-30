# Tanzanite HyperCore v3.6 (Fast Charge Restoration Update)

### ⚡ Unrestricted Fast Charging & Core Enhancements
- **Removed Charging Rate Throttling**: Completely removed `apply_battery_thermal_guard` current limit caps (2.0A cap and 0 mA locks) in native C daemon engine (`src/main.c`).
- **Restored Unrestricted Fast Charging**: Full fast charging speeds (33W, 67W, 120W Super Charge) now operate at 100% native hardware speed during and after gaming sessions without daemon interference.
- **Fixed Post-Gaming Charge Lock**: Eliminates kernel charge current lock issues where returning from games locked charging speed to minimum or zero.
- **Installed App Filtering**: WebUI `My games` now filters configured entries against installed user packages, displaying real App Labels and icons without uninstalled package bloat.
- **Kernel String Overflow Protection**: Added text truncation and tap-to-expand toggle for long Linux kernel version strings in Device Info.
