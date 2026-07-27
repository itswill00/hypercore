/*
 * HyperCore - Thermal Zone Scanner & Hysteresis Implementation
 * Author: @itswill00
 */

#include "thermal.hpp"

void scan_thermal_zones(void) {
    DIR *d = opendir("/sys/class/thermal");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strncmp(ent->d_name, "thermal_zone", 12) != 0) continue;

            char path[256];
            snprintf(path, sizeof(path), "/sys/class/thermal/%s/type", ent->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;

            char type[64];
            ssize_t n = read(fd, type, sizeof(type) - 1);
            close(fd);

            if (n <= 0) continue;
            type[n] = '\0';

            if (g_nodes.cpu_temp[0] == '\0' && (strstr(type, "cpu") || strstr(type, "soc") || strstr(type, "CPU"))) {
                snprintf(g_nodes.cpu_temp, sizeof(g_nodes.cpu_temp), "/sys/class/thermal/%s/temp", ent->d_name);
            }
            if (g_nodes.bat_temp[0] == '\0' && (strstr(type, "bat") || strstr(type, "battery"))) {
                snprintf(g_nodes.bat_temp, sizeof(g_nodes.bat_temp), "/sys/class/thermal/%s/temp", ent->d_name);
            }
        }
        closedir(d);
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
    }
}

int check_charging_status(void) {
    if (g_nodes.bat_status[0] == '\0') return 0;
    int fd = open(g_nodes.bat_status, O_RDONLY);
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

    int t3_cpu = is_chg ? 58 : 56;
    int t3_bat = is_chg ? 47 : 45;

    if (cpu_temp >= t3_cpu || bat_temp >= t3_bat) {
        tier = 3;
        g_state.thermal_hold_ticks = 4;
    } else if (g_state.thermal_hold_ticks > 0) {
        g_state.thermal_hold_ticks--;
        tier = 3;
    } else if (g_state.thermal_tier == 3) {
        if (cpu_temp <= 51 && bat_temp <= 43) {
            tier = (cpu_temp >= 48 || bat_temp >= 41) ? 2 : 1;
        }
    } else if (cpu_temp >= 51 || bat_temp >= 44) {
        tier = 2;
    } else if (g_state.thermal_tier == 2) {
        if (cpu_temp < 47 && bat_temp < 41) {
            tier = (cpu_temp >= 43 || bat_temp >= 39) ? 1 : 0;
        }
    } else if (cpu_temp >= 43 || bat_temp >= 39) {
        tier = 1;
    } else {
        tier = 0;
    }

    return tier;
}
