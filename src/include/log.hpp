/*
 * HyperCore - Logging Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_LOG_HPP
#define HYPERCORE_LOG_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void log_write(const char *fmt, ...);
void rotate_log(void);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_LOG_HPP */
