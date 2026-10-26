#ifndef HYPERCORE_CPU_H
#define HYPERCORE_CPU_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

void detect_cpu_hardware_limits(void);

/* Apply a complete profile: CPU, cgroups, GPU devfreq, GED/FPSGO, touch and
 * VM policy. Idempotent and safe to call on every transition. */
void apply_profile(profile_t prof);

/* Steady-state read-ahead for a profile. The launch/app boost window owns the
 * transient 512 KB value; this is what the boost code re-applies when the
 * window closes, so two subsystems no longer disagree about the node. */
const char *profile_read_ahead(profile_t prof);

#ifdef __cplusplus
}
#endif

#endif
