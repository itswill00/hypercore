# Tanzanite HyperCore v3.5 (Major Release)

### 🌟 Major Architecture & Interface Upgrades
- **Complete Material Design 3 WebUI Redesign**: Rebuilt the entire WebUI interface with modern MD3 dark-theme design tokens, responsive cards, floating bottom navigation bar, and clean sentence-case typography.
- **Native System Font Stack**: Removed external network dependencies for web fonts, enforcing native phone font integration (`system-ui`, `MiSans`, `Roboto`, `One UI Font`) for zero-delay instant rendering.
- **Interactive App Mode Cycler**: Added one-tap profile mode cycling (`GAMING`, `TOUCH`, `BALANCED`, `SAVER`) directly on configured game tags in `My games`.
- **Live Terminal Syntax Highlighting**: Realtime event log console in `System log` now highlights daemon events (Gaming boost, Thermal Tiers, profile locks, service status) with distinct color coding.
- **Visual Memory & Storage Gauges**: Added dynamic visual progress bars for physical RAM and compressed ZRAM swap usage percentages.
- **Hysteresis Thermal Protection**: Introduced a 4°C hysteresis margin for Thermal Tier transitions (T1 triggers at 58°C, releases at 54°C CPU / 42°C Battery) to eliminate thermal profile flapping.
- **Debounced Game Detector**: Implemented a 3-tick (6-second) hysteresis debounce before marking games as exited, resolving false-positive duplicate toasts and profile ping-ponging.
- **Dynamic CPU Scaling Governors**: Upgraded CPU governor policies to dynamically balance between `sugov_ext` and `schedutil` across interactive and gaming states.
- **Device-Mapper & UFS Queue Tuning**: Extended storage scheduler and read-ahead tuning to Device-Mapper (`dm-`) and UFS block devices.
- **Flash Memory Protection**: Throttled `module.prop` status writes with time-based debouncing to protect internal storage from unnecessary flash wear.
