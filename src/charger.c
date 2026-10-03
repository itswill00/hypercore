
#include "charger.hpp"
#include "sysfs.hpp"
#include "thermal.hpp"
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
#define CHG_CURRENT_NODE "/sys/class/power_supply/battery/current_now"
#define CHG_CAPACITY_NODE "/sys/class/power_supply/battery/capacity"

/* Level values written to charge_control_limit per mode */
#define LIMIT_FAST     0   /* Level 0:  3,897 mA (~3.9A / ~15.9W) - Fast / Violent / OEM Stock */
#define LIMIT_BALANCED 10  /* Level 10: 2,318 mA (~2.3A / ~9.3W)  - Balanced (cool active usage) */
#define LIMIT_SAFE     14  /* Level 14: 727 mA   (~0.73A / ~2.8W) - Safe (overnight & GPS) */

/* Thermal ladder thresholds.
 * The user's selected mode is a CEILING, never a guarantee: temperature may
 * only lower it. Without this, a 30W+ mode charges a Li-Po cell at full rate
 * regardless of temperature and we depend entirely on mi_thermald to stop us —
 * which several ROMs do not do for charge current. */
#define TEMP_OVERRIDE_ENTER   45   /* >= 45°C: step down one rung                 */
#define TEMP_EMERGENCY        50   /* >= 50°C: hard floor, force Safe Mode        */
#define TEMP_OVERRIDE_CLEAR   41   /* <= 41°C: eligible to climb back up          */
#define TEMP_STEP_UP_HOLD    180   /* seconds below CLEAR before climbing back up */

#define CAP_MIN_BYPASS        10   /* <  10%:  auto-resume from BYPASS to SAFE    */
#define CAP_MIN_BYPASS_RESTORE 12  /* >= 12%:  hysteresis clear for BYPASS resume */

/* Module-level state */
static int s_nodes_available     = 0;  /* 1 if charger nodes exist on this device */
static int s_user_charge_mode    = CHARGE_MODE_OEM;  /* user's persistent choice   */
static int s_custom_charge_limit = LIMIT_BALANCED;   /* custom slider hardware limit (0-15), default 10 */
static int s_night_charging      = 0;  /* 1 = night charging protection (pauses at 80% overnight) */
static int s_smart_chg           = 0;  /* 1 = smart charging curve enabled */
static int s_protect_80          = 0;  /* 1 = user hard limit to stop charging at 80% */

/* Thermal ladder state. s_thermal_rung is the rung currently forced by
 * temperature (-1 = user's own mode is in effect). */
static int   s_thermal_rung   = -1;
static time_t s_cool_since    = 0;

/* Rung 0 = most aggressive, rung 3 = coolest. OEM and BYPASS are deliberately
 * absent: OEM means "hands off to the vendor stack" and BYPASS means the user
 * already asked for the coolest thing possible — neither is ours to override. */
#define RUNG_VIOLENT  0
#define RUNG_FAST     1
#define RUNG_BALANCED 2
#define RUNG_SAFE     3

static const int s_ladder_modes[] = {
    CHARGE_MODE_VIOLENT, CHARGE_MODE_FAST, CHARGE_MODE_BALANCED, CHARGE_MODE_SAFE
};
#define LADDER_COUNT ((int)(sizeof(s_ladder_modes) / sizeof(s_ladder_modes[0])))

/* Map a mode to its rung, or -1 if the mode is not ladder-managed. */
static int ladder_rung(int mode) {
    switch (mode) {
        case CHARGE_MODE_VIOLENT:  return RUNG_VIOLENT;
        case CHARGE_MODE_FAST:     return RUNG_FAST;
        /* Custom is user-tuned but sits between FAST and BALANCED in risk. */
        case CHARGE_MODE_CUSTOM:   return RUNG_FAST;
        case CHARGE_MODE_BALANCED: return RUNG_BALANCED;
        case CHARGE_MODE_SAFE:     return RUNG_SAFE;
        default:                   return -1;
    }
}

/* Rung back to a mode, clamping anything out of range to the coolest rung. */
static int rung_mode(int rung) {
    if (rung < 0) rung = 0;
    if (rung >= LADDER_COUNT) rung = LADDER_COUNT - 1;
    return s_ladder_modes[rung];
}

/* Sleep boost: screen off means cooler SoC and zero interaction, so raise the
 * ceiling by one rung. Balanced -> Fast, Safe -> Balanced. Custom keeps its
 * mode but drops 5 hardware levels toward 0. OEM and Bypass are exempt by
 * design, Fast and Violent are already at the ceiling. Thermal ladder still
 * runs after this and may step the boosted base back down. */
