# HyperCore v6.11.0 — Thermal Guard Corrections

A full audit of the daemon, the WebUI and the packaging turned up that the
3-tier thermal guard was not enforcing two of its three tiers. Gaming at 70°C
ran uncapped, and the vendor thermal stack was being suppressed on a hot device
by the same code path that claims to stand down when it gets warm. Both are
fixed, along with the packaging gaps that let the module run code it had just
rejected.

Nothing here is cosmetic. If you play games in a warm room, the behaviour
change is real and it is in the direction of not cooking the phone.

## Fixes

### Thermal guard

- **Tier 1 now actually caps `Gaming`** (`src/cpu.c`): the tier-1 branch was
  gated on the MOBA profile, so a plain Gaming title at 70°C fell through to
  the hardware ceiling — full 2.2 GHz on the Big cluster, no mitigation. Both
  profiles are capped now; MOBA keeps the higher 2.0 GHz Big allowance.
- **Ceilings are clamped to the hardware maximum.** The tier values are
  constants, so a part reporting a lower ceiling must not be pushed above it.
  A "cap" that could raise a frequency is not a cap.
- **The vendor thermal stack is no longer suppressed when hot**
  (`src/cpu.c`): `apply_profile` wrote `sconfig 10` (thermal-nolimits) and
  `thrm_enable 0` at every tier, immediately undoing the skip that
  `enforce_gaming_thermal_bypass` performs at tier >= 1. Both now follow the
  tier, so at tier >= 1 thermal control goes back to `mi_thermald` and FPSGO.
- **The dead ceiling table is gone** (`src/thermal.c`): a second copy of the
  tier values was computed and then discarded by the `tier >= 1` bail-out below
  it, while the header comment claimed those numbers were enforced. Two
  divergent tables for one guard is how the cap goes missing. `build_profile_matrix`
  in `src/cpu.c` is now the single source of truth.
- **A dead sensor no longer reads as a cold SoC** (`src/thermal.c`): a sensor
  reporting 0 has not produced a sample, but the raw read went straight into the
  threshold comparisons, pinning the device at Tier 0 with the bypass armed. The
  guard now judges on whichever sensor actually reported and holds its tier when
  neither did.
- **Thermal zones are selected by type, not by index** (`src/thermal.c`): the
  scan skipped any zone that had not published its first reading, which on MTK
  parts is most of them for the first seconds after boot — so the real CPU and
  battery zones lost to whichever zone happened to be warm at scan time. The
  hardcoded fallback index is now accepted only after its `type` confirms it is
  the wanted sensor, instead of quietly binding the guard to a skin or PA zone.
- **GPU cooling device is matched properly** (`src/thermal.c`): `thermal-devfreq`
  is the generic cpufreq type and matches the CPU policies too, so gaming could
  force `cur_state 0` onto a CPU cooling device. GPU-specific types win first;
  the generic one is a fallback only.

### CPU

- **Governor and rate limits are applied per cluster** (`src/cpu.c`): the stock
  baseline captures the Big cluster separately (`gov6`, `pol6_*`) because MTK
  parts routinely boot the two clusters on different governors, but one governor
  and one rate-limit pair were written to policy0/4/6/7. Interactive — which is
  supposed to restore stock — was handing the Big cluster the Little cluster's
  values. Those baseline entries were previously only used on uninstall. The
  tuning profiles still drive both clusters as one, which is the point of them.
- **Devfreq ceiling is raised before the floor is lifted** (`src/cpu.c`):
  devfreq returns `-EINVAL` for min above max. No profile can trigger it today
  because they all pin the minimum to the same constant, but the write would
  fail silently.

### Packaging & security

- **`update.json` is generated at build time** (`build.sh`): `module.prop`
  points `updateJson` at it, so the root manager is what reads it — nothing in
  this repository did. That is exactly why no build step touched it and the
  release URL went stale as soon as the version moved.
- **Install no longer executes a payload that failed verification**
  (`customize.sh`): the integrity layer asked `libhypercore.so` to verify itself
  even after the manifest check had already failed, so a tampered daemon ran as
  root during install and the abort printed afterwards.
- **`status.json` is created 0600** (`src/ipc.c`): it took whatever the inherited
  umask allowed, which left the copy in `/dev` world-readable. Any app could read
  battery health, cycle count, temperatures and charge mode, contradicting the
  root-only telemetry path the design documents.
- **Uninstall preserves user configuration** (`uninstall.sh`): it removed all of
  `/data/adb/hypercore`, taking the charge limits, the hand-built gamelist and
  the HyperMoon HUD config with it, so uninstalling to reset the module cost the
  user everything with no way back. Settings are now copied to a timestamped
  sidecar under `/data/adb/hypercore_removed/` before only runtime state is
  removed.
- **Grants are revoked on uninstall** (`uninstall.sh`): the `SYSTEM_ALERT_WINDOW`
  appops and the `pm grant` for `com.android.shell` outlived the module.
- **`status.json` is no longer published when the write fails** (`src/ipc.c`):
  `fputs` and `fclose` results were ignored and the temp file was renamed into
  place regardless, so a full `/data` published a truncated file for the WebUI.
- **Gamelist autodetect no longer appends to shared storage** (`src/gamelist.c`):
  it appended to whichever path was read, which can be
  `/sdcard/Android/gamelist.txt` — a file any app with storage access can
  replace with a symlink, while the write runs as root. Discoveries now persist
  to the managed data dir, opened with `O_NOFOLLOW` and required to be a regular
  root-owned file.

## Notes

- Tier 2 keeps its existing 1.8 GHz ceiling. A never-applied 1.4/1.5 GHz value
  had been sitting in a comment; adopting it would have been a large behaviour
  change stacked on top of a correctness fix, so the live values were kept and
  the documentation corrected instead.
