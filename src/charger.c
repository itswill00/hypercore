
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
#define LIMIT_FAST     0   /* Level 0:  3,897 mA (~3.9A / ~15.9W) - Fast / Violent / OEM Stock */
#define LIMIT_BALANCED 10  /* Level 10: 2,318 mA (~2.3A / ~9.3W)  - Balanced (cool active usage) */
#define LIMIT_SAFE     14  /* Level 14: 727 mA   (~0.73A / ~2.8W) - Safe (overnight & GPS) */

/* Safety thresholds */
#define TEMP_OVERRIDE_ENTER   45   /* >= 45°C: start gradual step-down            */
#define TEMP_EMERGENCY        50   /* >= 50°C: force BYPASS (emergency cutoff)    */
#define TEMP_OVERRIDE_CLEAR   41   /* <= 41°C: sustained cool temp for step-up    */
#define CAP_MIN_BYPASS        10   /* <  10%:  auto-resume from BYPASS to SAFE    */
#define CAP_MIN_BYPASS_RESTORE 12  /* >= 12%:  hysteresis clear for BYPASS resume */
#define CAP_FAST_LIMIT        80   /* >= 80%:  drop FAST to BALANCED              */

/* Module-level state */
static int s_nodes_available     = 0;  /* 1 if charger nodes exist on this device */
static int s_user_charge_mode    = CHARGE_MODE_OEM;  /* user's persistent choice   */
static int s_custom_charge_limit = LIMIT_BALANCED;   /* custom slider hardware limit (0-15), default 10 */
static int s_thermal_step        = 0;  /* 0=User mode, 1=Balanced step, 2=Safe step, 3=Bypass cutoff */
static time_t s_last_step_time   = 0;  /* timestamp of last thermal ladder step change */

/* Helper: map thermal ladder step to effective charge mode */
static int get_mode_for_step(int user_mode, int step) {
    if (step <= 0) return user_mode;
    if (user_mode == CHARGE_MODE_SAFE) {
        /* Safe mode (3) should never be promoted to Balanced mode (2) when overheating */
        return (step >= 2) ? CHARGE_MODE_BYPASS : CHARGE_MODE_SAFE;
    }
    if (user_mode == CHARGE_MODE_CUSTOM) {
        if (step >= 3) return CHARGE_MODE_BYPASS;
        if (step == 2) return CHARGE_MODE_SAFE;
        return (s_custom_charge_limit < LIMIT_BALANCED) ? CHARGE_MODE_BALANCED : CHARGE_MODE_SAFE;
    }
    if (step == 1) return CHARGE_MODE_BALANCED;
    if (step == 2) return CHARGE_MODE_SAFE;
    return CHARGE_MODE_BYPASS;
}

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
    if (mode < CHARGE_MODE_OEM || mode > CHARGE_MODE_CUSTOM) mode = CHARGE_MODE_OEM;
    return mode;
}

static void save_custom_charge_limit_conf(int limit) {
    char path[300];
    snprintf(path, sizeof(path), "%s/custom_charge_limit.conf", g_nodes.mod_dir);
    char tmp[310];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (f) {
        fprintf(f, "%d\n", limit);
        fclose(f);
        rename(tmp, path);
    }
}