#define SLEEP_BOOST_DELTA 5
static int s_sleep_boost_active = 0;
static int s_active_custom_limit = LIMIT_BALANCED;
static int s_prev_sleep_base_mode = -99;

static int sleep_boost_base_mode(int user_mode) {
    if (g_state.current_profile != PROFILE_Sleep) return user_mode;
    switch (user_mode) {
        case CHARGE_MODE_BALANCED: return CHARGE_MODE_FAST;
        case CHARGE_MODE_SAFE:     return CHARGE_MODE_BALANCED;
        default:                   return user_mode;
    }
}

static int sleep_boosted_limit(int user_limit) {
    if (g_state.current_profile != PROFILE_Sleep) return user_limit;
    int b = user_limit - SLEEP_BOOST_DELTA;
    if (b < 0) b = 0;
    return b;
}

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

/* ponytail: generic int conf helpers — 10 copy-paste save/load funcs collapsed into 2, ceiling is plain text ints, no schema needed */
static void save_int_conf(const char *fname, int val) {
    char path[300];
    snprintf(path, sizeof(path), "%s/%s", g_nodes.data_dir, fname);
    char tmp[310];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (f) { fprintf(f, "%d\n", val); fclose(f); chmod(tmp, 0600); rename(tmp, path); chmod(path, 0600); }
}

static int load_int_conf(const char *fname, int dflt) {
    char path[300];
    snprintf(path, sizeof(path), "%s/%s", g_nodes.data_dir, fname);
    FILE *f = fopen(path, "r");
    if (!f && g_nodes.mod_dir[0] && strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        char old_path[300];
        snprintf(old_path, sizeof(old_path), "%s/%s", g_nodes.mod_dir, fname);
        f = fopen(old_path, "r");
        if (f) {
            int v = dflt;
            if (fscanf(f, "%d", &v) != 1) v = dflt;
            fclose(f); unlink(old_path); save_int_conf(fname, v); return v;
        }
    }
    if (!f) return dflt;
    int v = dflt;
    if (fscanf(f, "%d", &v) != 1) v = dflt;
    fclose(f); return v;
}

static void save_charge_mode_conf(int mode) { save_int_conf("charge_mode.conf", mode); }
static int load_charge_mode_conf(void) {
    int v = load_int_conf("charge_mode.conf", CHARGE_MODE_OEM);
    if (v < CHARGE_MODE_OEM || v > CHARGE_MODE_CUSTOM) v = CHARGE_MODE_OEM; return v;
}
static void save_custom_charge_limit_conf(int l) { save_int_conf("custom_charge_limit.conf", l); }
static int load_custom_charge_limit_conf(void) {
    int v = load_int_conf("custom_charge_limit.conf", LIMIT_BALANCED);
    if (v < 0 || v > 15) v = LIMIT_BALANCED; return v;
}
static void save_night_charging_conf(int v) { save_int_conf("night_charging.conf", v ? 1 : 0); }
static int load_night_charging_conf(void) { return load_int_conf("night_charging.conf", 0) == 1 ? 1 : 0; }
static void save_smart_chg_conf(int v) { save_int_conf("smart_chg.conf", v ? 1 : 0); }
static int load_smart_chg_conf(void) { return load_int_conf("smart_chg.conf", 0) == 1 ? 1 : 0; }
static void save_protect_80_conf(int v) { save_int_conf("protect_80.conf", v ? 1 : 0); }
static int load_protect_80_conf(void) { return load_int_conf("protect_80.conf", 0) == 1 ? 1 : 0; }

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
static int s_prev_effective_limit = -1;

