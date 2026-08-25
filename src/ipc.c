
#include "ipc.hpp"
#include "cpu.hpp"
#include "sysfs.hpp"
#include "memory.hpp"
#include "log.hpp"
#include "thermal.hpp"
#include "charger.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <errno.h>

static int  s_server_fd = -1;
static char s_sock_path[256] = "";
static time_t s_start_time = 0;

int init_ipc_socket(void) {
    s_start_time = time(NULL);

    /* Use /dev/hypercore.sock as primary socket path.
     * /dev/ is the standard POSIX location for IPC sockets and is NOT
     * in the /data/adb/ root-manager zone that banking app scanners target.
     * A backward-compat symlink is created at the old mod_dir path. */
    snprintf(s_sock_path, sizeof(s_sock_path), "/dev/hypercore.sock");

    s_server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s_server_fd < 0) {
        log_error("Ipc", "Failed to create UNIX domain socket: %s", strerror(errno));
        return -1;
    }

    int flags = fcntl(s_server_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(s_server_fd, F_SETFL, flags | O_NONBLOCK);
    }

    unlink(s_sock_path);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, s_sock_path, sizeof(addr.sun_path) - 1);

    if (bind(s_server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("Ipc", "Failed to bind socket to %s: %s", s_sock_path, strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    /* Allow root, webui sandbox, and shell clients to connect to socket (0666) */
    chmod(s_sock_path, 0666);

    if (listen(s_server_fd, 5) < 0) {
        log_error("Ipc", "Failed to listen on socket: %s", strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    log_info("Ipc", "Socket IPC server listening at %s", s_sock_path);

    /* Create backward-compat symlinks at legacy paths so older WebUI nc commands
     * and external scripts that reference the old /data/adb/ path still work.
     * These are symlinks only — the actual socket is in /dev/ (scanner-safe). */
    char legacy_mod[300], legacy_data[300];
    snprintf(legacy_mod,  sizeof(legacy_mod),  "%s/hypercore.sock", g_nodes.mod_dir);
    snprintf(legacy_data, sizeof(legacy_data), "/data/adb/hypercore/hypercore.sock");
    unlink(legacy_mod);  symlink(s_sock_path, legacy_mod);
    unlink(legacy_data); symlink(s_sock_path, legacy_data);

    return 0;
}

void close_ipc_socket(void) {
    if (s_server_fd >= 0) {
        close(s_server_fd);
        s_server_fd = -1;
    }
    if (s_sock_path[0] != '\0') {
        unlink(s_sock_path);
        /* Clean up backward-compat symlinks too */
        char legacy_mod[300], legacy_data[300];
        snprintf(legacy_mod,  sizeof(legacy_mod),  "%s/hypercore.sock", g_nodes.mod_dir);
        snprintf(legacy_data, sizeof(legacy_data), "/data/adb/hypercore/hypercore.sock");
        unlink(legacy_mod);
        unlink(legacy_data);
    }
}

static void process_client(int client_fd) {
    char req[256];
    ssize_t n = read(client_fd, req, sizeof(req) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    req[n] = '\0';

    while (n > 0 && (req[n - 1] == '\r' || req[n - 1] == '\n' || req[n - 1] == ' ')) {
        req[--n] = '\0';
    }

    if (strncmp(req, "GET_STATUS", 10) == 0 || strncmp(req, "STATUS", 6) == 0) {
        int cpu_temp = sysfs_read_int(g_nodes.cpu_temp);
        int bat_temp = sysfs_read_int(g_nodes.bat_temp);
        if (cpu_temp > 1000) cpu_temp /= 1000;
        if (bat_temp > 1000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;

        int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");
        const char *prof_str = (g_state.current_profile >= 0 && g_state.current_profile < 4) ?
                                g_profile_names[g_state.current_profile] : "Unknown";

        time_t now = time(NULL);
        long uptime_sec = (long)difftime(now, s_start_time);
        int bat_cycles = get_true_battery_cycles();

        char bat_health[32] = "Good";
        if (!sysfs_read_str("/sys/class/power_supply/battery/health", bat_health, sizeof(bat_health))) {
            sysfs_read_str("/sys/class/power_supply/bms/health", bat_health, sizeof(bat_health));
        }

        char bat_status[32] = "Discharging";
        if (!sysfs_read_str("/sys/class/power_supply/battery/status", bat_status, sizeof(bat_status))) {
            sysfs_read_str("/sys/class/power_supply/bms/status", bat_status, sizeof(bat_status));
        }

        char bat_tech[32] = "Li-poly";
        if (!sysfs_read_str("/sys/class/power_supply/battery/technology", bat_tech, sizeof(bat_tech))) {
            sysfs_read_str("/sys/class/power_supply/bms/technology", bat_tech, sizeof(bat_tech));
        }

        /* Read dedicated GPU and charger thermal zones with CPU/battery fallback */
        int gpu_temp = 0;
        const char *gpu_therm_nodes[] = {
            "/sys/class/thermal/thermal_zone30/temp",
            "/sys/class/thermal/thermal_zone28/temp",
            "/sys/class/thermal/thermal_zone26/temp",
            NULL
        };
        for (int gi = 0; gpu_therm_nodes[gi]; gi++) {
            int v = sysfs_read_int(gpu_therm_nodes[gi]);
            if (v > 0) { gpu_temp = (v > 1000) ? v / 1000 : v; break; }
        }
        if (gpu_temp <= 0) gpu_temp = cpu_temp;

        int chg_temp = 0;
        const char *chg_therm_nodes[] = {
            "/sys/class/power_supply/mtk-master-charger/temp",
            "/sys/class/power_supply/charger/temp",
            "/sys/class/power_supply/battery/temp_ambient",
            NULL
        };
        for (int ci = 0; chg_therm_nodes[ci]; ci++) {
            int v = sysfs_read_int(chg_therm_nodes[ci]);
            if (v > 0) { chg_temp = (v > 1000) ? v / 1000 : (v > 100) ? v / 10 : v; break; }
        }
        if (chg_temp <= 0) chg_temp = bat_temp;

        char json[1024];
        snprintf(json, sizeof(json),
            "{\"status\":\"ok\",\"pid\":%d,\"profile\":\"%s\",\"thermal_tier\":%d,"
            "\"cpu_temp\":%d,\"bat_temp\":%d,\"gpu_temp\":%d,\"chg_temp\":%d,\"is_charging\":%d,\"gpu_load\":%d,\"battery_cycles\":%d,\"uptime_sec\":%ld,"
            "\"bat_health\":\"%s\",\"bat_status\":\"%s\",\"bat_tech\":\"%s\","
            "\"charge_mode\":%d,\"charge_mode_name\":\"%s\",\"charge_thermal_override\":%d,\"charger_supported\":%d}\n",
            getpid(), prof_str, g_state.thermal_tier, cpu_temp, bat_temp, gpu_temp, chg_temp,
            g_state.is_charging, gpu_load, bat_cycles, uptime_sec,
            bat_health, bat_status, bat_tech,
            g_state.user_charge_mode, charge_mode_name(g_state.user_charge_mode),
            g_state.charge_mode_thermal_override, g_state.charger_supported);

        write(client_fd, json, strlen(json));
    } else if (strncmp(req, "SET_PROFILE:", 12) == 0) {
        const char *pname = req + 12;
        profile_t new_prof = PROFILE_Interactive;
        if (strncasecmp(pname, "SLEEP", 5) == 0) new_prof = PROFILE_Sleep;
        else if (strncasecmp(pname, "GAMING_MOBA", 11) == 0 || strncasecmp(pname, "MOBA", 4) == 0) new_prof = PROFILE_Gaming_MOBA;
        else if (strncasecmp(pname, "GAMING", 6) == 0) new_prof = PROFILE_Gaming;
        else if (strncasecmp(pname, "INTERACTIVE", 11) == 0) new_prof = PROFILE_Interactive;

        apply_profile(new_prof, g_state.thermal_tier, 0);
        g_state.current_profile = new_prof;
        log_state("Ipc", "Manual profile switch via IPC -> %s", g_profile_names[new_prof]);

        char res[256];
        snprintf(res, sizeof(res), "{\"status\":\"ok\",\"message\":\"Profile switched to %s\"}\n", g_profile_names[new_prof]);
        write(client_fd, res, strlen(res));
    } else if (strncmp(req, "PURGE_RAM", 9) == 0 || strncmp(req, "CLEAR_CACHE", 11) == 0) {
        trigger_purge_ram_cache();
        const char *res = "{\"status\":\"ok\",\"message\":\"RAM and Cache purged successfully\"}\n";
        write(client_fd, res, strlen(res));
    } else if (strncmp(req, "PING", 4) == 0) {
        const char *pong = "PONG\n";
        write(client_fd, pong, strlen(pong));
    } else if (strncmp(req, "SET_CHARGE_MODE:", 16) == 0) {
        int mode = atoi(req + 16);
        if (mode < CHARGE_MODE_OEM || mode > CHARGE_MODE_VIOLENT) {
            const char *err = "{\"status\":\"error\",\"message\":\"Invalid charge mode (0-5 only)\"}\n";
            write(client_fd, err, strlen(err));
        } else {
            set_charge_mode(mode);
            int cpu_temp = sysfs_read_int(g_nodes.cpu_temp);
            int bat_temp = sysfs_read_int(g_nodes.bat_temp);
            if (cpu_temp > 1000) cpu_temp /= 1000;
            if (bat_temp > 1000) bat_temp /= 1000;
            else if (bat_temp > 100) bat_temp /= 10;
            update_status_json_file(cpu_temp, bat_temp);

            char res[256];
            snprintf(res, sizeof(res),
                "{\"status\":\"ok\",\"charge_mode\":%d,\"charge_mode_name\":\"%s\",\"charger_supported\":%d}\n",
                mode, charge_mode_name(mode), g_state.charger_supported);
            write(client_fd, res, strlen(res));
        }
    } else if (strncmp(req, "GET_CHARGE_MODE", 15) == 0) {
        int bat_temp = sysfs_read_int(g_nodes.bat_temp);
        if (bat_temp > 1000) bat_temp /= 1000;
        else if (bat_temp > 100) bat_temp /= 10;
        char res[256];
        snprintf(res, sizeof(res),
            "{\"status\":\"ok\",\"charge_mode\":%d,\"charge_mode_name\":\"%s\","
            "\"charge_thermal_override\":%d,\"charger_supported\":%d,\"bat_temp\":%d}\n",
            g_state.user_charge_mode, charge_mode_name(g_state.user_charge_mode),
            g_state.charge_mode_thermal_override, g_state.charger_supported, bat_temp);
        write(client_fd, res, strlen(res));
    } else {
        const char *err = "{\"status\":\"error\",\"message\":\"Unknown command\"}\n";
        write(client_fd, err, strlen(err));
    }

    close(client_fd);
}

/* Event flags set by inotify/netlink handlers to trigger immediate main-loop evaluation */
volatile int g_screen_changed = 0;
volatile int g_uevent_received = 0;

void handle_ipc_events(int timeout_ms) {
    struct pollfd pfds[3];
    int nfds = 0;

    if (s_server_fd >= 0) {
        pfds[nfds].fd = s_server_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;
    }

    if (g_nodes.inotify_fd >= 0) {
        pfds[nfds].fd = g_nodes.inotify_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;
    }

    if (g_nodes.netlink_fd >= 0) {
        pfds[nfds].fd = g_nodes.netlink_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;
    }

    if (nfds == 0) {
        usleep(timeout_ms * 1000);
        return;
    }

    int ret = poll(pfds, nfds, timeout_ms);
    if (ret > 0) {
        for (int i = 0; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == s_server_fd) {
                    int client_fd = accept(s_server_fd, NULL, NULL);
                    if (client_fd >= 0) {
                        /* Use SO_RCVTIMEO instead of O_NONBLOCK on the client socket.
                         * O_NONBLOCK causes read() to return EAGAIN immediately if the
                         * client hasn't written the request yet (e.g. nc takes ~1-2ms
                         * after connect to write). A 50ms recv timeout is generous enough
                         * to always receive the command without hanging on rogue clients. */
                        struct timeval tv = { .tv_sec = 0, .tv_usec = 50000 };
                        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                        process_client(client_fd);
                    }
                } else if (pfds[i].fd == g_nodes.inotify_fd) {
                    /* Use 4096-byte buffer to drain multiple inotify events per syscall.
                     * The kernel often queues several events between poll wakeups and
                     * a min-sized buffer forces N separate read() calls per event. */
                    char ev_buf[4096];
                    while (read(g_nodes.inotify_fd, ev_buf, sizeof(ev_buf)) > 0) {
                        g_screen_changed = 1;
                    }
                } else if (pfds[i].fd == g_nodes.netlink_fd) {
                    /* Read kernel uevents (up to 2048 bytes) and flag screen/power change */
                    char nl_buf[2048];
                    while (read(g_nodes.netlink_fd, nl_buf, sizeof(nl_buf)) > 0) {
                        g_uevent_received = 1;
                    }
                }
            }
        }
    }
}

void update_status_json_file(int cpu_temp, int bat_temp) {
    const char *prof_str = (g_state.current_profile >= 0 && g_state.current_profile < 4)
                           ? g_profile_names[g_state.current_profile] : "Interactive";

    /* cpu_temp and bat_temp are pre-read by the main loop — no redundant sysfs open here */
    int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");
    int bat_cycles = get_true_battery_cycles();

    char json[1024];
    snprintf(json, sizeof(json),
        "{\"status\":\"ok\",\"pid\":%d,\"profile\":\"%s\",\"thermal_tier\":%d,"
        "\"cpu_temp\":%d,\"bat_temp\":%d,\"is_charging\":%d,\"gpu_load\":%d,\"battery_cycles\":%d,"
        "\"charge_mode\":%d,\"charge_mode_name\":\"%s\",\"charge_thermal_override\":%d,\"charger_supported\":%d}\n",
        getpid(), prof_str, g_state.thermal_tier, cpu_temp, bat_temp,
        g_state.is_charging, gpu_load, bat_cycles,
        g_state.user_charge_mode, charge_mode_name(g_state.user_charge_mode),
        g_state.charge_mode_thermal_override, g_state.charger_supported);

    const char *paths[] = {
        "/data/adb/modules/hypercore/status.json",
        "/data/adb/hypercore/status.json",
        NULL
    };

    for (int i = 0; paths[i]; i++) {
        char tmp[300];
        snprintf(tmp, sizeof(tmp), "%s.tmp", paths[i]);
        FILE *f = fopen(tmp, "w");
        if (f) {
            fputs(json, f);
            fclose(f);
            rename(tmp, paths[i]);
        }
    }
}
