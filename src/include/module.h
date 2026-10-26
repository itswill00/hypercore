#ifndef HYPERCORE_MODULE_H
#define HYPERCORE_MODULE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Rewrite the module.prop description so the root manager UI shows the live
 * profile. Throttled to one write per distinct status per 30 s. */
void update_module_prop_status(const char *status);

#ifdef __cplusplus
}
#endif

#endif