static int load_custom_charge_limit_conf(void) {
    char path[300];
    snprintf(path, sizeof(path), "%s/custom_charge_limit.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "r");
    if (!f) return LIMIT_BALANCED;
    int limit = LIMIT_BALANCED;
    if (fscanf(f, "%d", &limit) != 1) limit = LIMIT_BALANCED;
    fclose(f);
    if (limit < 0 || limit > 15) limit = LIMIT_BALANCED;
    return limit;
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

/* Pulse input_suspend (1 -> 100ms -> 0) to force MediaTek TCPC driver to clear
 * latched PMIC restrictions and execute a fresh USB PD CC-pin handshake (unlocks 13.5W-15W+). */
static void trigger_tcpc_pd_renegotiation(void) {
    sysfs_write(CHG_SUSPEND_NODE, "1");
    usleep(100000); /* 100ms pulse */
    sysfs_write(CHG_SUSPEND_NODE, "0");
}

static int s_prev_effective_mode = -1;

/* Apply the target effective mode to hardware nodes. */
static void apply_effective_mode(int effective_mode) {
    int mode_changed = (s_prev_effective_mode != effective_mode);

    switch (effective_mode) {
    case CHARGE_MODE_VIOLENT:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "0");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "0");
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_FAST:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_BALANCED:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_BALANCED); /* 10 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        break;
    case CHARGE_MODE_SAFE:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_SAFE); /* 14 */
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        break;
    case CHARGE_MODE_BYPASS:
        /* Force hardware cutoff limit=16 (0 mA) AND input_suspend=1 */
        apply_limit_if_needed(16);
        apply_suspend_if_needed(1);
        sysfs_write(CHG_SCONFIG_NODE, "0");
        break;
    case CHARGE_MODE_CUSTOM:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(s_custom_charge_limit);
        sysfs_write(CHG_SCONFIG_NODE, "0");
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_OEM:
    default:
        /* Restore hardware nodes to OEM baseline defaults so ROM takes back full control */
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "1");
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    }

    s_prev_effective_mode = effective_mode;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void init_charge_control(void) {
    /* Check if this device actually has the HuaQin charger manager nodes */
    if (access(CHG_LIMIT_NODE, F_OK) != 0 && access(CHG_SUSPEND_NODE, F_OK) != 0) {
        log_warn("Charger", "charge_control_limit not found — charger control disabled");
        s_nodes_available = 0;
        g_state.charger_supported = 0;
        return;
    }
    s_nodes_available = 1;
    g_state.charger_supported = 1;

    /* Load persisted user choice from disk */
    s_user_charge_mode = load_charge_mode_conf();
    s_custom_charge_limit = load_custom_charge_limit_conf();
    g_state.user_charge_mode = s_user_charge_mode;
    g_state.custom_charge_limit = s_custom_charge_limit;
    g_state.charge_mode = s_user_charge_mode;
    g_state.charge_mode_thermal_override = 0;

    if (s_user_charge_mode == CHARGE_MODE_OEM) {
        /* In OEM Stock mode, ensure input_suspend is cleared in case prior session was Bypass,
         * then leave all charging regulation 100% to OEM kernel and mi_thermald. */
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "1");
        s_prev_effective_mode = CHARGE_MODE_OEM;
        log_info("Charger", "Charger control init: OEM Stock (100%% kernel/ROM managed)");
        return;
    }

    /* Apply immediately */
    apply_effective_mode(s_user_charge_mode);

    log_info("Charger", "Charger control init: mode=%s (%d)",
             charge_mode_name(s_user_charge_mode), s_user_charge_mode);
}

