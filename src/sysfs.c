
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
    if (fd < 0 && errno == EACCES) {
        chmod(path, 0666);
        fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    }
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

#include "log.hpp"
#include "gpu.hpp"

stock_baseline_t g_stock_baseline;

static void restore_policy_freqs(const char *pol_path, int min_f, int max_f) {
    if (min_f <= 0 || max_f <= 0) return;
    char path[256], buf[32];
    snprintf(path, sizeof(path), "%s/scaling_min_freq", pol_path);
    int cur_min = sysfs_read_int(path);
    if (cur_min > max_f) {
        snprintf(buf, sizeof(buf), "%d", min_f);
        sysfs_write(path, buf);
        snprintf(path, sizeof(path), "%s/scaling_max_freq", pol_path);
        snprintf(buf, sizeof(buf), "%d", max_f);
        sysfs_write(path, buf);
    } else {
        snprintf(path, sizeof(path), "%s/scaling_max_freq", pol_path);
        snprintf(buf, sizeof(buf), "%d", max_f);
        sysfs_write(path, buf);
        snprintf(path, sizeof(path), "%s/scaling_min_freq", pol_path);
        snprintf(buf, sizeof(buf), "%d", min_f);
        sysfs_write(path, buf);
    }
}

void save_stock_baseline(void) {
    char path[300];
    snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.data_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "# HyperCore Stock Factory Baseline\n");
    fprintf(f, "# Captured on first installation\n");
    fprintf(f, "has_baseline=1\n");
    fprintf(f, "gov0=%s\n", g_stock_baseline.gov0);
    fprintf(f, "gov6=%s\n", g_stock_baseline.gov6);
    fprintf(f, "lit_min_freq=%d\n", g_stock_baseline.lit_min_freq);
    fprintf(f, "lit_max_freq=%d\n", g_stock_baseline.lit_max_freq);
    fprintf(f, "big_min_freq=%d\n", g_stock_baseline.big_min_freq);
    fprintf(f, "big_max_freq=%d\n", g_stock_baseline.big_max_freq);
    fprintf(f, "pol0_up_rate=%s\n", g_stock_baseline.pol0_up_rate);
    fprintf(f, "pol0_down_rate=%s\n", g_stock_baseline.pol0_down_rate);
    fprintf(f, "pol6_up_rate=%s\n", g_stock_baseline.pol6_up_rate);
    fprintf(f, "pol6_down_rate=%s\n", g_stock_baseline.pol6_down_rate);
    fprintf(f, "bg_cpus=%s\n", g_stock_baseline.bg_cpus);
    fprintf(f, "sys_bg_cpus=%s\n", g_stock_baseline.sys_bg_cpus);
    fprintf(f, "top_app_cpus=%s\n", g_stock_baseline.top_app_cpus);
    fprintf(f, "bg_shares=%s\n", g_stock_baseline.bg_shares);
    fprintf(f, "bg_uclamp_min=%s\n", g_stock_baseline.bg_uclamp_min);
    fprintf(f, "bg_uclamp_max=%s\n", g_stock_baseline.bg_uclamp_max);
    fprintf(f, "sys_bg_uclamp_max=%s\n", g_stock_baseline.sys_bg_uclamp_max);
    fprintf(f, "top_app_shares=%s\n", g_stock_baseline.top_app_shares);
    fprintf(f, "top_app_uclamp_min=%s\n", g_stock_baseline.top_app_uclamp_min);
    fprintf(f, "top_app_uclamp_max=%s\n", g_stock_baseline.top_app_uclamp_max);
    fprintf(f, "mali_policy=%s\n", g_stock_baseline.mali_policy);
    fprintf(f, "mali_gpu_gov=%s\n", g_stock_baseline.mali_gpu_gov);
    fprintf(f, "mali_poll_int=%s\n", g_stock_baseline.mali_poll_int);
    fprintf(f, "mali_upthresh=%s\n", g_stock_baseline.mali_upthresh);
    fprintf(f, "mali_downdiff=%s\n", g_stock_baseline.mali_downdiff);
    fprintf(f, "mali_min_freq=%s\n", g_stock_baseline.mali_min_freq);
    fprintf(f, "mali_max_freq=%s\n", g_stock_baseline.mali_max_freq);
    fprintf(f, "boost_gpu_enable=%s\n", g_stock_baseline.boost_gpu_enable);
    fprintf(f, "ged_smart_boost=%s\n", g_stock_baseline.ged_smart_boost);
    fprintf(f, "ged_boost_enable=%s\n", g_stock_baseline.ged_boost_enable);
    fprintf(f, "enable_gpu_boost=%s\n", g_stock_baseline.enable_gpu_boost);
    fprintf(f, "gpu_cust_boost_freq=%s\n", g_stock_baseline.gpu_cust_boost_freq);
    fprintf(f, "gpu_cust_upbound_freq=%s\n", g_stock_baseline.gpu_cust_upbound_freq);
    fprintf(f, "gpu_bottom_freq=%s\n", g_stock_baseline.gpu_bottom_freq);
    fprintf(f, "g_fb_dvfs_threshold=%s\n", g_stock_baseline.g_fb_dvfs_threshold);
    fprintf(f, "gx_fb_dvfs_margin=%s\n", g_stock_baseline.gx_fb_dvfs_margin);
    fprintf(f, "gx_game_mode=%s\n", g_stock_baseline.gx_game_mode);
    fprintf(f, "fpsgo_force_onoff=%s\n", g_stock_baseline.fpsgo_force_onoff);
    fprintf(f, "fpsgo_boost_ta=%s\n", g_stock_baseline.fpsgo_boost_ta);
    fprintf(f, "fpsgo_ultra_rescue=%s\n", g_stock_baseline.fpsgo_ultra_rescue);
    fprintf(f, "fpsgo_light_loading=%s\n", g_stock_baseline.fpsgo_light_loading);
    fprintf(f, "fpsgo_idleprefer=%s\n", g_stock_baseline.fpsgo_idleprefer);
    fprintf(f, "fpsgo_thrm_enable=%s\n", g_stock_baseline.fpsgo_thrm_enable);
    fprintf(f, "sconfig=%s\n", g_stock_baseline.sconfig);
    fprintf(f, "vm_swappiness=%s\n", g_stock_baseline.vm_swappiness);
    fprintf(f, "vm_dirty_ratio=%s\n", g_stock_baseline.vm_dirty_ratio);
    fprintf(f, "vm_dirty_bg_ratio=%s\n", g_stock_baseline.vm_dirty_bg_ratio);
    fprintf(f, "vm_vfs_cache_pressure=%s\n", g_stock_baseline.vm_vfs_cache_pressure);
    fprintf(f, "vm_stat_interval=%s\n", g_stock_baseline.vm_stat_interval);
    fprintf(f, "vm_dirty_writeback=%s\n", g_stock_baseline.vm_dirty_writeback);
    fprintf(f, "vm_page_cluster=%s\n", g_stock_baseline.vm_page_cluster);
    fprintf(f, "io_read_ahead=%s\n", g_stock_baseline.io_read_ahead);
    fprintf(f, "io_nr_requests=%s\n", g_stock_baseline.io_nr_requests);
    fprintf(f, "io_iostats=%s\n", g_stock_baseline.io_iostats);
    fprintf(f, "sched_migration_cost=%s\n", g_stock_baseline.sched_migration_cost);
    fprintf(f, "sched_latency=%s\n", g_stock_baseline.sched_latency);
    fprintf(f, "sched_nr_migrate=%s\n", g_stock_baseline.sched_nr_migrate);
    fprintf(f, "charge_limit=%s\n", g_stock_baseline.charge_limit);
    fclose(f);
    chmod(path, 0644);
}

