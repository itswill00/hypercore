/*
 * HyperCore - CPU Governor & Scaling Tuning Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_CPU_HPP
#define HYPERCORE_CPU_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void set_rate_limits(int cpu, const char *up, const char *down);
void apply_cpuset(void);
void set_cpu_freqs(int min_lit, int max_lit, int min_big, int max_big, const char *up_rate, const char *down_rate);
void apply_profile(profile_t prof, int tier);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_CPU_HPP */
