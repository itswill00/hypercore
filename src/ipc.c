/* Required for struct ucred / SO_PEERCRED before any header is pulled in. */
#define _GNU_SOURCE

#include "ipc.h"
#include "cpu.h"
#include "common.h"
#include "sysfs.h"
#include "memory.h"
#include "log.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <errno.h>

static int  s_server_fd = -1;
static char s_sock_path[256] = "";
static time_t s_start_time = 0;

/* Read-only hardware telemetry for status/IPC (zero tuning, informational
 * only). Missing nodes yield 0 and the WebUI hides the corresponding rows.
 * NOTE: dvfsrc cur_freq is ~4.3 GHz, which overflows a 32-bit int, so it is
 * reported in MHz (fits) and the WebUI formats GHz. */
static void read_extra_telemetry(int *dvfsrc_mhz) {
    char buf[32] = "";
    long hz = 0;
    if (sysfs_read_str("/sys/class/devfreq/mtk-dvfsrc-devfreq/cur_freq", buf, sizeof(buf))) {
        hz = strtol(buf, NULL, 10);
    }
    *dvfsrc_mhz = (hz > 0) ? (int)(hz / 1000000L) : 0;
}

/* One helper for the status-refresh blocks after a state change. */
static void ipc_sync_status(void) {
    update_status_json_file();
}

/* ---------------------------------------------------------------------------
 * Client authorisation
 *
 * The socket handles privileged actions (profile switching, global
 * page-cache purge), so "can connect" must not imply "may command".
 * /dev is world-traversable and the socket file mode cannot express
 * "root or shell", so the file stays connectable and the real gate is the
 * kernel-reported peer credential.
 *
 * Trusted UIDs:
 *   0    — root: KSU/APatch/Magisk WebUI bridges and hypercore-bugreport all
 *          spawn their helper as root, so every legitimate client lands here.
 *   2000 — adb shell, for `echo GET_STATUS | nc -U /dev/hypercore.sock`.
 *
 * Adding a UID here is a privilege grant. Do not add 1000/1001 (system) or any
 * app UID: any app in those UIDs would gain profile control.
 *
 * Note on uid 2000: it is not read-only. Anyone holding an adb pairing token —
 * which on a device with wireless debugging enabled is anyone near it who has
 * paired once — can drive PURGE_RAM and profile switching. This is deliberate,
 * because the documented `adb shell` workflow depends on it, but it is control
 * rather than telemetry readback, so it is called out here rather than left
 * implicit. Tighten it to GET_* only if that tradeoff is ever revisited.
 * ------------------------------------------------------------------------- */
static int client_is_trusted(int client_fd) {
    struct ucred cred;
    socklen_t cred_len = sizeof(cred);

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) != 0) {
        return 0;
    }
    if (cred_len != sizeof(cred)) {
        return 0; /* kernel returned a short struct — refuse rather than guess */
    }
    return cred.uid == 0 || cred.uid == 2000;
}

/* Read a full request. A single read() truncates commands that arrive split
 * across segments, which silently mis-routed "SET_PROFILE:GAM" to Interactive
 * because the prefix compare fell through. Loop until newline, EOF, or full
 * buffer, and rely on SO_RCVTIMEO to bound the wait. */
static int read_request(int client_fd, char *buf, size_t cap) {
    size_t total = 0;

    while (total + 1 < cap) {
        ssize_t n = read(client_fd, buf + total, cap - 1 - total);
        if (n > 0) {
            total += (size_t)n;
            buf[total] = '\0';
            if (memchr(buf, '\n', total) != NULL) break; /* complete request */
            continue;
        }
        if (n == 0) break;              /* peer closed */
        if (errno == EINTR) continue;
        break;                          /* EAGAIN / recv timeout */
    }
    buf[total] = '\0';
    return (int)total;
}

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

    /* The socket file must stay connectable so that non-root clients (adb
     * shell, uid 2000) can reach accept() and be evaluated by
     * client_is_trusted(). Authorisation is enforced from the kernel-reported
     * peer credential, not from this mode — a mode of 0600 would lock out
     * `adb shell` without adding any real protection. */
    chmod(s_sock_path, 0666);

    if (listen(s_server_fd, 5) < 0) {
        log_error("Ipc", "Failed to listen on socket: %s", strerror(errno));
        close(s_server_fd);
        s_server_fd = -1;
        return -1;
    }

    log_info("Ipc", "Socket IPC server listening at %s", s_sock_path);

    /* Create backward-compat symlink in persistent data_dir so external scripts
     * that reference /data/adb/hypercore/hypercore.sock still work.
     * The actual primary socket is in /dev/ (scanner-safe). */
    char legacy_data[300];
    snprintf(legacy_data, sizeof(legacy_data), "%s/hypercore.sock", g_nodes.data_dir);
    unlink(legacy_data); symlink(s_sock_path, legacy_data);

    /* Ensure old mod_dir symlink is removed */
    if (strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        char old_sock[300];
        snprintf(old_sock, sizeof(old_sock), "%s/hypercore.sock", g_nodes.mod_dir);
        unlink(old_sock);
    }

    return 0;
}

