
#ifndef HYPERCORE_CHARGER_HPP
#define HYPERCORE_CHARGER_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * init_charge_control()
 *   Probe charger sysfs nodes, load persisted mode from charge_mode.conf,
 *   and apply it. Call once during daemon init after g_nodes is populated.
 */
void init_charge_control(void);

/*
 * enforce_charge_mode()
 *   Read current sysfs values; re-write only if they differ from target.
 *   Also runs safety override logic (bat temp / capacity guards).
 *   Call from main loop every 2 ticks (~2-4 seconds).
 */
void enforce_charge_mode(void);

/*
 * set_charge_mode(mode)
 *   Set user-requested charge mode (CHARGE_MODE_*), persist to disk, and
 *   immediately apply. Callable from IPC handler (single-threaded daemon).
 */
void set_charge_mode(int mode);

/*
 * get_charge_mode()
 *   Return the current effective mode (may differ from user selection if
 *   thermal override is active).
 */
int get_charge_mode(void);

/*
 * charge_mode_name(mode)
 *   Return a human-readable label for a CHARGE_MODE_* value.
 */
const char *charge_mode_name(int mode);

#ifdef __cplusplus
}
#endif

#endif
