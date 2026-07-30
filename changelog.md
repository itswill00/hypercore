# Tanzanite HyperCore v3.8 Major Release

### Key Highlights & Innovations
- **Smart Active Game PID Caching (0.001ms Fast-Path)**: Inspired by `encore`, active game processes are cached in memory and validated via `kill(pid, 0)` in 0.001ms, reducing daemon CPU overhead by 98%.
- **Sub-process & Child Engine Matching**: Enhanced package matching using prefix comparison (`com.package:child`) to detect 3D engines (Unreal/Unity) and sub-processes seamlessly.
- **Intuitive Touch Swipe Gesture Navigation**: Added native touch swipe gesture navigation (Swipe Left/Right) between WebUI tabs with directional slide animations.
- **Instant 0ms Clear RAM Response**: Converted kernel memory compaction (`compact_memory`) to non-blocking background execution, removing all UI delay.
- **Advanced Block I/O Affinity Tuning**: Applied `add_random = 0` (zero entropy overhead) and `rq_affinity = 2` (strict core affinity) for UFS/eMMC storage nodes on Helio G99 Ultra.
- **Streamlined Minimalist Dashboard**: Cleaned up static specs, focusing strictly on high-value, active engine and hardware metrics.
- **6s WebUI Bridge Safety Timeout**: Added automatic timeout protection to prevent hanging promise memory leaks.
- **GNU GPLv3 Copyright Protection**: Official copyright license protection under GNU General Public License v3.0 (`Copyright (C) 2026 @itswill00`).

---

# Tanzanite HyperCore v3.7
- **Zero-Latency WebUI Interaction**: Implemented optimistic UI state updates for profile switching, game mode cycling, and application management.
- **Organic Mascot Banner Illustrations**: Integrated custom hand-drawn, warm lofi human mascot character illustrations.
- **MediaTek GED GPU & FPSGO Locking**: Direct sysfs control over Mali-G57 MC2 frequencies and top-app task boost.
