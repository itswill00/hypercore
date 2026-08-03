
#ifndef HYPERCORE_LOG_HPP
#define HYPERCORE_LOG_HPP

#include "common.hpp"

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_STATE,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

#ifdef __cplusplus
extern "C" {
#endif

void log_write(log_level_t level, const char *tag, const char *fmt, ...);
void rotate_log(void);

#define log_info(tag, fmt, ...)  log_write(LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define log_state(tag, fmt, ...) log_write(LOG_LEVEL_STATE, tag, fmt, ##__VA_ARGS__)
#define log_warn(tag, fmt, ...)  log_write(LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define log_error(tag, fmt, ...) log_write(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif 
