# Tanzanite HyperCore v3.3

### Major Enhancements & Architectural Upgrades
- **KernelSU Live Status Integration**: Real-time daemon status dynamically rendered in KernelSU / APatch / Magisk Manager description line (`[Active: Interactive]`, `[Active: Gaming (PUBG)]`, `[Active: Touch]`).
- **Linux inotify Event Watcher**: Implemented non-blocking kernel `inotify(2)` watcher on `gamelist.txt` and `.hypercore_lock` for 0ms instant configuration reloads.
- **KernelSU WebUI App Picker Modal**: Interactive installed app selector in WebUI featuring real Android application icons (`ksu://icon/`) and human-readable app labels.
- **Direct Game Launcher (`[Launch]`)**: One-click game launcher buttons added directly inside the WebUI game list.
- **Battery Charge Thermal Guard**: Dynamically limits charging current to 2.0A (`charge_control_limit`) during plugged-in gaming sessions to prevent 45°C+ battery heat spikes.
- **Android Toast Notifications**: System notification alerts automatically posted upon game launch (`Gaming Profile Activated [com.tencent.ig]`).
- **Relaxed Thermal Headroom**: Expanded CPU thermal thresholds (Tier 1 @ 58°C, Tier 2 @ 65°C, Tier 3 @ 75°C) to prevent premature thermal throttling.
- **Smooth Scrolling & App Switching**: Boosted interactive CPU governor down-rate limits (20ms–25ms) and UFS read-ahead queues (256 KB baseline, 512 KB launch burst).
- **Strict Compiler Verification**: Compiled with `-O3 -flto -s -Wall -Wextra -Werror` (0 warnings, 0 errors).
