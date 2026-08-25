
#include "sysfs.hpp"

int sysfs_read_str(const char *path, char *out_buf, size_t max_len) {
    if (!path || path[0] == '\0' || !out_buf || max_len == 0) return 0;
    out_buf[0] = '\0';

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    ssize_t n = read(fd, out_buf, max_len - 1);
    close(fd);
    if (n <= 0) return 0;
    out_buf[n] = '\0';

    size_t len = strlen(out_buf);
    while (len > 0 && (out_buf[len - 1] == '\r' || out_buf[len - 1] == '\n' || out_buf[len - 1] == ' ')) {
        out_buf[--len] = '\0';
    }
    return 1;
}

int sysfs_read_str_fallback(const char *paths[], char *out_buf, size_t max_len) {
    if (!paths || !out_buf || max_len == 0) return 0;
    for (int i = 0; paths[i]; i++) {
        if (paths[i][0] != '\0' && sysfs_read_str(paths[i], out_buf, max_len)) {
            return 1;
        }
    }
    return 0;
}

#include "log.hpp"

/* Per-path error log throttling table with static storage to prevent dangling pointers */
#define SYSFS_ERR_SLOTS 16
static struct {
    char path[128];
    time_t last_err;
} s_err_tbl[SYSFS_ERR_SLOTS];

static int sysfs_should_log_err(const char *path) {
    if (!path || path[0] == '\0') return 0;
    time_t now = time(NULL);
    unsigned h = 0;
    for (const char *p = path; *p; p++) h = h * 31u + (unsigned char)*p;
    int slot = (int)(h & (SYSFS_ERR_SLOTS - 1));

    if (s_err_tbl[slot].path[0] != '\0' &&
        strcmp(s_err_tbl[slot].path, path) == 0 &&
        now - s_err_tbl[slot].last_err < 60) return 0;

    strncpy(s_err_tbl[slot].path, path, sizeof(s_err_tbl[slot].path) - 1);
    s_err_tbl[slot].path[sizeof(s_err_tbl[slot].path) - 1] = '\0';
    s_err_tbl[slot].last_err = now;
    return 1;
}

void sysfs_write(const char *path, const char *val) {
    if (!path || path[0] == '\0' || !val) return;

    /* Trim trailing newlines and whitespace from input value copy */
    char clean_val[64];
    strncpy(clean_val, val, sizeof(clean_val) - 1);
    clean_val[sizeof(clean_val) - 1] = '\0';
    size_t vlen = strlen(clean_val);
    while (vlen > 0 && (clean_val[vlen - 1] == '\r' || clean_val[vlen - 1] == '\n' || clean_val[vlen - 1] == ' ')) {
        clean_val[--vlen] = '\0';
    }

    /* Skip write if node value already matches target value */
    char current_val[64];
    if (sysfs_read_str(path, current_val, sizeof(current_val))) {
        if (strcmp(current_val, clean_val) == 0) {
            return;
        }
    }

    int fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (errno != ENOENT && sysfs_should_log_err(path)) {
            log_info("Kernel", "Vendor sysfs node access error (errno=%d, target: '%s')", errno, path);
        }
        return;
    }
    write(fd, clean_val, strlen(clean_val));
    close(fd);
}

void sysfs_write_fallback(const char *paths[], const char *val) {
    if (!paths || !val) return;

    /* Trim value once here to avoid repeated trimming inside sysfs_write */
    char clean_val[64];
    strncpy(clean_val, val, sizeof(clean_val) - 1);
    clean_val[sizeof(clean_val) - 1] = '\0';
    size_t vlen = strlen(clean_val);
    while (vlen > 0 && (clean_val[vlen - 1] == '\r' || clean_val[vlen - 1] == '\n' || clean_val[vlen - 1] == ' '))
        clean_val[--vlen] = '\0';

    for (int i = 0; paths[i]; i++) {
        if (!paths[i] || paths[i][0] == '\0') continue;

        /* Try reading current value first to skip redundant writes.
         * If read fails, the node may be write-only — still attempt the write.
         * Do NOT skip (continue) on read failure; that silently loses writes to
         * write-only sysfs nodes which are common in kernel devfreq/thermal tuning. */
        char current_val[64];
        if (sysfs_read_str(paths[i], current_val, sizeof(current_val))) {
            if (strcmp(current_val, clean_val) == 0) return; /* node exists, value already set */
        }

        int fd = open(paths[i], O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd >= 0) {
            write(fd, clean_val, vlen);
            close(fd);
            return;
        }
    }
}

int sysfs_read_int(const char *path) {
    if (!path || path[0] == '\0') return 0;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return atoi(buf);
}

static struct {
    char gov0[64];
    char gov6[64];
    char mali_policy[64];
    char mali_gpu_gov[64];
    char mali_poll_int[64];
    char migration_cost[64];
    char charge_limit[64];
    int  has_baseline;
} s_baseline = { "", "", "", "", "", "", "", 0 };

