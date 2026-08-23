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
