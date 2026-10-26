#ifndef HYPERCORE_MEMORY_H
#define HYPERCORE_MEMORY_H

#include "common.h"
#include "sysfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Gaming-only adaptive pressure relief. Interactive and Sleep are
 * stock-restoring profiles, so this must not touch VM knobs there. */
void tune_memory_pressure(void);
int trigger_purge_ram_cache(void);

#ifdef __cplusplus
}
#endif

#endif