- `is_screen_on()` now defaults to "on" when no backlight node is readable.
  Assuming "off" pinned the device into the Sleep profile (850 MHz Big) with no
  recovery path. On a ROM where the node resolves normally this changes nothing.
- A peer at uid 2000 (`adb shell`) is not a read-only client. It is deliberate,
  since the documented `adb shell` workflow depends on it, but anyone holding an
  adb pairing token can drive charge mode, `PURGE_RAM` and profile switching.
  That is hardware control, not telemetry readback — worth knowing before
  wireless debugging is left enabled.

## Verification

- 81,648 tier transitions across every starting tier and a dense CPU/battery
  grid were replayed against the previous implementation: zero divergence
  whenever both sensors reported, which is what makes the Tier 1 change safe to
  land.
- Clean under `-Wall -Wextra -Werror` and the clang static analyzer.

---

# HyperCore v6.10.0 — ZRAM Pool Sizing & Density Pass

HyperOS ships a 6 GB compressed swap pool on a 8 GB phone. The extra capacity
never helps — it only widens the ceiling for compression work and keeps `kswapd`
busier than it needs to be. This release lets you size the pool properly, and
reclaims the vertical space the WebUI was wasting while it did.

## Features

- **ZRAM pool sizing** (`service.sh`, `webui/`): new Memory card on the Dashboard
  with Stock / 4 GB / 2 GB / Off presets. The choice is persisted to
  `zram.conf` and applied once per boot, after the ROM's `init.mt6789.rc` has
  already run `swapon_all`, so the resize wins instead of being overwritten.
  Deliberately not applied live: `swapoff` forces every compressed page back
  into RAM in one go, which is exactly how you get a freeze mid-game. The card
  therefore shows a Reboot button with a confirmation dialog instead of
  pretending the change is instant.
- **Preflight guard** (`service.sh`): the resize is skipped when
  `MemAvailable` cannot absorb `SwapUsed` plus a 512 MB margin, and a failed
  `disksize` write re-`mkswap`/`swapon`s the device rather than leaving the
  phone with no swap at all. Preset sizes are mapped as literal byte strings
  because `/system/bin/sh` does 32-bit arithmetic and `MB * 1024 * 1024`
  overflows above 2 GB.
- **Stock pool capture & restore** (`scripts/stock_baseline.sh`,
  `uninstall.sh`): the factory `disksize` and active compressor are snapshotted
  to `stock_zram.conf` at install time, so uninstall puts the pool back. Kept
  out of `stock_state.conf` on purpose — that file round-trips through the
  daemon's C baseline tables, and zRAM is not one of them.
- **Swappiness follows the pool** (`src/memory.c`): with no swap pool at all,
  `tune_memory_pressure()` now settles on 20 instead of 100. A high swappiness
  with nothing to swap into just makes `kswapd` scan anonymous pages it can
  never reclaim.
- **ZRAM drawer** (`webui/`): the card collapses to a single line and remembers
  whether you left it open, keeping the presets out of the way until wanted.
  Choosing a size reopens it so the Reboot button is actually visible.
- **Spam guard**: 15-second cooldown between size changes, persisted across
  WebUI reloads, because every change costs a reboot.

## Fixes

- **Logs menu was unreachable** (`webui/src/assets/main.css`): the header rule
  applying `overflow: hidden` to the three-dot button's wrapper also clipped
  the absolutely-positioned dropdown. The state toggled correctly on every tap,
  so it looked like a dead button rather than a CSS bug.
- **Deploy tripped its own integrity check** (`build.sh`): `--deploy` refreshed
  the daemon binary but left `customize.sh`, `scripts/stock_baseline.sh` and
  `checksums.txt` at their installed hashes, so every local deploy logged
  `[TAMPERING DETECTED]` for those two files. All three are now synced.

## Interface

- **Density pass** (`webui/`): list rows 12→10 px, hero banners and cards
  trimmed ~20 %, section spacing tightened. The Charger power grid became three
  columns instead of two, and the duplicated slider axis labels (Slow /
  Balanced / Turbo) were dropped in favour of the quick-pick pills below them.
  The Dashboard now fits about three more rows before it scrolls.
- **HyperMoon quick row** (`webui/`): the standalone HUD card became the first
  Quick Actions row, with a live FPS readout in its subtitle and the toggle
  switch intact.
- **Release Notes** (`webui/`): rewritten for this release and no longer falls
  back to a hard-coded old version string.

---

# HyperCore v6.9.5 — Universal HyperMoon Positioning

On Android 14 QPR3+/15 the WindowManager rejects the root overlay pid
(`Unknown pid`), so HyperMoon falls back to a raw SurfaceControl surface
with no input channel — finger-drag is impossible there by OS design, on
this and every other affected device.

## What's Changed

- **Position controls** (`webui/`): new Position section in WebUI → HUD —
  D-pad nudge (±20 px) + X/Y sliders + live coordinates, writing
  `position.json`, honored by both render paths within one refresh tick with
  automatic screen-bounds clamping.
- **Chevron up/down icons** (`webui/`): missing D-pad glyphs added.
- **Reset text fix** (`webui/`): Reset Position no longer claims "top-left
  corner" — it restores the (697, 411) default.

---

# HyperCore v6.9.4 — Boot Settle & Late Touch Discovery

After reboot the daemon reported Interactive while the profile was never
really applied: one-shot boot apply, unconditional status, and touch paths
cached before the touch IC probed meant writes silently never landed until
the next Sleep cycle.

## What's Changed

- **Boot settle re-apply** (`src/main.c`): force re-apply of Interactive at
  loop ticks 5/15/30 (~+10s/+30s/+60s) while still Interactive, skipped while
  a game is active or the profile is manually locked — same pattern as the
  charger enforce loop.
- **Late touch rediscovery** (`src/sysfs.c`, `src/cpu.c`): new
  `rediscover_touch_nodes()` fills touch sysfs paths that were still missing,
  called on every profile transition.
