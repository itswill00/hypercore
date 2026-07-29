/*
 * HyperCore - Dynamic Game List & Foreground Game Detector Implementation
 * Author: @itswill00
 */

#include "gamelist.hpp"

#define MAX_GAMES 256
#define PKG_NAME_LEN 128

static char s_games[MAX_GAMES][PKG_NAME_LEN];
static int  s_game_count = 0;
static time_t s_last_load_time = 0;

void load_gamelist(void) {
    s_game_count = 0;
    char path[256];
    snprintf(path, sizeof(path), "%s/gamelist.txt", g_nodes.mod_dir);

    FILE *f = fopen(path, "r");
    if (!f) {
        f = fopen("/sdcard/Android/gamelist.txt", "r");
    }
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f) && s_game_count < MAX_GAMES) {
        // Strip trailing whitespace / newline
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ')) {
            line[--len] = '\0';
        }
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') continue;

        strncpy(s_games[s_game_count], line, PKG_NAME_LEN - 1);
        s_games[s_game_count][PKG_NAME_LEN - 1] = '\0';
        s_game_count++;
    }
    fclose(f);
    s_last_load_time = time(NULL);
}

int is_game_in_foreground(char *out_game_name, size_t max_len) {
    if (out_game_name && max_len > 0) out_game_name[0] = '\0';

    time_t now = time(NULL);
    if (s_game_count == 0 || (now - s_last_load_time) > 30) {
        load_gamelist();
    }

    if (s_game_count == 0) return 0;

    FILE *ftasks = fopen("/dev/cpuset/top-app/tasks", "r");
    if (!ftasks) return 0;

    char pid_str[32];
    int checked_count = 0;

    while (fgets(pid_str, sizeof(pid_str), ftasks) && checked_count < 40) {
        int pid = atoi(pid_str);
        if (pid <= 0) continue;
        checked_count++;

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
            if (strcmp(cmdline, s_games[i]) == 0 || strstr(cmdline, s_games[i]) != NULL) {
                fclose(ftasks);
                if (out_game_name && max_len > 0) {
                    strncpy(out_game_name, s_games[i], max_len - 1);
                    out_game_name[max_len - 1] = '\0';
                }
                return 1;
            }
        }
    }

    fclose(ftasks);
    return 0;
}