void close_ipc_socket(void) {
    if (s_server_fd >= 0) {
        close(s_server_fd);
        s_server_fd = -1;
    }
    if (s_sock_path[0] != '\0') {
        unlink(s_sock_path);
        /* Clean up backward-compat symlink */
        char legacy_data[300];
        snprintf(legacy_data, sizeof(legacy_data), "%s/hypercore.sock", g_nodes.data_dir);
        unlink(legacy_data);
    }
}

/* One serializer for the whole status document.
 *
 * GET_STATUS and the periodic status.json writer used to carry two separate
 * snprintf format strings that had already drifted: the socket reply reported
 * uptime_sec, bat_health, bat_status and bat_tech, while status.json — the
 * source the WebUI polls first — silently omitted all four. Two copies of a
 * 25-field format string is a contract nobody can keep in sync, so both paths
 * now render through here. */
static int build_status_json(char *out, size_t cap) {
    int gpu_load = sysfs_read_int("/sys/module/ged/parameters/gpu_loading");
    const char *prof_str = (g_state.current_profile >= 0 && g_state.current_profile < 4)
                           ? g_profile_names[g_state.current_profile] : "Interactive";

    long uptime_sec = (long)difftime(time(NULL), s_start_time);

    /* Informatif saja — tidak dipakai untuk keputusan tuning apa pun. */
    int cpu_temp = sysfs_read_int(g_nodes.cpu_temp);
    int bat_temp = sysfs_read_int(g_nodes.bat_temp);
    normalize_thermal_temps(&cpu_temp, &bat_temp);
    int gpu_temp = 0;
    if (g_nodes.gpu_temp[0] != '\0') {
        gpu_temp = sysfs_read_int(g_nodes.gpu_temp);
        if (gpu_temp > 1000) gpu_temp /= 1000;
    }

    /* Read-only hardware telemetry (zero tuning, informational only) */
    int dvfsrc_mhz = 0;
    read_extra_telemetry(&dvfsrc_mhz);

    int jn = snprintf(out, cap,
        "{\"status\":\"ok\",\"pid\":%d,\"profile\":\"%s\","
        "\"cpu_temp\":%d,\"bat_temp\":%d,\"gpu_temp\":%d,"
        "\"gpu_load\":%d,\"uptime_sec\":%ld,"
        "\"dvfsrc_mhz\":%d}\n",
        getpid(), prof_str, cpu_temp, bat_temp, gpu_temp,
        gpu_load, uptime_sec, dvfsrc_mhz);

    /* Truncation is a bug, not a condition to paper over: a partial document
     * parses as corrupt JSON in the WebUI. Report it as an empty reply instead
     * so the caller never writes half an object. */
    if (jn < 0 || (size_t)jn >= cap) return 0;
    return jn;
}

