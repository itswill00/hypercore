
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

#ifdef __cplusplus
}
#endif

#endif 
