
#ifndef HYPERCORE_GPU_HPP
#define HYPERCORE_GPU_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void apply_gpu_tuning(void);
void enforce_interactive_gpu_polling(int target_poll_ms);
int read_mali_power_policy(char *out_buf, size_t max_len);
int read_mali_governor(char *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif 