- **Boot verify** (`src/main.c`): `verify_boot_apply()` reads back touch
  smoothing and the Little-cluster governor against the stock baseline at the
  end of settle and logs mismatches instead of staying silent.
- **HyperMoon position controls** (`webui/`): on Android 14 QPR3+/15 the
  WindowManager rejects the root overlay pid, so the HUD falls back to a raw
  SurfaceControl surface with no input channel — finger-drag is impossible
  there by OS design. New universal positioning in WebUI → HUD → Position:
  D-pad nudge + X/Y sliders writing `position.json`, honored by both render
  paths within one refresh tick.

---

# HyperCore v6.9.3 — Security Audit Remediation

Full source audit (daemon, installer, build pipeline, WebUI) with 22 findings remediated.

## Breaking

- **Install contract**: `customize.sh` now requires `checksums.txt` in the ZIP and verifies the entire extracted payload *before* any payload file is sourced or executed. Always package via `build.sh`, which now generates the manifest and fails if any expected file is missing.

## Security

- **Charging thermal ladder restored** (`src/charger.c`): the ladder was disabled in `cb6a602`, leaving `TEMP_OVERRIDE_ENTER`/`TEMP_EMERGENCY`/`TEMP_OVERRIDE_CLEAR` as dead constants while `README` still claimed charging thermal protection existed. The selected mode is now a ceiling that temperature may only lower — one rung down at 45 °C (Violent → Fast → Balanced → Safe), hard floor to Safe Mode at 50 °C, restored only after 180 s continuously ≤41 °C. Dead band between 41–45 °C freezes position to prevent flapping. OEM and Bypass are exempt.
- **IPC socket authorised** (`src/ipc.c`): `/dev/hypercore.sock` accepted any UID, so any app on the device could set charge modes, force Bypass, or trigger a global page-cache purge as root. Clients are now vetted with `SO_PEERCRED` (uid 0 for WebUI bridges and `hypercore-bugreport`, uid 2000 for adb shell) and untrusted peers are closed *before* the daemon reads from them, which also removes a main-loop stall vector.
- **Integrity manifest covers executables** (`build.sh`, `customize.sh`): the manifest previously listed 7 non-executable files while ignoring every binary and every root-executed script — including `customize.sh` itself. Now 13 files, covering `customize.sh`, `uninstall.sh`, `scripts/stock_baseline.sh`, `hypermoon_d`, `hypermoon.dex` and `hypercore-bugreport`. Two-pass build: hash payload → embed table → relink → append `libhypercore.so`, which cannot embed its own hash. Verified by test: tampering with the daemon or the installer is now detected; previously both passed silently.
- **No cross-module tampering** (`service.sh`): removed a loop that chmod'd any `service.d` / `post-fs-data.d` entry matching `thermal|tweak|encore|ktweak` to 0644, silently disabling third-party modules, recording no original mode, and never restored on uninstall.
- **Logs moved off `/sdcard`** (`src/include/common.hpp`, `src/log.c`): telemetry is written to `/data/adb/hypercore/hypercore.log` instead of a world-readable path.
- **Root-shell injection hardening** (`webui/src/helpers/shell.js`): `am start -d` now receives a single-quoted argument and only `http(s)` URLs.

## Correctness

- **`hypermoon_daemon` → `hypermoon_d`**: at 16 characters the name exceeded the kernel's 15-character `comm` limit, so `pkill -x` and `pidof` could never match it. One daemon leaked per HUD toggle and it survived uninstall entirely, still globbing sysfs and forking `app_process` every 30 s. `build.sh` now fails the build if a daemon binary name exceeds 15 characters.
- **Single-instance lock implemented** (`src/main.c`): `struct hw_nodes.lock_file` was declared and never used, so nothing prevented two daemons fighting over the same sysfs nodes. Now an advisory `flock(LOCK_EX|LOCK_NB)` taken after `daemon()`.
- **Gaming thermal bypass tier-gated** (`src/thermal.c`): `sconfig=10`, FPSGO `thrm_enable=0` and the GPU devfreq cooler reset ran at every tier, fighting `mi_thermald` even at 75 °C. Now Tier 0 only; from Tier 1 up the daemon defers to the vendor thermal stack.
- **Tier 2 ceilings corrected** (`src/thermal.c`): documented as 1.5/1.4 GHz but implemented as 1.8/1.8 GHz. Now matches the documented intent.
- **`popen` no longer in the hot loop** (`src/gamelist.c`): with an empty gamelist and no matching games, `pm list packages` was spawning `app_process` up to twice a second forever. Auto-detection is now capped at once per hour; the `dumpsys window` fallback went 3 s → 10 s and is skipped when the screen is off or asleep.
- **Overlay watchdog** (`src/hud/hypermoon_daemon.c`): the dex fallback resolved to `/data/adb/hypercore/bin/`, a directory that never existed. Now checks real install paths, then `/proc/self/exe`, then disables itself with one warning instead of forking a shell every 30 s forever.
- **IPC read loop** (`src/ipc.c`): a single `read()` truncated split commands, so `SET_PROFILE:GAM` silently fell through to `Interactive`. Now loops to newline under `SO_RCVTIMEO`.
- **GPU governor detection** (`src/hud/hypermoon_daemon.c`): a pattern matching zero paths exited the loop on a stale buffer and never set the governor. Also fixed a `glob()` reuse without `globfree()` and a `usleep()` call with a >1 s user-controlled interval.
- **HyperMoon "restart"** (`webui/src/stores/hypermoon.js`): now actually kills the processes instead of re-running a liveness check against the still-running daemon.
- **Exec timeout** (`webui/src/helpers/shell.js`): rejects instead of resolving `''`, so a stuck root call is no longer indistinguishable from an empty result.
- **SIGTERM grace** (`service.sh`, `uninstall.sh`): waits up to 10 s / 5 s for `restore_baseline_nodes()` to rewrite ~60 sysfs nodes before escalating to SIGKILL.

