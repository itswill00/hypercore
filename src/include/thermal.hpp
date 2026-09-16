
#ifndef HYPERCORE_THERMAL_HPP
#define HYPERCORE_THERMAL_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void scan_thermal_zones(void);
int check_charging_status(void);
int get_true_battery_cycles(void);
void sync_battery_cycle_count(void);
int get_thermal_tier(void);
int update_thermal_guard(int cpu_temp, int bat_temp);
void enforce_gaming_thermal_bypass(profile_t prof, int tier);
/* ponytail: normalize raw mili-degree readings to degrees C — deduplicates 6 copy-paste blocks in main/ipc */
static inline void normalize_thermal_temps(int *cpu_temp, int *bat_temp) {
    if (cpu_temp && *cpu_temp > 1000) *cpu_temp /= 1000;
    if (bat_temp) { if (*bat_temp > 1000) *bat_temp /= 1000; else if (*bat_temp > 100) *bat_temp /= 10; }
}

#ifdef __cplusplus
}
#endif

#endif 
