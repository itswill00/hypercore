# Tanzanite HyperCore v3.9 Major Release

### Key Highlights & Innovations
- **Smart Automated Adaptive Engine**: Completely automated background state evaluation machine (inspired by `encore` @Rem01Gaming) that seamlessly adjusts operational profiles without requiring manual user toggles.
- **MediaTek DVFSRC Interconnect Bandwidth Boost**: Unlocks LPDDR RAM throughput up to 4.266 GHz during active gaming sessions (`dvfsrc_qos_mode`), eliminating memory bandwidth bottlenecks.
- **MediaTek DRM Screen Off Lockscreen Detection**: Integrated MediaTek DRM backlight status monitoring (`/sys/class/leds/lcd-backlight/brightness`), ensuring 100% reliable deep sleep mode (`PROFILE_SLEEP`) upon screen lock.
- **ARM Mali-G57 MC2 Power Policy Management**: Dynamically switches GPU power policy between `coarse_demand` (deep power gating for daily use) and `always_on` (zero-latency frame rendering for gaming).
- **Enterprise Millisecond Precision Structured Logging**: Standardized format `[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [TAG] Message` with zero-heap-churn 250-line static ring buffer log rotation.
- **Clean Pure Package Game Management**: Streamlined `gamelist.txt` to store pure package names line-by-line, eliminating mode tag clutter and duplicate line parsing bugs.
- **Streamlined Material Design 3 WebUI**: Redesigned WebUI Dashboard featuring a clean status banner, smart adaptive engine indicator, game manager, structured log console, and quick actions.

---

# Tanzanite HyperCore v3.8
- **Smart Active Game PID Caching (0.001ms Fast-Path)**: Inspired by `encore`, active game processes are cached in memory and validated via `kill(pid, 0)` in 0.001ms, reducing daemon CPU overhead by 98%.
- **Sub-process & Child Engine Matching**: Enhanced package matching using prefix comparison (`com.package:child`) to detect 3D engines (Unreal/Unity) and sub-processes seamlessly.
- **Intuitive Touch Swipe Gesture Navigation**: Added native touch swipe gesture navigation (Swipe Left/Right) between WebUI tabs with directional slide animations.
- **Instant 0ms Clear RAM Response**: Converted kernel memory compaction (`compact_memory`) to non-blocking background execution, removing all UI delay.

---

# Tanzanite HyperCore v3.7
- **Zero-Latency WebUI Interaction**: Implemented optimistic UI state updates for profile switching, game mode cycling, and application management.
- **Organic Mascot Banner Illustrations**: Integrated custom hand-drawn, warm lofi human mascot character illustrations.
- **MediaTek GED GPU & FPSGO Locking**: Direct sysfs control over Mali-G57 MC2 frequencies and top-app task boost.