/* Apply the target effective mode to hardware nodes. */
static void apply_effective_mode(int effective_mode) {
    int mode_changed = (s_prev_effective_mode != effective_mode);
    int limit_changed = (s_active_custom_limit != s_prev_effective_limit);
    const char *smart_str = s_smart_chg ? "1" : "0";
    const char *night_str = s_night_charging ? "1" : "0";

    switch (effective_mode) {
    case CHARGE_MODE_VIOLENT:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "0");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "0");
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_FAST:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_FAST); /* 0 */
        sysfs_write("/sys/class/power_supply/battery/smart_chg", smart_str);
        sysfs_write("/sys/class/power_supply/battery/night_charging", night_str);
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_BALANCED:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_BALANCED); /* 10 */
        sysfs_write("/sys/class/power_supply/battery/smart_chg", smart_str);
        sysfs_write("/sys/class/power_supply/battery/night_charging", night_str);
        break;
    case CHARGE_MODE_SAFE:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(LIMIT_SAFE); /* 14 */
        sysfs_write("/sys/class/power_supply/battery/smart_chg", smart_str);
        sysfs_write("/sys/class/power_supply/battery/night_charging", night_str);
        break;
    case CHARGE_MODE_BYPASS:
        /* Force hardware cutoff limit=16 (0 mA) AND input_suspend=1 */
        apply_limit_if_needed(16);
        apply_suspend_if_needed(1);
        break;
    case CHARGE_MODE_CUSTOM:
        apply_suspend_if_needed(0);
        apply_limit_if_needed(s_active_custom_limit);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", smart_str);
        sysfs_write("/sys/class/power_supply/battery/night_charging", night_str);
        if (mode_changed || limit_changed) trigger_tcpc_pd_renegotiation();
        break;
    case CHARGE_MODE_OEM:
    default:
        /* Restore hardware nodes to OEM baseline defaults so ROM takes back full control */
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", smart_str);
        sysfs_write("/sys/class/power_supply/battery/night_charging", night_str);
        if (mode_changed) trigger_tcpc_pd_renegotiation();
        break;
    }

    s_prev_effective_mode = effective_mode;
    s_prev_effective_limit = s_active_custom_limit;
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
    s_night_charging = load_night_charging_conf();
    s_smart_chg = load_smart_chg_conf();
    s_protect_80 = load_protect_80_conf();
    s_active_custom_limit = s_custom_charge_limit;
    s_sleep_boost_active = 0;
    s_prev_sleep_base_mode = -99;
    s_prev_effective_limit = -1;
    g_state.user_charge_mode = s_user_charge_mode;
    g_state.custom_charge_limit = s_custom_charge_limit;
    g_state.effective_custom_limit = s_custom_charge_limit;
    g_state.sleep_boost_active = 0;
    g_state.night_charging = s_night_charging;
    g_state.smart_chg = s_smart_chg;
    g_state.protect_80 = s_protect_80;
    g_state.charge_mode = s_user_charge_mode;

    if (s_user_charge_mode == CHARGE_MODE_OEM) {
        /* In OEM Stock mode, ensure input_suspend is cleared in case prior session was Bypass,
         * then leave all charging regulation 100% to OEM kernel and mi_thermald. */
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", s_smart_chg ? "1" : "0");
        sysfs_write("/sys/class/power_supply/battery/night_charging", s_night_charging ? "1" : "0");
        s_prev_effective_mode = CHARGE_MODE_OEM;
        log_info("Charger", "Charger control init: OEM Stock (100%% kernel/ROM managed)");
        return;
    }

    /* Apply immediately */
    apply_effective_mode(s_user_charge_mode);

    log_info("Charger", "Charger control init: mode=%s (%d), night_chg=%d, smart_chg=%d, protect_80=%d",
             charge_mode_name(s_user_charge_mode), s_user_charge_mode,
             s_night_charging, s_smart_chg, s_protect_80);
}

