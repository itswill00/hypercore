# Tanzanite HyperCore v3.2

### Major Enhancements & Bug Fixes
- **Profile Scaling Logic Refinement**: Eliminated false `PROFILE_GAMING` triggers during non-gaming usage (video playback, UI animations). Non-game GPU load spikes are now gracefully handled by `PROFILE_TOUCH`.
- **Exact Package Matching**: Enhanced `gamelist.c` foreground detector to use strict exact package matching (`strcmp`), eliminating false positive service triggers.
- **Expanded Game Detector (`gamelist.txt`)**: Preloaded 40+ popular global, regional (SEA, Vietnam, Taiwan), and anime game package titles. Memory capacity expanded to 256 games (`MAX_GAMES=256`).
- **WebUI Redesign & Overhaul**: Developer theme in Sentence Case, interactive multi-profile selector (`Auto`, `Gaming`, `Balanced`, `Touch`, `Saver`), and live `gamelist.txt` manager.
- **HTML & CSS Webroot Bug Fixes**: Fixed `removeGamePkg` button layout (`flex: none;`), DOM `data-pkg` attribute escaping, and log parser active profile name extraction.
- **Internal Storage Auto-Sync**: Compiled zip packages are automatically saved to `/sdcard/HyperCore_Releases/Tanzanite-HyperCore-v3.2.zip`.
- **1-Click Online Module Updates**: Added `updateJson` support for KernelSU, APatch, and Magisk App Manager.
- **Strict Compiler Verification**: Compiled with `-O3 -flto -s -Wall -Wextra -Werror` (0 warnings, 0 errors).
