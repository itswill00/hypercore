
#include <stdarg.h>

#include "thermal.hpp"
#include "sysfs.hpp"
#include "log.hpp"

/* Last-resort thermal zone picker, used only when score-based discovery found
 * nothing. Zone numbering is not stable across ROMs or kernel versions, so a
 * hardcoded index is a guess that can quietly bind the whole thermal guard to
 * an unrelated sensor — skin, PA, DRAM — and that sensor then drives both the
 * frequency ceilings and the charger ladder. Confirm the zone's own `type`
 * string looks like the sensor we actually want before accepting it. */
static void pick_fallback_zone(char *out, size_t out_len, const char *sensor,
                               const int *indices, int count, ...) {
    out[0] = '\0';
    va_list ap;

    for (int i = 0; i < count; i++) {
        char type_path[256], temp_path[256], type[64] = "";
        snprintf(type_path, sizeof(type_path), "/sys/class/thermal/thermal_zone%d/type", indices[i]);
        snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/thermal_zone%d/temp", indices[i]);
        if (access(temp_path, F_OK) != 0) continue;
        if (!sysfs_read_str(type_path, type, sizeof(type))) continue;

        va_start(ap, count);
        int wanted = 0;
        for (const char *w = va_arg(ap, const char *); w; w = va_arg(ap, const char *)) {
            if (strstr(type, w)) { wanted = 1; break; }
        }
        va_end(ap);
        if (!wanted) continue;

        strncpy(out, temp_path, out_len - 1);
        out[out_len - 1] = '\0';
        log_warn("Thermal", "Zone scan matched no %s sensor; falling back to thermal_zone%d (%s)",
                 sensor, indices[i], type);
        return;
    }
}

