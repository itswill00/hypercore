
#include "thermal.hpp"
#include "sysfs.hpp"
#include "log.hpp"

void scan_thermal_zones(void) {
    /* [M-3 FIX] readdir() order is non-deterministic. The original first-match
     * approach could pick a zone whose type contains "soc" but represents the
     * entire SoC envelope (10-15 degC cooler than CPU cores), causing thermal
     * tier under-estimation during gaming. Use score-based selection: more
     * specific zone type names get higher scores, ensuring the best candidate
     * is chosen regardless of readdir() ordering. */
    char best_cpu_path[256] = "";
    char best_bat_path[256] = "";
    int  best_cpu_score = 0;
    int  best_bat_score = 0;

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

            /* Score CPU thermal zone candidates — higher = more specific */
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
            }

            /* Score battery thermal zone candidates */
            int bat_score = 0;
            if (strstr(type, "battery"))  bat_score = 10;
            else if (strstr(type, "bat")) bat_score = 8;

            if (bat_score > best_bat_score) {
                best_bat_score = bat_score;
                strncpy(best_bat_path, temp_path, sizeof(best_bat_path) - 1);
            }
        }
        closedir(d);
    }

    if (best_cpu_path[0] != '\0') {
        strncpy(g_nodes.cpu_temp, best_cpu_path, sizeof(g_nodes.cpu_temp) - 1);
        log_info("Thermal", "CPU thermal zone selected (score=%d): %s", best_cpu_score, g_nodes.cpu_temp);
    }
    if (best_bat_path[0] != '\0') {
        strncpy(g_nodes.bat_temp, best_bat_path, sizeof(g_nodes.bat_temp) - 1);
    }

    if (g_nodes.cpu_temp[0] == '\0') {
        strcpy(g_nodes.cpu_temp, access("/sys/class/thermal/thermal_zone16/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone16/temp" : "/sys/class/thermal/thermal_zone0/temp");
    }
    if (g_nodes.bat_temp[0] == '\0') {
        strcpy(g_nodes.bat_temp, access("/sys/class/thermal/thermal_zone25/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone25/temp" : "/sys/class/thermal/thermal_zone1/temp");
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

int calculate_thermal_tier(int cpu_temp, int bat_temp) {
    int tier = g_state.thermal_tier;
    int is_chg = g_state.is_charging;

    int t3_cpu = is_chg ? 72 : 75;
    int t3_bat = is_chg ? 50 : 53;

    if (cpu_temp >= t3_cpu || bat_temp >= t3_bat) {
        tier = 3;
        g_state.thermal_hold_ticks = 3;
    } else if (g_state.thermal_hold_ticks > 0) {
        g_state.thermal_hold_ticks--;
        tier = 3;
    } else if (g_state.thermal_tier == 3) {
        if (cpu_temp <= 64 && bat_temp <= 46) {
            tier = (cpu_temp >= 58 || bat_temp >= 44) ? 2 : 1;
        }
    } else if (cpu_temp >= 65 || bat_temp >= 48) {
        tier = 2;
    } else if (g_state.thermal_tier == 2) {
        if (cpu_temp < 58 && bat_temp < 44) {
            tier = (cpu_temp >= 54 || bat_temp >= 42) ? 1 : 0;
        }
    } else if (g_state.thermal_tier == 1) {
        if (cpu_temp < 52 && bat_temp < 40) {
            tier = 0;
        }
    } else if (cpu_temp >= 58 || bat_temp >= 44) {
        tier = 1;
    } else {
        tier = 0;
    }

    return tier;
}

static float s_accumulated_pct = 0.0f;
static int   s_prev_cap_for_cycle = -1;
static int   s_added_cycles = 0;
static int   s_tracker_loaded = 0;

static void load_cycle_tracker(void) {
    if (s_tracker_loaded) return;
    char path[256];
    snprintf(path, sizeof(path), "%s/cycle_tracker.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "r");
    if (f) {
        if (fscanf(f, "%d %f", &s_added_cycles, &s_accumulated_pct) != 2) {
            s_added_cycles = 0;
            s_accumulated_pct = 0.0f;
        }
        fclose(f);
    }
    s_tracker_loaded = 1;
}

static void save_cycle_tracker(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/cycle_tracker.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%d %.2f\n", s_added_cycles, s_accumulated_pct);
    fclose(f);
}

void track_charging_cycles(int is_charging) {
    load_cycle_tracker();

    int current_cap = sysfs_read_int("/sys/class/power_supply/battery/capacity");
    if (current_cap <= 0) {
        current_cap = sysfs_read_int("/sys/class/power_supply/bms/capacity");
    }

    if (is_charging && s_prev_cap_for_cycle > 0 && current_cap > s_prev_cap_for_cycle) {
        int delta = current_cap - s_prev_cap_for_cycle;
        if (delta > 0 && delta <= 50) {
            s_accumulated_pct += (float)delta;
            if (s_accumulated_pct >= 100.0f) {
                int new_added = (int)(s_accumulated_pct / 100.0f);
                s_added_cycles += new_added;
                s_accumulated_pct -= (float)(new_added * 100);
                log_info("Battery", "Dynamic full charge cycle reached! Total added cycles: %d", s_added_cycles);
            }
            save_cycle_tracker();
        }
    }
    s_prev_cap_for_cycle = current_cap;
}

int get_true_battery_cycles(void) {
    load_cycle_tracker();

    int base_cycles = 0;

    /* Hardware PMIC MT6366/MT6358 FG Gauge node (HuaQin / MTK MT6789) */
    const char *hw_gauge_nodes[] = {
        "/sys/devices/platform/soc/10026000.pwrap/10026000.pwrap:mt6366/mt6358-gauge/power_supply/bms/cycle_count",
        "/sys/class/power_supply/bms/cycle_count",
        "/sys/class/power_supply/battery/cycle_count",
        "/sys/class/power_supply/battery/fg1_cycle",
        "/sys/class/power_supply/battery/auth_dev_batt_cycle",
        NULL
    };

    for (int i = 0; hw_gauge_nodes[i]; i++) {
        if (access(hw_gauge_nodes[i], F_OK) == 0) {
            int val = sysfs_read_int(hw_gauge_nodes[i]);
            if (val > 0) {
                /* Hardware PMIC node (index 0) returns exact unscaled cycle count.
                 * Legacy sysfs nodes > 1000 may require /16 scaling. */
                if (val > 1000 && i > 0) {
                    base_cycles = val / 16;
                } else {
                    base_cycles = val;
                }
                break;
            }
        }
    }

    return base_cycles + s_added_cycles;
}

void fix_battery_cycle_count(void) {
    static time_t s_last_fix_time = 0;
    static int s_last_synced_cycles[4] = {-1, -1, -1, -1};
    time_t now = time(NULL);
    if (now - s_last_fix_time < 300) return;
    s_last_fix_time = now;

    /* Device-specific hardware safety guard:
     * Only perform cycle count correction if the MT6366/MT6358 PMIC gauge node
     * or the HuaQin charger manager node exists on this specific device.
     * On other devices, skip forced cycle correction to avoid modifying non-matching sysfs nodes. */
    const char *spec_pmic_src = "/sys/devices/platform/soc/10026000.pwrap/10026000.pwrap:mt6366/mt6358-gauge/power_supply/bms/cycle_count";
    const char *spec_hq_dest  = "/sys/devices/platform/hq_chg_manager/power_supply/battery/cycle_count";

    if (access(spec_pmic_src, F_OK) != 0 && access(spec_hq_dest, F_OK) != 0) {
        return;
    }

    int true_cycles = get_true_battery_cycles();
    if (true_cycles <= 0) return;

    /* Destination nodes to update with true cycle count */
    const char *dest_nodes[] = {
        spec_hq_dest,
        "/sys/class/power_supply/battery/cycle_count",
        "/sys/class/power_supply/battery/auth_dev_batt_cycle",
        NULL
    };

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", true_cycles);

    for (int i = 0; dest_nodes[i]; i++) {
        if (access(dest_nodes[i], F_OK) == 0) {
            int current_val = sysfs_read_int(dest_nodes[i]);
            if (current_val == true_cycles || s_last_synced_cycles[i] == true_cycles) {
                s_last_synced_cycles[i] = true_cycles;
                continue;
            }

            int fd = open(dest_nodes[i], O_WRONLY | O_CLOEXEC);
            if (fd >= 0) {
                ssize_t w = write(fd, buf, strlen(buf));
                close(fd);
                if (w > 0) {
                    s_last_synced_cycles[i] = true_cycles;
                    log_info("Battery", "Synced battery cycle count (%d cycles) to %s", true_cycles, dest_nodes[i]);
                } else {
                    log_warn("Battery", "Failed to write cycle count to %s (errno=%d)", dest_nodes[i], errno);
                }
            } else {
                log_warn("Battery", "Failed to open %s (errno=%d)", dest_nodes[i], errno);
            }
        }
    }
}




