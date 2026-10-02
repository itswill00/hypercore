
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "gamelist.hpp"
#include "sysfs.hpp"
#include "log.hpp"

#define MAX_GAMES 256
#define PKG_NAME_LEN 128

/* Open the managed gamelist for appending, refusing anything that is not a
 * regular root-owned file. data_dir lives under /data/adb, which is root-only,
 * so a symlink or a file owned by an app there means something is already wrong
 * and there is no legitimate reason to follow it.
 * O_CREAT is required: without it the first-boot auto-detect silently wrote
 * nothing whenever the file did not exist yet, so discoveries lived only in
 * memory and the WebUI showed an empty list. */
static FILE *open_gamelist_for_append(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0644);
    if (fd < 0) {
        log_warn("Gamelist", "Refusing to append to %s: %s", path, strerror(errno));
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != 0) {
        log_warn("Gamelist", "Refusing to append to %s: not a regular root-owned file", path);
        close(fd);
        return NULL;
    }

    FILE *f = fdopen(fd, "a");
    if (!f) {
        close(fd);
        return NULL;
    }
    return f;
}

static char      s_games[MAX_GAMES][PKG_NAME_LEN];
static profile_t s_profiles[MAX_GAMES];
static int       s_game_count = 0;
static int       s_inotify_fd = -1;
/* Pre-computed package name lengths — cached across calls, invalidated on reload */
static size_t    s_pkg_lens[MAX_GAMES];
static int       s_lens_cached = 0;

/* Strip leading and trailing whitespace (space, tab, CR, LF) in place.
 * The parser used to trim trailing spaces only, so a hand-edited line like
 * " com.foo:GAMING" kept its leading space and never matched any process. */
static void trim_ws(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
        s[--n] = '\0';
    size_t off = 0;
    while (s[off] == ' ' || s[off] == '\t' || s[off] == '\r' || s[off] == '\n') off++;
    if (off > 0) memmove(s, s + off, n - off + 1);
}

/* First-boot auto-detect must not resurrect a list the user deleted on
 * purpose. It used to re-run on every daemon restart whenever the list was
 * empty, so clearing the gamelist never stuck. This stamp records that a
 * detection pass already ran; deleting gamelist.txt afterwards is honoured,
 * and a fresh scan stays available from the WebUI button or by removing
 * the stamp file. */
static void autodetect_stamp_path(char *out, size_t cap) {
    snprintf(out, cap, "%s/.gamelist_autodetected", g_nodes.data_dir);
}


/* Auto-detection shells out to `pm list packages`, which spawns app_process and
 * costs a few hundred milliseconds. is_game_in_foreground() re-enters
 * load_gamelist() on every main-loop tick while the list is empty, so without a
 * rate limit a device with no recognisable games would fork a Java process up
 * to twice a second, forever. */
#define AUTODETECT_INTERVAL 3600  /* re-probe at most hourly */
#define DUMPSYS_INTERVAL     10   /* `dumpsys window` fallback scan, seconds  */

static time_t s_last_autodetect = 0;
static int    s_autodetect_done = 0;