void scan_thermal_zones(void) {
    /* Score-based thermal zone selection to pick core sensor over broad SoC envelope */
    char best_cpu_path[256] = "";
    char best_bat_path[256] = "";
    char best_gpu_path[256] = "";
    char best_chg_path[256] = "";
    int  best_cpu_score = 0;
    int  best_bat_score = 0;
    int  best_gpu_score = 0;
    int  best_chg_score = 0;

    DIR *d = opendir("/sys/class/thermal");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strncmp(ent->d_name, "thermal_zone", 12) != 0) continue;

            char path[256];
            snprintf(path, sizeof(path), "/sys/class/thermal/%s/type", ent->d_name);
            int fd = open(path, O_RDONLY | O_CLOEXEC);
            if (fd < 0) continue;

            char type[64];
            ssize_t n = read(fd, type, sizeof(type) - 1);
            close(fd);
            if (n <= 0) continue;
            type[n] = '\0';

            char temp_path[256];
            snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/%s/temp", ent->d_name);

            /* Selection is driven by the zone's type, so a temp node that has not
             * published its first sample yet must not disqualify it. Many MTK
             * zones legitimately read 0 for the first few seconds after boot,
             * and skipping those meant the real CPU/battery zones lost to
             * whichever zone happened to be warm at scan time. A dead node that
             * does get selected now degrades safely: update_thermal_guard()
             * holds the current tier instead of reading 0 as a cold SoC. */

            int cpu_score = 0;
            if (strstr(type, "cpu_big") || strstr(type, "cpu-big")) cpu_score = 14;
            else if (strstr(type, "cpu-0"))                         cpu_score = 12;
            else if (strstr(type, "cpu0"))                          cpu_score = 11;
            else if (strstr(type, "cpu-1"))                         cpu_score = 10;
            else if (strstr(type, "cpu_little") || strstr(type, "cpu-little")) cpu_score = 9;
            else if (strstr(type, "cpu"))                           cpu_score = 8;
            else if (strstr(type, "CPU"))                           cpu_score = 7;
            else if (strstr(type, "soc-max"))                       cpu_score = 6;
            else if (strstr(type, "soc"))                           cpu_score = 4;

            if (cpu_score > best_cpu_score) {
                best_cpu_score = cpu_score;
                strncpy(best_cpu_path, temp_path, sizeof(best_cpu_path) - 1);
                best_cpu_path[sizeof(best_cpu_path) - 1] = '\0';
            }

            int bat_score = 0;
            if (strstr(type, "battery"))  bat_score = 10;
            else if (strstr(type, "bat")) bat_score = 8;

            if (bat_score > best_bat_score) {
                best_bat_score = bat_score;
                strncpy(best_bat_path, temp_path, sizeof(best_bat_path) - 1);
                best_bat_path[sizeof(best_bat_path) - 1] = '\0';
            }

            int gpu_score = 0;
            if (strstr(type, "gpu1") || strstr(type, "gpu2")) gpu_score = 12;
            else if (strstr(type, "gpu") || strstr(type, "GPU")) gpu_score = 10;
            else if (strstr(type, "mali")) gpu_score = 8;

            if (gpu_score > best_gpu_score) {
                best_gpu_score = gpu_score;
                strncpy(best_gpu_path, temp_path, sizeof(best_gpu_path) - 1);
                best_gpu_path[sizeof(best_gpu_path) - 1] = '\0';
            }

            int chg_score = 0;
            if (strstr(type, "charge_therm") || strstr(type, "chg_therm")) chg_score = 12;
            else if (strstr(type, "charger") || strstr(type, "charge")) chg_score = 8;

            if (chg_score > best_chg_score) {
                best_chg_score = chg_score;
                strncpy(best_chg_path, temp_path, sizeof(best_chg_path) - 1);
                best_chg_path[sizeof(best_chg_path) - 1] = '\0';
            }
        }
        closedir(d);
    }

    if (best_cpu_path[0] != '\0') {
        strncpy(g_nodes.cpu_temp, best_cpu_path, sizeof(g_nodes.cpu_temp) - 1);
        g_nodes.cpu_temp[sizeof(g_nodes.cpu_temp) - 1] = '\0';
        log_info("Thermal", "CPU thermal zone selected (score=%d): %s", best_cpu_score, g_nodes.cpu_temp);
    }
    if (best_bat_path[0] != '\0') {
        strncpy(g_nodes.bat_temp, best_bat_path, sizeof(g_nodes.bat_temp) - 1);
        g_nodes.bat_temp[sizeof(g_nodes.bat_temp) - 1] = '\0';
    }
    if (best_gpu_path[0] != '\0') {
        strncpy(g_nodes.gpu_temp, best_gpu_path, sizeof(g_nodes.gpu_temp) - 1);
        g_nodes.gpu_temp[sizeof(g_nodes.gpu_temp) - 1] = '\0';
        log_info("Thermal", "GPU thermal zone selected (score=%d): %s", best_gpu_score, g_nodes.gpu_temp);
    }
    if (best_chg_path[0] != '\0') {
        strncpy(g_nodes.chg_temp, best_chg_path, sizeof(g_nodes.chg_temp) - 1);
        g_nodes.chg_temp[sizeof(g_nodes.chg_temp) - 1] = '\0';
        log_info("Thermal", "Charger thermal zone selected (score=%d): %s", best_chg_score, g_nodes.chg_temp);
    }

    if (g_nodes.cpu_temp[0] == '\0')
        pick_fallback_zone(g_nodes.cpu_temp, sizeof(g_nodes.cpu_temp), "cpu",
                           (const int[]){16, 0}, 2, "cpu", "soc", NULL);
    if (g_nodes.bat_temp[0] == '\0')
        pick_fallback_zone(g_nodes.bat_temp, sizeof(g_nodes.bat_temp), "battery",
                           (const int[]){25, 1}, 2, "bat", NULL);
    if (g_nodes.gpu_temp[0] == '\0')
        pick_fallback_zone(g_nodes.gpu_temp, sizeof(g_nodes.gpu_temp), "gpu",
                           (const int[]){10, 9}, 2, "gpu", "mali", NULL);
    if (g_nodes.chg_temp[0] == '\0') {
        pick_fallback_zone(g_nodes.chg_temp, sizeof(g_nodes.chg_temp), "charger",
                           (const int[]){17}, 1, "chg", "charg", "charge", NULL);
        if (g_nodes.chg_temp[0] == '\0') {
            if (access("/sys/class/power_supply/mtk-master-charger/temp", F_OK) == 0)
                strcpy(g_nodes.chg_temp, "/sys/class/power_supply/mtk-master-charger/temp");
            else if (access("/sys/class/power_supply/charger/temp", F_OK) == 0)
                strcpy(g_nodes.chg_temp, "/sys/class/power_supply/charger/temp");
        }
    }

    /* Discover the GPU devfreq cooling device so thermal downclocking during
     * gaming can be undone.
     *
     * "thermal-devfreq" is the generic cpufreq/dvfsrc cooler type and matches
     * the CPU policies too, so it cannot be treated as a GPU hint. Matching it
     * first meant whichever cooling_deviceN happened to be numbered lowest won,
     * and gaming would then force cur_state 0 onto a CPU cooler every tick.
     * GPU-specific types win outright; the generic one is a fallback only. */
    int found = 0;
    for (int pass = 0; pass < 2 && !found; pass++) {
        for (int i = 0; i < 16; i++) {
            char type_path[256], cur_path[256];
            snprintf(type_path, sizeof(type_path), "/sys/class/thermal/cooling_device%d/type", i);
            snprintf(cur_path, sizeof(cur_path), "/sys/class/thermal/cooling_device%d/cur_state", i);
            if (access(type_path, F_OK) != 0) continue;

            char ctype[64] = "";
            sysfs_read_str(type_path, ctype, sizeof(ctype));

            int is_gpu = strstr(ctype, "mali") || strstr(ctype, "gpu");
            int is_generic = !is_gpu && strstr(ctype, "thermal-devfreq") != NULL;
            if (!(pass == 0 ? is_gpu : is_generic)) continue;

            strncpy(g_nodes.devfreq_cooler, cur_path, sizeof(g_nodes.devfreq_cooler) - 1);
            g_nodes.devfreq_cooler[sizeof(g_nodes.devfreq_cooler) - 1] = '\0';
            log_info("Thermal", "GPU Devfreq cooling device detected: %s (%s)", cur_path, ctype);
            found = 1;
            break;
        }
        if (found) break;
    }

    if (access("/sys/class/power_supply/battery/status", F_OK) == 0) {
        strcpy(g_nodes.bat_status, "/sys/class/power_supply/battery/status");
    } else {
        const char *st_paths[] = {
            "/sys/class/power_supply/bms/status",
            "/sys/class/power_supply/usb/status",
            "/sys/class/power_supply/main/status",
            NULL
        };
        for (int i = 0; st_paths[i]; i++) {
            if (access(st_paths[i], F_OK) == 0) {
                strcpy(g_nodes.bat_status, st_paths[i]);
                break;
            }
        }
    }
}

