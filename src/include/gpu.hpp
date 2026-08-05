
#ifndef HYPERCORE_GPU_HPP
#define HYPERCORE_GPU_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void apply_gpu_tuning(void);
void enforce_interactive_gpu_polling(int target_poll_ms);

#ifdef __cplusplus
}
#endif

#endif 
