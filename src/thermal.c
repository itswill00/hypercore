
#include "thermal.hpp"
#include "sysfs.hpp"
#include "log.hpp"

void scan_thermal_zones(void) {
    /* Score-based thermal zone selection to pick core sensor over broad SoC envelope */
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

/* Battery cycle persistence and auto-correction */
static int s_verified_cycles = 0;
static int s_cycles_loaded = 0;

static void load_verified_cycles(void) {
    if (s_cycles_loaded) return;
    char path[256];
    snprintf(path, sizeof(path), "%s/battery_cycle.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "r");
    if (f) {
        if (fscanf(f, "%d", &s_verified_cycles) != 1 || s_verified_cycles < 0 || s_verified_cycles > 4000) {
            s_verified_cycles = 0;
        }
        fclose(f);
    }
    s_cycles_loaded = 1;
}

static void save_verified_cycles(int cycles) {
    if (cycles <= 0 || cycles > 4000) return;
    char path[256];
    snprintf(path, sizeof(path), "%s/battery_cycle.conf", g_nodes.mod_dir);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d\n", cycles);
        fclose(f);
    }
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




