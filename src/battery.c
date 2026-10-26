#include "battery.h"
#include "nodes.h"
#include "sysfs.h"
#include "log.h"

/* A gauge above this is not a phone battery cycle counter. */
#define BMS_CYCLE_MAX       3000
/* Only correct values that are clearly from another source: more than 5x the
 * gauge and at least 1000 cycles apart. Normal drift (a few cycles, or a
 * gauge that is momentarily one behind) is left alone. */
#define SYNC_RATIO          5
#define SYNC_MIN_DELTA      1000
/* One attempt at startup, one more after boot settle. If the driver reverts
 * the write twice, stop rather than hammering a battery EEPROM. */
#define SYNC_MAX_ATTEMPTS   2

static int s_attempts;

void battery_sync_cycle(void) {
    if (s_attempts >= SYNC_MAX_ATTEMPTS) return;

    int bms = node_read_int(N_BMS_CYCLE);
    int batt = node_read_int(N_BATT_CYCLE);

    if (bms <= 0 || bms > BMS_CYCLE_MAX || batt <= 0) return;
    if (batt <= bms) return;
    if (batt <= bms * SYNC_RATIO) return;
    if (batt - bms < SYNC_MIN_DELTA) return;

    s_attempts++;
    log_warn("Battery", "Cycle count mismatch (battery=%d, bms=%d) - syncing battery node to BMS",
             batt, bms);

    char val[16];
    snprintf(val, sizeof(val), "%d", bms);
    sysfs_write(N_BATT_CYCLE[0], val);

    int after = node_read_int(N_BATT_CYCLE);
    if (after == bms) {
        log_info("Battery", "Battery cycle count synced to %d (was %d)", bms, batt);
    } else {
        log_warn("Battery", "Cycle sync did not stick (battery=%d, bms=%d); driver may reject writes",
                 after, bms);
    }
}
