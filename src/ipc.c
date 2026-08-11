
#include "ipc.hpp"
#include "sysfs.hpp"
#include "memory.hpp"
#include "log.hpp"
#include "thermal.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <errno.h>

static int  s_server_fd = -1;
static char s_sock_path[256] = "";
static time_t s_start_time = 0;

int init_ipc_socket(void) {
    s_start_time = time(NULL);

    snprintf(s_sock_path, sizeof(s_sock_path), "%s/hypercore.sock", g_nodes.mod_dir);

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

    chmod(s_sock_path, 0666);

    if (listen(s_server_fd, 5) < 0) {
        log_error("Ipc", "Failed to listen on socket: %s", strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    log_info("Ipc", "Socket IPC server listening at %s", s_sock_path);
    return 0;
}

void close_ipc_socket(void) {
    if (s_server_fd >= 0) {
        close(s_server_fd);
        s_server_fd = -1;
    }
    if (s_sock_path[0] != '\0') {
        unlink(s_sock_path);
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

        /* [BUG-10 FIX] gpu_temp and chg_temp were previously hardcoded to
         * cpu_temp and bat_temp respectively. Read from actual thermal nodes.
         * MT6789 GPU thermal zone is typically near thermal_zone30+ (ged/gpufreq).
         * Charger temperature is from PMIC/charger thermal zone if present.
         * Fall back to cpu_temp / bat_temp if the dedicated nodes are absent. */
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

        /* [BUG-9 FIX] Expand json buffer 640→1024 bytes. The old 640-byte buffer
         * could be exceeded when sysfs string fields (bat_health, bat_status,
         * bat_tech) returned long values, causing snprintf to truncate the JSON
         * string mid-field, producing invalid JSON that silently broke the WebUI
         * parser and dropped all telemetry. 1024 bytes provides safe headroom. */
        char json[1024];
        snprintf(json, sizeof(json),
            "{\"status\":\"ok\",\"pid\":%d,\"profile\":\"%s\",\"thermal_tier\":%d,"
            "\"cpu_temp\":%d,\"bat_temp\":%d,\"gpu_temp\":%d,\"chg_temp\":%d,\"is_charging\":%d,\"gpu_load\":%d,\"battery_cycles\":%d,\"uptime_sec\":%ld,"
            "\"bat_health\":\"%s\",\"bat_status\":\"%s\",\"bat_tech\":\"%s\"}\n",
            getpid(), prof_str, g_state.thermal_tier, cpu_temp, bat_temp, gpu_temp, chg_temp,
            g_state.is_charging, gpu_load, bat_cycles, uptime_sec,
            bat_health, bat_status, bat_tech);

        write(client_fd, json, strlen(json));
    } else if (strncmp(req, "PURGE_RAM", 9) == 0 || strncmp(req, "CLEAR_CACHE", 11) == 0) {
        trigger_purge_ram_cache();
        const char *res = "{\"status\":\"ok\",\"message\":\"RAM and Cache purged successfully\"}\n";
        write(client_fd, res, strlen(res));
    } else if (strncmp(req, "PING", 4) == 0) {
        const char *pong = "PONG\n";
        write(client_fd, pong, strlen(pong));
    } else {
        const char *err = "{\"status\":\"error\",\"message\":\"Unknown command\"}\n";
        write(client_fd, err, strlen(err));
    }

    close(client_fd);
}

/* [M-1 FIX] Flags set by inotify/netlink handlers so the main loop can
 * react immediately to screen-state changes without waiting for the full
 * poll_timeout. Declared volatile because they are written here and read
 * in the main daemon loop. */
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
                        int cfl = fcntl(client_fd, F_GETFL, 0);
                        if (cfl != -1) fcntl(client_fd, F_SETFL, cfl | O_NONBLOCK);
                        process_client(client_fd);
                    }
                } else if (pfds[i].fd == g_nodes.inotify_fd) {
                    /* [M-1 FIX] Drain the inotify queue AND signal main loop.
                     * Previously events were drained silently. Now we set
                     * g_screen_changed so the next loop iteration re-evaluates
                     * is_screen_on() immediately instead of waiting up to 8s. */
                    char ev_buf[sizeof(struct inotify_event) + NAME_MAX + 1];
                    while (read(g_nodes.inotify_fd, ev_buf, sizeof(ev_buf)) > 0) {
                        g_screen_changed = 1;
                    }
                } else if (pfds[i].fd == g_nodes.netlink_fd) {
                    /* [BUG-8 FIX] Drain netlink uevent queue and flag for main loop.
                     * Android kernel uevent datagrams can be 800–2048 bytes.
                     * The old nl_buf[512] caused EMSGSIZE on many events, silently
                     * dropping battery/USB plug events and delaying screen-state
                     * reactions by up to the full poll timeout. Use 2048 bytes. */
                    char nl_buf[2048];
                    while (read(g_nodes.netlink_fd, nl_buf, sizeof(nl_buf)) > 0) {
                        g_uevent_received = 1;
                    }
                }
            }
        }
    }
}
