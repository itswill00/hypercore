# Tanzanite HyperFlow

Kernel and system performance tuner tailored for the MediaTek Helio G99 Ultra (MT6789) platform running HyperOS, MIUI, or AOSP.

Developed by **@itswill00**.

---

## Overview

Tanzanite HyperFlow replaces static kernel parameters with a lightweight native daemon (`hyperflow_engine`) written in C. The daemon dynamically adjusts CPU governor rate limits, GPU bottom frequencies, storage queue schedulers, and ZRAM parameters based on system load, thermal state, and active foreground applications.

## Features

- **Native C Engine:** Compiled AArch64 ELF binary (`hyperflow_engine`) operating with zero shell parsing overhead.
- **Dynamic Profile Governor:** Transitions between `Sleep`, `Interactive`, `Touch Boost`, `Gaming`, and `Thermal Protection` states.
- **Storage Queue Tuning:** Configures UFS block queues to `none` scheduler to eliminate software queuing latency.
- **Memory Optimization:** Sets `vm.page-cluster = 0` for direct ZRAM swap I/O and manages proactive compaction.
- **Network Tuning:** Configures `cubic` / `bbr` TCP congestion control and enables Wi-Fi low-latency mode.
- **SurfaceFlinger Alignment:** Applies phase offset tuning to reduce touch rendering latency without forcing panel refresh rates.
- **KernelSU WebUI & CLI:** Includes a native web interface for KernelSU and a terminal management utility (`tanzanite`).

## Installation

1. Open **KernelSU Manager**, **APatch**, or **Magisk**.
2. Flash `Tanzanite-HyperFlow-v2.5.zip`.
3. Reboot your device.

## Command Line Interface

You can control and monitor the module using the `tanzanite` CLI utility in a root terminal:

```bash
tanzanite          # Display live CPU, GPU, temperature, and profile status
tanzanite -g       # Lock system to Gaming mode
tanzanite -u       # Restore automatic dynamic profile governor
tanzanite -f       # Flush RAM page cache and compact memory
tanzanite -s       # Display active kernel and network tunables
tanzanite -l       # Show recent engine log output
```

## Structure

```
Tanzanite_HyperFlow_Release/
├── Tanzanite-HyperFlow-v2.5.zip
├── README.md
└── LICENSE
```

## License

This project is licensed under the [MIT License](LICENSE).