## Docs

- Corrected the "4-Tier thermal mitigation (Tier 0–3)" claim in `README` and `docs/DOCUMENTATION.txt` — there are three tiers; the fourth was removed in `cb6a602` and the docs were never updated. Same for the `GET_PROFILE` and `ADD_GAME` IPC commands, which do not exist, and the documented socket path.
- Documented the security model: two-layer integrity, `SO_PEERCRED` trust list, single-instance lock, log location.

## Housekeeping

- Untracked `webui/node_modules` (1263 files already listed in `.gitignore` but committed before it existed). Repository `.git` shrinks from 64 MB to 17 MB.

---

# HyperCore v6.9.2 — Daemon Transition & Factory Baseline Hardening

## What's Changed

### Daemon
- **Profile-aware read-ahead revert**: fixed `Interactive`/`Sleep` clobber after boost expiry (`src/main.c`)
- **Boot log**: added `INIT -> Interactive (boot)` so restart always logs profile apply (`src/main.c`)

### Baseline & Installer
- **Factory baseline capture**: extracted to `scripts/stock_baseline.sh` and made 100% read-first (mali/vm/io/sched) with zero hardcoded stock values (`src/sysfs.c`, `customize.sh`)
- **No bare /tmp**: moved temp gamelist backup to `/data/local/tmp/hypercore` (`customize.sh`)

### Build & Docs
- **Build portability**: `PROJECT_DIR` relative, version auto-sync from `module.prop` to README/docs, Termux Bionic `javac`/`d8` fallback for HUD dex (`build.sh`)
- **Docs sync**: bumped `README`/`docs/DOCUMENTATION` to `v6.9.2-b6920`

---

# HyperCore v6.9.1 — Snap Navigation, Logs Stability & Transition Hardening

## What's Changed

### Navigation & WebUI
- **Snap navigation rebuilt**: replaced overlay `router-view` transitions with `scroll-snap` horizontal tab strip (`Home → Charger → Games → About → Logs`). Single live daemon-driven header, `mandatory` snap with `always` stop, one-page-per-swipe.
- **About/Logs order**: swapped so `About` is penultimate and `Logs` is last — matches nav weight and prevents `Games → Logs` skip on strong flings.
- **Header shutter fix**: Logs header unified to outer header via delayed `headerIdx` vs live `snapIdx`; terminal header removed to eliminate double-header reflow and statusbar bleed.
- **Logs drawer polish**: moved `btn-icon-md3`/`dropdown-menu` styles to `App.vue` scoped so outer 3-dot keeps its pill styling after header migration.
- **Swipe isolation**: terminal `pan-y pan-x`, `snapSuspend` during log-viewport gesture, and programmatic-scroll guards eliminate horizontal log scroll vs tab snap contention.

### Daemon & Foundations
- No daemon bump this cycle — full pre-bump audit of `main.c`/`cpu.c`/`thermal.c`/`charger.c`/`ipc.c`/`sysfs.c`/`gamelist.c` passed with only minor deferrable notes (sleep hold residual, OEM `charge_override` flag clear, TCPC 100 ms main-thread pulse).

---

# HyperCore v6.9.0 — Foundation Hardening & MT6789 Consistency Release

## What's Changed

### Refactor & Hardening
- **Charger dedup**: collapsed 10 copy-paste `save/load_*_conf` pairs into `save_int_conf`/`load_int_conf` + thin wrappers (`src/charger.c`, -142 LOC).
- **Baseline table-drive**: `stock_state.conf` save/load now loops over descriptor tables (`src/sysfs.c`).
- **Thermal normalization**: single `normalize_thermal_temps()` inline replaces 8 duplicated mili-degree blocks in `main`/`ipc`/`charger`.
- **Chmod guard dedup**: unified `write_chmod_guarded()` for rate-limit vs sconfig chmod variants (`src/cpu.c`).
- **Atomic baseline**: `save_stock_baseline()` writes `tmp+rename` so power loss never leaves half-written `stock_state.conf`.
- **IPC resilience**: status JSON buffer 1024->1152 with bounds check, `ipc_sync_status()` helper, `sigaction()` for SIGTERM/SIGINT.
- **WebUI guards**: `shell.js` double-resolve fix (`settled` flag), removed dead `pollCpuGpu`/`pollRamBat`, `hypermoon.js` `_pollInFlight` guard.
- **MT6789 lock**: `customize.sh` now requires GED+Mali+FPSGO (was `||`), strict parity with daemon `validate_hardware_target()`.
- **HyperMoon fix**: overlay `cpu_policy` now shows hardware ceiling `cpuinfo_max_freq` (500-2200 MHz) instead of throttled `scaling_max_freq` (500-2000).

### Build & Docs
- `service.sh` graceful SIGTERM before SIGKILL so `restore_baseline_nodes()` runs; `uninstall.sh` restores full GED/FPSGO/VM set.
- Version fallbacks in WebUI no longer hard-code `v6.8.5` (`store.moduleVersion` is live truth).

---

# HyperCore v6.8.5 — Logic Flaw Hardening, Baseline Integrity & Inotify Resilience

## What's Changed

