
#ifndef HYPERCORE_IO_H
#define HYPERCORE_IO_H

#include "common.h"
#include "sysfs.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_read_ahead(const char *kb_val);
void set_io_nr_requests(const char *nr_str);
void apply_irq_tuning(profile_t prof);

#ifdef __cplusplus
}
#endif

#endif 