void load_gamelist(void) {
    s_game_count = 0;
    s_lens_cached = 0; /* invalidate pkg_len cache on every reload */
    char path[256];
    path[0] = '\0';

    char data_gl[256];
    snprintf(data_gl, sizeof(data_gl), "%s/gamelist.txt", g_nodes.data_dir);
    FILE *f = fopen(data_gl, "r");
    if (f) {
        snprintf(path, sizeof(path), "%s", data_gl);
    } else {
        /* Check legacy mod_dir and migrate if found */
        char old_path[256];
        snprintf(old_path, sizeof(old_path), "%s/gamelist.txt", g_nodes.mod_dir);
        f = fopen(old_path, "r");
        if (f) {
            snprintf(path, sizeof(path), "%s", data_gl);
            /* Copy via tmp+rename so a crash mid-migration cannot leave a
             * truncated live gamelist behind. */
            char tmp[300];
            snprintf(tmp, sizeof(tmp), "%s.tmp", data_gl);
            FILE *fw = fopen(tmp, "w");
            if (fw) {
                char buf[512];
                while (fgets(buf, sizeof(buf), f)) fputs(buf, fw);
                if (fclose(fw) == 0) rename(tmp, data_gl);
                else unlink(tmp);
            }
            fclose(f);
            unlink(old_path);
            f = fopen(path, "r");
        } else {
            f = fopen("/sdcard/Android/gamelist.txt", "r");
            if (f) snprintf(path, sizeof(path), "/sdcard/Android/gamelist.txt");
        }
    }

    if (!path[0]) snprintf(path, sizeof(path), "%s", data_gl);

    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f) && s_game_count < MAX_GAMES) {

            trim_ws(line);

            if (line[0] == '#' || line[0] == '\0') continue;

            char *colon = strchr(line, ':');
            profile_t prof = PROFILE_Gaming;

            if (colon) {
                *colon = '\0';
                trim_ws(line);
                char *prof_str = colon + 1;
                trim_ws(prof_str);
                /* Normalize: uppercase + strip every non-letter so
                 * "Gaming MOBA", "gaming-moba" and "GAMING_MOBA" all match. */
                char norm[32];
                size_t ni = 0;
                for (const char *p = prof_str; *p && ni < sizeof(norm) - 1; p++) {
                    if (*p >= 'a' && *p <= 'z') norm[ni++] = (char)(*p - 'a' + 'A');
                    else if ((*p >= 'A' && *p <= 'Z')) norm[ni++] = *p;
                }
                norm[ni] = '\0';
                if (strcmp(norm, "INTERACTIVE") == 0 || strcmp(norm, "BALANCED") == 0) prof = PROFILE_Interactive;
                else if (strcmp(norm, "SLEEP") == 0 || strcmp(norm, "SAVER") == 0) prof = PROFILE_Sleep;
                else if (strcmp(norm, "GAMINGMOBA") == 0 || strcmp(norm, "MOBA") == 0) prof = PROFILE_Gaming_MOBA;
                else if (strcmp(norm, "GAMING") == 0 || norm[0] == '\0') prof = PROFILE_Gaming;
                else {
                    log_warn("Gamelist", "Unknown profile '%s', defaulting to Gaming", prof_str);
                    prof = PROFILE_Gaming;
                }
            }

            /* An empty package (" :GAMING", ":GAMING") matches nothing and only
             * burns a MAX_GAMES slot, so skip it instead of storing it. */
            if (line[0] == '\0') continue;

            strncpy(s_games[s_game_count], line, PKG_NAME_LEN - 1);
            s_games[s_game_count][PKG_NAME_LEN - 1] = '\0';
            s_profiles[s_game_count] = prof;
            s_game_count++;
        }
        fclose(f);
    }

    if (s_game_count == 0 && !s_autodetect_done) {
        char stamp[300];
        autodetect_stamp_path(stamp, sizeof(stamp));
        if (access(stamp, F_OK) == 0) {
            /* A previous boot already ran detection. An empty list now means
             * the user cleared it, so leave it empty. */
            s_autodetect_done = 1;
            return;
        }
        time_t now = time(NULL);
        if (now - s_last_autodetect < AUTODETECT_INTERVAL) {
            return;   /* f is already closed by the parse block above */
        }
        s_last_autodetect = now;
        s_autodetect_done = 1;

        FILE *pp = popen("pm list packages -3 2>/dev/null | cut -d: -f2 | grep -iE 'game|legend|pubg|mihoyo|genshin|honkai|freefire|roblox|activision|shooter|mojang|minecraft|supercell|brawl|clash|garena|stumble|pokemon|wanda|maleo|konami|krafton|netmarble|nexon|ea[.]gp|riotgames|square_enix|bandainamco|gameloft|zynga|rovio|miniclip|yostar|ubisoft|subwaysurf|bussimulator|carxtech|slither|angrybirds|asphalt|shadowfight|realracing|needforspeed|efootball|nintendo|sega|squareenix|capcom|kiloo|innersloth|levelinfinite'", "r");
        if (pp) {
            /* Detection ran, whether or not it found anything — stamp it so a
             * later restart does not resurrect a user-cleared list. A failed
             * popen leaves no stamp so the next restart retries. */
            int stamp_fd = open(stamp, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
            if (stamp_fd >= 0) close(stamp_fd);
            char pkg_buf[128];
            /* Always persist discoveries to the data dir, never to whichever
             * path happened to be read. `path` can be a /sdcard location, which
             * any app holding storage access can replace with a symlink — and
             * this runs as root, so the append would land wherever the link
             * pointed. Autodetected results are the one thing here the daemon
             * regenerates on its own, so writing them to the managed path loses
             * nothing. */
            FILE *fw = open_gamelist_for_append(data_gl);
            while (fgets(pkg_buf, sizeof(pkg_buf), pp) && s_game_count < MAX_GAMES) {
                trim_ws(pkg_buf);
                if (pkg_buf[0] == '\0') continue;

                /* Skip duplicate packages */
                int already_exists = 0;
                for (int d = 0; d < s_game_count; d++) {
                    if (strcmp(s_games[d], pkg_buf) == 0) { already_exists = 1; break; }
                }
                if (already_exists) continue;

                strncpy(s_games[s_game_count], pkg_buf, PKG_NAME_LEN - 1);
                s_games[s_game_count][PKG_NAME_LEN - 1] = '\0';
                s_profiles[s_game_count] = PROFILE_Gaming;
                s_game_count++;

                if (fw) {
                    fprintf(fw, "%s:GAMING\n", pkg_buf);
                }
            }
            if (fw) fclose(fw);
            pclose(pp);
        }
    }
}


