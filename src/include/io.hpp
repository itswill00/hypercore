/*
 * HyperCore - Storage I/O Queue Tuning Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_IO_HPP
#define HYPERCORE_IO_HPP

#include "common.hpp"
#include "sysfs.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void apply_io_tuning(void);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_IO_HPP */
