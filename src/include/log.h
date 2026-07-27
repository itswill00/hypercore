/*
 * HyperCore - Logging Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_LOG_H
#define HYPERCORE_LOG_H

#include "common.h"

void log_write(const char *fmt, ...);
void rotate_log(void);

#endif /* HYPERCORE_LOG_H */
