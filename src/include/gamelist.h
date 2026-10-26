#ifndef HYPERCORE_GAMELIST_H
#define HYPERCORE_GAMELIST_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int pid;                  /* first eligible top-app pid, 0 when unknown */
    char package[128];        /* that process's cmdline, for change detection */
    int is_game;
    char game_name[128];
    profile_t game_profile;
    int game_pid;
} foreground_t;

void load_gamelist(void);
void init_gamelist_watcher(void);
void check_gamelist_inotify(void);

/* One pass over the top-app cgroup produces both the foreground pid (used for
 * app-switch detection) and the gamelist match. The old code walked the same
 * cgroup.procs and opened every /proc/<pid>/cmdline twice per tick. */
void foreground_scan(foreground_t *out);

#ifdef __cplusplus
}
#endif

#endif
