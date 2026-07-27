/*
 * HyperCore - Thermal Zone Scanner & Hysteresis Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_THERMAL_HPP
#define HYPERCORE_THERMAL_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void scan_thermal_zones(void);
int check_charging_status(void);
int calculate_thermal_tier(int cpu_temp, int bat_temp);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_THERMAL_HPP */