static void process_client(int client_fd) {
    char req[256];
    int n = read_request(client_fd, req, sizeof(req));
    if (n <= 0) {
        close(client_fd);
        return;
    }

    while (n > 0 && (req[n - 1] == '\r' || req[n - 1] == '\n' || req[n - 1] == ' ')) {
        req[--n] = '\0';
    }

    if (strncmp(req, "GET_STATUS", 10) == 0 || strncmp(req, "STATUS", 6) == 0) {
        char json[STATUS_JSON_MAX];
        int jn = build_status_json(json, sizeof(json));
        if (jn > 0) write(client_fd, json, (size_t)jn);
    } else if (strncmp(req, "SET_PROFILE:", 12) == 0) {
        const char *pname = req + 12;
        if (strncasecmp(pname, "AUTO", 4) == 0 || strncasecmp(pname, "DYNAMIC", 7) == 0 || strncasecmp(pname, "DEFAULT", 7) == 0) {
            g_state.manual_profile = -1;
            ipc_sync_status();
            log_state("Ipc", "Profile reverted to Autonomous (Auto) via IPC");
            const char *res = "{\"status\":\"ok\",\"message\":\"Profile reverted to Autonomous (Auto)\"}\n";
            write(client_fd, res, strlen(res));
        } else {
            profile_t new_prof = PROFILE_Interactive;
            int known = 1;
            if (strncasecmp(pname, "SLEEP", 5) == 0) new_prof = PROFILE_Sleep;
            else if (strncasecmp(pname, "GAMING_MOBA", 11) == 0 || strncasecmp(pname, "MOBA", 4) == 0) new_prof = PROFILE_Gaming_MOBA;
            else if (strncasecmp(pname, "GAMING", 6) == 0) new_prof = PROFILE_Gaming;
            else if (strncasecmp(pname, "INTERACTIVE", 11) == 0) new_prof = PROFILE_Interactive;
            else known = 0;

            /* Unknown names used to silently lock the device to Interactive.
             * Refuse instead so a typo cannot pin the profile. */
            if (!known) {
                const char *err = "{\"status\":\"error\",\"message\":\"Unknown profile name\"}\n";
                write(client_fd, err, strlen(err));
                close(client_fd);
                return;
            }

            g_state.manual_profile = (int)new_prof;
            apply_profile(new_prof, 0);
            g_state.current_profile = new_prof;
            update_module_prop_status(g_profile_names[new_prof]);
            ipc_sync_status();

            log_state("Ipc", "Manual profile switch via IPC -> %s (locked)", g_profile_names[new_prof]);

            char res[256];
            snprintf(res, sizeof(res), "{\"status\":\"ok\",\"message\":\"Profile switched to %s\"}\n", g_profile_names[new_prof]);
            write(client_fd, res, strlen(res));
        }
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
        usleep((useconds_t)timeout_ms * 1000);
        return;
    }

    int ret = poll(pfds, (nfds_t)nfds, timeout_ms);
    if (ret > 0) {
        for (int i = 0; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == s_server_fd) {
                    /* Drain pending client connections, but bound the work per
                     * poll cycle: each client can cost up to the SO_RCVTIMEO
                     * below, and an unbounded drain lets a single peer stall
                     * the tuning loop indefinitely. */
                    int client_fd;
                    int max_accept = 4;
                    while (max_accept-- > 0 && (client_fd = accept(s_server_fd, NULL, NULL)) >= 0) {
                        /* Authorise BEFORE reading. Rejecting untrusted peers
                         * without a syscall on their socket is what keeps this
                         * from being a cheap way to hold up the main loop. */
                        if (!client_is_trusted(client_fd)) {
                            static int s_reject_logged = 0;
                            if (!s_reject_logged) {
                                struct ucred c;
                                socklen_t cl = sizeof(c);
                                uid_t uid = (uid_t)-1;
                                if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &c, &cl) == 0)
                                    uid = c.uid;
                                log_warn("Security", "Rejected unprivileged IPC client (uid=%u) — further attempts will not be logged", (unsigned)uid);
                                s_reject_logged = 1;
                            }
                            close(client_fd);
                            continue;
                        }

                        /* Use SO_RCVTIMEO instead of O_NONBLOCK on the client
                         * socket. O_NONBLOCK makes read() return EAGAIN the
                         * instant a client connects but has not written yet
                         * (nc takes ~1-2ms to do so). The peer is already
                         * trusted at this point, so the timeout only guards
                         * against a stalled root client. */
                        struct timeval tv = { .tv_sec = 0, .tv_usec = 25000 };
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

void update_status_json_file(void) {
    char json[STATUS_JSON_MAX];
    if (build_status_json(json, sizeof(json)) <= 0) {
        return;
    }

    char data_status[300];
    snprintf(data_status, sizeof(data_status), "%s/status.json", g_nodes.data_dir);

    const char *paths[] = {
        "/dev/hypercore_status.json",
        data_status,
        NULL
    };

    for (int i = 0; paths[i]; i++) {
        char tmp[300];
        snprintf(tmp, sizeof(tmp), "%s.tmp", paths[i]);
        /* Unlink-then-exclusive-create so a planted symlink at the tmp path
         * can never turn this into a root file-overwrite primitive. The
         * parent dirs are root-only, so the race window is not reachable
         * by apps, but exclusive create costs nothing. */
        unlink(tmp);
        int tfd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                       S_IRUSR | S_IWUSR);
        if (tfd < 0) continue;
        FILE *f = fdopen(tfd, "w");
        if (!f) {
            close(tfd);
            unlink(tmp);
            continue;
        }

        /* Both fputs and fclose can fail — a full /data or an I/O error leaves
         * a short or empty tmp file, and renaming that into place publishes a
         * truncated status.json for the WebUI to parse. Only swap it in once the
         * bytes are known to have landed, and tidy up the tmp on failure so a
         * later run does not inherit a stale one.
         *
         * The mode is pinned before the swap because fopen honours the inherited
         * umask. Left alone, the copy in /dev landed as 0644 and every app on
         * the device could read temperatures and profile state — the opposite of
         * the root-only telemetry path
         * common.h documents, since /dev is not behind /data/adb's 0700. */
        if (fchmod(fileno(f), S_IRUSR | S_IWUSR) != 0) {
            fclose(f);
            unlink(tmp);
            continue;
        }

        int ok = (fputs(json, f) >= 0);
        if (fclose(f) != 0) ok = 0;
        if (!ok) {
            unlink(tmp);
            continue;
        }
        rename(tmp, paths[i]);
    }
}
