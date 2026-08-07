
#include "common.hpp"
#include "sysfs.hpp"
#include "log.hpp"
#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "thermal.hpp"
#include "gamelist.hpp"
#include "ipc.hpp"

const char *g_profile_names[] = {
    "Sleep", "Interactive", "Gaming", "Gaming MOBA"
};

volatile sig_atomic_t g_running = 1;
struct hw_nodes g_nodes;
struct core_state g_state;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void async_system_cmd(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    pid_t pid = fork();
    if (pid == 0) {
        pid_t grandchild = fork();
        if (grandchild == 0) {
            int null_fd = open("/dev/null", O_RDWR);
            if (null_fd >= 0) {
                dup2(null_fd, STDIN_FILENO);
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
            execl("/system/bin/sh", "sh", "-c", cmd, (char *)NULL);
            _exit(127);
        }
        _exit(0);
    }
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

static void send_game_toast(const char *game_name, profile_t prof) {
    if (!game_name || game_name[0] == '\0') return;
    const char *prof_label = (prof >= 0 && prof < 4) ? g_profile_names[prof] : "Unknown";
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "cmd notification post -t 'HyperCore' -S bigtext 'HyperCore' 'hypercore_game' '%s Profile Active [%s]'",
        prof_label, game_name);
    async_system_cmd(cmd);
}

static void init_hardware_nodes(void) {
    memset(&g_nodes, 0, sizeof(g_nodes));

    if (access("/data/adb/modules/hypercore", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/hypercore");
    } else if (access("/data/adb/modules/tanzanite_hypercore", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/tanzanite_hypercore");
    } else if (access("/data/adb/modules/tanzanite_hyperflow", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/tanzanite_hyperflow");
    } else {
        strcpy(g_nodes.mod_dir, "/sdcard/Android");
    }

    snprintf(g_nodes.pid_file, sizeof(g_nodes.pid_file), "%s/hypercore.pid", g_nodes.mod_dir);

    scan_thermal_zones();

    const char *bl_paths[] = {
        "/sys/class/leds/lcd-backlight/brightness",
        "/sys/class/backlight/panel0-backlight/brightness",
        "/sys/class/backlight/lcd-backlight/brightness",
        "/sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness",
        NULL
    };
    for (int i = 0; bl_paths[i]; i++) {
        if (access(bl_paths[i], F_OK) == 0) {
            strcpy(g_nodes.backlight, bl_paths[i]);
            break;
        }
    }

    const char *thp_paths[] = {
        "/sys/class/touch/touch_dev/touch_thp_smooth",
        "/sys/class/touch/touch_dev/thp_smooth",
        "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_thp_smooth",
        NULL
    };
    for (int i = 0; thp_paths[i]; i++) {
        if (access(thp_paths[i], F_OK) == 0) {
            strcpy(g_nodes.touch_thp_smooth, thp_paths[i]);
            break;
        }
    }

    const char *edge_paths[] = {
        "/sys/class/touch/touch_dev/touch_edge",
        "/sys/class/touch/touch_dev/edge_mode",
        "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_edge_mode",
        NULL
    };
    for (int i = 0; edge_paths[i]; i++) {
        if (access(edge_paths[i], F_OK) == 0) {
            strcpy(g_nodes.touch_edge, edge_paths[i]);
            break;
        }
    }

    const char *chg_paths[] = {
        "/sys/class/power_supply/battery/constant_charge_current_max",
        "/sys/class/power_supply/battery/charge_control_limit",
        "/sys/class/power_supply/battery/input_current_limit",
        "/sys/class/power_supply/main/constant_charge_current_max",
        "/sys/class/power_supply/bms/constant_charge_current_max",
        NULL
    };
    for (int i = 0; chg_paths[i]; i++) {
        if (access(chg_paths[i], F_OK) == 0) {
            strcpy(g_nodes.charge_control, chg_paths[i]);
            break;
        }
    }

    g_nodes.has_sugov_ext = (access("/sys/devices/system/cpu/cpu0/cpufreq/sugov_ext", F_OK) == 0);
    g_nodes.has_schedutil = (access("/sys/devices/system/cpu/cpufreq/policy0/schedutil", F_OK) == 0);

    g_nodes.inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (g_nodes.inotify_fd >= 0 && g_nodes.backlight[0] != '\0') {
        inotify_add_watch(g_nodes.inotify_fd, g_nodes.backlight, IN_MODIFY | IN_ATTRIB);
    }

    g_nodes.netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (g_nodes.netlink_fd >= 0) {
        struct sockaddr_nl nladdr;
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_pid = getpid();
        nladdr.nl_groups = 1;
        bind(g_nodes.netlink_fd, (struct sockaddr *)&nladdr, sizeof(nladdr));
        int flags = fcntl(g_nodes.netlink_fd, F_GETFL, 0);
        if (flags != -1) {
            fcntl(g_nodes.netlink_fd, F_SETFL, flags | O_NONBLOCK);
        }
    }
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
    if (g_nodes.backlight[0] != '\0' && access(g_nodes.backlight, F_OK) == 0) {
        int val = sysfs_read_int(g_nodes.backlight);
        return val > 0;
    }

    const char *bl_paths[] = {
        "/sys/class/leds/lcd-backlight/brightness",
        "/sys/class/backlight/panel0-backlight/brightness",
        "/sys/class/backlight/lcd-backlight/brightness",
        "/sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness",
        NULL
    };
    for (int i = 0; bl_paths[i]; i++) {
        if (access(bl_paths[i], F_OK) == 0) {
            strcpy(g_nodes.backlight, bl_paths[i]);
            int val = sysfs_read_int(bl_paths[i]);
            return val > 0;
        }
    }

    return 1;
}

static int get_top_app_pid(void) {
    FILE *f = fopen("/dev/cpuset/top-app/tasks", "r");
    if (!f) return 0;
    char line[32];
    int pid = 0;
    if (fgets(line, sizeof(line), f)) {
        pid = atoi(line);
    }
    fclose(f);
    return pid;
}

static int check_and_recover_sysfs_tampering(profile_t current_prof) {
    if (current_prof < 0 || current_prof > 3) return 0;

    /* Strictly guard Gaming profile only — Interactive/Sleep are relaxed profiles
     * and should not trigger re-enforce on minor transient hardware deviations. */
    if (current_prof == PROFILE_Gaming || current_prof == PROFILE_Gaming_MOBA) {
        char curr_gov[64] = "";
        if (sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", curr_gov, sizeof(curr_gov))) {
            size_t len = strlen(curr_gov);
            while (len > 0 && (curr_gov[len - 1] == '\n' || curr_gov[len - 1] == '\r' || curr_gov[len - 1] == ' ')) {
                curr_gov[--len] = '\0';
            }
            if (curr_gov[0] != '\0' && strcmp(curr_gov, "performance") != 0) {
                log_warn("Guard", "External CPU governor mutation detected [%s -> performance]! Re-enforcing...", curr_gov);
                return 1;
            }
        }

        char curr_mali[64] = "";
        if (read_mali_power_policy(curr_mali, sizeof(curr_mali))) {
            if (strstr(curr_mali, "always_on") == NULL) {
                log_warn("Guard", "External Mali GPU power_policy mutation detected! Re-enforcing...");
                return 1;
            }
        }

        char curr_gpu_gov[64] = "";
        if (read_mali_governor(curr_gpu_gov, sizeof(curr_gpu_gov))) {
            if (strstr(curr_gpu_gov, "performance") == NULL) {
                log_warn("Guard", "External Mali GPU governor mutation detected! Re-enforcing...");
                return 1;
            }
        }
    }

    /* Soft enforcement for non-Gaming profiles via polling guard */
    if (current_prof == PROFILE_Interactive) {
        enforce_interactive_gpu_polling(20);
    } else if (current_prof == PROFILE_Sleep) {
        enforce_interactive_gpu_polling(100);
    }

    return 0;
}

#include "integrity.hpp"

static int validate_hardware_target(void) {
    if (access("/sys/module/ged", F_OK) != 0 || access("/sys/devices/system/cpu/cpufreq/policy0", F_OK) != 0) {
        fprintf(stderr, "ERROR: Incompatible hardware target detected. HyperCore is strictly designed for MediaTek MT6789 Family (Kernel 5.10.x).\n");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--verify-integrity") == 0) {
        return verify_module_integrity(argv[2]);
    }

    if (!validate_hardware_target()) {
        return 1;
    }

    if (daemon(0, 0) != 0) {
        perror("daemonization failed");
        return 1;
    }

    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGHUP, SIG_IGN);

    init_hardware_nodes();
    verify_module_integrity(g_nodes.mod_dir);
    save_baseline_nodes();
    write_pid_file();
    init_ipc_socket();

    log_info("Daemon", "HyperCore v4.4 started.");

    apply_cpuset();
    apply_memory_tuning();
    apply_io_tuning();
    apply_gpu_tuning();
    apply_irq_tuning();
    load_gamelist();
    init_gamelist_watcher();

    memset(&g_state, 0, sizeof(g_state));
    g_state.current_profile = (profile_t)-1;

    while (g_running) {
        int cpu_temp = sysfs_read_int(g_nodes.cpu_temp);
        int bat_temp = sysfs_read_int(g_nodes.bat_temp);

        if (cpu_temp > 1000) cpu_temp /= 1000;
        if (bat_temp > 1000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;

        g_state.is_charging = check_charging_status();

        int thermal_tier = calculate_thermal_tier(cpu_temp, bat_temp);
        int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");

        char active_game[128] = "";
        profile_t custom_profile = PROFILE_Gaming;
        int game_active = is_game_in_foreground(active_game, sizeof(active_game), &custom_profile);

        int current_top_pid = get_top_app_pid();
        static int s_prev_top_pid = 0;
        if (current_top_pid > 0 && s_prev_top_pid > 0 && current_top_pid != s_prev_top_pid && !game_active) {
            g_state.app_boost_ticks = 3;
            set_read_ahead("384");
        }
        s_prev_top_pid = current_top_pid;

        if (g_state.app_boost_ticks > 0) {
            g_state.app_boost_ticks--;
            if (g_state.app_boost_ticks == 0 && g_state.launch_boost_ticks == 0) {
                set_read_ahead("256");
            }
        }

        static int       s_game_absent_ticks = 0;
        static int       s_prev_game_active  = 0;
        static profile_t s_last_game_profile = PROFILE_Interactive;

        if (game_active) {
            s_game_absent_ticks = 0;
            if (!s_prev_game_active) {
                set_read_ahead("512");
                g_state.launch_boost_ticks = 3;
                send_game_toast(active_game, custom_profile);
                log_info("Game", "Game Launch Boost: 512KB Read-Ahead [%s] Profile: %s",
                         active_game, g_profile_names[custom_profile]);
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
            if (g_state.launch_boost_ticks == 0 && g_state.app_boost_ticks == 0) {
                set_read_ahead("256");
            }
        }

        profile_t next_profile = PROFILE_Interactive;

        if (!is_screen_on()) {
            next_profile = PROFILE_Sleep;
            g_state.gaming_hold_ticks = 0;
        } else if (game_active) {
            next_profile = custom_profile;
            s_last_game_profile = custom_profile;
            /* Hold ticks for Gaming profiles only */
            g_state.gaming_hold_ticks = (custom_profile == PROFILE_Gaming || custom_profile == PROFILE_Gaming_MOBA) ? 2 : 0;
        } else if (g_state.gaming_hold_ticks > 0) {
            g_state.gaming_hold_ticks--;
            next_profile = s_last_game_profile;
        } else {
            next_profile = PROFILE_Interactive;
        }

        /* s_prev_game_active is managed inside the game_active block above
         * with a 3-tick debounce. Do not override it here to preserve debounce. */

        static int s_prev_cpu_temp = 0;
        int temp_delta = (s_prev_cpu_temp > 0) ? abs(cpu_temp - s_prev_cpu_temp) : 0;
        s_prev_cpu_temp = cpu_temp;

        static int s_prev_gpu_heavy = -1;
        static int s_prev_app_boost = 0;
        int is_gpu_heavy = (gpu_load >= 20);
        int is_tampered = check_and_recover_sysfs_tampering(g_state.current_profile);

        if (next_profile != g_state.current_profile || thermal_tier != g_state.thermal_tier ||
            g_state.app_boost_ticks != s_prev_app_boost || is_tampered ||
            ((next_profile == PROFILE_Gaming || next_profile == PROFILE_Gaming_MOBA) && is_gpu_heavy != s_prev_gpu_heavy)) {
            s_prev_gpu_heavy = is_gpu_heavy;
            s_prev_app_boost = g_state.app_boost_ticks;
            const char *prev_name = (g_state.current_profile >= 0 && g_state.current_profile < 4) ? g_profile_names[g_state.current_profile] : "INIT";
            if (next_profile != g_state.current_profile) {
                char status_buf[128];
                if ((next_profile == PROFILE_Gaming || next_profile == PROFILE_Gaming_MOBA) && active_game[0] != '\0') {
                    snprintf(status_buf, sizeof(status_buf), "%s (%s)", g_profile_names[next_profile], active_game);
                    log_state("Profiler", "Profile: %s -> %s [%s] (CPU: %d°C, Bat: %d°C %s, GPU: %d%%)",
                              prev_name, g_profile_names[next_profile], active_game, cpu_temp, bat_temp,
                              g_state.is_charging ? "[CHG]" : "", gpu_load);
                } else {
                    snprintf(status_buf, sizeof(status_buf), "%s", g_profile_names[next_profile]);
                    log_state("Profiler", "Profile: %s -> %s (CPU: %d°C, Bat: %d°C %s, GPU: %d%%)",
                              prev_name, g_profile_names[next_profile], cpu_temp, bat_temp,
                              g_state.is_charging ? "[CHG]" : "", gpu_load);
                }
                update_module_prop_status(status_buf);
            } else if (thermal_tier != g_state.thermal_tier) {
                log_warn("Temp", "Temp Tier: T%d -> T%d (CPU: %d°C, Bat: %d°C %s)",
                         g_state.thermal_tier, thermal_tier, cpu_temp, bat_temp,
                         g_state.is_charging ? "[CHG]" : "");
            }
            apply_profile(next_profile, thermal_tier, gpu_load);
            g_state.current_profile = next_profile;
            g_state.thermal_tier = thermal_tier;
        }

        if (g_state.is_charging) {
            if (bat_temp >= 45) {
                /* Thermal Protection: Suspend charging to protect battery & reduce heat */
                sysfs_write(g_nodes.charge_control, "0");
                sysfs_write("/sys/class/power_supply/battery/charging_enabled", "0");
                sysfs_write("/sys/class/power_supply/battery/input_suspend", "1");
            } else if (bat_temp >= 42) {
                sysfs_write(g_nodes.charge_control, "2");
                sysfs_write("/sys/class/power_supply/battery/charging_enabled", "1");
                sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
            } else if (bat_temp >= 37) {
                sysfs_write(g_nodes.charge_control, "4");
                sysfs_write("/sys/class/power_supply/battery/charging_enabled", "1");
                sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
            } else {
                sysfs_write(g_nodes.charge_control, "8");
                sysfs_write("/sys/class/power_supply/battery/charging_enabled", "1");
                sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
            }
        } else {
            sysfs_write(g_nodes.charge_control, "8");
            sysfs_write("/sys/class/power_supply/battery/charging_enabled", "1");
            sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
        }

        if (g_state.current_profile == PROFILE_Interactive) {
            enforce_interactive_gpu_polling(20);
        }

        tune_memory_pressure();
        fix_battery_cycle_count();
        track_charging_cycles(g_state.is_charging);

        static int s_rotate_counter = 0;
        if (++s_rotate_counter >= 30) {
            rotate_log();
            s_rotate_counter = 0;
        }

        int poll_timeout_ms = 2000;
        if (next_profile == PROFILE_Sleep) {
            poll_timeout_ms = (bat_temp >= 45) ? 3000 : 8000;
        } else if (next_profile == PROFILE_Gaming || next_profile == PROFILE_Gaming_MOBA) {
            poll_timeout_ms = (temp_delta >= 3 || cpu_temp >= 65) ? 500 : 1000;
        } else if (next_profile == PROFILE_Interactive) {
            poll_timeout_ms = (temp_delta >= 3 || cpu_temp >= 65) ? 1000 : 2000;
        }

        handle_ipc_events(poll_timeout_ms);
    }

    log_info("Daemon", "HyperCore daemon stopped. Restoring ROM baseline nodes...");
    restore_baseline_nodes();
    update_module_prop_status("Stopped");
    close_ipc_socket();
    remove_pid_file();
    return 0;
}