int load_stock_baseline(void) {
    char path[300];
    snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.data_dir);
    FILE *f = fopen(path, "r");
    if (!f && g_nodes.mod_dir[0] && strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        snprintf(path, sizeof(path), "%s/stock_state.conf", g_nodes.mod_dir);
        f = fopen(path, "r");
    }
    if (!f) return 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        char *nl = strpbrk(val, "\r\n");
        if (nl) *nl = '\0';

        if (strcmp(key, "has_baseline") == 0) g_stock_baseline.has_baseline = atoi(val);
        else if (strcmp(key, "gov0") == 0) strncpy(g_stock_baseline.gov0, val, sizeof(g_stock_baseline.gov0)-1);
        else if (strcmp(key, "gov6") == 0) strncpy(g_stock_baseline.gov6, val, sizeof(g_stock_baseline.gov6)-1);
        else if (strcmp(key, "lit_min_freq") == 0) g_stock_baseline.lit_min_freq = atoi(val);
        else if (strcmp(key, "lit_max_freq") == 0) g_stock_baseline.lit_max_freq = atoi(val);
        else if (strcmp(key, "big_min_freq") == 0) g_stock_baseline.big_min_freq = atoi(val);
        else if (strcmp(key, "big_max_freq") == 0) g_stock_baseline.big_max_freq = atoi(val);
        else if (strcmp(key, "pol0_up_rate") == 0) strncpy(g_stock_baseline.pol0_up_rate, val, sizeof(g_stock_baseline.pol0_up_rate)-1);
        else if (strcmp(key, "pol0_down_rate") == 0) strncpy(g_stock_baseline.pol0_down_rate, val, sizeof(g_stock_baseline.pol0_down_rate)-1);
        else if (strcmp(key, "pol6_up_rate") == 0) strncpy(g_stock_baseline.pol6_up_rate, val, sizeof(g_stock_baseline.pol6_up_rate)-1);
        else if (strcmp(key, "pol6_down_rate") == 0) strncpy(g_stock_baseline.pol6_down_rate, val, sizeof(g_stock_baseline.pol6_down_rate)-1);
        else if (strcmp(key, "bg_cpus") == 0) strncpy(g_stock_baseline.bg_cpus, val, sizeof(g_stock_baseline.bg_cpus)-1);
        else if (strcmp(key, "sys_bg_cpus") == 0) strncpy(g_stock_baseline.sys_bg_cpus, val, sizeof(g_stock_baseline.sys_bg_cpus)-1);
        else if (strcmp(key, "top_app_cpus") == 0) strncpy(g_stock_baseline.top_app_cpus, val, sizeof(g_stock_baseline.top_app_cpus)-1);
        else if (strcmp(key, "bg_shares") == 0) strncpy(g_stock_baseline.bg_shares, val, sizeof(g_stock_baseline.bg_shares)-1);
        else if (strcmp(key, "bg_uclamp_min") == 0) strncpy(g_stock_baseline.bg_uclamp_min, val, sizeof(g_stock_baseline.bg_uclamp_min)-1);
        else if (strcmp(key, "bg_uclamp_max") == 0) strncpy(g_stock_baseline.bg_uclamp_max, val, sizeof(g_stock_baseline.bg_uclamp_max)-1);
        else if (strcmp(key, "sys_bg_uclamp_max") == 0) strncpy(g_stock_baseline.sys_bg_uclamp_max, val, sizeof(g_stock_baseline.sys_bg_uclamp_max)-1);
        else if (strcmp(key, "top_app_shares") == 0) strncpy(g_stock_baseline.top_app_shares, val, sizeof(g_stock_baseline.top_app_shares)-1);
        else if (strcmp(key, "top_app_uclamp_min") == 0) strncpy(g_stock_baseline.top_app_uclamp_min, val, sizeof(g_stock_baseline.top_app_uclamp_min)-1);
        else if (strcmp(key, "top_app_uclamp_max") == 0) strncpy(g_stock_baseline.top_app_uclamp_max, val, sizeof(g_stock_baseline.top_app_uclamp_max)-1);
        else if (strcmp(key, "mali_policy") == 0) strncpy(g_stock_baseline.mali_policy, val, sizeof(g_stock_baseline.mali_policy)-1);
        else if (strcmp(key, "mali_gpu_gov") == 0) strncpy(g_stock_baseline.mali_gpu_gov, val, sizeof(g_stock_baseline.mali_gpu_gov)-1);
        else if (strcmp(key, "mali_poll_int") == 0) strncpy(g_stock_baseline.mali_poll_int, val, sizeof(g_stock_baseline.mali_poll_int)-1);
        else if (strcmp(key, "mali_upthresh") == 0) strncpy(g_stock_baseline.mali_upthresh, val, sizeof(g_stock_baseline.mali_upthresh)-1);
        else if (strcmp(key, "mali_downdiff") == 0) strncpy(g_stock_baseline.mali_downdiff, val, sizeof(g_stock_baseline.mali_downdiff)-1);
        else if (strcmp(key, "mali_min_freq") == 0) strncpy(g_stock_baseline.mali_min_freq, val, sizeof(g_stock_baseline.mali_min_freq)-1);
        else if (strcmp(key, "mali_max_freq") == 0) strncpy(g_stock_baseline.mali_max_freq, val, sizeof(g_stock_baseline.mali_max_freq)-1);
        else if (strcmp(key, "boost_gpu_enable") == 0) strncpy(g_stock_baseline.boost_gpu_enable, val, sizeof(g_stock_baseline.boost_gpu_enable)-1);
        else if (strcmp(key, "ged_smart_boost") == 0) strncpy(g_stock_baseline.ged_smart_boost, val, sizeof(g_stock_baseline.ged_smart_boost)-1);
        else if (strcmp(key, "ged_boost_enable") == 0) strncpy(g_stock_baseline.ged_boost_enable, val, sizeof(g_stock_baseline.ged_boost_enable)-1);
        else if (strcmp(key, "enable_gpu_boost") == 0) strncpy(g_stock_baseline.enable_gpu_boost, val, sizeof(g_stock_baseline.enable_gpu_boost)-1);
        else if (strcmp(key, "gpu_cust_boost_freq") == 0) strncpy(g_stock_baseline.gpu_cust_boost_freq, val, sizeof(g_stock_baseline.gpu_cust_boost_freq)-1);
        else if (strcmp(key, "gpu_cust_upbound_freq") == 0) strncpy(g_stock_baseline.gpu_cust_upbound_freq, val, sizeof(g_stock_baseline.gpu_cust_upbound_freq)-1);
        else if (strcmp(key, "gpu_bottom_freq") == 0) strncpy(g_stock_baseline.gpu_bottom_freq, val, sizeof(g_stock_baseline.gpu_bottom_freq)-1);
        else if (strcmp(key, "g_fb_dvfs_threshold") == 0) strncpy(g_stock_baseline.g_fb_dvfs_threshold, val, sizeof(g_stock_baseline.g_fb_dvfs_threshold)-1);
        else if (strcmp(key, "gx_fb_dvfs_margin") == 0) strncpy(g_stock_baseline.gx_fb_dvfs_margin, val, sizeof(g_stock_baseline.gx_fb_dvfs_margin)-1);
        else if (strcmp(key, "gx_game_mode") == 0) strncpy(g_stock_baseline.gx_game_mode, val, sizeof(g_stock_baseline.gx_game_mode)-1);
        else if (strcmp(key, "fpsgo_force_onoff") == 0) strncpy(g_stock_baseline.fpsgo_force_onoff, val, sizeof(g_stock_baseline.fpsgo_force_onoff)-1);
        else if (strcmp(key, "fpsgo_boost_ta") == 0) strncpy(g_stock_baseline.fpsgo_boost_ta, val, sizeof(g_stock_baseline.fpsgo_boost_ta)-1);
        else if (strcmp(key, "fpsgo_ultra_rescue") == 0) strncpy(g_stock_baseline.fpsgo_ultra_rescue, val, sizeof(g_stock_baseline.fpsgo_ultra_rescue)-1);
        else if (strcmp(key, "fpsgo_light_loading") == 0) strncpy(g_stock_baseline.fpsgo_light_loading, val, sizeof(g_stock_baseline.fpsgo_light_loading)-1);
        else if (strcmp(key, "fpsgo_idleprefer") == 0) strncpy(g_stock_baseline.fpsgo_idleprefer, val, sizeof(g_stock_baseline.fpsgo_idleprefer)-1);
        else if (strcmp(key, "fpsgo_thrm_enable") == 0) strncpy(g_stock_baseline.fpsgo_thrm_enable, val, sizeof(g_stock_baseline.fpsgo_thrm_enable)-1);
        else if (strcmp(key, "sconfig") == 0) strncpy(g_stock_baseline.sconfig, val, sizeof(g_stock_baseline.sconfig)-1);
        else if (strcmp(key, "vm_swappiness") == 0) strncpy(g_stock_baseline.vm_swappiness, val, sizeof(g_stock_baseline.vm_swappiness)-1);
        else if (strcmp(key, "vm_dirty_ratio") == 0) strncpy(g_stock_baseline.vm_dirty_ratio, val, sizeof(g_stock_baseline.vm_dirty_ratio)-1);
        else if (strcmp(key, "vm_dirty_bg_ratio") == 0) strncpy(g_stock_baseline.vm_dirty_bg_ratio, val, sizeof(g_stock_baseline.vm_dirty_bg_ratio)-1);
        else if (strcmp(key, "vm_vfs_cache_pressure") == 0) strncpy(g_stock_baseline.vm_vfs_cache_pressure, val, sizeof(g_stock_baseline.vm_vfs_cache_pressure)-1);
        else if (strcmp(key, "vm_stat_interval") == 0) strncpy(g_stock_baseline.vm_stat_interval, val, sizeof(g_stock_baseline.vm_stat_interval)-1);
        else if (strcmp(key, "vm_dirty_writeback") == 0) strncpy(g_stock_baseline.vm_dirty_writeback, val, sizeof(g_stock_baseline.vm_dirty_writeback)-1);
        else if (strcmp(key, "vm_page_cluster") == 0) strncpy(g_stock_baseline.vm_page_cluster, val, sizeof(g_stock_baseline.vm_page_cluster)-1);
        else if (strcmp(key, "io_read_ahead") == 0) strncpy(g_stock_baseline.io_read_ahead, val, sizeof(g_stock_baseline.io_read_ahead)-1);
        else if (strcmp(key, "io_nr_requests") == 0) strncpy(g_stock_baseline.io_nr_requests, val, sizeof(g_stock_baseline.io_nr_requests)-1);
        else if (strcmp(key, "io_iostats") == 0) strncpy(g_stock_baseline.io_iostats, val, sizeof(g_stock_baseline.io_iostats)-1);
        else if (strcmp(key, "sched_migration_cost") == 0) strncpy(g_stock_baseline.sched_migration_cost, val, sizeof(g_stock_baseline.sched_migration_cost)-1);
        else if (strcmp(key, "sched_latency") == 0) strncpy(g_stock_baseline.sched_latency, val, sizeof(g_stock_baseline.sched_latency)-1);
        else if (strcmp(key, "sched_nr_migrate") == 0) strncpy(g_stock_baseline.sched_nr_migrate, val, sizeof(g_stock_baseline.sched_nr_migrate)-1);
        else if (strcmp(key, "charge_limit") == 0) strncpy(g_stock_baseline.charge_limit, val, sizeof(g_stock_baseline.charge_limit)-1);
    }
    fclose(f);
    g_stock_baseline.has_baseline = 1;
    return 1;
}