int check_charging_status(void) {
    if (g_nodes.bat_status[0] == '\0') return 0;
    int fd = open(g_nodes.bat_status, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return (strstr(buf, "Charging") != NULL);
}

/* Battery cycle persistence and auto-correction */
static int s_verified_cycles = 0;
static int s_cycles_loaded = 0;

static void save_verified_cycles(int cycles) {
    if (cycles <= 0 || cycles > 4000) return;
    char path[256];
    snprintf(path, sizeof(path), "%s/battery_cycle.conf", g_nodes.data_dir);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d\n", cycles);
        fclose(f);
    }
}

static void load_verified_cycles(void) {
    if (s_cycles_loaded) return;
    char path[256];
    snprintf(path, sizeof(path), "%s/battery_cycle.conf", g_nodes.data_dir);
    FILE *f = fopen(path, "r");
    if (!f && g_nodes.mod_dir[0] && strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        char old_path[256];
        snprintf(old_path, sizeof(old_path), "%s/battery_cycle.conf", g_nodes.mod_dir);
        f = fopen(old_path, "r");
        if (f) {
            if (fscanf(f, "%d", &s_verified_cycles) != 1 || s_verified_cycles < 0 || s_verified_cycles > 4000) {
                s_verified_cycles = 0;
            }
            fclose(f);
            unlink(old_path);
            save_verified_cycles(s_verified_cycles);
            s_cycles_loaded = 1;
            return;
        }
    }
    if (f) {
        if (fscanf(f, "%d", &s_verified_cycles) != 1 || s_verified_cycles < 0 || s_verified_cycles > 4000) {
            s_verified_cycles = 0;
        }
        fclose(f);
    }
    s_cycles_loaded = 1;
}

