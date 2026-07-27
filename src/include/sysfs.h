/*
 * HyperCore - Sysfs Utilities Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_SYSFS_H
#define HYPERCORE_SYSFS_H

#include "common.h"

void sysfs_write(const char *path, const char *val);
int sysfs_read_int(const char *path);

#endif /* HYPERCORE_SYSFS_H */