void init_stock_baseline(void) {
    if (g_stock_baseline.has_baseline) return;

    if (load_stock_baseline()) {
        log_info("Baseline", "Loaded existing factory stock baseline from stock_state.conf");
        return;
    }

    log_info("Baseline", "No saved stock baseline found. Capturing untouched factory baseline...");
    memset(&g_stock_baseline, 0, sizeof(g_stock_baseline));

    /* Governor */
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", g_stock_baseline.gov0, sizeof(g_stock_baseline.gov0));
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", g_stock_baseline.gov6, sizeof(g_stock_baseline.gov6));
    if (g_stock_baseline.gov0[0] == '\0') strcpy(g_stock_baseline.gov0, "sugov_ext");
    if (g_stock_baseline.gov6[0] == '\0') strcpy(g_stock_baseline.gov6, "sugov_ext");

    /* Frequencies */
    int l_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq");
    int l_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq");
    if (l_max <= 0) l_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq");
    int b_min = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/scaling_min_freq");
    int b_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_max_freq");
    if (b_max <= 0) b_max = sysfs_read_int("/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq");

    g_stock_baseline.lit_min_freq = (l_min > 0) ? l_min : (g_nodes.lit_hw_min_freq > 0 ? g_nodes.lit_hw_min_freq : 500000);
    g_stock_baseline.lit_max_freq = (l_max > 0) ? l_max : (g_nodes.lit_hw_max_freq > 0 ? g_nodes.lit_hw_max_freq : 2000000);
    g_stock_baseline.big_min_freq = (b_min > 0) ? b_min : (g_nodes.big_hw_min_freq > 0 ? g_nodes.big_hw_min_freq : 725000);
    g_stock_baseline.big_max_freq = (b_max > 0) ? b_max : (g_nodes.big_hw_max_freq > 0 ? g_nodes.big_hw_max_freq : 2200000);

    /* Rate limits */
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us", g_stock_baseline.pol0_up_rate, sizeof(g_stock_baseline.pol0_up_rate));
    if (g_stock_baseline.pol0_up_rate[0] == '\0') strcpy(g_stock_baseline.pol0_up_rate, "1000");
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us", g_stock_baseline.pol0_down_rate, sizeof(g_stock_baseline.pol0_down_rate));
    if (g_stock_baseline.pol0_down_rate[0] == '\0') strcpy(g_stock_baseline.pol0_down_rate, "1000");

    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us", g_stock_baseline.pol6_up_rate, sizeof(g_stock_baseline.pol6_up_rate));
    if (g_stock_baseline.pol6_up_rate[0] == '\0') strcpy(g_stock_baseline.pol6_up_rate, "1000");
    sysfs_read_str("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us", g_stock_baseline.pol6_down_rate, sizeof(g_stock_baseline.pol6_down_rate));
    if (g_stock_baseline.pol6_down_rate[0] == '\0') strcpy(g_stock_baseline.pol6_down_rate, "1000");

    /* Cgroups & UCLAMP */
    sysfs_read_str("/dev/cpuset/background/cpus", g_stock_baseline.bg_cpus, sizeof(g_stock_baseline.bg_cpus));
    if (g_stock_baseline.bg_cpus[0] == '\0') strcpy(g_stock_baseline.bg_cpus, "0-3");
    sysfs_read_str("/dev/cpuset/system-background/cpus", g_stock_baseline.sys_bg_cpus, sizeof(g_stock_baseline.sys_bg_cpus));
    if (g_stock_baseline.sys_bg_cpus[0] == '\0') strcpy(g_stock_baseline.sys_bg_cpus, "0-5");
    sysfs_read_str("/dev/cpuset/top-app/cpus", g_stock_baseline.top_app_cpus, sizeof(g_stock_baseline.top_app_cpus));
    if (g_stock_baseline.top_app_cpus[0] == '\0') strcpy(g_stock_baseline.top_app_cpus, "0-7");

    strcpy(g_stock_baseline.bg_shares, "1024");
    strcpy(g_stock_baseline.bg_uclamp_min, "0");
    strcpy(g_stock_baseline.bg_uclamp_max, "max");
    strcpy(g_stock_baseline.sys_bg_uclamp_max, "max");
    strcpy(g_stock_baseline.top_app_shares, "1024");
    strcpy(g_stock_baseline.top_app_uclamp_min, "0");
    strcpy(g_stock_baseline.top_app_uclamp_max, "max");

    /* GPU Devfreq */
    const char *power_policy_nodes[] = {
        "/sys/devices/platform/soc/13000000.mali/power_policy",
        "/sys/devices/platform/soc/soc:mali/power_policy",
        "/sys/class/devfreq/13000000.mali/power_policy",
        "/sys/class/devfreq/soc:mali/power_policy",
        NULL
    };
    for (int i = 0; power_policy_nodes[i]; i++) {
        if (sysfs_read_str(power_policy_nodes[i], g_stock_baseline.mali_policy, sizeof(g_stock_baseline.mali_policy))) break;
    }
    if (g_stock_baseline.mali_policy[0] == '\0') strcpy(g_stock_baseline.mali_policy, "coarse_demand");

    const char *devfreq_gov_nodes[] = {
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/soc:mali/governor",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
        NULL
    };
    for (int i = 0; devfreq_gov_nodes[i]; i++) {
        if (sysfs_read_str(devfreq_gov_nodes[i], g_stock_baseline.mali_gpu_gov, sizeof(g_stock_baseline.mali_gpu_gov))) break;
    }
    if (g_stock_baseline.mali_gpu_gov[0] == '\0') strcpy(g_stock_baseline.mali_gpu_gov, "dummy");

    const char *devfreq_poll_nodes[] = {
        "/sys/class/devfreq/13000000.mali/polling_interval",
        "/sys/class/devfreq/soc:mali/polling_interval",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
        NULL
    };
    for (int i = 0; devfreq_poll_nodes[i]; i++) {
        if (sysfs_read_str(devfreq_poll_nodes[i], g_stock_baseline.mali_poll_int, sizeof(g_stock_baseline.mali_poll_int))) break;
    }
    if (g_stock_baseline.mali_poll_int[0] == '\0') strcpy(g_stock_baseline.mali_poll_int, "0");

    strcpy(g_stock_baseline.mali_upthresh, "80");
    strcpy(g_stock_baseline.mali_downdiff, "20");
    strcpy(g_stock_baseline.mali_min_freq, "390000000");
    strcpy(g_stock_baseline.mali_max_freq, get_max_gpu_freq_hz());

    /* MediaTek GED & FPSGO */
    sysfs_read_str("/sys/module/ged/parameters/boost_gpu_enable", g_stock_baseline.boost_gpu_enable, sizeof(g_stock_baseline.boost_gpu_enable));
    if (g_stock_baseline.boost_gpu_enable[0] == '\0') strcpy(g_stock_baseline.boost_gpu_enable, "0");

    sysfs_read_str("/sys/module/ged/parameters/ged_smart_boost", g_stock_baseline.ged_smart_boost, sizeof(g_stock_baseline.ged_smart_boost));
    if (g_stock_baseline.ged_smart_boost[0] == '\0') strcpy(g_stock_baseline.ged_smart_boost, "0");

    sysfs_read_str("/sys/module/ged/parameters/ged_boost_enable", g_stock_baseline.ged_boost_enable, sizeof(g_stock_baseline.ged_boost_enable));
    if (g_stock_baseline.ged_boost_enable[0] == '\0') strcpy(g_stock_baseline.ged_boost_enable, "1");

    sysfs_read_str("/sys/module/ged/parameters/enable_gpu_boost", g_stock_baseline.enable_gpu_boost, sizeof(g_stock_baseline.enable_gpu_boost));
    if (g_stock_baseline.enable_gpu_boost[0] == '\0') strcpy(g_stock_baseline.enable_gpu_boost, "1");

    sysfs_read_str("/sys/module/ged/parameters/gpu_cust_boost_freq", g_stock_baseline.gpu_cust_boost_freq, sizeof(g_stock_baseline.gpu_cust_boost_freq));
    if (g_stock_baseline.gpu_cust_boost_freq[0] == '\0') strcpy(g_stock_baseline.gpu_cust_boost_freq, "390000");

    sysfs_read_str("/sys/module/ged/parameters/gpu_cust_upbound_freq", g_stock_baseline.gpu_cust_upbound_freq, sizeof(g_stock_baseline.gpu_cust_upbound_freq));
    if (g_stock_baseline.gpu_cust_upbound_freq[0] == '\0') strcpy(g_stock_baseline.gpu_cust_upbound_freq, "1003000");

    sysfs_read_str("/sys/module/ged/parameters/gpu_bottom_freq", g_stock_baseline.gpu_bottom_freq, sizeof(g_stock_baseline.gpu_bottom_freq));
    if (g_stock_baseline.gpu_bottom_freq[0] == '\0') strcpy(g_stock_baseline.gpu_bottom_freq, "390000");

    sysfs_read_str("/sys/module/ged/parameters/g_fb_dvfs_threshold", g_stock_baseline.g_fb_dvfs_threshold, sizeof(g_stock_baseline.g_fb_dvfs_threshold));
    if (g_stock_baseline.g_fb_dvfs_threshold[0] == '\0') strcpy(g_stock_baseline.g_fb_dvfs_threshold, "80");

    sysfs_read_str("/sys/module/ged/parameters/gx_fb_dvfs_margin", g_stock_baseline.gx_fb_dvfs_margin, sizeof(g_stock_baseline.gx_fb_dvfs_margin));
    if (g_stock_baseline.gx_fb_dvfs_margin[0] == '\0') strcpy(g_stock_baseline.gx_fb_dvfs_margin, "40");

    strcpy(g_stock_baseline.gx_game_mode, "0");
    strcpy(g_stock_baseline.fpsgo_force_onoff, "0");
    strcpy(g_stock_baseline.fpsgo_boost_ta, "0");
    strcpy(g_stock_baseline.fpsgo_ultra_rescue, "0");
    strcpy(g_stock_baseline.fpsgo_light_loading, "0");
    strcpy(g_stock_baseline.fpsgo_idleprefer, "0");
    strcpy(g_stock_baseline.fpsgo_thrm_enable, "1");

    /* Xiaomi Thermal / sconfig */
    sysfs_read_str("/sys/class/thermal/thermal_message/sconfig", g_stock_baseline.sconfig, sizeof(g_stock_baseline.sconfig));
    if (g_stock_baseline.sconfig[0] == '\0' || strcmp(g_stock_baseline.sconfig, "10") == 0 || strcmp(g_stock_baseline.sconfig, "-1") == 0) {
        strcpy(g_stock_baseline.sconfig, "0");
    }

    /* Memory VM */
    sysfs_read_str("/proc/sys/vm/swappiness", g_stock_baseline.vm_swappiness, sizeof(g_stock_baseline.vm_swappiness));
    if (g_stock_baseline.vm_swappiness[0] == '\0') strcpy(g_stock_baseline.vm_swappiness, "100");
    sysfs_read_str("/proc/sys/vm/dirty_ratio", g_stock_baseline.vm_dirty_ratio, sizeof(g_stock_baseline.vm_dirty_ratio));
    if (g_stock_baseline.vm_dirty_ratio[0] == '\0') strcpy(g_stock_baseline.vm_dirty_ratio, "20");
    sysfs_read_str("/proc/sys/vm/dirty_background_ratio", g_stock_baseline.vm_dirty_bg_ratio, sizeof(g_stock_baseline.vm_dirty_bg_ratio));
    if (g_stock_baseline.vm_dirty_bg_ratio[0] == '\0') strcpy(g_stock_baseline.vm_dirty_bg_ratio, "10");
    strcpy(g_stock_baseline.vm_vfs_cache_pressure, "100");
    strcpy(g_stock_baseline.vm_stat_interval, "1");
    strcpy(g_stock_baseline.vm_dirty_writeback, "500");

    sysfs_read_str("/proc/sys/vm/page-cluster", g_stock_baseline.vm_page_cluster, sizeof(g_stock_baseline.vm_page_cluster));
    if (g_stock_baseline.vm_page_cluster[0] == '\0') strcpy(g_stock_baseline.vm_page_cluster, "3");

    /* Storage I/O */
    strcpy(g_stock_baseline.io_read_ahead, "1024");
    strcpy(g_stock_baseline.io_nr_requests, "128");
    strcpy(g_stock_baseline.io_iostats, "1");

    /* Scheduler */
    sysfs_read_str("/proc/sys/kernel/sched_migration_cost_ns", g_stock_baseline.sched_migration_cost, sizeof(g_stock_baseline.sched_migration_cost));
    if (g_stock_baseline.sched_migration_cost[0] == '\0') strcpy(g_stock_baseline.sched_migration_cost, "200000");
    strcpy(g_stock_baseline.sched_latency, "10000000");
    strcpy(g_stock_baseline.sched_nr_migrate, "32");

    /* Charger */
    sysfs_read_str("/sys/class/power_supply/battery/constant_charge_current_max", g_stock_baseline.charge_limit, sizeof(g_stock_baseline.charge_limit));

    g_stock_baseline.has_baseline = 1;
    save_stock_baseline();
    log_info("Baseline", "Untouched factory baseline captured and saved to %s/stock_state.conf", g_nodes.data_dir);
}

