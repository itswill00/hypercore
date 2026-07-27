/*
 * HyperCore - Main Daemon Entry Point
 * Author: @itswill00
 */

#include "common.hpp"
#include "sysfs.hpp"
#include "log.hpp"
#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "thermal.hpp"

const char *g_profile_names[] = {
    "Sleep", "Interactive", "Touch", "Gaming", "Thermal"
};

volatile sig_atomic_t g_running = 1;
struct hw_nodes g_nodes;
struct core_state g_state;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
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

    snprintf(g_nodes.pid_file, sizeof(g_nodes.pid_file), "%s/hypercore.pid", g_nodes.mod_dir);
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

static void apply_base_tuning(void) {
    apply_memory_tuning();
    apply_io_tuning();
    apply_gpu_tuning();

    system("sysctl -w net.ipv4.tcp_congestion_control=cubic >/dev/null 2>&1 || sysctl -w net.ipv4.tcp_congestion_control=bbr >/dev/null 2>&1");
    system("sysctl -w net.core.default_qdisc=fq_codel >/dev/null 2>&1 || sysctl -w net.core.default_qdisc=fq >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_low_latency=1 >/dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_fastopen=3 >/dev/null 2>&1");

    system("setprop persist.sys.smartpower.display_camera_fps_enable false 2>/dev/null");
    system("setprop persist.sys.smartpower.display.enable false 2>/dev/null");
    system("setprop persist.vendor.fps.switch.thermal false 2>/dev/null");
    system("setprop persist.vendor.display.touch.idle.enable false 2>/dev/null");
    system("setprop persist.sys.joyose.fps_boost true 2>/dev/null");
    system("setprop persist.sys.smartpower.game.enable true 2>/dev/null");
    system("setprop persist.sys.wifi.low_latency 1 2>/dev/null");
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

    log_write("Tanzanite HyperCore v3.0 started.");
    apply_base_tuning();
    apply_cpuset();

    g_state.current_profile = -1;
    g_state.thermal_tier = 0;
    g_state.thermal_hold_ticks = 0;
    g_state.touch_boost_ticks = 0;
    g_state.prev_load = 0;
    g_state.is_charging = 0;

    int iteration = 0;

    while (g_running) {
        iteration++;

        g_state.is_charging = check_charging_status();

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
            const char *prev_name = (g_state.current_profile >= 0 && g_state.current_profile < 5) ? g_profile_names[g_state.current_profile] : "INIT";
            log_write("Profile: %s -> %s (CPU: %d°C, Bat: %d°C %s, GPU: %d%%, Load: %d)",
                      prev_name, g_profile_names[next_profile], cpu_temp, bat_temp,
                      g_state.is_charging ? "[CHG]" : "", gpu_load, load_val);
            apply_profile(next_profile, thermal_tier);
            g_state.current_profile = next_profile;
            g_state.thermal_tier = thermal_tier;
        }

        if (iteration % 20 == 0) {
            apply_cpuset();
            rotate_log();
            sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", "40");
            sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "0");
        }

        sleep(2);
    }

    log_write("Tanzanite HyperCore stopped cleanly.");
    unlink(g_nodes.pid_file);
    return 0;
}
