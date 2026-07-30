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
#include "gamelist.hpp"

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

static void send_game_toast(const char *game_name) {
    if (!game_name || game_name[0] == '\0') return;
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "cmd notification post -t 'Tanzanite HyperCore' -S bigtext 'HyperCore Engine' 'hypercore_game' 'Gaming Profile Activated [%s]' >/dev/null 2>&1 &",
        game_name);
    system(cmd);
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

static void write_pid_file(void) {
    FILE *f = fopen(g_nodes.pid_file, "w");
    if (f) {
        fprintf(f, "%d\n", getpid());
        fclose(f);
    }
}

static void remove_pid_file(void) {
    unlink(g_nodes.pid_file);
}

static int is_screen_on(void) {
    if (g_nodes.backlight[0] != '\0') {
        int val = sysfs_read_int(g_nodes.backlight);
        return val > 0;
    }
    return 1;
}

static int read_initial_load(void) {
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) return 0;
    float l1 = 0;
    if (fscanf(f, "%f", &l1) != 1) l1 = 0;
    fclose(f);
    return (int)(l1 * 100);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGHUP, SIG_IGN);

    init_hardware_nodes();
    write_pid_file();

    log_write("Tanzanite HyperCore v3.6 started.");

    apply_cpuset();
    apply_memory_tuning();
    apply_io_tuning();
    apply_gpu_tuning();
    apply_irq_tuning();
    load_gamelist();
    init_gamelist_watcher();

    memset(&g_state, 0, sizeof(g_state));
    g_state.current_profile = (profile_t)-1;
    g_state.prev_load = read_initial_load();

    while (g_running) {
        int cpu_temp = sysfs_read_int(g_nodes.cpu_temp);
        int bat_temp = sysfs_read_int(g_nodes.bat_temp);

        if (cpu_temp > 1000) cpu_temp /= 1000;
        if (bat_temp > 1000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;

        g_state.is_charging = check_charging_status();

        int thermal_tier = calculate_thermal_tier(cpu_temp, bat_temp);
        int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");
        int load_val = read_initial_load();

        char manual_lock[32] = "";
        FILE *flock = fopen(g_nodes.lock_file, "r");
        if (flock) {
            if (fscanf(flock, "%31s", manual_lock) != 1) manual_lock[0] = '\0';
            fclose(flock);
        }

        char active_game[128] = "";
        profile_t custom_profile = PROFILE_GAMING;
        int game_active = is_game_in_foreground(active_game, sizeof(active_game), &custom_profile);

        static int s_game_absent_ticks = 0;
        static int s_prev_game_active = 0;

        if (game_active) {
            s_game_absent_ticks = 0;
            if (!s_prev_game_active) {
                set_read_ahead("512");
                g_state.launch_boost_ticks = 3;
                send_game_toast(active_game);
                log_write("Game Launch Boost: 512KB Read-Ahead [%s]", active_game);
                s_prev_game_active = 1;
            }
        } else {
            s_game_absent_ticks++;
            if (s_game_absent_ticks >= 3) {
                s_prev_game_active = 0;
            }
        }

        if (g_state.launch_boost_ticks > 0) {
            g_state.launch_boost_ticks--;
            if (g_state.launch_boost_ticks == 0) {
                set_read_ahead("256");
            }
        }

        profile_t next_profile = PROFILE_INTERACTIVE;

        if (!is_screen_on()) {
            next_profile = PROFILE_SLEEP;
            g_state.touch_boost_ticks = 0;
            g_state.gaming_hold_ticks = 0;
        } else if (thermal_tier == 3 && cpu_temp >= 75) {
            next_profile = PROFILE_THERMAL;
            g_state.touch_boost_ticks = 0;
            g_state.gaming_hold_ticks = 0;
        } else if (strcmp(manual_lock, "GAMING") == 0) {
            next_profile = PROFILE_GAMING;
            g_state.gaming_hold_ticks = 10;
            g_state.touch_boost_ticks = 0;
        } else if (strcmp(manual_lock, "INTERACTIVE") == 0 || strcmp(manual_lock, "BALANCED") == 0) {
            next_profile = PROFILE_INTERACTIVE;
            g_state.gaming_hold_ticks = 0;
            g_state.touch_boost_ticks = 0;
        } else if (strcmp(manual_lock, "TOUCH") == 0) {
            next_profile = PROFILE_TOUCH;
            g_state.gaming_hold_ticks = 0;
            g_state.touch_boost_ticks = 3;
        } else if (strcmp(manual_lock, "SLEEP") == 0 || strcmp(manual_lock, "SAVER") == 0) {
            next_profile = PROFILE_SLEEP;
            g_state.gaming_hold_ticks = 0;
            g_state.touch_boost_ticks = 0;
        } else if (game_active) {
            next_profile = custom_profile;
            g_state.gaming_hold_ticks = 10;
            g_state.touch_boost_ticks = 0;
        } else if (g_state.gaming_hold_ticks > 0) {
            g_state.gaming_hold_ticks--;
            next_profile = PROFILE_GAMING;
            g_state.touch_boost_ticks = 0;
        } else if (gpu_load >= 25 || (load_val - g_state.prev_load) > 75 || g_state.touch_boost_ticks > 0) {
            g_state.gaming_hold_ticks = 0;
            if (g_state.current_profile != PROFILE_TOUCH && g_state.touch_boost_ticks == 0) {
                g_state.touch_boost_ticks = 3;
            } else if (g_state.touch_boost_ticks > 0) {
                g_state.touch_boost_ticks--;
            }
            next_profile = (g_state.touch_boost_ticks > 0) ? PROFILE_TOUCH : PROFILE_INTERACTIVE;
        } else {
            g_state.gaming_hold_ticks = 0;
            next_profile = PROFILE_INTERACTIVE;
            g_state.touch_boost_ticks = 0;
        }

        s_prev_game_active = game_active;
        g_state.prev_load = load_val;

        if (next_profile != g_state.current_profile || thermal_tier != g_state.thermal_tier) {
            const char *prev_name = (g_state.current_profile >= 0 && g_state.current_profile < 5) ? g_profile_names[g_state.current_profile] : "INIT";
            if (next_profile != g_state.current_profile) {
                char status_buf[128];
                if (next_profile == PROFILE_GAMING && active_game[0] != '\0') {
                    snprintf(status_buf, sizeof(status_buf), "Gaming (%s)", active_game);
                    log_write("Profile: %s -> %s [%s] (CPU: %d°C, Bat: %d°C %s, GPU: %d%%, Load: %d)",
                              prev_name, g_profile_names[next_profile], active_game, cpu_temp, bat_temp,
                              g_state.is_charging ? "[CHG]" : "", gpu_load, load_val);
                } else {
                    snprintf(status_buf, sizeof(status_buf), "%s", g_profile_names[next_profile]);
                    log_write("Profile: %s -> %s (CPU: %d°C, Bat: %d°C %s, GPU: %d%%, Load: %d)",
                              prev_name, g_profile_names[next_profile], cpu_temp, bat_temp,
                              g_state.is_charging ? "[CHG]" : "", gpu_load, load_val);
                }
                update_module_prop_status(status_buf);
            } else {
                log_write("Thermal Tier: T%d -> T%d (CPU: %d°C, Bat: %d°C %s)",
                          g_state.thermal_tier, thermal_tier, cpu_temp, bat_temp,
                          g_state.is_charging ? "[CHG]" : "");
            }
            apply_profile(next_profile, thermal_tier);
            g_state.current_profile = next_profile;
            g_state.thermal_tier = thermal_tier;
        }

        static int s_rotate_counter = 0;
        if (++s_rotate_counter >= 30) {
            rotate_log();
            s_rotate_counter = 0;
        }

        sleep(2);
    }

    log_write("Tanzanite HyperCore daemon stopped.");
    update_module_prop_status("Stopped");
    remove_pid_file();
    return 0;
}
