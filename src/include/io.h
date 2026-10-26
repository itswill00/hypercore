#ifndef HYPERCORE_IO_H
#define HYPERCORE_IO_H

#include "common.h"
#include "sysfs.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_read_ahead(const char *kb_val);
void set_io_nr_requests(const char *nr_str);

/* Resolve the GPU/display/touch IRQ numbers once from /proc/interrupts. The
 * old implementation walked every /proc/irq/N directory on every profile
 * transition looking for a device name that is not stored there, so it did
 * nothing except burn a few hundred syscalls per game launch. */
void init_irq_targets(void);
void apply_irq_tuning(profile_t prof);

#ifdef __cplusplus
}
#endif

#endif