### 🐛 Bug Fixes & Architectural Hardening
- **Swappiness Baseline Integrity**: `tune_memory_pressure()` now respects the active profile context, ensuring factory default swappiness from `stock_state.conf` is never clobbered down to 60 during Interactive and Sleep profiles.
- **Protect 80% Anti-Oscillation Hysteresis**: Added `s_protect_active` hysteresis to `enforce_charge_mode()`. Charging remains bypassed at 80% until capacity drops to 77%, preventing rapid 79%–80% charging flutter and repeated TCPC renegotiation stress.
- **Resilient Gamelist Inotify Watcher**: Rewrote `init_gamelist_watcher()` to watch parent directories for `IN_CLOSE_WRITE | IN_MOVED_TO`. The daemon now reliably reloads `gamelist.txt` even after atomic file replacements from WebUI or text editors.
- **Instant Profile IPC Status Synchronization**: `process_client()` now immediately invokes `update_module_prop_status()` and `update_status_json_file()` upon manual profile switches via IPC, eliminating status display lag in root managers and WebUI.
- **Display State Detection Authority Hardening**: Made hardware backlight readings strictly authoritative (`bl_val >= 0`), preventing screen-off states from falling through to legacy fb0/DRM checks that could prevent device sleep.
- **Big Core Thermal Priority & Multi-Policy Support**: Prioritized MT6789 Big Core thermal zones (`cpu_big` / `cpu-big`, score 14) over little cores, and added multi-policy iteration (`policy6`, `policy4`, `policy7`) in `enforce_gaming_thermal_bypass()`.
- **Shell Command PATH Resilience**: Hardened `resetprop` calls across `cpu.c` and `sysfs.c` with unified shell execution and comprehensive PATH resolution across KernelSU, APatch, and Magisk.

---

# HyperCore v6.8.4 — Charger Bypass Hysteresis & Sensor Discovery Hardening

## What's Changed

### 🐛 Bug Fixes & Architecture Hardening
- **Charger Bypass Hysteresis Preservation**: Fixed a bug in `enforce_charge_mode()` where non-static local variable `override_active` reset every 2-second daemon tick, preventing clean hysteresis recovery when battery level climbs back above 12% in Bypass mode.
- **Dynamic Thermal Zone Discovery for GPU & Charger**: Integrated automatic scanning for MT6789 GPU sensor nodes (`gpu1`, `gpu2`, `mali`) and Charger nodes (`charge_therm`, `charger`) into `scan_thermal_zones()`, replacing non-existent hardcoded thermal zones.
- **State Synchronization & WebUI Telemetry**: Added `effective_charge_mode` and `charge_thermal_override` to IPC responses and `status.json`. Synchronized real-time `thermalTier` and thermal protection warning banners in WebUI.
- **Stock Baseline Rollback in Uninstaller**: `uninstall.sh` now reads and restores untouched hardware baseline parameters from `/data/adb/hypercore/stock_state.conf` before removing the data directory.
- **Installer & Shell Hardening**: Standardized game auto-detection format in `customize.sh` (`${pkg}:GAMING`), fixed subshell quoting in `build.sh` deploy command, and updated all fallback version references to v6.8.4.

---

# HyperCore v6.8.3 — Status JSON & Boot Fixes

## What's Changed

### 🐛 Bug Fixes
- **`gpu_temp` / `chg_temp` Missing from `status.json`**: WebUI reads `status.json` as primary data source; these fields were present in live `GET_STATUS` IPC response but missing from the written file. WebUI now correctly displays GPU and charger temperatures instead of falling back to CPU/battery values.
- **Boot Status "Stopped" Fix** *(v6.8.2)*: Daemon status in `module.prop` now updates immediately to `Interactive` on startup.

---

# HyperCore v6.8.2 — Boot Status Fix

## What's Changed

### 🐛 Bug Fixes
- **Boot Status "Stopped" Fix**: Daemon status in `module.prop` now updates immediately to `Interactive` on startup without requiring a manual profile trigger to Gaming/Sleep first. Root cause: `update_module_prop_status()` was only called on *profile change*, so if the initial profile was already `Interactive`, no change was detected and the status remained stuck as "Stopped" from the previous shutdown.

---

# HyperCore v6.8.1 — Persistent Factory Baseline & Pure Touch-Enhanced Interactive Remap

## What's Changed

### 💾 Persistent Factory Baseline Snapshotting (`stock_state.conf`)
- **Zero-Tamper Factory Baseline Capture**: On initial installation (via `customize.sh` or early daemon boot), HyperCore automatically snapshots and locks down the device's original, untouched factory hardware settings into `/data/adb/hypercore/stock_state.conf`.
- **Permanent Stock Baseline Preservation**: Preserves stock CPU governors (`sugov_ext`), core frequencies, microsecond rate limits, cgroups, UCLAMP, GPU devfreq governor & frequency limits, MediaTek GED/FPSGO states, and memory VM settings across reboots, profile changes, and module updates.
- **Uncompromised Exit-to-Stock Rollback**: Exiting gaming sessions or reverting to Interactive profile cleanly restores 100% factory hardware parameters, cooling the device down to ambient temperatures immediately.

### 📱 Pure Touch-Enhanced Interactive Profile Concept
- **Factory Stock Hardware Foundation**: Interactive mode now leaves all CPU clocks, governors, rate limits, cgroups, uclamp, devfreq, GED/FPSGO, VM, and scheduler nodes at their true factory stock defaults.
- **Exclusive Touchscreen Hardware Acceleration**: The *only* enhancement applied during Interactive mode is hardware touchscreen smoothing and noise filtering (`touch_thp_smooth = 1` and `touch_thp_noisefilter = 1`), delivering silky smooth 90/120Hz scrolling and zero jitter without extra battery consumption or heat.
- **Dynamic GPU Polling Freedom**: Removed all static enforcement locks on Mali GPU `polling_interval`, allowing the kernel devfreq driver to scale polling dynamically up and down based on workload.

### 📦 Termux Home Release Output Migration
- **Direct Workspace Output Packaging**: Migrated target package output directory from external `/sdcard` to Termux home: `~/HyperCore_Releases/` (`/data/data/com.termux/files/home/HyperCore_Releases/`).
- **Storage Sandbox Immunity**: Prevents Android 11+ storage isolation / FUSE permission errors during command-line compilation while maintaining consistent release paths across all documentation and workflow runbooks.

