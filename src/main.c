
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
#include "charger.hpp"

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
    if (pid < 0) {
        log_warn("Async", "fork() failed: %s (cmd skipped: %.48s)", strerror(errno), cmd);
        return;
    }

    if (pid == 0) {
        /* Close inherited FDs before second fork to prevent socket leaks to grandchild */
        long max_fd = sysconf(_SC_OPEN_MAX);
        if (max_fd < 0 || max_fd > 1024) max_fd = 256;
        for (long fd = STDERR_FILENO + 1; fd < max_fd; fd++) close((int)fd);

        pid_t grandchild = fork();
        if (grandchild == 0) {
            setsid();

            int null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
            if (null_fd >= 0) {
                dup2(null_fd, STDIN_FILENO);
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                if (null_fd > STDERR_FILENO) close(null_fd);
            }
            execl("/system/bin/sh", "sh", "-c", cmd, (char *)NULL);
            _exit(127);
        }
        /* Double-fork: middle child exits so grandchild is adopted by init */
        _exit((grandchild < 0) ? 1 : 0);
    }

    waitpid(pid, NULL, 0);
}

static void send_game_toast(const char *game_name, profile_t prof) {
    if (!game_name || game_name[0] == '\0') return;
    const char *prof_label = (prof >= 0 && prof < 4) ? g_profile_names[prof] : "Unknown";

    /* Escape single quotes to prevent injection in shell notification command */
    char safe_name[256];
    int si = 0;
    for (int gi = 0; game_name[gi] && si < (int)sizeof(safe_name) - 5; gi++) {
        if (game_name[gi] == '\'') {
            if (si + 4 < (int)sizeof(safe_name)) {
                safe_name[si++] = '\'';
                safe_name[si++] = '\\';
                safe_name[si++] = '\'';
                safe_name[si++] = '\'';
            }
        } else {
            safe_name[si++] = game_name[gi];
        }
    }
    safe_name[si] = '\0';

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "cmd notification post -t 'HyperCore' -S bigtext 'HyperCore' 'hypercore_game' '%s Profile Active [%s]'",
        prof_label, safe_name);
    async_system_cmd(cmd);
}

