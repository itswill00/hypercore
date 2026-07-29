/*
 * HyperCore - Sysfs Utilities Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_SYSFS_HPP
#define HYPERCORE_SYSFS_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void sysfs_write(const char *path, const char *val);
int sysfs_read_int(const char *path);
void update_module_prop_status(const char *status);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_SYSFS_HPP */
