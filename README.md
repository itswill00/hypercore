# Tanzanite HyperCore v3.0

Lightweight C-native kernel tuner and dynamic performance optimizer tailored for **MediaTek Helio G99 Ultra** devices running **Linux Kernel 5.10.x**.

## System Requirements

- **Processor**: MediaTek Helio G99 Ultra (MT6789/MT6877 architecture)
- **Kernel Version**: **Linux 5.10.x** *(Installation will automatically abort on Kernel 6.12 or other incompatible versions)*
- **Root Solution**: Magisk / KernelSU / APatch

## Features

- **HyperCore C-Native Daemon**: Zero-overhead compiled C daemon running with minimal footprint (< 0.1% CPU, ~3 MB RAM).
- **Thermal Hysteresis Protection**: Prevents thermal profile thrashing and rapid governor state flapping.
- **Dynamic Profile Switching**:
  - `Sleep`: Low power scaling when screen is turned off.
  - `Interactive`: Balanced daily performance and power usage.
  - `Touch`: Instant latency boost during dynamic UI touch events.
  - `Gaming`: Peak performance mode for high GPU/CPU workloads.
  - `Thermal`: Gradual throttling hold for thermal dissipation.
- **System Tuning**: UFS/MMC IO queue tuning (`none` scheduler, 128KB read-ahead), TCP low latency tunables, ZRAM/VM optimization.
- **KernelSU / APatch WebUI**: Integrated status dashboard for real-time monitoring and control.

## Installation

1. Build or download `Tanzanite-HyperCore-v3.0.zip`.
2. Flash via Magisk, KernelSU, or APatch manager.
3. Reboot device.

## Control

Manual profile locks can be placed at `/data/adb/modules/tanzanite_hypercore/.hypercore_lock`:
- Write `GAMING` to lock Gaming profile.
- Remove lock file to restore auto governor scaling.
