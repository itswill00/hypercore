
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

int get_true_battery_cycles(void) {
    /* Hardware PMIC MT6366/MT6358 FG Gauge node (MediaTek MT6789 / BMS) */
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
                return val;
            }
        }
    }

    return 0;
}