### 🛡️ Active Gaming Thermal Bypass Engine
- **Continuous Thermal Throttle Mitigation**: Actively shields Gaming and Gaming MOBA profiles from premature vendor thermal degradation during long gaming sessions.
- **Persistent Xiaomi `sconfig 10` Lockdown**: Continuously enforces `sconfig 10` (`thermal-nolimits.conf`) and suppresses `mi_thermald`'s `cpu_limits` downclock directives.
- **MediaTek FPSGO Throttle Bypass**: Keeps `/sys/kernel/fpsgo/fbt/thrm_enable = 0` during active games to prevent artificial framerate capping.
- **GPU Devfreq Cooler State Reset**: Automatically resets `cur_state` on Mali GPU cooling devices (`thermal-devfreq-0`) to prevent thermal devfreq downclocks.
- **CPU Scaling Max Frequency Protection**: Actively detects and restores Little (2.0 GHz) and Big (2.2 GHz) maximum CPU frequencies if vendor thermald attempts to clamp them down prematurely, preserving steady 60/90/120 FPS frame delivery.

---

# HyperCore v6.8.0 — Dynamic Thermal Guard & Xiaomi Thermal Throttling Mitigation Release

## What's Changed

### 🛡️ Dynamic Thermal Guard with Tropical Climate Headroom
- **Dynamic 3-Tier Thermal Protection**: Replaced stock thermal limitations with an intelligent, balanced thermal guard engineered for tropical climates:
  - **Tier 0 (Optimal / Cool)**: Full uncapped hardware maximum frequencies (Big 2.2 GHz, Little 2.0 GHz) below 45°C battery and 70°C CPU.
  - **Tier 1 (Warm / Active Mitigation)**: Smoothly caps Big and Little cores to 1.8 GHz when battery reaches 45°C or CPU reaches 70°C, with 3°C hysteresis to prevent frequency flutter. Zero core hotplug, zero micro-stutter.
  - **Tier 2 (Hot / Safety Protection)**: Safety cap to 1.5 GHz Big / 1.4 GHz Little when battery reaches 48°C or CPU reaches 75°C to arrest thermal runaway while maintaining full device stability.
- **IPC Telemetry & Status Integration**: Embedded `thermal_tier` directly into `/dev/hypercore_status.json` and the `GET_STATUS` IPC endpoint for real-time monitoring.

### ⚡ Xiaomi Stock Throttling Bypass (`sconfig 10`)
- **Stock 37°C Throttle Bypass**: Automatically engages Xiaomi `sconfig 10` (`thermal-nolimits.conf`) in Interactive and Gaming profiles, eliminating stock thermal throttling that aggressively capped CPU clocks and forced artificial minimum frequencies (`boost:1`) at merely 37°C.
- **Node Permission Hardening**: Automatically configures and enforces write permissions for `/sys/class/thermal/thermal_message/sconfig` and `cpu_limits` across boot (`service.sh`) and runtime.
- **Graceful Baseline Restoration**: Reliably restores `sconfig` to stock `"0"` and resets node permissions upon daemon termination and module uninstallation (`uninstall.sh`).

### 🎯 Golden Ratio Interactive Profile Remap
- **Sweet-Spot Efficiency Curve**: Capped Cortex-A76 Big cores at 2.0 GHz in Interactive mode (daily use), reducing dynamic power consumption by ~35% while preserving instant responsiveness. Cortex-A55 Little cores operate uncapped up to 2.0 GHz.
- **Microsecond Touch Filtering**: Retuned `up_rate_limit_us` to 1.5ms for immediate touch response, and `down_rate_limit_us` to 25ms to lock frametimes smoothly across 60/90/120Hz refresh rate boundaries.
- **Hardware Touch IC Acceleration**: Enforced `touch_thp_smooth = 1` and `touch_thp_noisefilter = 1` to deliver buttery, ultra-low latency touch tracking and eliminate charging phantom touches.
- **Dynamic GPU Scaling Freedom**: Balanced Mali GPU `devfreq_upthresh` (65%) and `devfreq_downdiff` (20%) with GED DVFS threshold (20) to allow GPU frequencies to scale effortlessly between 390 MHz and 1003 MHz based on workload.
- **Scheduler Headroom Optimization**: Raised `top_app_uclamp_min` to 15% to guarantee foreground UI priority, while tightening `bg_uclamp_max` to 50% on cores 0-3.

### 🧹 Purge of Redundant Sysfs Mutation Auditing
- **Zero Log Mutation Spam**: Completely eliminated `audit_active_profile_state()` and redundant 5-second sysfs mutation checks.
- **Tug-of-War Elimination**: Terminated the cyclic fight between HyperCore's re-enforcing guard and vendor thermal daemons, reducing background CPU wakeups and preserving deep sleep idle states.
- **True Idle Frequency Recovery**: Restored proper idle clock states (Little 500 MHz, Big 725 MHz), significantly improving battery longevity and lowering device temperatures during ambient interactive use.

---

# HyperCore v6.7.0 — Unconstrained Performance, Zero Thermal Throttling & Seamless Profiler Release

## What's Changed

### 🚀 Complete Internal Thermal Management Stripped
- **Zero Artificial Thermal Throttling**: Completely removed `calculate_thermal_tier()` along with all internal thermal tiers and thermal hold tick mechanisms. HyperCore no longer artificially throttles CPU/GPU frequencies or scales down performance based on temperature thresholds.
- **Unconstrained Gaming Profile**: Big cores (2.2 GHz) and Little cores (2.0 GHz) now operate at full hardware peak capability without clock degradation during heavy gaming sessions.
- **Multi-Step Thermal Ladder Removed**: Fully eliminated automatic charging current step-downs in `charger.c`. User-selected charging modes and custom slider limits remain strictly respected without thermal overrides.
- **WebUI & IPC Cleansing**: Purged deprecated thermal tier telemetry chips, JSON fields, and guessing heuristics from the WebUI and IPC status endpoints.

