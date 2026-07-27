/*
 * Tanzanite HyperFlow Native C Engine v2.5 (Smooth UI & Latency Optimized)
 * Ultimate Performance & Latency Tuner for MediaTek Helio G99 Ultra
 * Author: @itswill00
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_FILE      "/sdcard/Android/tanzanite_hyperflow.log"
#define LOCK_FILE_MOD "/data/adb/modules/tanzanite_hyperflow/.hyperflow_lock"
#define LOCK_FILE_ALT "/sdcard/Android/.hyperflow_lock"
#define PID_FILE      "/data/adb/modules/tanzanite_hyperflow/engine.pid"

#define LITTLE_MAX 2000000
#define BIG_MAX    2200000

static volatile int running = 1;

static char cpu_tz_path[256] = "";
static char bat_tz_path[256] = "";
static char backlight_path[256] = "";
static char lock_file_path[256] = "";

static int has_sugov_ext = 0;
static int has_schedutil = 0;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static void write_node(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, strlen(val));
        close(fd);
    }
}

static int read_int(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return atoi(buf);
}

static void log_msg(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(f, "%s %s\n", time_str, msg);
    fclose(f);
}

static void rotate_log(void) {
    struct stat st;
    if (stat(LOG_FILE, &st) == 0 && st.st_size > 102400) {
        FILE *fin = fopen(LOG_FILE, "r");
        if (!fin) return;
        char lines[300][256];
        int count = 0;
        char buf[256];
        while (fgets(buf, sizeof(buf), fin)) {
            strncpy(lines[count % 300], buf, sizeof(lines[0]) - 1);
            count++;
        }
        fclose(fin);

        FILE *fout = fopen(LOG_FILE, "w");
        if (!fout) return;
        int start = (count > 200) ? (count - 200) : 0;
        for (int i = start; i < count; i++) {
            fputs(lines[i % 300], fout);
        }
        fclose(fout);
    }
}

static void discover_nodes(void) {
    DIR *d = opendir("/sys/class/thermal");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "thermal_zone", 12) == 0) {
                char type_path[256];
                snprintf(type_path, sizeof(type_path), "/sys/class/thermal/%s/type", dir->d_name);
                int fd = open(type_path, O_RDONLY);
                if (fd >= 0) {
                    char type_buf[64];
                    ssize_t n = read(fd, type_buf, sizeof(type_buf) - 1);
                    close(fd);
                    if (n > 0) {
                        type_buf[n] = '\0';
                        if (cpu_tz_path[0] == '\0' && (strstr(type_buf, "cpu") || strstr(type_buf, "soc") || strstr(type_buf, "CPU"))) {
                            snprintf(cpu_tz_path, sizeof(cpu_tz_path), "/sys/class/thermal/%s/temp", dir->d_name);
                        }
                        if (bat_tz_path[0] == '\0' && (strstr(type_buf, "bat") || strstr(type_buf, "battery"))) {
                            snprintf(bat_tz_path, sizeof(bat_tz_path), "/sys/class/thermal/%s/temp", dir->d_name);
                        }
                    }
                }
            }
        }
        closedir(d);
    }

    if (cpu_tz_path[0] == '\0') {
        if (access("/sys/class/thermal/thermal_zone16/temp", F_OK) == 0)
            strcpy(cpu_tz_path, "/sys/class/thermal/thermal_zone16/temp");
        else
            strcpy(cpu_tz_path, "/sys/class/thermal/thermal_zone0/temp");
    }

    if (bat_tz_path[0] == '\0') {
        if (access("/sys/class/thermal/thermal_zone25/temp", F_OK) == 0)
            strcpy(bat_tz_path, "/sys/class/thermal/thermal_zone25/temp");
        else
            strcpy(bat_tz_path, "/sys/class/thermal/thermal_zone1/temp");
    }

    const char *bl_candidates[] = {
        "/sys/class/backlight/panel0-backlight/brightness",
        "/sys/class/backlight/lcd-backlight/brightness",
        "/sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness",
        "/sys/class/leds/lcd-backlight/brightness",
        NULL
    };
    for (int i = 0; bl_candidates[i]; i++) {
        if (access(bl_candidates[i], F_OK) == 0) {
            strcpy(backlight_path, bl_candidates[i]);
            break;
        }
    }

    if (access("/data/adb/modules/tanzanite_hyperflow", F_OK) == 0) {
        strcpy(lock_file_path, LOCK_FILE_MOD);
    } else {
        strcpy(lock_file_path, LOCK_FILE_ALT);
    }

    if (access("/sys/devices/system/cpu/cpu0/cpufreq/sugov_ext", F_OK) == 0) has_sugov_ext = 1;
    if (access("/sys/devices/system/cpu/cpufreq/schedutil", F_OK) == 0) has_schedutil = 1;
}

static void set_cpu_rate_limits(int cpu, const char *up, const char *down) {
    char path[256];
    if (has_sugov_ext) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/up_rate_limit_us", cpu);
        write_node(path, up);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/down_rate_limit_us", cpu);
        write_node(path, down);
    }
    if (has_schedutil) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us");
        write_node(path, up);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us");
        write_node(path, down);
    }
}

static void apply_base_tuning(void) {
    write_node("/proc/sys/vm/swappiness", "65");
    write_node("/proc/sys/vm/dirty_ratio", "15");
    write_node("/proc/sys/vm/dirty_background_ratio", "5");
    write_node("/proc/sys/vm/vfs_cache_pressure", "90");
    write_node("/proc/sys/vm/stat_interval", "2");
    write_node("/proc/sys/vm/compaction_proactiveness", "20");
    write_node("/proc/sys/vm/page-cluster", "0");
    write_node("/proc/sys/vm/extfrag_threshold", "750");

    system("sysctl -w net.ipv4.tcp_congestion_control=cubic >/dev/null 2>&1 || sysctl -w net.ipv4.tcp_congestion_control=bbr >/dev/null 2>&1");
    system("sysctl -w net.core.default_qdisc=fq_codel >/dev/null 2>&1 || sysctl -w net.core.default_qdisc=fq >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_low_latency=1 >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_fastopen=3 >/dev/null 2>&1");

    DIR *d = opendir("/sys/block");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "sd", 2) == 0 || strncmp(dir->d_name, "mmcblk", 6) == 0) {
                char p[256];
                snprintf(p, sizeof(p), "/sys/block/%s/queue/scheduler", dir->d_name);
                write_node(p, "none");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", dir->d_name);
                write_node(p, "128");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/iostats", dir->d_name);
                write_node(p, "0");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/nr_requests", dir->d_name);
                write_node(p, "64");
            }
        }
        closedir(d);
    }

    write_node("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    write_node("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
    write_node("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");

    write_node("/sys/module/ged/parameters/boost_gpu_enable", "1");
    write_node("/sys/module/ged/parameters/enable_gpu_boost", "1");
    write_node("/sys/module/ged/parameters/ged_smart_boost", "1");
    write_node("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
    write_node("/sys/module/ged/parameters/gx_fb_dvfs_margin", "50");

    system("setprop persist.sys.smartpower.display_camera_fps_enable false 2>/dev/null");
    system("setprop persist.sys.smartpower.display.enable false 2>/dev/null");
    system("setprop persist.vendor.fps.switch.thermal false 2>/dev/null");
    system("setprop persist.vendor.display.touch.idle.enable false 2>/dev/null");
    system("setprop persist.sys.joyose.fps_boost true 2>/dev/null");
    system("setprop persist.sys.smartpower.game.enable true 2>/dev/null");
    system("setprop persist.sys.wifi.low_latency 1 2>/dev/null");
}

static void apply_cpuset(void) {
    if (access("/dev/cpuset", F_OK) == 0) {
        write_node("/dev/cpuset/top-app/cpus", "0-7");
        write_node("/dev/cpuset/foreground/cpus", "0-7");
        write_node("/dev/cpuset/background/cpus", "0-3");
        write_node("/dev/cpuset/system-background/cpus", "0-3");
    }
}

static void apply_profile_nodes(const char *prof, int ttier) {
    char path[256];
    char val[32];

    if (strcmp(prof, "Sleep") == 0) {
        for (int i = 0; i <= 5; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "500000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            write_node(path, "1450000");
            set_cpu_rate_limits(i, "2000", "500");
        }
        for (int i = 6; i <= 7; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "725000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            write_node(path, "1300000");
            set_cpu_rate_limits(i, "2000", "500");
        }
        write_node("/sys/kernel/fpsgo/fbt/boost_ta", "0");
        write_node("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
    }
    else if (strcmp(prof, "Interactive") == 0) {
        int max_big = (ttier >= 2) ? 2000000 : BIG_MAX;

        for (int i = 0; i <= 5; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "800000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", LITTLE_MAX);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "1000");
        }
        for (int i = 6; i <= 7; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "1000000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", max_big);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "1000");
        }
        write_node("/sys/kernel/fpsgo/fbt/boost_ta", "1");
        write_node("/sys/module/ged/parameters/gpu_bottom_freq", "550000");
        write_node("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
    }
    else if (strcmp(prof, "Touch") == 0) {
        for (int i = 0; i <= 5; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "1150000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", LITTLE_MAX);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "1200");
        }
        for (int i = 6; i <= 7; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "1500000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", BIG_MAX);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "1200");
        }
        write_node("/sys/kernel/fpsgo/fbt/boost_ta", "1");
        write_node("/sys/module/ged/parameters/gpu_bottom_freq", "700000");
        write_node("/sys/module/ged/parameters/g_fb_dvfs_threshold", "15");
    }
    else if (strcmp(prof, "Gaming") == 0) {
        const char *gpu_freq = (ttier == 2) ? "700000" : "800000";

        for (int i = 0; i <= 5; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "1200000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", LITTLE_MAX);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "2500");
        }
        for (int i = 6; i <= 7; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "1600000");
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
            snprintf(val, sizeof(val), "%d", BIG_MAX);
            write_node(path, val);
            set_cpu_rate_limits(i, "0", "2500");
        }
        write_node("/sys/kernel/fpsgo/fbt/boost_ta", "1");
        write_node("/sys/module/ged/parameters/gpu_bottom_freq", gpu_freq);
        write_node("/sys/module/ged/parameters/g_fb_dvfs_threshold", "25");
    }
    else if (strcmp(prof, "Thermal") == 0) {
        for (int i = 0; i <= 5; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "500000");
        }
        for (int i = 6; i <= 7; i++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
            write_node(path, "725000");
        }
        write_node("/sys/kernel/fpsgo/fbt/boost_ta", "0");
        write_node("/sys/module/ged/parameters/gpu_bottom_freq", "400000");
    }
}

int main(void) {
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    FILE *fpid = fopen(PID_FILE, "w");
    if (fpid) {
        fprintf(fpid, "%d\n", getpid());
        fclose(fpid);
    }

    log_msg("Tanzanite HyperFlow Native C Engine v2.5 started.");
    discover_nodes();
    apply_base_tuning();
    apply_cpuset();

    char current_profile[32] = "INIT";
    int current_thermal_tier = 0;
    int iteration = 0;
    int prev_load = 0;
    int touch_boost_ticks = 0;

    while (running) {
        iteration++;

        int screen_on = 0;
        if (backlight_path[0] != '\0') {
            int b = read_int(backlight_path);
            if (b > 0) screen_on = 1;
        } else {
            screen_on = 1;
        }

        int raw_cpu = read_int(cpu_tz_path);
        int cpu_temp = raw_cpu / 1000;
        if (cpu_temp <= 0) cpu_temp = 40;

        int raw_bat = read_int(bat_tz_path);
        int bat_temp = raw_bat;
        if (bat_temp > 10000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;
        if (bat_temp <= 0) bat_temp = 35;

        int thermal_tier = 0;
        if (cpu_temp >= 55 || bat_temp >= 45) {
            thermal_tier = 3;
        } else if (cpu_temp >= 49 || bat_temp >= 43) {
            thermal_tier = 2;
        } else if (cpu_temp >= 42 || bat_temp >= 39) {
            thermal_tier = 1;
        }

        int gpu_load = read_int("/sys/module/ged/parameters/gpu_loading");

        int load_val = 0;
        FILE *favg = fopen("/proc/loadavg", "r");
        if (favg) {
            float l1 = 0;
            if (fscanf(favg, "%f", &l1) == 1) {
                load_val = (int)(l1 * 100.0f);
            }
            fclose(favg);
        }

        char manual_lock[32] = "";
        FILE *flock = fopen(lock_file_path, "r");
        if (flock) {
            if (fscanf(flock, "%31s", manual_lock) != 1) {
                manual_lock[0] = '\0';
            }
            fclose(flock);
        }

        char next_profile[32] = "Interactive";

        if (screen_on == 0) {
            strcpy(next_profile, "Sleep");
            touch_boost_ticks = 0;
        } else if (thermal_tier == 3) {
            strcpy(next_profile, "Thermal");
            touch_boost_ticks = 0;
        } else if (strcmp(manual_lock, "GAMING") == 0) {
            strcpy(next_profile, "Gaming");
            touch_boost_ticks = 0;
        } else if (gpu_load >= 35 || (gpu_load >= 15 && load_val >= 1200)) {
            strcpy(next_profile, "Gaming");
            touch_boost_ticks = 0;
        } else if ((load_val - prev_load) > 180 || touch_boost_ticks > 0) {
            if (strcmp(current_profile, "Touch") != 0 && touch_boost_ticks == 0) {
                touch_boost_ticks = 3;
            } else {
                touch_boost_ticks--;
            }
            if (touch_boost_ticks > 0) {
                strcpy(next_profile, "Touch");
            } else {
                strcpy(next_profile, "Interactive");
            }
        } else {
            strcpy(next_profile, "Interactive");
            touch_boost_ticks = 0;
        }

        prev_load = load_val;

        if (strcmp(next_profile, current_profile) != 0 || thermal_tier != current_thermal_tier) {
            char log_buf[256];
            snprintf(log_buf, sizeof(log_buf), "Profile: %s -> %s (CPU: %d°C, Bat: %d°C, GPU: %d%%, Load: %d)",
                     current_profile, next_profile, cpu_temp, bat_temp, gpu_load, load_val);
            log_msg(log_buf);
            apply_profile_nodes(next_profile, thermal_tier);
            strcpy(current_profile, next_profile);
            current_thermal_tier = thermal_tier;
        }

        if (iteration % 20 == 0) {
            apply_cpuset();
            rotate_log();
            write_node("/sys/module/ged/parameters/gx_fb_dvfs_margin", "50");
        }

        sleep(2);
    }

    log_msg("Tanzanite HyperFlow Native C Engine stopped cleanly.");
    unlink(PID_FILE);
    return 0;
}