int get_true_battery_cycles(void) {
    load_verified_cycles();

    /* Hardware PMIC MT6366/MT6358 FG Gauge node (MediaTek MT6789 / BMS).
     * Strictly prioritize genuine fuel-gauge coulomb counters over
     * battery auth chip registers (which report raw signatures like 5662 on Xiaomi). */
    const char *hw_bms_nodes[] = {
        "/sys/devices/platform/soc/10026000.pwrap/10026000.pwrap:mt6366/mt6358-gauge/power_supply/bms/cycle_count",
        "/sys/class/power_supply/bms/cycle_count",
        NULL
    };

    int bms_val = 0;
    for (int i = 0; hw_bms_nodes[i]; i++) {
        if (access(hw_bms_nodes[i], F_OK) == 0) {
            int val = sysfs_read_int(hw_bms_nodes[i]);
            if (val > 0 && val <= 4000) {
                bms_val = val;
                break;
            }
        }
    }

    static int s_reset_candidate = 0;
    static int s_reset_count = 0;

    if (bms_val > 0) {
        if (s_verified_cycles == 0) {
            s_verified_cycles = bms_val;
            save_verified_cycles(s_verified_cycles);
        } else if (bms_val > s_verified_cycles && bms_val <= 4000) {
            /* Monotonic forward progression (+1 or incremental cycle increase) */
            s_verified_cycles = bms_val;
            save_verified_cycles(s_verified_cycles);
            log_info("Battery", "Battery cycle naturally incremented to %d (persisted)", s_verified_cycles);
            s_reset_count = 0;
        } else if (bms_val > 0 && bms_val < s_verified_cycles) {
            /* Battery replacement detection: 5 consecutive reads of lower cycle count confirms new cell */
            if (bms_val == s_reset_candidate) {
                if (++s_reset_count >= 5) {
                    s_verified_cycles = bms_val;
                    save_verified_cycles(s_verified_cycles);
                    log_info("Battery", "Battery replacement detected: cycle count recalibrated to %d", s_verified_cycles);
                    s_reset_count = 0;
                }
            } else {
                s_reset_candidate = bms_val;
                s_reset_count = 1;
            }
        }
        return s_verified_cycles;
    }

    /* Fallback: if device has no BMS node, check battery/cycle_count with strict filter */
    int bat_val = sysfs_read_int("/sys/class/power_supply/battery/cycle_count");
    if (bat_val > 0 && bat_val <= 3000) {
        if (s_verified_cycles == 0) {
            s_verified_cycles = bat_val;
            save_verified_cycles(s_verified_cycles);
        } else if (bat_val > s_verified_cycles && bat_val <= 3000) {
            s_verified_cycles = bat_val;
            save_verified_cycles(s_verified_cycles);
            log_info("Battery", "Battery cycle naturally incremented to %d (persisted)", s_verified_cycles);
        }
        return s_verified_cycles;
    }

    return s_verified_cycles;
}