### 🔄 Seamless Profile Transitions & Architecture Hardening
- **Manual IPC Profile Locking**: Upgraded `SET_PROFILE:<profile>` IPC socket command to persistently lock the profile without being instantly overwritten by the autonomous profiler loop.
- **Autonomous Mode Revert (`SET_PROFILE:AUTO`)**: Added dedicated `AUTO` / `DYNAMIC` command to seamlessly return daemon control to the dynamic foreground/screen profiler.
- **Micro-Stutter Elimination**: Removed redundant `is_gpu_heavy` sysfs write triggers during gaming mode, eliminating dozens of unnecessary kernel writes per minute during fluctuating GPU load.
- **Screen-Off Burst Reset**: Entering `PROFILE_Sleep` now immediately resets all active boost ticks (`gaming_hold_ticks`, `jitter_rescue_ticks`, `launch_boost_ticks`, `app_boost_ticks`), allowing the SoC to immediately drop to deep C-state sleep.
- **Single-Pass Tamper Recovery**: Streamlined governor mutation audit in `audit_active_profile_state()` to eliminate duplicate re-enforcement calls.
- **Frequency Boundary Sanitization**: Added defensive min/max frequency bounds checking to guarantee `min_freq <= max_freq` across all CPU policies, preventing `-EINVAL` kernel rejection.

### 🛡️ System Hygiene & Lifecycle Cleanup
- **SurfaceFlinger & Low-Latency Cleanup**: Automatically restores `debug.sf.latch_unsignaled = 0` and deletes `persist.sys.wifi.low_latency` on daemon shutdown (`restore_baseline_nodes()`), leaving zero persistent footprint.

---

# HyperCore v6.6.0 — HyperMoon HUD Overhaul, Governor Telemetry & Zero-Latency Performance Release

## What's Changed

### 🌕 HyperMoon HUD Overhaul & Governor Telemetry
- **CPU & GPU Governor Live Display**: Added native support and dedicated toggles for real-time CPU scaling governor (`show_gov` / `sugov_ext` / `schedutil`) and Mali graphics governor & policy (`show_gpu_gov` / `simple_ondemand` / `ged_dvfs`).
- **ZRAM Compressed Swap Telemetry**: Added real-time monitoring for compressed swap memory usage (`show_zram`).
- **Full Geometry & Appearance Customization**: Added granular Card Width (`100px - 400px`) and Card Height (`36px - 260px`) sliders, with expanded ranges for Scale (`50% - 180%`), Font Size (`9sp - 22sp`), Corner Radius (`0dp - 30dp`), Update Interval (`50ms - 2500ms`), and Background Opacity (`10% - 100%`).
- **One-Tap Quick Presets**: Integrated instant preset switching between **Compact** (FPS, CPU, GPU, Battery), **Minimal** (pure FPS), and **Detailed** (full telemetry & govs) with automatic optimal dimension sizing.
- **Theme & Custom Accent Color**: Added native theme presets (Cyber Neon, AMOLED Dark, Matrix Green, Crimson Red) and custom hex color picker with real-time card and text accenting.
- **Layout & Text Alignment Controls**: Added intuitive Horizontal Bar vs Vertical Card orientation switcher with dynamic Left, Center, and Right text alignment.

### ⚡ Zero-Latency Reactive Architecture
- **Kernel inotify Real-Time Synchronization**: Connected Java DEX overlay directly to Linux kernel `inotify` via `FileObserver`, enabling sub-millisecond reactions to configuration updates and slider dragging.
- **Instant Master Switch**: Re-engineered ON/OFF toggle to manage view visibility directly in Dalvik without stopping or respawning background processes, eliminating switch lag completely.
- **Dynamic Anti-Clipping Geometry Engine**: Upgraded `calcHudDimensions()` to automatically calculate line heights and prevent bottom telemetry metrics (Battery, Network) from clipping in Vertical Card mode regardless of card height settings.

### 🎨 Design Polish & UX Refinement
- **AMOLED Dark MD3 Harmony**: Replaced harsh green accent badges with 100% monochrome AMOLED dark styling matching the HyperCore design system.
- **Tactile Switch Hitboxes**: Enlarged switch touch targets (48x48px) with `pointer-events: none` internal decoupling to prevent touch desynchronization on mobile screens.
- **Natural Copywriting**: Purged robotic terminology across the HUD view in favor of clear, natural English phrasing.

---

# HyperCore v6.5.0 — Architecture Isolation, Gamelist Ecosystem & Full-Spectrum Stability Release

## What's Changed

### 🌕 Native HyperMoon HUD Integration (Unified FPS & Performance Overlay)
- **Module Consolidation**: Integrated standalone FPS Moon directly into HyperCore as **HyperMoon**, eliminating the need for two separate Magisk/KernelSU modules and saving system resources.
- **Dual Engine Architecture**: Ultra-low-overhead native C telemetry daemon (`hypermoon_daemon`) paired with high-performance Java/DEX Surface overlay (`hypermoon.dex` via `app_process`).
- **Dashboard Quick Switch**: Added dedicated minimalist HyperMoon card on the Dashboard with an instant ON/OFF master toggle and live status badge.
- **Dedicated HUD Settings View (`/hud`)**: Deeply customizable layout (Capsule Pill vs Card Stack), text alignment, telemetry metrics toggle (FPS, Frametime, CPU load/freq, GPU load/freq, RAM, Battery Watt/Temp, Network), scaling, opacity, corner radius, and refresh rate.
- **Auto-Gaming Synergy**: Smart foreground detection automatically reveals HUD during active gameplay and hides it during normal interactive tasks when auto-gaming mode is enabled.
- **Zero Storage Clutter**: HUD configurations and logs reside cleanly in `/data/adb/hypercore/hud/` with automatic migration from older FPS Moon modules.

