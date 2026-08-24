
#ifndef HYPERCORE_MEMORY_HPP
#define HYPERCORE_MEMORY_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void apply_memory_tuning(void);
void set_memory_sleep_mode(int enable);
void tune_memory_pressure(void);
int trigger_purge_ram_cache(void);

#ifdef __cplusplus
}
#endif

#endif 