void sync_battery_cycle_count(void) {
    int cycles = get_true_battery_cycles();
    if (cycles <= 0) return;

    char str_cycles[32];
    snprintf(str_cycles, sizeof(str_cycles), "%d", cycles);

    /* Target sysfs nodes that Android system services and third-party apps query */
    const char *dest_nodes[] = {
        "/sys/class/power_supply/battery/cycle_count",
        "/sys/devices/platform/hq_chg_manager/power_supply/battery/cycle_count",
        NULL
    };

    for (int i = 0; dest_nodes[i]; i++) {
        if (access(dest_nodes[i], F_OK) == 0) {
            int cur = sysfs_read_int(dest_nodes[i]);
            /* If node has abnormal signature (e.g. 5662) or is out of sync with genuine cycles */
            if (cur != cycles) {
                sysfs_write(dest_nodes[i], str_cycles);
                log_info("Battery", "Synced genuine battery cycles (%d) to %s (was %d)",
                         cycles, dest_nodes[i], cur);
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * HyperCore Dynamic Thermal Guard
 * --------------------------------------------------------------------------
 * Replaces aggressive vendor throttling (which kicks in at 35°C–39°C) with
 * balanced, high-headroom protection designed for tropical climates:
 *
 * Tier 0 (Optimal / Cool):
 *   - Normal operation below 45°C Battery & 70°C CPU.
 *   - 100% uncapped performance (Big 2.2 GHz, Little 2.0 GHz).
 *
 * Tier 1 (Warm / Active Mitigation):
 *   - Trigger: Bat >= 45°C OR CPU >= 70°C.
 *   - Clear:   Bat <= 42°C AND CPU <= 64°C (3°C hysteresis).
 *   - Action:  Cap Little at 1.8 GHz and Big at 1.8 GHz. MOBA titles keep Big
 *              at 2.0 GHz, because their frametime budget is tighter.
 *
 * Tier 2 (Hot / Safety Protection):
 *   - Trigger: Bat >= 48°C OR CPU >= 75°C.
 *   - Clear:   Bat <= 44°C AND CPU <= 68°C.
 *   - Action:  Cap both clusters at 1.8 GHz to arrest heat.
 *
 * Ceilings are implemented in build_profile_matrix() and are always clamped
 * down to the hardware's own maximum, so a part that reports a lower ceiling
 * is never pushed above it.
 * -------------------------------------------------------------------------- */
static int s_thermal_tier = 0;

int get_thermal_tier(void) {
    return s_thermal_tier;
}

int update_thermal_guard(int cpu_temp, int bat_temp) {
    int prev_tier = s_thermal_tier;

    /* A sensor reporting 0 has not produced a sample — that is not the same as
     * "cold". The raw reads used to go straight into the comparisons below, so a
     * failed or not-yet-sampled node read as a comfortable 0°C and pinned the
     * device at tier 0 with the vendor thermal stack suppressed, which is the
     * exact state that gets a hot SoC cooked. Judge on whichever sample
     * actually arrived, and hold the current tier when neither did. */
    int have_cpu = cpu_temp > 0;
    int have_bat = bat_temp > 0;
    if (!have_cpu && !have_bat) {
        return 0; /* no usable data this tick: keep the tier we already had */
    }

    int hot, warm, mid, cool, calm;
    if (have_cpu && have_bat) {
        hot  = cpu_temp >= 75 || bat_temp >= 48;
        warm = cpu_temp >= 70 || bat_temp >= 45;
        mid  = cpu_temp >= 65 || bat_temp >= 42;
        calm = cpu_temp <= 68 && bat_temp <= 44;
        cool = cpu_temp <= 64 && bat_temp <= 42;
    } else if (have_cpu) {
        hot = cpu_temp >= 75; warm = cpu_temp >= 70; mid = cpu_temp >= 65;
        calm = cpu_temp <= 68; cool = cpu_temp <= 64;
    } else {
        hot = bat_temp >= 48; warm = bat_temp >= 45; mid = bat_temp >= 42;
        calm = bat_temp <= 44; cool = bat_temp <= 42;
    }

    if (s_thermal_tier == 2) {
        if (calm) {
            s_thermal_tier = mid ? 1 : 0;
        }
    } else if (s_thermal_tier == 1) {
        if (hot) {
            s_thermal_tier = 2;
        } else if (cool) {
            s_thermal_tier = 0;
        }
    } else {
        if (hot) {
            s_thermal_tier = 2;
        } else if (warm) {
            s_thermal_tier = 1;
        }
    }

    g_state.thermal_tier = s_thermal_tier;

    if (s_thermal_tier != prev_tier) {
        log_warn("Thermal", "Thermal Guard: Tier %d -> Tier %d (CPU: %d°C, Bat: %d°C)",
                 prev_tier, s_thermal_tier, cpu_temp, bat_temp);
        return 1; /* Tier transition occurred */
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * Gaming Thermal Bypass Engine  (tier 0 only)
 * --------------------------------------------------------------------------
 * While the SoC is cool, stop over-eager vendor throttling from costing frames:
 * 1. Enforces Xiaomi sconfig 10 (thermal-nolimits.conf) against mi_thermald resets
 * 2. Suppresses Xiaomi cpu_limits injection
 * 3. Prevents MediaTek FPSGO thermal frametime degradation (thrm_enable = 0)
 * 4. Clears GPU devfreq cooler state to prevent Mali downclocking
 * 5. Restores scaling_max_freq if vendor thermald artificially caps CPU clocks
 *
 * At tier >= 1 items 1-4 are skipped: we stop fighting the vendor thermal stack
 * and rely on the frequency ceilings in build_profile_matrix(). See
 * update_thermal_guard() for the tier definitions.
 * -------------------------------------------------------------------------- */
void enforce_gaming_thermal_bypass(profile_t prof, int tier) {
    if (prof != PROFILE_Gaming && prof != PROFILE_Gaming_MOBA) return;

    /* Frequency ceilings live in build_profile_matrix() (src/cpu.c), which is
     * the single source of truth and already clamps both clusters at every
     * tier. An earlier copy of that table lived here and was unreachable: the
     * tier >= 1 bail-out below ran before the values were ever used, so the
     * tier-1/tier-2 numbers were computed and thrown away while the header
     * comment claimed they were enforced. Two divergent tables for one guard
     * is how a 70°C device ends up uncapped, so only one remains.
     *
     * At tier >= 1 we stop suppressing the vendor thermal stack entirely and
     * rely on those ceilings. The bypass below exists to stop over-eager
     * throttling during gameplay while the SoC is genuinely cool — it is not a
     * substitute for thermal protection, and winning the race against
     * mi_thermald at 75°C is how you cook the device. */
    if (tier >= 1) return;

    int target_lit_max = (g_nodes.lit_hw_max_freq > 0) ? g_nodes.lit_hw_max_freq : 2000000;
    int target_big_max = (g_nodes.big_hw_max_freq > 0) ? g_nodes.big_hw_max_freq : 2200000;

    /* 1. Enforce Xiaomi sconfig 10 (thermal-nolimits.conf) */
    char sconfig_val[32] = "";
    if (sysfs_read_str("/sys/class/thermal/thermal_message/sconfig", sconfig_val, sizeof(sconfig_val))) {
        if (strcmp(sconfig_val, "10") != 0) {
            chmod("/sys/class/thermal/thermal_message/sconfig", 0666);
            sysfs_write("/sys/class/thermal/thermal_message/sconfig", "10");
        }
    }
    if (sysfs_read_str("/sys/devices/virtual/thermal/thermal_message/sconfig", sconfig_val, sizeof(sconfig_val))) {
        if (strcmp(sconfig_val, "10") != 0) {
            chmod("/sys/devices/virtual/thermal/thermal_message/sconfig", 0666);
            sysfs_write("/sys/devices/virtual/thermal/thermal_message/sconfig", "10");
        }
    }

    /* 2. Clear Xiaomi cpu_limits message */
    char cpu_limits[64] = "";
    if (sysfs_read_str("/sys/class/thermal/thermal_message/cpu_limits", cpu_limits, sizeof(cpu_limits))) {
        if (cpu_limits[0] != '\0') {
            chmod("/sys/class/thermal/thermal_message/cpu_limits", 0666);
            sysfs_write("/sys/class/thermal/thermal_message/cpu_limits", "");
        }
    }

    /* 3. MediaTek FPSGO thermal bypass */
    char fpsgo_thrm[16] = "";
    if (sysfs_read_str("/sys/kernel/fpsgo/fbt/thrm_enable", fpsgo_thrm, sizeof(fpsgo_thrm))) {
        if (strcmp(fpsgo_thrm, "0") != 0) {
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");
        }
    }

    /* 4. Reset GPU Devfreq cooler if vendor thermal attempted to throttle */
    if (g_nodes.devfreq_cooler[0] != '\0') {
        int cooler_state = sysfs_read_int(g_nodes.devfreq_cooler);
        if (cooler_state > 0) {
            sysfs_write(g_nodes.devfreq_cooler, "0");
        }
    }

    /* 5. CPU scaling_max_freq protection: restore any cap the vendor thermal
     *    daemon applied. Only ever raises a ceiling, and only up to what the
     *    active tier permits. */
    int cur_lit_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    if (cur_lit_max > 0 && cur_lit_max < target_lit_max) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", target_lit_max);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", buf);
    }

    const char *big_policies[] = {
        "/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq",
        "/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq",
        "/sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq",
        NULL
    };
    for (int i = 0; big_policies[i]; i++) {
        if (access(big_policies[i], F_OK) == 0) {
            int cur_big_max = sysfs_read_int(big_policies[i]);
            if (cur_big_max > 0 && cur_big_max < target_big_max) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", target_big_max);
                sysfs_write(big_policies[i], buf);
            }
            break;
        }
    }
}




