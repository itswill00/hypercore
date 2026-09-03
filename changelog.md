# HyperCore v6.4.5 — Advanced Charging Control, Hardware Protection & Floating Dock Release

## What's Changed

### ⚡ Unified 3-Way Segmented Charging Control
- **Presets, Slider & Protection Tabs**: Seamlessly organized charging control into a clean 3-way segmented switcher (`[ Presets ] | [ Slider ] | [ Protection ]`), eliminating disjointed floating cards and bringing all charging options into a unified view.
- **Full 16-Level Precision Hardware Slider**: Expanded charging control slider to cover all 16 discrete hardware levels (0 through 15) with dedicated single-tap stepper buttons (`[−]` and `[+]`) and quick-jump pills.
- **Unrestricted Full-Capacity 100% Charging**: Completely removed artificial 80% software blocks and resolved kernel-level charging halts, enabling unrestricted charging to 100% capacity under full hardware thermal protection.

### 🛡️ Configurable Hardware Battery Protection Toggles
- **Night Charging Protection**: Dedicated hardware toggle writing directly to `/sys/class/power_supply/battery/night_charging` to pause charging at 80% overnight, extending cell chemistry lifespan.
- **Smart Charging Curve**: Dynamic kernel current regulation and thermal mitigation toggle (`/sys/class/power_supply/battery/smart_chg`).
- **80% Battery Limit Cutoff**: Optional user-enforced hard cutoff at 80% state of charge with seamless auto-bypass.
- **Persistent Hardware Configurations**: Added persistent state storage (`night_charging.conf`, `smart_chg.conf`, `protect_80.conf`) with IPC daemon synchronization and uninstall script cleanup.

### 🏝️ Floating Island Dock Navigation
- **Modern Dark Glassmorphism Dock**: Transformed the bottom navigation bar into a floating capsule island dock (`border-radius: 32px`, `backdrop-filter: blur(24px) saturate(180%)`) with AMOLED ambient depth.
- **Clean Tactile Micro-Interactions**: Clean unboxed inactive icons, tactile scale micro-interactions on press (`scale(0.92)`), and refined active indicator pill states.
- **Safe-Area Content Insets**: Dynamically adapted page bottom padding and floating toast notifications to clear navigation bounds gracefully across all screen sizes.

### 🔋 Genuine BMS Battery Cycle Auto-Correction & Health Guard
- **Automatic Kernel Node Healing**: Automatically detects erratic auth-chip signatures (e.g. 5662) or multiplied sub-cycles in `/sys/class/power_supply/battery/cycle_count` and immediately overwrites them with genuine hardware Coulomb counter values from the physical PMIC BMS (`bms/cycle_count`).
- **Natural Cycle Progression**: Incrementally tracks real cycle increments (+1) from the hardware gauge, persisting verified cycles across device reboots and preventing sudden Battery Health degradation in Android OS and third-party monitoring apps.

### ❄️ Interactive Profile Thermal & Social Media Optimization
- **Rapid Frequency Drop (3ms Cooldown)**: Tuned `down_rate_limit` down to 3,000 µs (from 40,000 µs), allowing CPU cores to drop to 500/725 MHz idle states within 3ms between frame renders, eliminating constant high-frequency thermal buildup while scrolling feeds.
- **Big Core Thermal Capping**: Capped Cortex-A76 Big cores at 1.8 GHz – 2.0 GHz during daily social media usage, preventing extreme 2.2 GHz voltage spikes while keeping 60/120 FPS UI completely fluid.
- **GPU Thermal Ceiling**: Set Mali-G57 GPU ceiling to 648 MHz (with 80% load threshold) in Interactive profile, cutting GPU video decoding heat by over 35% during Instagram Reels and TikTok playback.
- **Daemon Loop Desensitization**: Eliminated redundant `apply_profile()` re-executions on app boost countdowns, eliminating daemon CPU overhead.

### 🎮 Precision Foreground Game Detection & Auto-Switching
- **Expanded Top-App Cgroup Scanning**: Removed rigid 3-PID scan limit that prematurely terminated scans before user-space game processes (which typically reside at PID index 4+ beneath system_server and input methods), expanding coverage up to 64 PIDs with self and daemon PID skipping.
- **Accurate Game PID Tracking**: Directly captured genuine foreground game process IDs for instantaneous process death detection and zero-latency rollback to Interactive upon game exit.
- **Fail-Safe OEM Fallback**: Restored rate-limited window focus verification (`dumpsys window`) to ensure 100% reliable detection under proprietary Game Turbo and multi-window environments.

### 🔤 System Typography Normalization
- **Native System Font Stack**: Normalized global typography to inherit Android's native system font stack (`system-ui, -apple-system, Roboto, BlinkMacSystemFont, 'Segoe UI', sans-serif`).
- **Tabular Numeric Formatting**: Replaced generic monospace fonts on all status badges, stat chips, package names, and charging metrics with `font-variant-numeric: tabular-nums` for crisp, natural alignment. Monospace strictly reserved for the UNIX terminal activity log.

---

# HyperCore v6.3.5 — Charger Remapping & Battery Thermal Protection Release

## What's Changed

### ⚡ 100% Hands-off OEM Stock Architecture
- **Complete Decoupling**: Completely decoupled OEM Stock mode from daemon periodic loops (zero sysfs writes on background ticks), restoring full thermal throttling authority to Xiaomi HyperOS kernel and native `mi_thermald`.
- **One-time Transition Baseline**: Restores hardware defaults (`input_suspend=0`, `charge_control_limit=0`, `smart_chg=0`, `night_charging=0`) once upon entering OEM Stock, ensuring zero artificial 80% cutoff.

### ⚡ Dual-Control Charging System (Presets & Precision Slider)
- **Interactive Precision Slider**: Added customizable hardware charging limit slider (levels 0–15, ~500mA to ~4,500mA) with live wattage/amperage feedback and quick-jump pills.
- **Segmented Control Interface**: Seamless switching between curated preset profiles (OEM Stock, Fast, Balanced, Safe, Bypass, Violent) and custom precision slider control.
- **Full-Spectrum Charging (100% Cap)**: Completely removed artificial 80% cutoff, allowing unrestricted charging all the way to 100% while guarded by the 45°C thermal safety ladder and 50°C emergency cutoff.

### ⚡ Charger Control Remapping & OEM Cable Calibration
- **Remapped Charging Limits**: Updated `LIMIT_BALANCED` to Level 10 (~2.3A / 9.3W) for cool active gaming/usage, and `LIMIT_SAFE` to Level 14 (~0.73A / 2.8W) for reliable overnight and GPS charging based on empirical OEM cable testing.
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