void init_gamelist_watcher(void) {
    if (s_inotify_fd >= 0) {
        close(s_inotify_fd);
        s_inotify_fd = -1;
    }
    s_inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (s_inotify_fd < 0) return;

    /* Watch parent directories for IN_CLOSE_WRITE | IN_MOVED_TO.
     * Watching directories ensures atomic replacements (mv tmp gamelist.txt)
     * and in-place file edits both reliably trigger reloads without inode invalidation. */
    if (g_nodes.data_dir[0] != '\0') {
        inotify_add_watch(s_inotify_fd, g_nodes.data_dir, IN_CLOSE_WRITE | IN_MOVED_TO);
    }
    if (access("/sdcard/Android", F_OK) == 0) {
        inotify_add_watch(s_inotify_fd, "/sdcard/Android", IN_CLOSE_WRITE | IN_MOVED_TO);
    }
}

void check_gamelist_inotify(void) {
    if (s_inotify_fd < 0) return;

    char buf[1024];
    int need_reload = 0;
    ssize_t len;
    while ((len = read(s_inotify_fd, buf, sizeof(buf))) > 0) {
        ssize_t i = 0;
        while (i < len) {
            struct inotify_event *event = (struct inotify_event *)&buf[i];
            if (event->len > 0 && strcmp(event->name, "gamelist.txt") == 0) {
                need_reload = 1;
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
    if (need_reload) {
        load_gamelist();
    }
}

static int find_pid_by_pkg(const char *pkg) {
    if (!pkg || !pkg[0]) return 0;
    DIR *d = opendir("/proc");
    if (!d) return 0;
    struct dirent *ent;
    int found_pid = 0;
    size_t pkg_len = strlen(pkg);
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] < '0' || ent->d_name[0] > '9') continue;
        int pid = atoi(ent->d_name);
        if (pid <= 100) continue;
        char cmdpath[64];
        snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/cmdline", pid);
        int fd = open(cmdpath, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            char cmdline[256];
            ssize_t n = read(fd, cmdline, sizeof(cmdline) - 1);
            close(fd);
            if (n > 0) {
                cmdline[n] = '\0';
                if (strncasecmp(cmdline, pkg, pkg_len) == 0 &&
                    (cmdline[pkg_len] == '\0' || cmdline[pkg_len] == ':' || cmdline[pkg_len] == ' ')) {
                    found_pid = pid;
                    break;
                }
            }
        }
    }
    closedir(d);
    return found_pid;
}

/* Substring match with package-name boundaries for the dumpsys fallback.
 * dumpsys prints "com.foo/com.foo.Activity", so a plain strstr() lets
 * "com.foo" match the unrelated package "com.foo.bar". Only accept a hit
 * when neither neighbour is a package character. */
static int dump_contains_pkg(const char *dump, const char *pkg) {
    size_t pl = strlen(pkg);
    if (pl == 0 || !dump) return 0;
    for (const char *m = dump; (m = strstr(m, pkg)) != NULL; m++) {
        char before = (m == dump) ? '\0' : m[-1];
        char after = m[pl];
        int before_ok = !(before == '.' || before == '_' ||
                          (before >= 'A' && before <= 'Z') ||
                          (before >= 'a' && before <= 'z') ||
                          (before >= '0' && before <= '9'));
        int after_ok = !(after == '.' || after == '_' ||
                         (after >= 'A' && after <= 'Z') ||
                         (after >= 'a' && after <= 'z') ||
                         (after >= '0' && after <= '9'));
        if (before_ok && after_ok) return 1;
    }
    return 0;
}

