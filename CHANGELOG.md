# Changelog

All notable changes to the Tanzanite HyperFlow module will be documented in this file.

## [v2.5] - 2026-07-27

### Added
- **Native C Daemon (`hyperflow_engine`):** Replaced shell-based engine loop with a compiled AArch64 C binary for zero subshell overhead and sub-millisecond sysfs execution.
- **Dynamic Thermal & Device Node Discovery:** Automatic runtime discovery of CPU/Battery thermal zones and display backlight interfaces.
- **UFS Queue Optimization:** Automatic configuration of UFS storage block queues to `none` scheduler and 128KB read-ahead.
- **ZRAM Memory Optimization:** Explicit `vm.page-cluster = 0` configuration to eliminate swap read-ahead latency.
- **SurfaceFlinger Rendering Properties:** Added phase offset and low-latency rendering properties in `system.prop`.
- **Auto Game Workload Detection:** Automatic profile switching when gaming applications are detected in the foreground.

### Changed
- Refactored CLI utility (`tanzanite`) for faster parameter output and status parsing.
- Reduced overall flashable zip package size to ~18 KB by stripping development source files from runtime package.
- Cleaned up documentation, comments, and project structure under MIT License.
