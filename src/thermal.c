
#include "thermal.hpp"
#include "sysfs.hpp"
#include "log.hpp"

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
            int val = sysfs_read_int(temp_path);
            if (val <= 0) continue;

            int cpu_score = 0;
            if (strstr(type, "cpu-0"))        cpu_score = 12;
            else if (strstr(type, "cpu0"))     cpu_score = 11;
            else if (strstr(type, "cpu-1"))    cpu_score = 10;
            else if (strstr(type, "cpu"))      cpu_score = 8;
            else if (strstr(type, "CPU"))      cpu_score = 7;
            else if (strstr(type, "soc-max"))  cpu_score = 6;
            else if (strstr(type, "soc"))      cpu_score = 4;

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

    if (g_nodes.cpu_temp[0] == '\0') {
        strcpy(g_nodes.cpu_temp, access("/sys/class/thermal/thermal_zone16/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone16/temp" : "/sys/class/thermal/thermal_zone0/temp");
    }
    if (g_nodes.bat_temp[0] == '\0') {
        strcpy(g_nodes.bat_temp, access("/sys/class/thermal/thermal_zone25/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone25/temp" : "/sys/class/thermal/thermal_zone1/temp");
    }
    if (g_nodes.gpu_temp[0] == '\0') {
        if (access("/sys/class/thermal/thermal_zone10/temp", F_OK) == 0)
            strcpy(g_nodes.gpu_temp, "/sys/class/thermal/thermal_zone10/temp");
        else if (access("/sys/class/thermal/thermal_zone9/temp", F_OK) == 0)
            strcpy(g_nodes.gpu_temp, "/sys/class/thermal/thermal_zone9/temp");
    }
    if (g_nodes.chg_temp[0] == '\0') {
        if (access("/sys/class/thermal/thermal_zone17/temp", F_OK) == 0)
            strcpy(g_nodes.chg_temp, "/sys/class/thermal/thermal_zone17/temp");
        else if (access("/sys/class/power_supply/mtk-master-charger/temp", F_OK) == 0)
            strcpy(g_nodes.chg_temp, "/sys/class/power_supply/mtk-master-charger/temp");
        else if (access("/sys/class/power_supply/charger/temp", F_OK) == 0)
            strcpy(g_nodes.chg_temp, "/sys/class/power_supply/charger/temp");
    }

    /* Discover GPU devfreq cooling device to prevent thermal downclocking during gaming */
    for (int i = 0; i < 16; i++) {
        char type_path[256], cur_path[256];
        snprintf(type_path, sizeof(type_path), "/sys/class/thermal/cooling_device%d/type", i);
        snprintf(cur_path, sizeof(cur_path), "/sys/class/thermal/cooling_device%d/cur_state", i);
        if (access(type_path, F_OK) != 0) continue;

        char ctype[64] = "";
        sysfs_read_str(type_path, ctype, sizeof(ctype));
        if (strstr(ctype, "thermal-devfreq") || strstr(ctype, "mali") || strstr(ctype, "gpu")) {
            strncpy(g_nodes.devfreq_cooler, cur_path, sizeof(g_nodes.devfreq_cooler) - 1);
            log_info("Thermal", "GPU Devfreq cooling device detected: %s (%s)", cur_path, ctype);
            break;
        }
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
 *   - Action:  Gracefully cap Big cores at 1.8 GHz, Little at 1.8 GHz.
 *              No core hotplug, zero micro-stutter.
 *
 * Tier 2 (Hot / Safety Protection):
 *   - Trigger: Bat >= 48°C OR CPU >= 75°C.
 *   - Clear:   Bat <= 44°C AND CPU <= 68°C.
 *   - Action:  Cap Big cores at 1.5 GHz, Little at 1.4 GHz to arrest heat.
 * -------------------------------------------------------------------------- */
static int s_thermal_tier = 0;

int get_thermal_tier(void) {
    return s_thermal_tier;
}

int update_thermal_guard(int cpu_temp, int bat_temp) {
    int prev_tier = s_thermal_tier;

    if (s_thermal_tier == 2) {
        if (bat_temp <= 44 && cpu_temp <= 68) {
            s_thermal_tier = (bat_temp >= 42 || cpu_temp >= 65) ? 1 : 0;
        }
    } else if (s_thermal_tier == 1) {
        if (bat_temp >= 48 || cpu_temp >= 75) {
            s_thermal_tier = 2;
        } else if (bat_temp <= 42 && cpu_temp <= 64) {
            s_thermal_tier = 0;
        }
    } else {
        if (bat_temp >= 48 || cpu_temp >= 75) {
            s_thermal_tier = 2;
        } else if (bat_temp >= 45 || cpu_temp >= 70) {
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
 * Gaming Thermal Bypass Engine
 * --------------------------------------------------------------------------
 * Active throttling mitigation during gaming workloads:
 * 1. Enforces Xiaomi sconfig 10 (thermal-nolimits.conf) against mi_thermald resets
 * 2. Suppresses Xiaomi cpu_limits injection
 * 3. Prevents MediaTek FPSGO thermal frametime degradation (thrm_enable = 0)
 * 4. Clears GPU devfreq cooler state to prevent Mali downclocking
 * 5. Restores scaling_max_freq if vendor thermald artificially caps CPU clocks
 * -------------------------------------------------------------------------- */
void enforce_gaming_thermal_bypass(profile_t prof, int tier) {
    if (prof != PROFILE_Gaming && prof != PROFILE_Gaming_MOBA) return;

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

    /* 5. CPU scaling_max_freq protection: prevent mi_thermald from capping clocks */
    int target_lit_max = (tier == 2) ? 1800000 : (g_nodes.lit_hw_max_freq > 0 ? g_nodes.lit_hw_max_freq : 2000000);
    int target_big_max = (tier == 2) ? 1800000 : ((tier == 1 && prof == PROFILE_Gaming_MOBA) ? 2000000 : (g_nodes.big_hw_max_freq > 0 ? g_nodes.big_hw_max_freq : 2200000));

    int cur_lit_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    if (cur_lit_max > 0 && cur_lit_max < target_lit_max) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", target_lit_max);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", buf);
    }

    int cur_big_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq");
    if (cur_big_max > 0 && cur_big_max < target_big_max) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", target_big_max);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq", buf);
    }
}




