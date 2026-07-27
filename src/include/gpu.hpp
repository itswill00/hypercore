/*
 * HyperCore - GPU DVFS & FPSGO Tuning Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_GPU_HPP
#define HYPERCORE_GPU_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void apply_gpu_tuning(void);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_GPU_HPP */
