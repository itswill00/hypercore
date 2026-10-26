#ifndef HYPERCORE_BASELINE_H
#define HYPERCORE_BASELINE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The stock baseline is captured once and then used for two things:
 *   - the Interactive profile (factory stock + touch smoothing), and
 *   - baseline_restore() on shutdown/uninstall.
 *
 * Capture, serialization, deserialization and restore are all driven by one
 * table (baseline.c), so adding a node is a single-line edit. The previous
 * layout kept four parallel 55-entry lists in sync by hand. */
void baseline_init(void);
void baseline_save(void);
void baseline_restore(void);

#ifdef __cplusplus
}
#endif

#endif
