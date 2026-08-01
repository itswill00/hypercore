/*
 * HyperCore - UNIX Domain Socket IPC Implementation
 * Author: @itswill00
 */

#include "ipc.hpp"
#include "sysfs.hpp"
#include "log.hpp"
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
        log_error("IPC", "Failed to create UNIX domain socket: %s", strerror(errno));
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
        log_error("IPC", "Failed to bind socket to %s: %s", s_sock_path, strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    chmod(s_sock_path, 0666);

    if (listen(s_server_fd, 5) < 0) {
        log_error("IPC", "Failed to listen on socket: %s", strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    log_info("IPC", "Socket IPC server listening at %s", s_sock_path);
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
        const char *prof_str = (g_state.current_profile >= 0 && g_state.current_profile < 5) ?
                                g_profile_names[g_state.current_profile] : "Unknown";

        time_t now = time(NULL);
        long uptime_sec = (long)difftime(now, s_start_time);

        char json[512];
        snprintf(json, sizeof(json),
            "{\"status\":\"ok\",\"pid\":%d,\"profile\":\"%s\",\"thermal_tier\":%d,"
            "\"cpu_temp\":%d,\"bat_temp\":%d,\"is_charging\":%d,\"gpu_load\":%d,\"uptime_sec\":%ld}\n",
            getpid(), prof_str, g_state.thermal_tier, cpu_temp, bat_temp,
            g_state.is_charging, gpu_load, uptime_sec);

        write(client_fd, json, strlen(json));
    } else if (strncmp(req, "PING", 4) == 0) {
        const char *pong = "PONG\n";
        write(client_fd, pong, strlen(pong));
    } else {
        const char *err = "{\"status\":\"error\",\"message\":\"Unknown command\"}\n";
        write(client_fd, err, strlen(err));
    }

    close(client_fd);
}

void handle_ipc_events(int timeout_ms) {
    if (s_server_fd < 0) {
        usleep(timeout_ms * 1000);
        return;
    }

    struct pollfd pfd;
    pfd.fd = s_server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret > 0 && (pfd.revents & POLLIN)) {
        int client_fd = accept(s_server_fd, NULL, NULL);
        if (client_fd >= 0) {
            process_client(client_fd);
        }
    }
}
