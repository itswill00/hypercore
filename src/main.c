#include "common.h"
#include "nodes.h"
#include "sysfs.h"
#include "baseline.h"
#include "module.h"
#include "util.h"
#include "log.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "io.h"
#include "gamelist.h"
#include "ipc.h"
#include "integrity.h"

const char *g_profile_names[] = { "Sleep", "Interactive", "Gaming", "Gaming MOBA" };

volatile sig_atomic_t g_running = 1;
struct hw_nodes g_nodes;
struct core_state g_state;

/* Fixed grace after a game leaves the foreground. Previously this was two
 * loop ticks, so the real duration depended on the gaming poll interval
 * (1-2 s) instead of being a defined window. */
#define GAMING_HOLD_SEC 3

static int s_lock_fd = -1;

typedef struct {
    int prev_top_pid;
    int game_absent_ticks;
    int prev_game_active;
    int last_game_pid;
    profile_t last_game_profile;
    time_t gaming_hold_until;
    time_t last_jitter_log;
    int prev_gpu_load;
    int boot_settle_tick;
    int rotate_counter;
} loop_state_t;

static loop_state_t s_loop = {
    .prev_top_pid = 0,
    .game_absent_ticks = 0,
    .prev_game_active = 0,
    .last_game_pid = 0,
    .last_game_profile = PROFILE_Interactive,
    .gaming_hold_until = 0,
    .last_jitter_log = 0,
    .prev_gpu_load = 0,
    .boot_settle_tick = 0,
    .rotate_counter = 0,
};

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void send_game_toast(const char *game_name, profile_t prof) {
    if (!game_name || game_name[0] == '\0') return;
    const char *label = (prof >= 0 && prof < PROFILE_COUNT) ? g_profile_names[prof] : "Unknown";

    /* Escape single quotes so the name cannot break out of the shell string. */
    char safe[256];
    int si = 0;
    for (int i = 0; game_name[i] && si < (int)sizeof(safe) - 5; i++) {
        if (game_name[i] == '\'') {
            safe[si++] = '\'';
            safe[si++] = '\\';
            safe[si++] = '\'';
            safe[si++] = '\'';
        } else {
            safe[si++] = game_name[i];
        }
    }
    safe[si] = '\0';

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "cmd notification post -t 'HyperCore' -S bigtext 'HyperCore' 'hypercore_game' '%s Profile Active [%s]'",
             label, safe);
    async_system_cmd(cmd);
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

/* Single-instance guard. Two daemons fighting over the same sysfs nodes is
 * unrecoverable: whichever exits last restores the baseline over the
 * survivor's tuning. flock() closes the start-up race that killing by name
 * leaves open, and the kernel drops it on SIGKILL too. */