void enforce_charge_mode(void) {
    if (!s_nodes_available) return;

    g_state.user_charge_mode = s_user_charge_mode;

    /* In OEM Stock mode, HyperCore is strictly hands-off.
     * All charging rate, current limits, and thermal throttling are 100%
     * governed by the device hardware, kernel drivers, and mi_thermald.
     * Zero sysfs writes on periodic ticks! */
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
    time_t now = time(NULL);

    /* SOC Tapering: At >= 80% SOC, taper high-speed charging down to BALANCED to protect battery longevity */
    if (bat_cap >= CAP_FAST_LIMIT) {
        if (s_user_charge_mode == CHARGE_MODE_FAST || s_user_charge_mode == CHARGE_MODE_VIOLENT) {
            effective_mode = CHARGE_MODE_BALANCED;
        } else if (s_user_charge_mode == CHARGE_MODE_CUSTOM && s_custom_charge_limit < LIMIT_BALANCED) {
            effective_mode = CHARGE_MODE_BALANCED;
        }
    }

    /* --- Safety Override Logic --- */

    if (s_user_charge_mode == CHARGE_MODE_BYPASS) {
        /* User explicitly chose BYPASS: respect it unless critically low battery (<10%).
         * Hysteresis: drop to SAFE when cap < CAP_MIN_BYPASS (10%), but only clear the
         * override and restore BYPASS when cap >= CAP_MIN_BYPASS_RESTORE (12%).
         * Without hysteresis, the mode would toggle rapidly at the 10% boundary. */
        if (bat_cap < CAP_MIN_BYPASS) {
            effective_mode = CHARGE_MODE_SAFE;
            if (!override_active) {
                log_warn("Charger", "BYPASS: battery critically low (%d%%) — auto-resume to SAFE",
                         bat_cap);
            }
            override_active = 1;
        } else if (override_active && bat_cap >= CAP_MIN_BYPASS_RESTORE) {
            /* Hysteresis clear: only restore BYPASS after cap recovers above 12% */
            override_active = 0;
            log_info("Charger", "BYPASS low-bat override cleared: bat_cap=%d%% >= %d%% — restoring BYPASS",
                     bat_cap, CAP_MIN_BYPASS_RESTORE);
        } else if (override_active) {
            /* Still recovering (cap between 10-12%) — stay in SAFE */
            effective_mode = CHARGE_MODE_SAFE;
        }
    } else {
        /* --- Gradual Multi-Step Thermal Ladder --- */
        if (bat_temp >= TEMP_EMERGENCY) {
            /* >= 50°C: Immediate Emergency Cutoff */
            s_thermal_step = 3;
            s_last_step_time = now;
            effective_mode = CHARGE_MODE_BYPASS;
            if (!override_active || g_state.charge_mode != CHARGE_MODE_BYPASS) {
                log_warn("Charger", "EMERGENCY: bat_temp=%d°C >= %d°C — forcing BYPASS cutoff",
                         bat_temp, TEMP_EMERGENCY);
            }
            override_active = 1;
        } else if (bat_temp >= TEMP_OVERRIDE_ENTER) {
            /* >= 45°C: Step down gradually if 10s have elapsed since last step */
            if (s_thermal_step == 0) {
                s_thermal_step = 1;
                s_last_step_time = now;
                log_warn("Charger", "Thermal ladder: bat_temp=%d°C >= %d°C -> Step 1 down (%s)",
                         bat_temp, TEMP_OVERRIDE_ENTER, charge_mode_name(get_mode_for_step(s_user_charge_mode, 1)));
            } else if (now - s_last_step_time >= 10) {
                /* Temp still >= 45°C after 10s dwell time — step down further */
                int max_step = (s_user_charge_mode == CHARGE_MODE_BALANCED) ? 2 : (s_user_charge_mode == CHARGE_MODE_SAFE ? 1 : 2);
                if (s_thermal_step < max_step) {
                    s_thermal_step++;
                    s_last_step_time = now;
                    log_warn("Charger", "Thermal ladder: bat_temp=%d°C persistent >= %d°C -> Step %d down (%s)",
                             bat_temp, TEMP_OVERRIDE_ENTER, s_thermal_step,
                             charge_mode_name(get_mode_for_step(s_user_charge_mode, s_thermal_step)));
                }
            }
            effective_mode = get_mode_for_step(s_user_charge_mode, s_thermal_step);
            override_active = (s_thermal_step > 0);
        } else if (override_active && bat_temp <= TEMP_OVERRIDE_CLEAR) {
            /* <= 41°C: Step up gradually towards user mode if 15s have elapsed at cool temp */
            if (s_last_step_time == 0) s_last_step_time = now;
            if (now - s_last_step_time >= 15) {
                s_thermal_step--;
                s_last_step_time = now;
                if (s_thermal_step <= 0) {
                    s_thermal_step = 0;
                    override_active = 0;
                    effective_mode = s_user_charge_mode;
                    log_info("Charger", "Thermal ladder cleared: bat_temp=%d°C <= %d°C — restored %s",
                             bat_temp, TEMP_OVERRIDE_CLEAR, charge_mode_name(s_user_charge_mode));
                } else {
                    effective_mode = get_mode_for_step(s_user_charge_mode, s_thermal_step);
                    log_info("Charger", "Thermal ladder step-up: bat_temp=%d°C <= %d°C -> Step %d (%s)",
                             bat_temp, TEMP_OVERRIDE_CLEAR, s_thermal_step, charge_mode_name(effective_mode));
                }
            } else {
                /* Keep current stepped mode while waiting for 15s cool stability */
                effective_mode = get_mode_for_step(s_user_charge_mode, s_thermal_step);
            }
        } else if (override_active) {
            /* Temp between 42°C and 44°C while in override: maintain current stepped level */
            effective_mode = get_mode_for_step(s_user_charge_mode, s_thermal_step);
        }
    }

    /* Update global state & apply to hardware nodes */
    g_state.charge_mode = effective_mode;
    g_state.charge_mode_thermal_override = override_active;
    apply_effective_mode(effective_mode);
}

