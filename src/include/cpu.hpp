
#ifndef HYPERCORE_CPU_HPP
#define HYPERCORE_CPU_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void detect_cpu_hardware_limits(void);
void set_rate_limits(const char *lit_up, const char *lit_down,
                     const char *big_up, const char *big_down);
void set_cpu_governor(const char *lit_gov, const char *big_gov);
void set_cpu_freqs(int min_lit, int max_lit, int min_big, int max_big,
                   const char *lit_up, const char *lit_down,
                   const char *big_up, const char *big_down);
void apply_profile(profile_t prof, int gpu_load);
void reset_to_interactive_baseline(void);

#ifdef __cplusplus
}
#endif

#endif 
