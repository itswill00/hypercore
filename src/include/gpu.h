#ifndef HYPERCORE_GPU_H
#define HYPERCORE_GPU_H

#include "common.h"
#include "sysfs.h"

#ifdef __cplusplus
extern "C" {
#endif

void detect_max_gpu_freq(void);
const char *get_max_gpu_freq_hz(void);
const char *get_max_gpu_freq_khz(void);
void set_mali_upthreshold(const char *val);
void set_mali_downdifferential(const char *val);

#ifdef __cplusplus
}
#endif

#endif