void set_charge_mode(int mode) {
    if (mode < CHARGE_MODE_OEM || mode > CHARGE_MODE_CUSTOM) {
        log_warn("Charger", "set_charge_mode: invalid mode %d ignored", mode);
        return;
    }
    if (!s_nodes_available && mode != CHARGE_MODE_OEM) {
        log_warn("Charger", "set_charge_mode: charger nodes not available on this device");
        return;
    }

    s_user_charge_mode = mode;
    s_thermal_step = 0;        /* reset thermal ladder step on manual user choice */
    s_last_step_time = 0;
    g_state.user_charge_mode = mode;
    g_state.charge_mode_thermal_override = 0; /* clear any active override */
    save_charge_mode_conf(mode);

    if (mode == CHARGE_MODE_OEM) {
        /* User switched to OEM Stock: restore hardware baseline ONCE,
         * then completely release control to OEM kernel/thermal engine. */
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "1");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "1");
        if (s_prev_effective_mode != CHARGE_MODE_OEM) {
            trigger_tcpc_pd_renegotiation();
        }
        s_prev_effective_mode = CHARGE_MODE_OEM;
        g_state.charge_mode = CHARGE_MODE_OEM;
        log_state("Charger", "Charge mode -> OEM Stock (hands-off: 100%% OEM kernel/thermal control)");
        return;
    }

    /* Apply immediately without waiting for next enforcement tick */
    enforce_charge_mode();

    log_state("Charger", "Charge mode -> %s (%d) via IPC", charge_mode_name(mode), mode);
}

void set_custom_charge_limit(int limit_level) {
    if (limit_level < 0 || limit_level > 15) {
        log_warn("Charger", "set_custom_charge_limit: invalid limit %d ignored", limit_level);
        return;
    }
    if (!s_nodes_available) {
        log_warn("Charger", "set_custom_charge_limit: charger nodes not available on this device");
        return;
    }

    s_custom_charge_limit = limit_level;
    s_user_charge_mode = CHARGE_MODE_CUSTOM;
    s_thermal_step = 0;        /* reset thermal ladder step on manual user choice */
    s_last_step_time = 0;
    g_state.user_charge_mode = CHARGE_MODE_CUSTOM;
    g_state.custom_charge_limit = limit_level;
    g_state.charge_mode_thermal_override = 0;
    save_charge_mode_conf(CHARGE_MODE_CUSTOM);
    save_custom_charge_limit_conf(limit_level);

    /* Apply immediately */
    enforce_charge_mode();

    log_state("Charger", "Charge limit set to level %d via Custom Slider", limit_level);
}

int get_custom_charge_limit(void) {
    return s_custom_charge_limit;
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
    case CHARGE_MODE_CUSTOM:   return "Custom Slider";
    default:                   return "Unknown";
    }
}