static void init_hardware_nodes(void) {
    memset(&g_nodes, 0, sizeof(g_nodes));

    if (access("/data/adb/modules/hypercore", F_OK) == 0) {
        strcpy(g_nodes.mod_dir, "/data/adb/modules/hypercore");
    } else if (access("/data/adb", F_OK) == 0) {
        mkdir("/data/adb/hypercore", 0755);
        strcpy(g_nodes.mod_dir, "/data/adb/hypercore");
    } else {
        strcpy(g_nodes.mod_dir, "/data/local/tmp");
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

    const char *gm_paths[] = {
        "/sys/class/touch/touch_dev/touch_thp_game",
        "/sys/class/touch/touch_dev/game_mode",
        "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_game_mode",
        NULL
    };
    for (int i = 0; gm_paths[i]; i++) {
        if (access(gm_paths[i], F_OK) == 0) {
            strcpy(g_nodes.touch_game_mode, gm_paths[i]);
            break;
        }
    }

    const char *sens_paths[] = {
        "/sys/class/touch/touch_dev/sensitivity",
        "/sys/class/touch/touch_dev/touch_sensitivity",
        "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_sensitivity",
        NULL
    };
    for (int i = 0; sens_paths[i]; i++) {
        if (access(sens_paths[i], F_OK) == 0) {
            strcpy(g_nodes.touch_sensitivity, sens_paths[i]);
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
    int bl_val = -1;
    if (g_nodes.backlight[0] != '\0' && access(g_nodes.backlight, F_OK) == 0) {
        bl_val = sysfs_read_int(g_nodes.backlight);
    } else {
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
                bl_val = sysfs_read_int(bl_paths[i]);
                break;
            }
        }
    }

    if (bl_val > 0) return 1;

    // [P1-C FIX] Secondary screen state check -- two separate paths:
    // (a) /sys/class/graphics/fb0/blank  -- integer: 0 = unblank (screen ON)
    // (b) /sys/class/drm/card0-DSI-N/dpms -- STRING: "On"/"Off"/"Standby"
    //     sysfs_read_int(dpms) always returns 0 since atoi("On") == atoi("Off") == 0,
    //     causing the old check (blank_val == 0 -> return 1) to always fire,
    //     making the device never enter PROFILE_Sleep on DRM-based panels.
    //     NOTE: Do NOT use card0-DSI-* glob inside a block comment -- the star-slash
    //     sequence prematurely closes the comment and causes compile errors.

    // (a) Framebuffer blank -- integer check
    if (access("/sys/class/graphics/fb0/blank", F_OK) == 0) {
        int blank_val = sysfs_read_int("/sys/class/graphics/fb0/blank");
        if (blank_val == 0) return 1; /* FB_BLANK_UNBLANK = 0 -> screen ON */
        if (blank_val >  0) return 0; /* FB_BLANK_POWERDOWN etc -> screen OFF */
    }

    // Read DRM DPMS as string since string nodes return 0 for sysfs_read_int
    {
        const char *dpms_paths[] = {
            "/sys/class/drm/card0-DSI-1/dpms",
            "/sys/class/drm/card0-DSI-2/dpms",
            "/sys/class/drm/card0/card0-DSI-1/dpms",
            NULL
        };
        for (int d = 0; dpms_paths[d]; d++) {
            if (access(dpms_paths[d], F_OK) == 0) {
                char dpms_buf[16] = "";
                if (sysfs_read_str(dpms_paths[d], dpms_buf, sizeof(dpms_buf))) {
                    if (strcmp(dpms_buf, "On") == 0) return 1;
                    return 0;
                }
            }
        }
    }

    return (bl_val > 0);
}

static int get_top_app_pid(void) {
    /* Prefer cgroup.procs over tasks to filter out system_server thread workers */
    const char *sources[] = {
        "/dev/cpuset/top-app/cgroup.procs",
        "/sys/fs/cgroup/top-app/cgroup.procs",
        NULL
    };
    pid_t self_pid = getpid();
    for (int s = 0; sources[s]; s++) {
        FILE *f = fopen(sources[s], "r");
        if (!f) continue;
        char line[32];
        int pid = 0;
        while (fgets(line, sizeof(line), f)) {
            int candidate = atoi(line);
            if (candidate <= 100 || candidate == self_pid) continue;
            char cpath[64];
            snprintf(cpath, sizeof(cpath), "/proc/%d/cmdline", candidate);
            int cfd = open(cpath, O_RDONLY | O_CLOEXEC);
            if (cfd >= 0) {
                char cmd[128];
                ssize_t n = read(cfd, cmd, sizeof(cmd) - 1);
                close(cfd);
                if (n > 0) {
                    cmd[n] = '\0';
                    /* Skip Android system services and input methods */
                    if (strncmp(cmd, "system_server", 13) == 0 ||
                        strncmp(cmd, "/system/", 8) == 0 ||
                        strncmp(cmd, "/vendor/", 8) == 0 ||
                        strstr(cmd, "inputmethod") != NULL ||
                        strstr(cmd, "libhypercore") != NULL) {
                        continue;
                    }
                    pid = candidate;
                    break;
                }
            }
        }
        fclose(f);
        if (pid > 0) return pid;
    }
    return 0;
}

static int is_process_alive(int pid) {
    if (pid <= 0) return 0;
    if (kill(pid, 0) != 0 && errno == ESRCH) return 0;
    char cpath[64];
    snprintf(cpath, sizeof(cpath), "/proc/%d/cmdline", pid);
    return (access(cpath, F_OK) == 0);
}

static int check_and_recover_sysfs_tampering(profile_t current_prof) {
    if ((int)current_prof < 0 || (int)current_prof >= 4) return 0;
    return audit_active_profile_state(current_prof, g_state.thermal_tier);
}

#include "integrity.hpp"

static int validate_hardware_target(void) {
    /* Verify MT6789 bi-cluster topology (policy0 + policy6) and GED/FPSGO stack */
    if (access("/sys/module/ged", F_OK) != 0 ||
        access("/sys/devices/system/cpu/cpufreq/policy0", F_OK) != 0 ||
        access("/sys/devices/system/cpu/cpufreq/policy6", F_OK) != 0 ||
        access("/sys/kernel/fpsgo", F_OK) != 0) {
        fprintf(stderr, "ERROR: Incompatible hardware target detected. "
                "HyperCore is strictly designed for MediaTek MT6789 Family "
                "(bi-cluster policy0+policy6, GED+FPSGO, Kernel 5.10.x).\n");
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
    signal(SIGPIPE, SIG_IGN);

    init_hardware_nodes();
    verify_module_integrity(g_nodes.mod_dir);
    save_baseline_nodes();
    write_pid_file();
    init_ipc_socket();

    log_info("Daemon", "HyperCore " VERSION " started.");

    memset(&g_state, 0, sizeof(g_state));
    g_state.current_profile = (profile_t)-1;

    detect_cpu_hardware_limits();
    apply_cpuset();
    apply_memory_tuning();
    apply_io_tuning();
    apply_gpu_tuning();
    apply_irq_tuning(PROFILE_Interactive);
    init_charge_control();
    sync_battery_cycle_count();
    load_gamelist();
    init_gamelist_watcher();

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
        int game_pid = 0;
        int game_active = is_game_in_foreground(active_game, sizeof(active_game), &custom_profile, &game_pid);

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
        static int       s_last_game_pid     = 0;
        static profile_t s_last_game_profile = PROFILE_Interactive;

        if (game_active) {
            if (game_pid > 0) s_last_game_pid = game_pid;
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
            /* Process death detection: if game PID terminated, immediately force return to Interactive */
            if (s_prev_game_active && (s_last_game_pid <= 0 || !is_process_alive(s_last_game_pid))) {
                s_game_absent_ticks = 3;
                g_state.gaming_hold_ticks = 0;
                s_prev_game_active = 0;
                log_info("Game", "Foreground game process death detected -> Immediate atomic rollback to Interactive");
            } else {
                s_game_absent_ticks++;
                if (s_game_absent_ticks >= 3) {
                    s_prev_game_active = 0;
                }
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
            g_state.gaming_hold_ticks = (custom_profile == PROFILE_Gaming || custom_profile == PROFILE_Gaming_MOBA) ? 2 : 0;
        } else if (g_state.gaming_hold_ticks > 0) {
            g_state.gaming_hold_ticks--;
            next_profile = s_last_game_profile;
        } else {
            next_profile = PROFILE_Interactive;
        }

        static int s_prev_cpu_temp = 0;
        int temp_delta = (s_prev_cpu_temp > 0) ? abs(cpu_temp - s_prev_cpu_temp) : 0;
        s_prev_cpu_temp = cpu_temp;

        static int s_prev_gpu_heavy = -1;
        int is_gpu_heavy = (gpu_load >= 20);

        /* Rate-limit anti-tamper audit to every 5 ticks (≈5s) to avoid reading
         * 3 sysfs nodes per second just for governor/GPU policy verification. */
        static int s_audit_tick = 0;
        int is_tampered = 0;
        if (++s_audit_tick >= 5) {
            s_audit_tick = 0;
            is_tampered = check_and_recover_sysfs_tampering(g_state.current_profile);
        }

        static int s_prev_gpu_load = 0;
        int gpu_spike = (gpu_load - s_prev_gpu_load >= 30);
        s_prev_gpu_load = gpu_load;

        if ((next_profile == PROFILE_Gaming || next_profile == PROFILE_Gaming_MOBA) && (gpu_spike || temp_delta >= 4)) {
            if (g_state.jitter_rescue_ticks == 0) {
                g_state.jitter_rescue_ticks = 2;
                sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "1");
                sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "1");

                static time_t s_last_jitter_log = 0;
                time_t now = time(NULL);
                if (now - s_last_jitter_log >= 5) {
                    log_warn("Jitter", "Frame-spike detected (GPU %d%%, ΔT %d°C) -> Ultra-Rescue Burst activated", gpu_load, temp_delta);
                    s_last_jitter_log = now;
                }
            }
        }

        if (g_state.jitter_rescue_ticks > 0) {
            g_state.jitter_rescue_ticks--;
            if (g_state.jitter_rescue_ticks == 0) {
                sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "0");
            }
        }

        if (next_profile != g_state.current_profile || thermal_tier != g_state.thermal_tier ||
            is_tampered ||
            ((next_profile == PROFILE_Gaming || next_profile == PROFILE_Gaming_MOBA) && is_gpu_heavy != s_prev_gpu_heavy)) {
            s_prev_gpu_heavy = is_gpu_heavy;
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

        tune_memory_pressure();

        /* Enforce user-selected charge mode every 2 ticks (~2-4s).
         * This re-writes charge_control_limit/input_suspend if mi_thermald
         * or ROM charger driver overwrote them since the last tick. */
        static int s_charge_enforce_counter = 0;
        if (++s_charge_enforce_counter >= 2) {
            s_charge_enforce_counter = 0;
            enforce_charge_mode();
        }

        update_status_json_file(cpu_temp, bat_temp);

        /* Auto-correct and synchronize true battery cycle count every 5 ticks (~10s) */
        static int s_cycle_sync_counter = 0;
        if (++s_cycle_sync_counter >= 5) {
            s_cycle_sync_counter = 0;
            sync_battery_cycle_count();
        }

        static int s_rotate_counter = 0;
        if (++s_rotate_counter >= 30) {
            rotate_log();
            log_reopen();
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

        if (g_screen_changed) {
            g_screen_changed = 0;
        }
        g_uevent_received = 0;
    }

    log_info("Daemon", "HyperCore daemon stopped. Restoring ROM baseline nodes...");
    restore_baseline_nodes();
    update_module_prop_status("Stopped");
    close_ipc_socket();

    if (g_nodes.inotify_fd >= 0) {
        close(g_nodes.inotify_fd);
        g_nodes.inotify_fd = -1;
    }
    if (g_nodes.netlink_fd >= 0) {
        close(g_nodes.netlink_fd);
        g_nodes.netlink_fd = -1;
    }
    remove_pid_file();
    return 0;
}
