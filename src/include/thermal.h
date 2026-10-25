
#ifndef HYPERCORE_THERMAL_H
#define HYPERCORE_THERMAL_H

#include "common.h"

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

/* Frequency ceilings (kHz) for the given thermal tier and profile.
 *
 * Single source of truth: src/cpu.c applies these and the documentation is
 * generated from the same numbers. Tier 1 and Tier 2 previously carried
 * identical ceilings, which silently collapsed a three-tier guard into two.
 * A ceiling is always clamped down to the hardware's own reported maximum by
 * the caller — these are upper bounds, never targets. */
void thermal_ceiling_mhz(int tier, profile_t prof, int *lit_khz, int *big_khz);

/* Normalize raw milli-degree thermal readings to degrees Celsius.
 * Deduplicates the same conversion that main.c and ipc.c both performed. */
static inline void normalize_thermal_temps(int *cpu_temp, int *bat_temp) {
    if (cpu_temp && *cpu_temp > 1000) *cpu_temp /= 1000;
    if (bat_temp) { if (*bat_temp > 1000) *bat_temp /= 1000; else if (*bat_temp > 100) *bat_temp /= 10; }
}

#ifdef __cplusplus
}
#endif

#endif 
