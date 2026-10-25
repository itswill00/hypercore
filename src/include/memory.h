
#ifndef HYPERCORE_MEMORY_H
#define HYPERCORE_MEMORY_H

#include "common.h"
#include "sysfs.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_memory_sleep_mode(int enable);
void tune_memory_pressure(void);
int trigger_purge_ram_cache(void);

#ifdef __cplusplus
}
#endif

#endif 
