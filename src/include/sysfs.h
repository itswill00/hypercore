#ifndef HYPERCORE_SYSFS_H
#define HYPERCORE_SYSFS_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Raw kernel-node I/O primitives. Profile decisions, baseline capture and
 * path resolution live elsewhere (cpu.c, baseline.c, nodes.c). */

int  sysfs_read_str(const char *path, char *out_buf, size_t max_len);
int  sysfs_read_str_fallback(const char *const paths[], char *out_buf, size_t max_len);
int  sysfs_read_int(const char *path);

void sysfs_write(const char *path, const char *val);
void sysfs_write_fallback(const char *const paths[], const char *val);
/* chmod-guarded write that restores the node's previous mode afterwards. */
void sysfs_write_temp_mode(const char *path, const char *val, mode_t temp_mode);

#ifdef __cplusplus
}
#endif

#endif
