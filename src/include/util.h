#ifndef HYPERCORE_UTIL_H
#define HYPERCORE_UTIL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Double-fork + setsid helper. The middle child is reaped immediately, so the
 * caller never blocks on the command and the grandchild is adopted by init.
 * This is the one implementation of the pattern that used to be copy-pasted
 * in main.c, cpu.c and sysfs.c. */
void async_system_cmd(const char *cmd);
void async_resetprop(const char *key, const char *val, int is_delete);

#ifdef __cplusplus
}
#endif

#endif
