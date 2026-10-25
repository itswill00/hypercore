
#ifndef HYPERCORE_IPC_H
#define HYPERCORE_IPC_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

int  init_ipc_socket(void);
void close_ipc_socket(void);
void handle_ipc_events(int timeout_ms);
void update_status_json_file(int cpu_temp, int bat_temp);

/* Capacity of the status document buffer. The single serializer in ipc.c
 * refuses to emit a truncated object, so this is the hard ceiling on how much
 * telemetry the socket reply and status.json can carry. */
#define STATUS_JSON_MAX 1280

/* Set by handle_ipc_events() when backlight inotify or netlink uevent fires.
 * Main loop should read and reset these after each handle_ipc_events() call. */
extern volatile int g_screen_changed;
extern volatile int g_uevent_received;

#ifdef __cplusplus
}
#endif

#endif 
