
#ifndef HYPERCORE_IPC_HPP
#define HYPERCORE_IPC_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

int  init_ipc_socket(void);
void close_ipc_socket(void);
void handle_ipc_events(int timeout_ms);
void update_status_json_file(int cpu_temp, int bat_temp);

/* Set by handle_ipc_events() when backlight inotify or netlink uevent fires.
 * Main loop should read and reset these after each handle_ipc_events() call. */
extern volatile int g_screen_changed;
extern volatile int g_uevent_received;

#ifdef __cplusplus
}
#endif

#endif 