void save_baseline_nodes(void) {
    if (s_baseline.has_baseline) return;

    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", s_baseline.gov0, sizeof(s_baseline.gov0));
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", s_baseline.gov6, sizeof(s_baseline.gov6));

    const char *power_policy_nodes[] = {
        "/sys/devices/platform/soc/13000000.mali/power_policy",
        "/sys/devices/platform/soc/soc:mali/power_policy",
        "/sys/class/devfreq/13000000.mali/power_policy",
        "/sys/class/devfreq/soc:mali/power_policy",
        NULL
    };
    for (int i = 0; power_policy_nodes[i]; i++) {
        if (sysfs_read_str(power_policy_nodes[i], s_baseline.mali_policy, sizeof(s_baseline.mali_policy))) break;
    }

    const char *devfreq_gov_nodes[] = {
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/soc:mali/governor",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
        NULL
    };
    for (int i = 0; devfreq_gov_nodes[i]; i++) {
        if (sysfs_read_str(devfreq_gov_nodes[i], s_baseline.mali_gpu_gov, sizeof(s_baseline.mali_gpu_gov))) break;
    }

    const char *devfreq_poll_nodes[] = {
        "/sys/class/devfreq/13000000.mali/polling_interval",
        "/sys/class/devfreq/soc:mali/polling_interval",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
        NULL
    };
    for (int i = 0; devfreq_poll_nodes[i]; i++) {
        if (sysfs_read_str(devfreq_poll_nodes[i], s_baseline.mali_poll_int, sizeof(s_baseline.mali_poll_int))) break;
    }

    sysfs_read_str("/proc/sys/kernel/sched_migration_cost_ns", s_baseline.migration_cost, sizeof(s_baseline.migration_cost));
    sysfs_read_str("/sys/class/power_supply/battery/constant_charge_current_max", s_baseline.charge_limit, sizeof(s_baseline.charge_limit));

    s_baseline.has_baseline = 1;
}

void restore_baseline_nodes(void) {
    if (!s_baseline.has_baseline) return;

    if (s_baseline.gov0[0] != '\0') sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", s_baseline.gov0);
    if (s_baseline.gov6[0] != '\0') sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", s_baseline.gov6);

    const char *power_policy_nodes[] = {
        "/sys/devices/platform/soc/13000000.mali/power_policy",
        "/sys/devices/platform/soc/soc:mali/power_policy",
        "/sys/class/devfreq/13000000.mali/power_policy",
        "/sys/class/devfreq/soc:mali/power_policy",
        NULL
    };
    if (s_baseline.mali_policy[0] != '\0') sysfs_write_fallback(power_policy_nodes, s_baseline.mali_policy);

    const char *devfreq_gov_nodes[] = {
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/soc:mali/governor",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
        NULL
    };
    if (s_baseline.mali_gpu_gov[0] != '\0') sysfs_write_fallback(devfreq_gov_nodes, s_baseline.mali_gpu_gov);

    const char *devfreq_poll_nodes[] = {
        "/sys/class/devfreq/13000000.mali/polling_interval",
        "/sys/class/devfreq/soc:mali/polling_interval",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
        NULL
    };
    if (s_baseline.mali_poll_int[0] != '\0' && atoi(s_baseline.mali_poll_int) > 0) {
        sysfs_write_fallback(devfreq_poll_nodes, s_baseline.mali_poll_int);
    } else {
        sysfs_write_fallback(devfreq_poll_nodes, "50");
    }

    if (s_baseline.migration_cost[0] != '\0') sysfs_write("/proc/sys/kernel/sched_migration_cost_ns", s_baseline.migration_cost);
    if (s_baseline.charge_limit[0] != '\0') sysfs_write("/sys/class/power_supply/battery/constant_charge_current_max", s_baseline.charge_limit);

    /* Restore charger nodes to safe ROM defaults on daemon exit/shutdown */
    sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
    sysfs_write("/sys/class/power_supply/battery/charge_control_limit", "0");
    sysfs_write("/sys/class/thermal/thermal_message/sconfig", "0");
}

void update_module_prop_status(const char *status) {
    static char s_last_status[128] = "";
    static time_t s_last_write_time = 0;
    time_t now = time(NULL);

    if (status && strcmp(s_last_status, status) == 0 && (now - s_last_write_time) < 30) {
        return;
    }

    if (status) {
        strncpy(s_last_status, status, sizeof(s_last_status) - 1);
        s_last_status[sizeof(s_last_status) - 1] = '\0';
    }
    s_last_write_time = now;

    char prop_path[256];
    snprintf(prop_path, sizeof(prop_path), "%s/module.prop", g_nodes.mod_dir);

    char tmp_path[270];
    snprintf(tmp_path, sizeof(tmp_path), "%s/module.prop.tmp", g_nodes.mod_dir);

    FILE *fin = fopen(prop_path, "r");
    if (!fin) {
        s_last_status[0] = '\0';
        return;
    }

    /* Stream line-by-line: no fixed-size array, no truncation risk regardless of
     * how many lines module.prop contains. */
    FILE *fout = fopen(tmp_path, "w");
    if (!fout) {
        fclose(fin);
        return;
    }

    char line[512];
    int write_ok = 1;
    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, "description=", 12) == 0) {
            if (fprintf(fout, "description=[Active: %s] Smart kernel optimizer & gaming daemon for MT6789 Family.\n", status) < 0)
                write_ok = 0;
        } else {
            if (fputs(line, fout) == EOF) write_ok = 0;
        }
    }
    fclose(fin);
    fclose(fout);

    if (write_ok) {
        rename(tmp_path, prop_path);
    } else {
        unlink(tmp_path);
    }
}
