# HyperCore v6.3.5 — Charger Remapping & Battery Thermal Guard Release

## What's Changed

### ⚡ 100% Hands-off OEM Stock Architecture
- **Complete Decoupling**: Completely decoupled OEM Stock mode from daemon periodic loops (zero sysfs writes on background ticks), restoring full thermal throttling authority to Xiaomi HyperOS kernel and native `mi_thermald`.
- **One-time Transition Baseline**: Smoothly restores hardware defaults (`input_suspend=0`, `charge_control_limit=0`, `smart_chg=1`, `night_charging=1`) once upon entering OEM Stock without interfering with `sconfig`.

### ⚡ Dual-Control Charging System (Presets & Precision Slider)
- **Interactive Precision Slider**: Added customizable hardware charging limit slider (levels 0–15, ~500mA to ~4,500mA) with live wattage/amperage feedback and quick-jump pills.
- **Segmented Control Interface**: Seamless switching between curated preset profiles (OEM Stock, Fast, Balanced, Safe, Bypass, Violent) and custom precision slider control.
- **Unified Thermal & SOC Protection**: Custom slider limits remain 100% guarded by the 45°C thermal stepped cooling ladder, 50°C emergency cutoff, and 80% SOC longevity tapering.

### ⚡ Charger Control Remapping & OEM Cable Calibration
- **Remapped Charging Limits**: Updated `LIMIT_BALANCED` to Level 10 (~2.3A / 9.3W) for cool active gaming/usage, and `LIMIT_SAFE` to Level 14 (~0.73A / 2.8W) for reliable overnight and GPS charging based on empirical OEM cable testing.
- **Automatic 80% SOC Tapering**: Added automatic speed tapering from Fast/Violent modes down to Balanced mode when battery capacity reaches >=80% to protect cell health.
- **TCPC PD Re-negotiation**: Enhanced `apply_effective_mode()` so TCPC CC-pin pulse re-handshake triggers reliably on mode transitions.
- **WebUI Stats Sync**: Updated `ChargerView.vue` UI labels, estimated current, and estimated power numbers to match empirical hardware readings.

### 🔋 Battery Cycle Stabilizer & Single Source of Truth
- **Hardware Fuel Gauge BMS Priority**: Directly reads physical MediaTek MT6366/MT6358 Fuel Gauge Coulomb counters (`bms/cycle_count`), eliminating conflicting WebUI background shell commands and software cycle accumulation.
- **Auth Chip Filter**: Added strict filter to discard raw STMicroelectronics authentication chip signature codes (>3000) on Xiaomi MT6789 devices.

### 📦 Build System & Internal Storage Cleanliness
- **Unified Release Naming**: Standardized release asset naming to `HyperCore-${VERSION}-b${VERSION_CODE}-Unified.zip`, completely eliminating duplicate git hash variations.
- **Internal Storage Output**: Restricts package output strictly to device internal storage (`/sdcard/HyperCore_Releases`) with zero pollution in Termux home directories.

---

# HyperCore v6.3.0 — Intelligent Multi-Step Thermal & USB PD Release

## What's Changed

### ⚡ Smart Charger Control & USB PD Re-negotiation
- **Automated TCPC USB PD Pulse**: Added 100ms MediaTek TCPC CC-pin pulse when switching to OEM Stock, Fast, or Violent modes. Forces instant USB PD re-handshake to unlock 13.5W - 15W+ fast charging without needing manual cable unplugging.
- **Gradual Multi-Step Thermal Ladder**: Upgraded thermal safety engine with a stepped cooling ladder (10s step-down dwell time, 15s cool recovery dwell <= 41°C). Prevents thermal shock and rapid pause-resume cycling.
- **Flawless OEM Stock Transition**: Fixed socket state machine and sysfs node reset (`input_suspend=0`, `charge_control_limit=0`) when switching from Bypass back to OEM Stock.
- **Smart Cable Hardware Ceiling Detection**: Real-time USB VBUS polling and automatic detection banner when standard/non-OEM cables are hardware-capped by the PMIC.
- **Empirical Hardware Calibration**: Re-calibrated MT6789 charging limits (Balanced = 6 for ~7.4W, Safe = 12 for ~4.2W) based on real-time hardware measurements.

---

# HyperCore v6.2.0 — MD3 Charger Dashboard & Device Safety Release

