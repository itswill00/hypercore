
#include "charger.hpp"
#include "sysfs.hpp"
#include "log.hpp"

/* -------------------------------------------------------------------------
 * Charger sysfs nodes (HuaQin hq_chg_manager on MT6789 / Xiaomi HyperOS)
 * -------------------------------------------------------------------------
 * charge_control_limit : 0=Unrestricted, 5=Balanced, 10=Safe, 16=Emergency cutoff
 * input_suspend        : 0=normal charging, 1=cell disconnected (bypass/suspend)
 * sconfig              : Xiaomi thermal_message profile signal
 *                        Writing 10 hints mi_thermald to back off from overriding
 *                        charge_control_limit automatically.
 * ------------------------------------------------------------------------- */
#define CHG_LIMIT_NODE  "/sys/class/power_supply/battery/charge_control_limit"
#define CHG_SUSPEND_NODE "/sys/class/power_supply/battery/input_suspend"
#define CHG_SCONFIG_NODE "/sys/class/thermal/thermal_message/sconfig"
#define CHG_CURRENT_NODE "/sys/class/power_supply/battery/current_now"
#define CHG_CAPACITY_NODE "/sys/class/power_supply/battery/capacity"

/* Level values written to charge_control_limit per mode */
#define LIMIT_FAST     0
#define LIMIT_BALANCED 5
#define LIMIT_SAFE     10

/* Safety thresholds */
#define TEMP_OVERRIDE_ENTER  45   /* >= 45°C: drop to SAFE                    */
#define TEMP_EMERGENCY       50   /* >= 50°C: force BYPASS (emergency cutoff) */
#define TEMP_OVERRIDE_CLEAR  40   /* <  40°C: restore user mode               */
#define CAP_MIN_BYPASS        10  /* < 10%:  auto-resume from BYPASS to SAFE  */
#define CAP_FAST_LIMIT        80  /* >= 80%:  drop FAST to BALANCED           */

/* Module-level state */
static int s_nodes_available  = 0;  /* 1 if charger nodes exist on this device */
static int s_user_charge_mode = CHARGE_MODE_OEM;  /* user's persistent choice   */

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static void save_charge_mode_conf(int mode) {
    char path[300];
    snprintf(path, sizeof(path), "%s/charge_mode.conf", g_nodes.mod_dir);
    char tmp[310];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (f) {
        fprintf(f, "%d\n", mode);
        fclose(f);
        rename(tmp, path);
    }
}

static int load_charge_mode_conf(void) {
    char path[300];
    snprintf(path, sizeof(path), "%s/charge_mode.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "r");
    if (!f) return CHARGE_MODE_OEM;
    int mode = CHARGE_MODE_OEM;
    if (fscanf(f, "%d", &mode) != 1) mode = CHARGE_MODE_OEM;
    fclose(f);
    if (mode < CHARGE_MODE_OEM || mode > CHARGE_MODE_BYPASS) mode = CHARGE_MODE_OEM;
    return mode;
}

/* Write charge_control_limit only if it differs from current value.
 * Read-before-write prevents unnecessary sysfs churn and mi_thermald
 * re-trigger (writing same value is still a kernel write call). */
static void apply_limit_if_needed(int limit_val) {
    int current = sysfs_read_int(CHG_LIMIT_NODE);
    if (current == limit_val) return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", limit_val);
    sysfs_write(CHG_LIMIT_NODE, buf);
}

/* Write input_suspend only if it differs. */
static void apply_suspend_if_needed(int val) {
    int current = sysfs_read_int(CHG_SUSPEND_NODE);
    if (current == val) return;
    sysfs_write(CHG_SUSPEND_NODE, val ? "1" : "0");
}

/* Apply the target effective mode to hardware nodes. */
static void apply_effective_mode(int effective_mode) {
    switch (effective_mode) {
    case CHARGE_MODE_VIOLENT:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "0");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "0");
        break;
    case CHARGE_MODE_FAST:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        break;
    case CHARGE_MODE_BALANCED:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_BALANCED); /* 5 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        break;
    case CHARGE_MODE_SAFE:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_SAFE); /* 10 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        break;
    case CHARGE_MODE_BYPASS:
        /* Force hardware cutoff limit=16 (0 mA) AND input_suspend=1 */
        apply_limit_if_needed(16);
        apply_suspend_if_needed(1);
        sysfs_write(CHG_SCONFIG_NODE, "0");
        break;
    case CHARGE_MODE_OEM:
    default:
        /* Remove any limit enforcement so ROM takes back full control.
         * Restore input_suspend to 0 in case user was previously in BYPASS. */
        apply_suspend_if_needed(0);
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        /* Do NOT touch charge_control_limit in OEM mode — leave ROM alone. */
        break;
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void init_charge_control(void) {
    /* Check if this device actually has the HuaQin charger manager nodes */
    if (access(CHG_LIMIT_NODE, F_OK) != 0 && access(CHG_SUSPEND_NODE, F_OK) != 0) {
        log_warn("Charger", "charge_control_limit not found — charger control disabled");
        s_nodes_available = 0;
        return;
    }
    s_nodes_available = 1;

    /* Load persisted user choice from disk */
    s_user_charge_mode = load_charge_mode_conf();
    g_state.user_charge_mode = s_user_charge_mode;
    g_state.charge_mode = s_user_charge_mode;
    g_state.charge_mode_thermal_override = 0;

    /* Apply immediately */
    apply_effective_mode(s_user_charge_mode);

    log_info("Charger", "Charger control init: mode=%s (%d)",
             charge_mode_name(s_user_charge_mode), s_user_charge_mode);
}