void save_baseline_nodes(void) {
    init_stock_baseline();
}

void restore_baseline_nodes(void) {
    if (!g_stock_baseline.has_baseline) return;

    /* Restore CPU Governors */
    if (g_stock_baseline.gov0[0] != '\0') sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", g_stock_baseline.gov0);
    if (g_stock_baseline.gov6[0] != '\0') sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/scaling_governor", g_stock_baseline.gov6);

    /* Restore CPU Frequencies */
    restore_policy_freqs("/sys/devices/system/cpu/cpufreq/policy0", g_stock_baseline.lit_min_freq, g_stock_baseline.lit_max_freq);
    restore_policy_freqs("/sys/devices/system/cpu/cpufreq/policy6", g_stock_baseline.big_min_freq, g_stock_baseline.big_max_freq);

    /* Restore CPU rate limits */
    if (g_stock_baseline.pol0_up_rate[0] != '\0') {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us", g_stock_baseline.pol0_up_rate);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us", g_stock_baseline.pol0_up_rate);
    }
    if (g_stock_baseline.pol0_down_rate[0] != '\0') {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us", g_stock_baseline.pol0_down_rate);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us", g_stock_baseline.pol0_down_rate);
    }
    if (g_stock_baseline.pol6_up_rate[0] != '\0') {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us", g_stock_baseline.pol6_up_rate);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us", g_stock_baseline.pol6_up_rate);
    }
    if (g_stock_baseline.pol6_down_rate[0] != '\0') {
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us", g_stock_baseline.pol6_down_rate);
        sysfs_write("/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us", g_stock_baseline.pol6_down_rate);
    }

    /* Restore Cgroups & UCLAMP */
    if (g_stock_baseline.bg_cpus[0] != '\0') sysfs_write("/dev/cpuset/background/cpus", g_stock_baseline.bg_cpus);
    if (g_stock_baseline.sys_bg_cpus[0] != '\0') sysfs_write("/dev/cpuset/system-background/cpus", g_stock_baseline.sys_bg_cpus);
    if (g_stock_baseline.top_app_cpus[0] != '\0') sysfs_write("/dev/cpuset/top-app/cpus", g_stock_baseline.top_app_cpus);

    sysfs_write("/dev/cpuctl/background/cpu.shares", "1024");
    sysfs_write("/dev/cpuctl/background/cpu.uclamp.min", "0");
    sysfs_write("/dev/cpuctl/background/cpu.uclamp.max", "max");
    sysfs_write("/dev/cpuctl/system-background/cpu.uclamp.max", "max");
    sysfs_write("/dev/cpuctl/top-app/cpu.shares", "1024");
    sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.min", "0");
    sysfs_write("/dev/cpuctl/top-app/cpu.uclamp.max", "max");

    /* Restore GPU Devfreq */
    const char *power_policy_nodes[] = {
        "/sys/devices/platform/soc/13000000.mali/power_policy",
        "/sys/devices/platform/soc/soc:mali/power_policy",
        "/sys/class/devfreq/13000000.mali/power_policy",
        "/sys/class/devfreq/soc:mali/power_policy",
        NULL
    };
    if (g_stock_baseline.mali_policy[0] != '\0') sysfs_write_fallback(power_policy_nodes, g_stock_baseline.mali_policy);

    const char *devfreq_gov_nodes[] = {
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/soc:mali/governor",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
        NULL
    };
    if (g_stock_baseline.mali_gpu_gov[0] != '\0') sysfs_write_fallback(devfreq_gov_nodes, g_stock_baseline.mali_gpu_gov);

    const char *devfreq_poll_nodes[] = {
        "/sys/class/devfreq/13000000.mali/polling_interval",
        "/sys/class/devfreq/soc:mali/polling_interval",
        "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
        "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
        NULL
    };
    if (g_stock_baseline.mali_poll_int[0] != '\0') {
        sysfs_write_fallback(devfreq_poll_nodes, g_stock_baseline.mali_poll_int);
    }

    const char *devfreq_min_nodes[] = {
        "/sys/class/devfreq/13000000.mali/min_freq",
        "/sys/class/devfreq/soc:mali/min_freq",
        NULL
    };
    if (g_stock_baseline.mali_min_freq[0] != '\0') sysfs_write_fallback(devfreq_min_nodes, g_stock_baseline.mali_min_freq);

    const char *devfreq_max_nodes[] = {
        "/sys/class/devfreq/13000000.mali/max_freq",
        "/sys/class/devfreq/soc:mali/max_freq",
        NULL
    };
    if (g_stock_baseline.mali_max_freq[0] != '\0') sysfs_write_fallback(devfreq_max_nodes, g_stock_baseline.mali_max_freq);

    /* Restore MediaTek GED & FPSGO */
    sysfs_write("/sys/module/ged/parameters/boost_gpu_enable", g_stock_baseline.boost_gpu_enable[0] ? g_stock_baseline.boost_gpu_enable : "0");
    sysfs_write("/sys/module/ged/parameters/ged_smart_boost", g_stock_baseline.ged_smart_boost[0] ? g_stock_baseline.ged_smart_boost : "0");
    sysfs_write("/sys/module/ged/parameters/ged_boost_enable", g_stock_baseline.ged_boost_enable[0] ? g_stock_baseline.ged_boost_enable : "1");
    sysfs_write("/sys/module/ged/parameters/enable_gpu_boost", g_stock_baseline.enable_gpu_boost[0] ? g_stock_baseline.enable_gpu_boost : "1");
    sysfs_write("/sys/module/ged/parameters/gpu_cust_boost_freq", g_stock_baseline.gpu_cust_boost_freq[0] ? g_stock_baseline.gpu_cust_boost_freq : "390000");
    sysfs_write("/sys/module/ged/parameters/gpu_cust_upbound_freq", g_stock_baseline.gpu_cust_upbound_freq[0] ? g_stock_baseline.gpu_cust_upbound_freq : "1003000");
    sysfs_write("/sys/module/ged/parameters/gpu_bottom_freq", g_stock_baseline.gpu_bottom_freq[0] ? g_stock_baseline.gpu_bottom_freq : "390000");
    sysfs_write("/sys/module/ged/parameters/g_fb_dvfs_threshold", g_stock_baseline.g_fb_dvfs_threshold[0] ? g_stock_baseline.g_fb_dvfs_threshold : "80");
    sysfs_write("/sys/module/ged/parameters/gx_fb_dvfs_margin", g_stock_baseline.gx_fb_dvfs_margin[0] ? g_stock_baseline.gx_fb_dvfs_margin : "40");
    sysfs_write("/sys/module/ged/parameters/gx_game_mode", g_stock_baseline.gx_game_mode[0] ? g_stock_baseline.gx_game_mode : "0");
    sysfs_write("/sys/kernel/fpsgo/common/force_onoff", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/boost_ta", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/ultra_rescue", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/light_loading_policy", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/switch_idleprefer", "0");
    sysfs_write("/sys/kernel/fpsgo/fbt/thrm_enable", "1");

    /* Restore Xiaomi Thermal / sconfig */
    chmod("/sys/class/thermal/thermal_message/sconfig", 0664);
    chmod("/sys/devices/virtual/thermal/thermal_message/sconfig", 0664);
    sysfs_write("/sys/class/thermal/thermal_message/sconfig", g_stock_baseline.sconfig[0] ? g_stock_baseline.sconfig : "0");
    sysfs_write("/sys/devices/virtual/thermal/thermal_message/sconfig", g_stock_baseline.sconfig[0] ? g_stock_baseline.sconfig : "0");

    /* Restore Touchscreen nodes to factory default (0) */
    if (g_nodes.touch_thp_smooth[0]) sysfs_write(g_nodes.touch_thp_smooth, "0");
    else sysfs_write("/sys/class/touch/touch_dev/touch_thp_smooth", "0");
    if (g_nodes.touch_thp_noisefilter[0]) sysfs_write(g_nodes.touch_thp_noisefilter, "0");
    else sysfs_write("/sys/class/touch/touch_dev/touch_thp_noisefilter", "0");
    sysfs_write(g_nodes.touch_game_mode, "0");
    sysfs_write(g_nodes.touch_sensitivity, "0");
    sysfs_write(g_nodes.touch_edge, "0");

    /* Restore Memory VM */
    if (g_stock_baseline.vm_swappiness[0] != '\0') sysfs_write("/proc/sys/vm/swappiness", g_stock_baseline.vm_swappiness);
    if (g_stock_baseline.vm_dirty_ratio[0] != '\0') sysfs_write("/proc/sys/vm/dirty_ratio", g_stock_baseline.vm_dirty_ratio);
    if (g_stock_baseline.vm_dirty_bg_ratio[0] != '\0') sysfs_write("/proc/sys/vm/dirty_background_ratio", g_stock_baseline.vm_dirty_bg_ratio);
    if (g_stock_baseline.vm_vfs_cache_pressure[0] != '\0') sysfs_write("/proc/sys/vm/vfs_cache_pressure", g_stock_baseline.vm_vfs_cache_pressure);
    if (g_stock_baseline.vm_stat_interval[0] != '\0') sysfs_write("/proc/sys/vm/stat_interval", g_stock_baseline.vm_stat_interval);
    if (g_stock_baseline.vm_dirty_writeback[0] != '\0') sysfs_write("/proc/sys/vm/dirty_writeback_centisecs", g_stock_baseline.vm_dirty_writeback);
    if (g_stock_baseline.vm_page_cluster[0] != '\0') sysfs_write("/proc/sys/vm/page-cluster", g_stock_baseline.vm_page_cluster);
    if (g_stock_baseline.sched_migration_cost[0] != '\0') sysfs_write("/proc/sys/kernel/sched_migration_cost_ns", g_stock_baseline.sched_migration_cost);

    /* Restore Charger */
    if (g_stock_baseline.charge_limit[0] != '\0') sysfs_write("/sys/class/power_supply/battery/constant_charge_current_max", g_stock_baseline.charge_limit);
    sysfs_write("/sys/class/power_supply/battery/input_suspend", "0");
    sysfs_write("/sys/class/power_supply/battery/charge_control_limit", "0");

    /* Clear runtime gaming properties */
    system("resetprop debug.sf.latch_unsignaled 0 2>/dev/null || true");
    system("resetprop --delete persist.sys.wifi.low_latency 2>/dev/null || true");

    /* Restore rate limit node permissions */
    const char *rate_limit_restore_paths[] = {
        "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us",
        "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
        NULL
    };
    for (int i = 0; rate_limit_restore_paths[i]; i++) {
        if (access(rate_limit_restore_paths[i], F_OK) == 0) {
            chmod(rate_limit_restore_paths[i], 0644);
        }
    }
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