void enforce_charge_mode(void) {
    if (!s_nodes_available) return;

    g_state.user_charge_mode = s_user_charge_mode;
    g_state.night_charging = s_night_charging;
    g_state.smart_chg = s_smart_chg;
    g_state.protect_80 = s_protect_80;

    /* In OEM Stock mode, HyperCore is strictly hands-off.
     * All charging rate, current limits, and thermal throttling are 100%
     * governed by the device hardware, kernel drivers, and mi_thermald.
     * Zero sysfs writes on periodic ticks! */
    if (s_user_charge_mode == CHARGE_MODE_OEM) {
        g_state.charge_mode = CHARGE_MODE_OEM;
        g_state.sleep_boost_active = 0;
        s_sleep_boost_active = 0;
        g_state.effective_custom_limit = s_custom_charge_limit;
        return;
    }

    /* Raw zone value is un-normalised (milli- or deci-degrees on most OEMs). */
    int bat_temp = sysfs_read_int(g_nodes.bat_temp);
    normalize_thermal_temps(NULL, &bat_temp);

    int bat_cap = sysfs_read_int(CHG_CAPACITY_NODE);
    if (bat_cap <= 0) bat_cap = 50; /* safe default if node unavailable */

    /* Sleep boost sits between user choice and thermal ladder: the boosted
     * base is still only a ceiling that heat may lower. */
    int sleep_base_mode = sleep_boost_base_mode(s_user_charge_mode);
    int sleep_base_limit = sleep_boosted_limit(s_custom_charge_limit);
    int new_sleep_active = (sleep_base_mode != s_user_charge_mode) ||
        (s_user_charge_mode == CHARGE_MODE_CUSTOM && sleep_base_limit != s_custom_charge_limit);
    if (new_sleep_active != s_sleep_boost_active) {
        s_sleep_boost_active = new_sleep_active;
        if (new_sleep_active) {
            log_info("Charger", "Sleep boost active: %s -> %s (screen off)",
                     charge_mode_name(s_user_charge_mode), charge_mode_name(sleep_base_mode));
        } else {
            log_info("Charger", "Sleep boost cleared: back to %s (screen on)",
                     charge_mode_name(s_user_charge_mode));
        }
    }
    s_active_custom_limit = sleep_base_limit;
    g_state.sleep_boost_active = s_sleep_boost_active;
    g_state.custom_charge_limit = s_custom_charge_limit;
    g_state.effective_custom_limit = s_active_custom_limit;

    /* Never weaken thermal protection on base change while hot.
     * Sleep engage and wake both shift the base; take the cooler of the old
     * rung and one-down-from-new-base so waking cannot erase step-down. */
    if (sleep_base_mode != s_prev_sleep_base_mode) {
        s_prev_sleep_base_mode = sleep_base_mode;
        int nb_rung = ladder_rung(sleep_base_mode);
        if (s_thermal_rung >= 0 && nb_rung >= 0) {
            int want = nb_rung + 1;
            if (want > s_thermal_rung) s_thermal_rung = want;
        }
    }

    int effective_mode = sleep_base_mode;
    static int s_override_active = 0;

    /* Battery Protect 80% Cap Toggle: If user enabled 80% stop limit,
     * suspend input charging once battery capacity reaches >= 80%.
     * Hysteresis: remain in BYPASS until capacity drops to <= 77% before resuming,
     * eliminating rapid 79%-80% charging flutter and PMIC renegotiation stress. */
    static int s_protect_active = 0;
    if (s_protect_80 && s_user_charge_mode != CHARGE_MODE_BYPASS) {
        if (bat_cap >= 80) {
            s_protect_active = 1;
        } else if (s_protect_active && bat_cap <= 77) {
            s_protect_active = 0;
        }
        if (s_protect_active) {
            effective_mode = CHARGE_MODE_BYPASS;
        }
    } else {
        s_protect_active = 0;
    }

    /* --- Safety Override Logic --- */

    if (s_user_charge_mode == CHARGE_MODE_BYPASS) {
        /* User explicitly chose BYPASS: respect it unless critically low battery (<10%).
         * Hysteresis: drop to SAFE when cap < CAP_MIN_BYPASS (10%), but only clear the
         * override and restore BYPASS when cap >= CAP_MIN_BYPASS_RESTORE (12%).
         * Without hysteresis, the mode would toggle rapidly at the 10% boundary. */
        if (bat_cap < CAP_MIN_BYPASS) {
            effective_mode = CHARGE_MODE_SAFE;
            if (!s_override_active) {
                log_warn("Charger", "BYPASS: battery critically low (%d%%) — auto-resume to SAFE",
                         bat_cap);
            }
            s_override_active = 1;
        } else if (s_override_active && bat_cap >= CAP_MIN_BYPASS_RESTORE) {
            /* Hysteresis clear: only restore BYPASS after cap recovers above 12% */
            s_override_active = 0;
            log_info("Charger", "BYPASS low-bat override cleared: bat_cap=%d%% >= %d%% — restoring BYPASS",
                     bat_cap, CAP_MIN_BYPASS_RESTORE);
        } else if (s_override_active) {
            /* Still recovering (cap between 10-12%) — stay in SAFE */
            effective_mode = CHARGE_MODE_SAFE;
        }
    } else {
        /* --- Thermal ladder ------------------------------------------------
         * The boosted base is a ceiling; temperature may only lower it.
         *   >= TEMP_EMERGENCY  hard floor to Safe Mode
         *   >= TEMP_OVERRIDE_ENTER  step down one rung
         *   <= TEMP_OVERRIDE_CLEAR   start a TEMP_STEP_UP_HOLD timer; only
         *                             climb back after it expires
         * Anything in the band between CLEAR and ENTER freezes the current
         * rung, which keeps a cell hovering near the threshold from flapping
         * between charge rates. */
        int base_rung = ladder_rung(sleep_base_mode);
        time_t now = time(NULL);

        /* Not BYPASS, so the low-battery BYPASS override no longer applies. */
        s_override_active = 0;

        if (base_rung < 0) {
            /* OEM / BYPASS: not ladder-managed — drop any stale override. */
            if (s_thermal_rung >= 0) {
                s_thermal_rung = -1;
                s_cool_since = 0;
            }
        } else if (bat_temp >= TEMP_EMERGENCY) {
            if (s_thermal_rung != RUNG_SAFE) {
                log_warn("Charger", "Battery %d°C >= %d°C — thermal override: forcing Safe Mode",
                         bat_temp, TEMP_EMERGENCY);
            }
            s_thermal_rung = RUNG_SAFE;
            s_cool_since = 0;
        } else if (bat_temp >= TEMP_OVERRIDE_ENTER) {
            if (s_thermal_rung < 0) {
                s_thermal_rung = base_rung + 1;
                log_warn("Charger", "Battery %d°C >= %d°C — thermal override: stepping down to %s",
                         bat_temp, TEMP_OVERRIDE_ENTER, charge_mode_name(rung_mode(s_thermal_rung)));
            }
            s_cool_since = 0;
        } else if (bat_temp <= TEMP_OVERRIDE_CLEAR) {
            if (s_cool_since == 0) {
                s_cool_since = now;
            } else if (now - s_cool_since >= TEMP_STEP_UP_HOLD) {
                if (s_thermal_rung >= 0) {
                    log_info("Charger", "Battery %d°C cool for %ds — thermal override cleared, restoring %s",
                             bat_temp, TEMP_STEP_UP_HOLD, charge_mode_name(sleep_base_mode));
                }
                s_thermal_rung = -1;
                s_cool_since = 0;
            }
        } else {
            /* Dead band: neither hot nor cool. Hold position. */
            s_cool_since = 0;
        }

        if (s_thermal_rung >= 0) {
            effective_mode = rung_mode(s_thermal_rung);
        } else {
            effective_mode = sleep_base_mode;
        }
    }

    /* Update global state & apply to hardware nodes */
    g_state.charge_mode = effective_mode;
    g_state.effective_custom_limit = s_active_custom_limit;
    g_state.charge_override = s_override_active || (effective_mode != sleep_base_mode);
    apply_effective_mode(effective_mode);
}

