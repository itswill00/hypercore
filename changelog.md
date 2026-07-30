# Tanzanite HyperCore v3.4

### Major Enhancements & Architectural Upgrades
- **Dynamic CPU & GPU Governor Scaling**: Profiling engine now dynamically toggles CPU governors per profile (`performance` for Gaming, `powersave` for Saver/Thermal, `sugov_ext` for Interactive/Touch) alongside MediaTek GED GPU floor & cap tuning.
- **Modern Vue 3 + Vite + Pinia WebUI**: Complete WebUI rewrite into a modular Vue 3 Single Page Application using Pinia state management, Vue Router, and `vite-plugin-singlefile` compilation.
- **Enriched Real-Time System Dashboard**: Expanded Dashboard with 4 distinct metric sections — System Status, Memory & Storage (RAM/ZRAM live % used), Power & Thermal (charge rate mA / voltage V / temps), and Device Info (Kernel, Chipset, Android SDK, SELinux, Uptime).
- **Streamlined Mobile Interface**: Clean sentence-case typography, compact 2-column action buttons ("Clear RAM", "Restart daemon", "Save log", "Create shortcut"), and focused tab navigation (Dashboard, Games, Logs).
- **Game Hold & Transition Safety**: Fixed game exit hold-down timer tracking so sustained performance frequency profiles persist cleanly during game state transitions.
- **Log Heap Retention & Protection**: Dynamic heap allocation for log line retention avoiding stack buffer overhead and preventing log file inflation beyond 200 lines.
- **Strict Compiler Verification**: Compiled with `-O3 -flto -s -Wall -Wextra -Werror` (0 warnings, 0 errors).