static int acquire_single_instance_lock(void) {
    snprintf(g_nodes.lock_file, sizeof(g_nodes.lock_file), "%s/.hypercore_lock", g_nodes.data_dir);

    s_lock_fd = open(g_nodes.lock_file, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (s_lock_fd >= 0) fchmod(s_lock_fd, 0600);
    if (s_lock_fd < 0) {
        log_warn("Lock", "Cannot open lock file '%s' (errno=%d) - running WITHOUT single-instance guard",
                 g_nodes.lock_file, errno);
        return 0;
    }

    if (flock(s_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        int err = errno;
        close(s_lock_fd);
        s_lock_fd = -1;
        if (err == EWOULDBLOCK) {
            log_error("Lock", "Another HyperCore daemon holds %s - refusing to start a second instance.",
                      g_nodes.lock_file);
        } else {
            log_error("Lock", "flock() failed on %s (errno=%d) - refusing to start.", g_nodes.lock_file, err);
        }
        return -1;
    }

    if (ftruncate(s_lock_fd, 0) == 0) {
        char pidbuf[16];
        int n = snprintf(pidbuf, sizeof(pidbuf), "%d\n", getpid());
        if (n > 0) {
            ssize_t w = pwrite(s_lock_fd, pidbuf, (size_t)n, 0);
            (void)w;
        }
    }
    return 0;
}

static int is_screen_on(void) {
    if (g_nodes.backlight[0] == '\0') nodes_resolve_backlight();

    int bl_val = -1;
    if (g_nodes.backlight[0] != '\0' && access(g_nodes.backlight, F_OK) == 0) {
        bl_val = sysfs_read_int(g_nodes.backlight);
    }

    /* A present backlight node is authoritative: 0 means the panel is off and
     * falling through to fb0/DRM would misread a dark screen as awake. */
    if (bl_val >= 0) return (bl_val > 0);

    if (access("/sys/class/graphics/fb0/blank", F_OK) == 0) {
        int blank_val = sysfs_read_int("/sys/class/graphics/fb0/blank");
        if (blank_val == 0) return 1;
        if (blank_val > 0) return 0;
    }

    {
        const char *dpms_paths[] = {
            "/sys/class/drm/card0-DSI-1/dpms",
            "/sys/class/drm/card0-DSI-2/dpms",
            "/sys/class/drm/card0/card0-DSI-1/dpms",
            NULL
        };
        for (int d = 0; dpms_paths[d]; d++) {
            if (access(dpms_paths[d], F_OK) != 0) continue;
            char dpms_buf[16] = "";
            if (sysfs_read_str(dpms_paths[d], dpms_buf, sizeof(dpms_buf))) {
                return (strcmp(dpms_buf, "On") == 0);
            }
        }
    }

    /* Assuming "off" would pin the device to Sleep with no recovery path.
     * An unrecognised node on a live device is far more likely awake. */
    log_warn("Profiler", "Screen state undetectable - assuming screen ON");
    return 1;
}

static void verify_boot_apply(void) {
    char val[64] = "";
    if (g_nodes.touch_thp_smooth[0] != '\0' &&
        sysfs_read_str(g_nodes.touch_thp_smooth, val, sizeof(val))) {
        if (strcmp(val, "1") != 0) {
            log_warn("Profiler", "Boot settle: touch smoothing is '%s', expected '1' - will retry on next transition", val);
        } else {
            log_info("Profiler", "Boot settle verified: Interactive fully applied (touch smoothing on)");
        }
    } else {
        log_warn("Profiler", "Boot settle: touch smoothing node still missing (late touch probe?)");
    }
    if (g_stock_baseline.gov0[0] != '\0' &&
        sysfs_read_str(N_CPU_GOV0[0], val, sizeof(val))) {
        if (strcmp(val, g_stock_baseline.gov0) != 0) {
            log_warn("Profiler", "Boot settle: policy0 governor is '%s', stock is '%s'", val, g_stock_baseline.gov0);
        }
    }
}

static int is_process_alive(int pid) {
    if (pid <= 0) return 0;
    if (kill(pid, 0) != 0 && errno == ESRCH) return 0;
    char cpath[64];
    snprintf(cpath, sizeof(cpath), "/proc/%d/cmdline", pid);
    return (access(cpath, F_OK) == 0);
}

static int validate_hardware_target(void) {
    /* MT6789 bi-cluster topology + the GED/FPSGO stack HyperCore drives. */
    if (access("/sys/module/ged", F_OK) == 0 &&
        access("/sys/devices/system/cpu/cpufreq/policy0", F_OK) == 0 &&
        access("/sys/devices/system/cpu/cpufreq/policy6", F_OK) == 0 &&
        access("/sys/kernel/fpsgo", F_OK) == 0) {
        return 1;
    }

    const char *msg = "FATAL: incompatible hardware target. HyperCore requires a "
                      "MediaTek MT6789-family device (policy0+policy6, GED+FPSGO, kernel 5.10-6.12).";
    fprintf(stderr, "%s\n", msg);

    /* service.sh redirects stderr to /dev/null, so a silent refusal used to be
     * invisible. Append to the module log directly when the dir already exists. */
    int fd = open(LOG_PATH, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (fd >= 0) {
        char line[320];
        int n = snprintf(line, sizeof(line), "[HYPERCORE] [ERROR] [Hardware] %s\n", msg);
        if (n > 0) {
            ssize_t w = write(fd, line, (size_t)n);
            (void)w;
        }
        close(fd);
    }
    return 0;
}

/* Transient read-ahead: launch boost outranks app-switch boost, which outranks
 * the profile's steady value. One owner (this function), one writer. */
static void refresh_read_ahead(profile_t prof) {
    if (g_state.launch_boost_ticks > 0) set_read_ahead("512");
    else if (g_state.app_boost_ticks > 0) set_read_ahead("384");
    else set_read_ahead(profile_read_ahead(prof));
}

static void clear_transient_boosts(void) {
    g_state.launch_boost_ticks = 0;
    g_state.app_boost_ticks = 0;
    if (g_state.jitter_rescue_ticks > 0) {
        sysfs_write_fallback(N_FPSGO_ULTRA, "0");
        g_state.jitter_rescue_ticks = 0;
    }
}

/* Game start / death edges. Called once per tick with the single foreground
 * scan result. */
static void tick_game_edges(const foreground_t *fg, time_t now) {
    if (fg->is_game) {
        if (fg->game_pid > 0) s_loop.last_game_pid = fg->game_pid;
        s_loop.game_absent_ticks = 0;
        if (!s_loop.prev_game_active) {
            g_state.launch_boost_ticks = 3;
            s_loop.prev_game_active = 1;
            send_game_toast(fg->game_name, fg->game_profile);
            log_info("Game", "Game launch boost: 512 KB read-ahead [%s] profile: %s",
                     fg->game_name, g_profile_names[fg->game_profile]);
        }
        return;
    }

    if (s_loop.prev_game_active && s_loop.last_game_pid > 0 &&
        !is_process_alive(s_loop.last_game_pid)) {
        /* Real death, not an alt-tab: roll back immediately instead of waiting
         * out the anti-flap hold. */
        s_loop.game_absent_ticks = 3;
        s_loop.gaming_hold_until = 0;
        s_loop.prev_game_active = 0;
        log_info("Game", "Foreground game process death detected - immediate rollback to Interactive");
        return;
    }

    s_loop.game_absent_ticks++;
    if (s_loop.game_absent_ticks >= 3) {
        if (s_loop.prev_game_active) {
            s_loop.gaming_hold_until = now + GAMING_HOLD_SEC;
        }
        s_loop.prev_game_active = 0;
    }
}

static profile_t decide_profile(const foreground_t *fg, int screen_on, time_t now) {
    if (!screen_on) {
        clear_transient_boosts();
        s_loop.gaming_hold_until = 0;
        return PROFILE_Sleep;
    }
    if (fg->is_game) {
        s_loop.last_game_profile = fg->game_profile;
        return fg->game_profile;
    }
    if (s_loop.gaming_hold_until > now) {
        return s_loop.last_game_profile;
    }
    if (g_state.manual_profile >= 0 && g_state.manual_profile < PROFILE_COUNT) {
        return (profile_t)g_state.manual_profile;
    }
    return PROFILE_Interactive;
}

static const char *profile_name_or_init(profile_t p) {
    return (p >= 0 && p < PROFILE_COUNT) ? g_profile_names[p] : "INIT";
}

static void apply_transition(profile_t next, const foreground_t *fg, int gpu_load) {
    const char *prev_name = profile_name_or_init(g_state.current_profile);

    char status[128];
    if ((next == PROFILE_Gaming || next == PROFILE_Gaming_MOBA) && fg->game_name[0] != '\0') {
        snprintf(status, sizeof(status), "%s (%s)", g_profile_names[next], fg->game_name);
        log_state("Profiler", "Profile: %s -> %s [%s] (GPU: %d%%)",
                  prev_name, g_profile_names[next], fg->game_name, gpu_load);
    } else {
        snprintf(status, sizeof(status), "%s", g_profile_names[next]);
        log_state("Profiler", "Profile: %s -> %s (GPU: %d%%)",
                  prev_name, g_profile_names[next], gpu_load);
    }

    update_module_prop_status(status);
    apply_profile(next);
    g_state.current_profile = next;
}

static void tick_jitter(profile_t next, int gpu_load) {
    int gpu_spike = (gpu_load - s_loop.prev_gpu_load >= 30);
    s_loop.prev_gpu_load = gpu_load;

    if ((next == PROFILE_Gaming || next == PROFILE_Gaming_MOBA) && gpu_spike &&
        g_state.jitter_rescue_ticks == 0) {
        g_state.jitter_rescue_ticks = 2;
        sysfs_write_fallback(N_FPSGO_ULTRA, "1");
        sysfs_write_fallback(N_FPSGO_BOOST_TA, "1");

        time_t now = time(NULL);
        if (now - s_loop.last_jitter_log >= 5) {
            log_warn("Jitter", "Frame-spike detected (GPU %d%%) - ultra-rescue burst", gpu_load);
            s_loop.last_jitter_log = now;
        }
    }

    if (g_state.jitter_rescue_ticks > 0) {
        g_state.jitter_rescue_ticks--;
        if (g_state.jitter_rescue_ticks == 0) {
            sysfs_write_fallback(N_FPSGO_ULTRA, "0");
        }
    }
}

/* Early boot: vendor init can overwrite nodes seconds after boot_completed and
 * touch ICs probe late, while the status already says Interactive. Re-apply a
 * few times instead of trusting the one-shot startup apply. */
static void tick_boot_settle(profile_t next, const foreground_t *fg, int gpu_load) {
    s_loop.boot_settle_tick++;
    if (s_loop.boot_settle_tick != 5 && s_loop.boot_settle_tick != 15 && s_loop.boot_settle_tick != 30) return;
    if (next != PROFILE_Interactive || g_state.current_profile != PROFILE_Interactive) return;
    if (fg->is_game || g_state.manual_profile >= 0) return;

    nodes_rediscover_touch();
    apply_profile(PROFILE_Interactive);
    if (s_loop.boot_settle_tick == 30) {
        verify_boot_apply();
    } else {
        log_info("Profiler", "Boot settle re-apply Interactive (%d/3)",
                 s_loop.boot_settle_tick == 5 ? 1 : 2);
    }
    (void)gpu_load;
}

static int poll_timeout_for(profile_t prof, int gpu_load) {
    if (prof == PROFILE_Sleep) return 8000;
    if (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA) return gpu_load >= 90 ? 500 : 1000;
    return 2000;
}

static void tick_log_rotation(void) {
    if (++s_loop.rotate_counter < 30) return;
    rotate_log();
    s_loop.rotate_counter = 0;
}

int main(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--verify-integrity") == 0) {
        return verify_module_integrity(argv[2]);
    }

    if (!validate_hardware_target()) return 1;

    if (daemon(0, 0) != 0) {
        perror("daemonization failed");
        return 1;
    }

    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
    }
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    nodes_init();
    if (acquire_single_instance_lock() != 0) return 1;

    /* Fail closed: a tampered payload must never run the tuning loop. */
    if (verify_module_integrity(g_nodes.mod_dir) != 0) {
        log_error("Security", "Payload integrity check failed - refusing to start.");
        return 1;
    }

    detect_cpu_hardware_limits();
    detect_max_gpu_freq();
    init_irq_targets();
    baseline_init();
    write_pid_file();
    init_ipc_socket();

    log_info("Daemon", "HyperCore " VERSION " started.");

    memset(&g_state, 0, sizeof(g_state));
    g_state.current_profile = PROFILE_NONE;
    g_state.manual_profile = -1;

    load_gamelist();
    init_gamelist_watcher();

    apply_profile(PROFILE_Interactive);
    g_state.current_profile = PROFILE_Interactive;
    log_state("Profiler", "Profile: INIT -> Interactive (boot)");
    update_module_prop_status("Interactive");

    while (g_running) {
        time_t now = time(NULL);
        int gpu_load = node_read_int(N_GED_GPU_LOAD);

        foreground_t fg;
        foreground_scan(&fg);

        int screen_on = is_screen_on();

        /* App-switch boost: a new top-app process without a game is usually a
         * launcher/hot-app cold start. */
        if (fg.pid > 0 && s_loop.prev_top_pid > 0 &&
            fg.pid != s_loop.prev_top_pid && !fg.is_game) {
            g_state.app_boost_ticks = 3;
        }
        s_loop.prev_top_pid = fg.pid;

        tick_game_edges(&fg, now);
        profile_t next = decide_profile(&fg, screen_on, now);

        if (next != g_state.current_profile) {
            apply_transition(next, &fg, gpu_load);
            /* Launch boost must survive the profile apply: the matrix writes
             * the steady 256 KB value for Gaming/MOBA. */
            refresh_read_ahead(next);
        }

        tick_jitter(next, gpu_load);

        if (g_state.launch_boost_ticks > 0) {
            g_state.launch_boost_ticks--;
            if (g_state.launch_boost_ticks == 0) refresh_read_ahead(next);
        }
        if (g_state.app_boost_ticks > 0) {
            g_state.app_boost_ticks--;
            if (g_state.app_boost_ticks == 0 && g_state.launch_boost_ticks == 0) {
                refresh_read_ahead(next);
            }
        }

        tick_boot_settle(next, &fg, gpu_load);

        tune_memory_pressure();

        update_status_json_file();
        tick_log_rotation();

        handle_ipc_events(poll_timeout_for(next, gpu_load));

        /* Edge flags only pull the poll() forward; the loop re-evaluates every
         * tick regardless. */
        g_screen_changed = 0;
        g_uevent_received = 0;
    }

    log_info("Daemon", "HyperCore daemon stopped. Restoring ROM baseline nodes...");
    baseline_restore();
    update_module_prop_status("Stopped");
    close_ipc_socket();

    if (g_nodes.inotify_fd >= 0) close(g_nodes.inotify_fd);
    if (g_nodes.netlink_fd >= 0) close(g_nodes.netlink_fd);

    if (s_lock_fd >= 0) {
        /* Never unlink the lock file: a successor may already hold a fresh
         * flock on it, and the kernel releases ours on exit anyway. */
        close(s_lock_fd);
        s_lock_fd = -1;
    }
    remove_pid_file();
    return 0;
}
