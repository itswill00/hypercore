#include "util.h"
#include "log.h"

#define ROOT_PATH "/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:/system/bin:/vendor/bin"

static void spawn_detached(const char *file, char *const argv[], int search_path) {
    if (!file || !argv) return;

    pid_t pid = fork();
    if (pid < 0) {
        log_warn("Async", "fork() failed: %s (command skipped: %s)", strerror(errno), file);
        return;
    }
    if (pid != 0) {
        int st = 0;
        waitpid(pid, &st, 0);
        return;
    }

    /* Middle child: close inherited descriptors so the grandchild cannot hold
     * the IPC socket or log fd open after the daemon exits. */
    long max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd < 0 || max_fd > 1024) max_fd = 256;
    for (long fd = STDERR_FILENO + 1; fd < max_fd; fd++) close((int)fd);

    pid_t g = fork();
    if (g == 0) {
        setsid();
        int null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
        if (null_fd >= 0) {
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        setenv("PATH", ROOT_PATH, 1);
        if (search_path) execvp(file, argv);
        else execv(file, argv);
        _exit(127);
    }
    _exit(g < 0 ? 1 : 0);
}

void async_system_cmd(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    char *argv[] = { (char *)"sh", (char *)"-c", (char *)cmd, NULL };
    spawn_detached("/system/bin/sh", argv, 0);
}

void async_resetprop(const char *key, const char *val, int is_delete) {
    if (!key || key[0] == '\0') return;
    if (is_delete) {
        char *argv[] = { (char *)"resetprop", (char *)"--delete", (char *)key, NULL };
        spawn_detached("resetprop", argv, 1);
    } else {
        char *argv[] = { (char *)"resetprop", (char *)key, (char *)(val ? val : ""), NULL };
        spawn_detached("resetprop", argv, 1);
    }
}
