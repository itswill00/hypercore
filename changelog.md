# Tanzanite HyperCore v3.1

### Key Changes & Optimizations
- **PUBG & High-FPS Gaming Stutter Fix**: Increased governor down-rate limit to 30ms, locked GPU floor to 800MHz, and set CPU minimum frequencies to 1.4GHz (Little) / 1.8GHz (Big).
- **ZRAM Latency Spike Elimination**: Optimized `vm.swappiness=15` and `vm.vfs_cache_pressure=100` to prevent memory compression stutters during heavy gameplay.
- **Dynamic Game Detector (cgroup top-app)**: Zero-overhead foreground game detection scanning `/dev/cpuset/top-app/tasks` with configurable `gamelist.txt` (40+ preloaded titles).
- **Game Launch Burst**: Dynamic UFS read-ahead burst (512 KB for 6s) on game launch.
- **Hardware IRQ Affinity Pinning**: Bound Mali GPU and MTK Display IRQs to CPU 6-7 Big cores.
- **WebUI Redesign & Overhaul**: Developer theme in Sentence Case, interactive multi-profile selector (`Auto`, `Gaming`, `Balanced`, `Touch`, `Saver`), and live `gamelist.txt` manager.
- **1-Click Online Module Updates**: Added `updateJson` support for KernelSU, APatch, and Magisk App Manager.