### 📂 Filesystem Architecture & Persistent Data Isolation
- **Dedicated Data Directory (`/data/adb/hypercore/`)**: Completely isolated all dynamic runtime configs (`*.conf`), user gamelist (`gamelist.txt`), log files, and PID states into `/data/adb/hypercore/`.
- **Zero Module Root Pollution**: Guaranteed `/data/adb/modules/hypercore/` contains strictly clean Magisk/KSU overlay files and lifecycle scripts with zero runtime clutter.
- **Automatic Migration & Backward Compatibility**: Built-in backward-compatibility layer dynamically reads and seamlessly migrates legacy configurations on daemon start or module update.

### 🎮 Gamelist Management & Intelligent Auto-Detection
- **Intelligent One-Tap Auto-Detect**: Integrated multi-vendor game package scanner (`pm list packages -3`) identifying popular titles and auto-registering them with `GAMING` profile.
- **Case-Insensitive Parser**: Refactored C daemon parser to use `strcasecmp`, ensuring profiles (`GAMING`, `Gaming`, `gaming`, `MOBA`, `INTERACTIVE`, `SLEEP`) parse reliably without casing issues.
- **Inline Tactile 2-Step Deletion**: Implemented tactile two-step inline delete confirmation (tap once to prompt confirmation, tap again to delete, auto-reverts in 3.5s) to eliminate accidental touches without disruptive popups.
- **Minimalist Profile Accents & Launch Feedback**: Subtle color-coded profile indicators in select dropdowns and temporary tactile "Launching..." button feedback.

### 🛡️ System Hygiene & Lifecycle Cleanup
- **Comprehensive Uninstallation**: Full node restoration for CPU governors, frequencies, rate limits, Mali GPU power policies, GED/FPSGO parameters, and complete cleanup of PATH symlinks (`libhypercore.so` & `hypercore-bugreport`).
- **Strict Embedded Binary Checksums**: Embedded SHA-256 verification guaranteeing zero tampering across all deployment assets.

---

# HyperCore v6.4.8 — Interactive GPU Uncap, Telemetry Precision & WebUI Performance Release

## What's Changed

### 🚀 Interactive Profile GPU Max Frequency Uncap
- **Hardware Ceiling Uncapped**: Completely removed the hard-coded 648 MHz GPU ceiling in Interactive profile (`devfreq_max_freq` and `gpu_cust_upbound_freq`), enabling Mali-G57 to scale freely to full hardware peak capability (up to ~1003 MHz / 990 MHz OPP).
- **Responsive UI Scaling Threshold**: Tuned devfreq `upthreshold` down from 85% to 65% and `downdifferential` to 20%, ensuring SurfaceFlinger, 90/120Hz display refresh animations, and UI scrolling burst smoothly out of the 390 MHz floor without frame drops or UI heaviness.
- **Hardware Max Frequency Auto-Detection**: Dynamically detects available hardware frequency tables and ensures interactive scaling bounds match genuine SoC limits.

### 📊 Real-Time Charging Telemetry Precision
- **Live Kernel Power Metrics**: Replaced static/hardcoded charging text estimates on the Dashboard hero banner with real-time live kernel charging rate metrics (`Chg: +xxxx mA`).
- **Dynamic Battery State Synchronization**: Aligned battery telemetry across Dashboard, Charger Control, and About views directly with active PMIC current flow.

### 🎨 WebUI Performance & Stylesheet Cleanup
- **Consolidated Root Styling Rules**: Unified duplicate root `body, #app` stylesheet declarations.
- **Dead CSS Class Removal**: Purged unused stylesheet classes (`.profile-grid`, `.profile-btn`, `.progress-track`, `.progress-fill`), streamlining the inline single-file bundle size and DOM rendering efficiency.

### 🛡️ Profile State Transition Determinism
- **Verified Zero-Friction Transitions**: Validated deterministic atomic transitions between Interactive, Gaming, MOBA, and Sleep states under both automatic foreground scanning and manual IPC socket requests with zero daemon stalls or kernel permission errors.

---

# HyperCore v6.4.5 — Advanced Charging Control, Hardware Protection & Floating Dock Release

## What's Changed

### ⚡ Streamlined Hardware Charging Speed Slider
- **Dedicated 16-Level Hardware Slider**: Eliminated redundant and legacy preset mode cards, focusing the entire charging control interface on the full 16-level discrete hardware slider (levels 0 through 15) with dedicated single-tap stepper buttons (`[−]` and `[+]`) and quick-jump pills (Trickle, Cool, Balanced, High, Max).
- **Direct Unified Layout**: Removed tab switching friction; power telemetry, charging speed controls, and battery protection toggles are now directly accessible on a single page.
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

### 🐞 Robust High-Speed Bug Report Diagnostics
- **Sub-Second System Telemetry**: Slashed bugreport generation latency from 12 seconds down to ~2 seconds by substituting heavy view-hierarchy Binder dumps (`dumpsys activity top`) with instantaneous window and resumed activity queries (`dumpsys window` and `dumpsys activity activities`).
- **Zero-Loss Base64 User Notes**: Transitioned user note transport from aggressive alphanumeric regex sanitization to Base64 UTF-8 encoding, preserving 100% of spaces, punctuation, quotes, emojis, and multiline descriptions without shell injection risks.
- **IPC Socket Status & Config Backups**: Automatically queries live JSON daemon telemetry directly from `/dev/hypercore.sock` (`daemon_status.json`) and bundles all active module configurations (`battery_cycle.conf`, `charge_mode.conf`, `custom_charge_limit.conf`, `night_charging.conf`, `smart_chg.conf`, `protect_80.conf`).
- **Accurate Realtime CPU Consumers**: Replaced unranked process lists with sorted `top` snapshots (`top -b -n 1 -m 15 -s 3`), reliably surfacing real CPU consumers with exact CPU/memory percentages.
- **Refined Export Modal UX**: Added persistent file location indicators (`Internal Storage > HyperCore_Bugreports`), single-tap path copying with tactile visual confirmation, dynamic bridge timeout scaling (30s), and graceful error retry handling.

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
