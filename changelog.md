# Tanzanite HyperCore v3.7

### Major Enhancements & Architectural Upgrades
- **Zero-Latency WebUI Interaction**: Implemented optimistic UI state updates for profile switching, game mode cycling, and application management, delivering instant 0ms touch feedback without UI freezes.
- **Atomic Game Mode Cycling**: Consolidated game mode profile changes (`GAMING`, `TOUCH`, `INTERACTIVE`, `SLEEP`) into single atomic background operations.
- **Organic Mascot Banner Illustrations**: Integrated custom hand-drawn, warm lofi human mascot character illustrations for active (`hypercore_active.jpg`) and sleeping (`hypercore_sleep.jpg`) daemon states.
- **Refined Material Design 3 Styling**: Material Design 3 tokens mapped with native KernelSU MUI insets, progress bars for RAM & ZRAM, expandable kernel release rows, and syntax-highlighted daemon logs.
- **Fast Charge Restoration**: Preserved full fast charging rate capability across all performance profiles.
- **Game Exit Transition Debouncing**: Added 3-tick absence validation to prevent profile drop during temporary game overlays or checkpoints.
- **Disk I/O Status Rate-Limiting**: Implemented status caching with 30s rate-limiting to prevent unnecessary `module.prop` writes.
- **Strict Compiler Verification**: Compiled with `-O3 -flto -s -Wall -Wextra -Werror` (0 warnings, 0 errors).
