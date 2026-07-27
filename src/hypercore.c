/*
 * HyperCore Engine v3.0 - MediaTek Helio G99 Ultra Tuner
 *
 * Compact C daemon for dynamic CPU scaling, GPU responsiveness,
 * thermal hysteresis management, and IO queue tuning.
 *
 * Author: @itswill00
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_PATH        "/sdcard/Android/hypercore.log"
#define FREQ_LITTLE_MAX 2000000
#define FREQ_BIG_MAX    2200000

typedef enum {
    PROFILE_SLEEP,
    PROFILE_INTERACTIVE,
    PROFILE_TOUCH,
    PROFILE_GAMING,
    PROFILE_THERMAL
} profile_t;

static const char *profile_names[] = {
    "Sleep", "Interactive", "Touch", "Gaming", "Thermal"
};

struct hw_nodes {
    char mod_dir[256];
    char pid_file[256];
    char lock_file[256];
    char cpu_temp[256];
    char bat_temp[256];
    char backlight[256];
    int  has_sugov_ext;
    int  has_schedutil;
};

struct engine_state {
    int current_profile;
    int thermal_tier;
    int thermal_hold_ticks;
    int touch_boost_ticks;
    int prev_load;
};

static volatile sig_atomic_t g_running = 1;
static struct hw_nodes g_nodes;
static struct engine_state g_state;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void sysfs_write(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, val, strlen(val));
    close(fd);
}

static int sysfs_read_int(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return atoi(buf);
}

static void log_write(const char *fmt, ...) {
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) return;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);

    va_list args;
    va_start(args, fmt);
    fprintf(f, "%s ", tbuf);
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);

    fclose(f);
}

static void rotate_log(void) {
    struct stat st;
    if (stat(LOG_PATH, &st) != 0 || st.st_size <= 102400) return;

    FILE *in = fopen(LOG_PATH, "r");
    if (!in) return;

    char lines[300][256];
    int count = 0;
    while (fgets(lines[count % 300], sizeof(lines[0]), in)) {
        count++;
    }
    fclose(in);

    FILE *out = fopen(LOG_PATH, "w");
    if (!out) return;

    int start = (count > 150) ? (count - 150) : 0;
    for (int i = start; i < count; i++) {
        fputs(lines[i % 300], out);
    }
    fclose(out);
}

static void scan_thermal_zones(void) {
    DIR *d = opendir("/sys/class/thermal");
    if (!d) return;

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

    if (g_nodes.cpu_temp[0] == '\0') {
        strcpy(g_nodes.cpu_temp, access("/sys/class/thermal/thermal_zone16/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone16/temp" : "/sys/class/thermal/thermal_zone0/temp");
    }
    if (g_nodes.bat_temp[0] == '\0') {
        strcpy(g_nodes.bat_temp, access("/sys/class/thermal/thermal_zone25/temp", F_OK) == 0 ?
               "/sys/class/thermal/thermal_zone25/temp" : "/sys/class/thermal/thermal_zone1/temp");
    }
}

static void init_hardware_nodes(void) {
    memset(&g_nodes, 0, sizeof(g_nodes));

    if (access("/data/adb/modules/tanzanite_hypercore", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/tanzanite_hypercore");
    } else if (access("/data/adb/modules/tanzanite_hyperflow", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/tanzanite_hyperflow");
    } else {
        strcpy(g_nodes.mod_dir, "/sdcard/Android");
    }

    snprintf(g_nodes.pid_file, sizeof(g_nodes.pid_file), "%s/engine.pid", g_nodes.mod_dir);
    snprintf(g_nodes.lock_file, sizeof(g_nodes.lock_file), "%s/.hypercore_lock", g_nodes.mod_dir);

    scan_thermal_zones();

    const char *bl_paths[] = {
        "/sys/class/backlight/panel0-backlight/brightness",
        "/sys/class/backlight/lcd-backlight/brightness",
        "/sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness",
        "/sys/class/leds/lcd-backlight/brightness",
        NULL
    };
    for (int i = 0; bl_paths[i]; i++) {
        if (access(bl_paths[i], F_OK) == 0) {
            strcpy(g_nodes.backlight, bl_paths[i]);
            break;
        }
    }

    g_nodes.has_sugov_ext = (access("/sys/devices/system/cpu/cpu0/cpufreq/sugov_ext", F_OK) == 0);
    g_nodes.has_schedutil = (access("/sys/devices/system/cpu/cpufreq/schedutil", F_OK) == 0);
}

static void set_rate_limits(int cpu, const char *up, const char *down) {
    char path[256];
    if (g_nodes.has_sugov_ext) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/up_rate_limit_us", cpu);
        sysfs_write(path, up);
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/sugov_ext/down_rate_limit_us", cpu);
        sysfs_write(path, down);
    }
    if (g_nodes.has_schedutil) {
        sysfs_write("/sys/devices/system/cpu/cpufreq/schedutil/up_rate_limit_us", up);
        sysfs_write("/sys/devices/system/cpu/cpufreq/schedutil/down_rate_limit_us", down);
    }
}

static void apply_base_tuning(void) {
    sysfs_write("/proc/sys/vm/swappiness", "65");
    sysfs_write("/proc/sys/vm/dirty_ratio", "15");
    sysfs_write("/proc/sys/vm/dirty_background_ratio", "5");
    sysfs_write("/proc/sys/vm/vfs_cache_pressure", "90");
    sysfs_write("/proc/sys/vm/stat_interval", "2");
    sysfs_write("/proc/sys/vm/compaction_proactiveness", "20");
    sysfs_write("/proc/sys/vm/page-cluster", "0");
    sysfs_write("/proc/sys/vm/extfrag_threshold", "750");

    system("sysctl -w net.ipv4.tcp_congestion_control=cubic >/dev/null 2>&1 || sysctl -w net.ipv4.tcp_congestion_control=bbr >/dev/null 2>&1");
    system("sysctl -w net.core.default_qdisc=fq_codel >/dev/null 2>&1 || sysctl -w net.core.default_qdisc=fq >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_low_latency=1 >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_fastopen=3 >/dev/null 2>&1");

    DIR *d = opendir("/sys/block");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strncmp(ent->d_name, "sd", 2) == 0 || strncmp(ent->d_name, "mmcblk", 6) == 0) {
                char p[256];
                snprintf(p, sizeof(p), "/sys/block/%s/queue/scheduler", ent->d_name);
                sysfs_write(p, "none");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
                sysfs_write(p, "128");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/iostats", ent->d_name);
                sysfs_write(p, "0");
                snprintf(p, sizeof(p), "/sys/block/%s/queue/nr_requests", ent->d_name);
                sysfs_write(p, "64");
            }
        }
        closedir(d);
    }

    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "10");

    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", "1");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", "1");
    sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "12");
    sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "40");

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
        sysfs_write("/dev/cpuset/top-app/cpus", "0-7");
        sysfs_write("/dev/cpuset/foreground/cpus", "0-7");
        sysfs_write("/dev/cpuset/background/cpus", "0-3");
        sysfs_write("/dev/cpuset/system-background/cpus", "0-3");
    }
}

static void set_cpu_freqs(int min_lit, int max_lit, int min_big, int max_big, const char *up_rate, const char *down_rate) {
    char path[256], buf[32];

    for (int i = 0; i <= 5; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        snprintf(buf, sizeof(buf), "%d", min_lit);
        sysfs_write(path, buf);

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        snprintf(buf, sizeof(buf), "%d", max_lit);
        sysfs_write(path, buf);

        set_rate_limits(i, up_rate, down_rate);
    }

    for (int i = 6; i <= 7; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", i);
        snprintf(buf, sizeof(buf), "%d", min_big);
        sysfs_write(path, buf);

        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
        snprintf(buf, sizeof(buf), "%d", max_big);
        sysfs_write(path, buf);

        set_rate_limits(i, up_rate, down_rate);
    }
}

static void apply_profile(profile_t prof, int tier) {
    switch (prof) {
        case PROFILE_SLEEP:
            set_cpu_freqs(500000, 1450000, 725000, 1300000, "2000", "500");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "300000");
            break;

        case PROFILE_INTERACTIVE: {
            int big_max = (tier >= 3) ? 2000000 : FREQ_BIG_MAX;
            set_cpu_freqs(900000, FREQ_LITTLE_MAX, 1100000, big_max, "0", "1000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "550000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "12");
            break;
        }

        case PROFILE_TOUCH:
            set_cpu_freqs(1200000, FREQ_LITTLE_MAX, 1500000, FREQ_BIG_MAX, "0", "1000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "700000");
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "10");
            break;

        case PROFILE_GAMING: {
            const char *gpu_freq = (tier >= 2) ? "700000" : "800000";
            set_cpu_freqs(1200000, FREQ_LITTLE_MAX, 1600000, FREQ_BIG_MAX, "0", "2500");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", gpu_freq);
            sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", "20");
            break;
        }

        case PROFILE_THERMAL:
            set_cpu_freqs(700000, FREQ_LITTLE_MAX, 1000000, 2000000, "0", "1000");
            sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");
            sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", "450000");
            break;
    }
}

static int calculate_thermal_tier(int cpu_temp, int bat_temp) {
    int tier = g_state.thermal_tier;

    if (cpu_temp >= 58 || bat_temp >= 47) {
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

int main(void) {
    setsid();
    daemon(0, 0);
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    init_hardware_nodes();

    FILE *fpid = fopen(g_nodes.pid_file, "w");
    if (fpid) {
        fprintf(fpid, "%d\n", getpid());
        fclose(fpid);
    }

    log_write("Tanzanite HyperCore Engine v3.0 started.");
    apply_base_tuning();
    apply_cpuset();

    g_state.current_profile = -1;
    g_state.thermal_tier = 0;
    g_state.thermal_hold_ticks = 0;
    g_state.touch_boost_ticks = 0;
    g_state.prev_load = 0;

    int iteration = 0;

    while (g_running) {
        iteration++;

        int screen_on = 1;
        if (g_nodes.backlight[0] != '\0') {
            screen_on = (sysfs_read_int(g_nodes.backlight) > 0);
        }

        int raw_cpu = sysfs_read_int(g_nodes.cpu_temp);
        int cpu_temp = raw_cpu / 1000;
        if (cpu_temp <= 0) cpu_temp = 40;

        int raw_bat = sysfs_read_int(g_nodes.bat_temp);
        int bat_temp = raw_bat;
        if (bat_temp > 10000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;
        if (bat_temp <= 0) bat_temp = 35;

        int thermal_tier = calculate_thermal_tier(cpu_temp, bat_temp);
        int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");

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
        FILE *flock = fopen(g_nodes.lock_file, "r");
        if (flock) {
            if (fscanf(flock, "%31s", manual_lock) != 1) manual_lock[0] = '\0';
            fclose(flock);
        }

        profile_t next_profile = PROFILE_INTERACTIVE;

        if (!screen_on) {
            next_profile = PROFILE_SLEEP;
            g_state.touch_boost_ticks = 0;
        } else if (thermal_tier == 3 && cpu_temp >= 60) {
            next_profile = PROFILE_THERMAL;
            g_state.touch_boost_ticks = 0;
        } else if (strcmp(manual_lock, "GAMING") == 0) {
            next_profile = PROFILE_GAMING;
            g_state.touch_boost_ticks = 0;
        } else if (gpu_load >= 35 || (gpu_load >= 15 && load_val >= 1200)) {
            next_profile = PROFILE_GAMING;
            g_state.touch_boost_ticks = 0;
        } else if ((load_val - g_state.prev_load) > 150 || g_state.touch_boost_ticks > 0) {
            if (g_state.current_profile != PROFILE_TOUCH && g_state.touch_boost_ticks == 0) {
                g_state.touch_boost_ticks = 3;
            } else {
                g_state.touch_boost_ticks--;
            }
            next_profile = (g_state.touch_boost_ticks > 0) ? PROFILE_TOUCH : PROFILE_INTERACTIVE;
        } else {
            next_profile = PROFILE_INTERACTIVE;
            g_state.touch_boost_ticks = 0;
        }

        g_state.prev_load = load_val;

        if (next_profile != g_state.current_profile || thermal_tier != g_state.thermal_tier) {
            const char *prev_name = (g_state.current_profile >= 0 && g_state.current_profile < 5) ? profile_names[g_state.current_profile] : "INIT";
            log_write("Profile: %s -> %s (CPU: %d°C, Bat: %d°C, GPU: %d%%, Load: %d)",
                      prev_name, profile_names[next_profile], cpu_temp, bat_temp, gpu_load, load_val);
            apply_profile(next_profile, thermal_tier);
            g_state.current_profile = next_profile;
            g_state.thermal_tier = thermal_tier;
        }

        if (iteration % 20 == 0) {
            apply_cpuset();
            rotate_log();
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "40");
        }

        sleep(2);
    }

    log_write("Tanzanite HyperCore Engine stopped cleanly.");
    unlink(g_nodes.pid_file);
    return 0;
}
