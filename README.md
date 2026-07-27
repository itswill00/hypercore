# Tanzanite HyperCore

Tanzanite HyperCore is a C-native background daemon and kernel tuner for MediaTek Helio G99 Ultra devices running Linux kernel 5.10.x.

It dynamically adjusts CPU governor rate limits, GPU frequency floors, storage queue parameters, and virtual memory tunables based on active load and thermal conditions.

## Requirements

- **Device**: MediaTek Helio G99 Ultra (MT6789 / MT6877)
- **Kernel**: Linux 5.10.x (*Installation aborts on 6.12+*)
- **Environment**: Rooted via KernelSU, APatch, or Magisk

## Architecture

The codebase is split into modular C components under `src/`:

```
src/
├── include/
│   ├── common.hpp     # Types, structs, and constants
│   ├── sysfs.hpp      # Sysfs read/write helpers
│   ├── log.hpp        # Logging and file rotation
│   ├── cpu.hpp        # Governor rate limits & scaling
│   ├── gpu.hpp        # GED & MediaTek FPSGO tuning
│   ├── memory.hpp     # VM & ZRAM tunables
│   ├── io.hpp         # Storage queue & scheduler config
│   └── thermal.hpp    # Thermal zone scanning & hysteresis
├── cpu.c
├── gpu.c
├── io.c
├── log.c
├── main.c
├── memory.c
├── sysfs.c
└── thermal.c
```

## Profiles & Thermal Logic

HyperCore evaluates system state every 2 seconds:

| Profile | Conditions | Core Clock Targets |
| :--- | :--- | :--- |
| `Sleep` | Screen off | Minimized Little / Big freqs, relaxed governor rates |
| `Interactive` | Daily usage | 900 MHz Little / 1100 MHz Big, zero ramp delay |
| `Touch` | Dynamic load spikes | 1200 MHz Little / 1500 MHz Big boost |
| `Gaming` | GPU load ≥ 35% | 1200 MHz Little / 1600 MHz Big, 800 MHz GPU floor |
| `Thermal` | CPU ≥ 58°C or Bat ≥ 47°C | Throttling hold with 4-tick hysteresis recovery |

## Building

To compile the native binary and package the release ZIP:

```bash
./build.sh
```

The output ZIP will be placed in `Tanzanite_HyperCore_Release/Tanzanite-HyperCore-v3.0.zip`.

## Manual Overrides

To lock the system into Gaming mode manually, create a lock file:

```bash
echo "GAMING" > /data/adb/modules/tanzanite_hypercore/.hypercore_lock
```

Remove the file to return to automatic governor scaling:

```bash
rm -f /data/adb/modules/tanzanite_hypercore/.hypercore_lock
```

## License

GPL-3.0
