#ifndef HYPERCORE_BATTERY_H
#define HYPERCORE_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Some MT6789 ROMs expose the battery authentication chip's cycle counter as
 * battery/cycle_count. That register can be unscaled or stale (for example
 * 5707 while the MTK gauge reports 180), and Android's healthd trusts it.
 * Sync the Android-facing counter to the gauge only when the mismatch is
 * implausible; never touch the raw auth/fg1 registers. */
void battery_sync_cycle(void);

#ifdef __cplusplus
}
#endif

#endif