## What's Changed

### ⚡ Charger Control & Thermal Safety
- **MD3 Charger Control Dashboard**: Brand new native Material Design 3 view with active power stats, current flow (mA), voltage (mV), power calculation (W), and mode selection.
- **Violent Charge Mode (up to 33W)**: Extreme fast charging profile unlocking full factory charging speed while disabling smart thermal limits. Includes interactive risk acknowledgement modal.
- **Hardware Bypass Mode Fix**: Double-guarded with `input_suspend=1` and hardware cutoff limit=16 to guarantee true 0 mA zero-current power bypass.
- **Persistent User Selection**: User-selected charge mode persists across WebUI restarts and daemon reboots without reverting to stock.
- **Device Support Probing**: Automatic sysfs node validation with graceful safety fallback UI ("Your device does not support this function properly") for unsupported kernel nodes.

---

# HyperCore v6.1.1 — Early Boot Maintenance Release

## What's Changed

### 🔧 Fixes & Enhancements
- **Metadata**: Restored missing `updateJson` property in `module.prop` for Magisk, KernelSU, and APatch auto-updater support.
- **Early-Boot Execution**: Enhanced `post-fs-data.sh` with direct `/proc/sys/vm/` procfs writes and sysctl fallbacks, guaranteeing 100% execution reliability across all Android 12-16 GKI environments.

---

# HyperCore v6.1.0 — Performance Optimization & mBanking Compatibility

## What's Changed

### 🔧 Performance & Bug Fixes (Daemon)
- **sysfs I/O**: Eliminated triple-open bottleneck in `sysfs_write_fallback` — halved syscall count per profile transition
- **IPC**: `update_status_json_file()` no longer re-reads thermal nodes already available in main loop (−2 sysfs reads/sec)
- **Gamelist**: Reduced PID scan cap 250→64 + pre-computed `pkg_len` cache — eliminates O(N²) per-tick overhead
- **Thermal**: Deferred cycle tracker disk writes from per-1% to every 5 minutes (−unnecessary flash I/O during charging)
- **Log**: Raised log rotate threshold 100 KB→512 KB — prevents excessive rotation during active gaming sessions
- **Anti-tamper audit**: Rate-limited to 1× per 5 seconds (−80% sysfs reads for governor verification)
- **CPU governor cache**: `get_best_governor()` result cached at startup, no more sysfs reads on profile transitions
- **IRQ affinity**: `apply_irq_tuning()` now profile-aware — Gaming pins GPU/touch IRQs to big cores (0xc0), others restore to all cores (0xff)

### 🏦 mBanking App Compatibility Fix
- **`system.prop` cleanup**: Removed `debug.sf.latch_unsignaled`, `persist.sys.smartpower.*`, `persist.sys.powerkeeper.*`, `persist.sys.joyose.*`, `persist.sys.wifi.low_latency` — all flagged by banking integrity scanners
- **Runtime props**: `debug.sf.latch_unsignaled=1` now applied via `resetprop` only during Gaming/MOBA profiles; auto-cleared when switching to any other app
- **IPC socket relocation**: Moved from `/data/adb/modules/hypercore/` to `/dev/hypercore.sock` (standard POSIX IPC path, not in root manager scan zone); backward-compat symlinks maintained at legacy paths

### 🌐 WebUI
- Fixed polling race: `pollCpuGpu` and `pollRamBat` intervals now skip tick when `refresh()` is in flight
- Reduced `nc` socket timeout 2s→1s to prevent pipeline stall on unavailable socket
- All nc commands updated to try `/dev/hypercore.sock` first, legacy paths as fallback

---

# HyperCore v6.0.0 — Universal Dual-Kernel Architecture Release

## What's Changed
- **Universal Dual-Kernel Support**: Full compatibility across Linux Kernel 5.10 to 6.12 GKI (Android 12 to 16).
- **Mali GPU Performance Scaling**: Dedicated GPU `performance` governor scaling, MTK GED tuning, and 30ms polling for Gaming profiles.
- **Zero-Flicker WebUI Telemetry**: Atomic `status.json` telemetry persistence and resilient state retention in Vue 3 Pinia store.
- **Installer & Process Hardening**: Safe daemon lifecycle management on upgrades, boot launch retry guard, and user gamelist preservation.