int is_charge_override_active(void) {
    return g_state.charge_override;
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
    g_state.user_charge_mode = mode;
    save_charge_mode_conf(mode);

    if (mode == CHARGE_MODE_OEM) {
        /* User switched to OEM Stock: restore hardware baseline ONCE,
         * then completely release control to OEM kernel/thermal engine. */
        s_thermal_rung = -1;
        s_cool_since = 0;
        s_sleep_boost_active = 0;
        s_prev_sleep_base_mode = -99;
        g_state.sleep_boost_active = 0;
        g_state.effective_custom_limit = s_custom_charge_limit;
        apply_suspend_if_needed(0);
        apply_limit_if_needed(0);
        sysfs_write("/sys/class/power_supply/battery/smart_chg", "0");
        sysfs_write("/sys/class/power_supply/battery/night_charging", "0");
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
    g_state.user_charge_mode = CHARGE_MODE_CUSTOM;
    g_state.custom_charge_limit = limit_level;
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

void set_night_charging(int enabled) {
    s_night_charging = enabled ? 1 : 0;
    g_state.night_charging = s_night_charging;
    save_night_charging_conf(s_night_charging);
    sysfs_write("/sys/class/power_supply/battery/night_charging", s_night_charging ? "1" : "0");
    log_state("Charger", "Night charging switch -> %d", s_night_charging);
}

int get_night_charging(void) {
    return s_night_charging;
}

void set_smart_chg(int enabled) {
    s_smart_chg = enabled ? 1 : 0;
    g_state.smart_chg = s_smart_chg;
    save_smart_chg_conf(s_smart_chg);
    sysfs_write("/sys/class/power_supply/battery/smart_chg", s_smart_chg ? "1" : "0");
    log_state("Charger", "Smart charging switch -> %d", s_smart_chg);
}

int get_smart_chg(void) {
    return s_smart_chg;
}

void set_protect_80(int enabled) {
    s_protect_80 = enabled ? 1 : 0;
    g_state.protect_80 = s_protect_80;
    save_protect_80_conf(s_protect_80);
    enforce_charge_mode();
    log_state("Charger", "Protect 80%% limit switch -> %d", s_protect_80);
}

int get_protect_80(void) {
    return s_protect_80;
}

int is_sleep_boost_active(void) {
    return s_sleep_boost_active;
}

int get_effective_custom_limit(void) {
    return s_active_custom_limit;
}