int is_game_in_foreground(char *out_game_name, size_t max_len, profile_t *out_profile, int *out_game_pid) {
    if (out_game_name && max_len > 0) out_game_name[0] = '\0';
    if (out_profile) *out_profile = PROFILE_Gaming;
    if (out_game_pid) *out_game_pid = 0;

    check_gamelist_inotify();

    if (s_game_count == 0) {
        load_gamelist();
    }

    if (s_game_count == 0) {
        return 0;
    }

    const char *procs_paths[] = {
        "/dev/cpuset/top-app/cgroup.procs",
        "/sys/fs/cgroup/top-app/cgroup.procs",
        "/dev/cpuset/top-app/tasks",
        NULL
    };

    pid_t self_pid = getpid();

    for (int p = 0; procs_paths[p]; p++) {
        FILE *fp = fopen(procs_paths[p], "r");
        if (!fp) continue;

        char line_str[32];
        int pid_scanned = 0;

        /* Pre-compute package name lengths once — avoids strlen() inside the hot N*M loop */
        if (!s_lens_cached) {
            for (int i = 0; i < s_game_count; i++)
                s_pkg_lens[i] = strlen(s_games[i]);
            s_lens_cached = 1;
        }

        /* Scan up to 64 PIDs: the active foreground app PID appears in top-app cgroup */
        while (fgets(line_str, sizeof(line_str), fp) && pid_scanned < 64) {
            int pid = atoi(line_str);
            if (pid <= 100 || pid == self_pid) continue;
            pid_scanned++;

            char cmdpath[64];
            snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/cmdline", pid);
            int fd = open(cmdpath, O_RDONLY | O_CLOEXEC);
            if (fd < 0) continue;

            char cmdline[256];
            ssize_t n = read(fd, cmdline, sizeof(cmdline) - 1);
            close(fd);

            if (n <= 0) continue;
            cmdline[n] = '\0';

            for (int i = 0; i < s_game_count; i++) {
                size_t pkg_len = s_pkg_lens[i];
                if (pkg_len == 0) continue;

                if (strncasecmp(cmdline, s_games[i], pkg_len) == 0 &&
                    (cmdline[pkg_len] == '\0' || cmdline[pkg_len] == ':' || cmdline[pkg_len] == ' ')) {
                    fclose(fp);

                    if (out_game_name && max_len > 0) {
                        strncpy(out_game_name, s_games[i], max_len - 1);
                        out_game_name[max_len - 1] = '\0';
                    }
                    if (out_profile) {
                        *out_profile = s_profiles[i];
                    }
                    if (out_game_pid) {
                        *out_game_pid = pid;
                    }
                    return 1;
                }
            }
        }
        fclose(fp);
    }

    /* Rate-limited dumpsys window fallback for OEM game spaces (e.g. Game Turbo)
     * that do not place the game in the top-app cgroup. `dumpsys window` dumps
     * the whole WindowManager state and costs ~100ms, so it only runs while the
     * screen is on and never more than once per DUMPSYS_INTERVAL. */
    static time_t s_last_dumpsys = 0;
    time_t now_ds = time(NULL);
    int screen_on = (g_nodes.backlight[0] != '\0') ? (sysfs_read_int(g_nodes.backlight) > 0) : 1;

    if (screen_on && g_state.current_profile != PROFILE_Sleep &&
        now_ds - s_last_dumpsys >= DUMPSYS_INTERVAL) {
        s_last_dumpsys = now_ds;
        FILE *pp = popen("dumpsys window 2>/dev/null | grep -m1 -E 'mCurrentFocus|mFocusedApp'", "r");
        if (pp) {
            char dump_buf[512];
            while (fgets(dump_buf, sizeof(dump_buf), pp)) {
                for (int i = 0; i < s_game_count; i++) {
                    if (s_games[i][0] != '\0' && dump_contains_pkg(dump_buf, s_games[i])) {
                        pclose(pp);
                        if (out_game_name && max_len > 0) {
                            strncpy(out_game_name, s_games[i], max_len - 1);
                            out_game_name[max_len - 1] = '\0';
                        }
                        if (out_profile) {
                            *out_profile = s_profiles[i];
                        }
                        if (out_game_pid) {
                            *out_game_pid = find_pid_by_pkg(s_games[i]);
                        }
                        return 1;
                    }
                }
            }
            pclose(pp);
        }
    }

    return 0;
}