void enforce_charge_mode(void) {
    if (!s_nodes_available) return;

    g_state.user_charge_mode = s_user_charge_mode;

    /* In OEM mode we do nothing — full ROM control, no interference. */
    if (s_user_charge_mode == CHARGE_MODE_OEM) {
        g_state.charge_mode = CHARGE_MODE_OEM;
        g_state.charge_mode_thermal_override = 0;
        return;
    }

    int bat_temp  = sysfs_read_int(g_nodes.bat_temp);
    if (bat_temp > 1000) bat_temp /= 1000;
    else if (bat_temp > 100) bat_temp /= 10;

    int bat_cap = sysfs_read_int(CHG_CAPACITY_NODE);
    if (bat_cap <= 0) bat_cap = 50; /* safe default if node unavailable */

    int effective_mode = s_user_charge_mode;
    int override_active = g_state.charge_mode_thermal_override;

    /* --- Safety Override Logic --- */

    if (s_user_charge_mode == CHARGE_MODE_BYPASS) {
        /* User explicitly chose BYPASS: respect it unless critically low battery (<10%) */
        if (bat_cap < CAP_MIN_BYPASS) {
            effective_mode = CHARGE_MODE_SAFE;
            if (!override_active) {
                log_warn("Charger", "BYPASS: battery critically low (%d%%) — auto-resume to SAFE",
                         bat_cap);
            }
            override_active = 1;
        } else {
            override_active = 0;
        }
    } else {
        /* Temperature-based override for FAST / BALANCED / SAFE / VIOLENT modes */
        if (bat_temp >= TEMP_EMERGENCY) {
            effective_mode = CHARGE_MODE_BYPASS;
            if (!override_active || g_state.charge_mode != CHARGE_MODE_BYPASS) {
                log_warn("Charger", "EMERGENCY: bat_temp=%d°C >= %d°C — forcing BYPASS",
                         bat_temp, TEMP_EMERGENCY);
            }
            override_active = 1;
        } else if (bat_temp >= TEMP_OVERRIDE_ENTER) {
            /* Override: force down to SAFE */
            effective_mode = CHARGE_MODE_SAFE;
            if (!override_active) {
                log_warn("Charger", "Thermal override: bat_temp=%d°C >= %d°C — dropping to SAFE (user wants %s)",
                         bat_temp, TEMP_OVERRIDE_ENTER, charge_mode_name(s_user_charge_mode));
            }
            override_active = 1;
        } else if (override_active && bat_temp < TEMP_OVERRIDE_CLEAR) {
            /* Clear override — restore user selection */
            override_active = 0;
            effective_mode = s_user_charge_mode;
            log_info("Charger", "Thermal override cleared: bat_temp=%d°C < %d°C — restoring %s",
                     bat_temp, TEMP_OVERRIDE_CLEAR, charge_mode_name(s_user_charge_mode));
        }
    }

    /* --- Enforcement (anti-tamper mi_thermald) --- */
    /* apply_effective_mode is read-before-write — only writes if node differs */
    apply_effective_mode(effective_mode);

    /* Update global state */
    g_state.user_charge_mode = s_user_charge_mode;
    g_state.charge_mode = effective_mode;
    g_state.charge_mode_thermal_override = override_active;
}

void set_charge_mode(int mode) {
    if (mode < CHARGE_MODE_OEM || mode > CHARGE_MODE_VIOLENT) {
        log_warn("Charger", "set_charge_mode: invalid mode %d ignored", mode);
        return;
    }
    if (!s_nodes_available && mode != CHARGE_MODE_OEM) {
        log_warn("Charger", "set_charge_mode: charger nodes not available on this device");
        return;
    }

    s_user_charge_mode = mode;
    g_state.user_charge_mode = mode;
    g_state.charge_mode_thermal_override = 0; /* clear any active override */
    save_charge_mode_conf(mode);

    /* Apply immediately without waiting for next enforcement tick */
    enforce_charge_mode();

    log_state("Charger", "Charge mode -> %s (%d) via IPC", charge_mode_name(mode), mode);
}

int get_charge_mode(void) {
    return g_state.charge_mode;
}

const char *charge_mode_name(int mode) {
    switch (mode) {
    case CHARGE_MODE_OEM:      return "OEM Stock";
    case CHARGE_MODE_FAST:     return "Fast Charge";
    case CHARGE_MODE_BALANCED: return "Balanced";
    case CHARGE_MODE_SAFE:     return "Safe Mode";
    case CHARGE_MODE_BYPASS:   return "Bypass Mode";
    case CHARGE_MODE_VIOLENT:  return "Violent Charge";
    default:                   return "Unknown";
    }
}
