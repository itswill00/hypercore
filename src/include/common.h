
#ifndef HYPERCORE_COMMON_H
#define HYPERCORE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <sys/file.h>

/* Telemetry stays inside /data/adb (root-only, 0700-ish) rather than /sdcard,
 * where every app with storage access could read usage history. */
#define LOG_PATH        "/data/adb/hypercore/hypercore.log"
#define FREQ_LITTLE_MAX 2000000
#define FREQ_BIG_MAX    2200000

typedef enum {
    PROFILE_Sleep,
    PROFILE_Interactive,
    PROFILE_Gaming,
    PROFILE_Gaming_MOBA
} profile_t;

extern const char *g_profile_names[];

/* Normalize raw milli-degree thermal readings to degrees Celsius.
 * Informatif saja (status.json), tidak dipakai untuk keputusan tuning. */
static inline void normalize_thermal_temps(int *cpu_temp, int *bat_temp) {
    if (cpu_temp && *cpu_temp > 1000) *cpu_temp /= 1000;
    if (bat_temp) { if (*bat_temp > 1000) *bat_temp /= 1000; else if (*bat_temp > 100) *bat_temp /= 10; }
}


struct hw_nodes {
    char mod_dir[256];
    char data_dir[256];
    char pid_file[256];
    char lock_file[256];
    char cpu_temp[256];
    char bat_temp[256];
    char gpu_temp[256];
    char chg_temp[256];
    char bat_status[256];
    char backlight[256];
    char devfreq_cooler[256];
    char touch_thp_smooth[256];
    char touch_thp_noisefilter[256];
    char touch_edge[256];
    char touch_game_mode[256];
    char touch_sensitivity[256];
    char boot_cpu_gov[64];
    char cached_gov_perf[64];    /* best governor for Gaming/MOBA profiles, cached at startup */
    char cached_gov_balanced[64];/* best governor for Interactive/Sleep profiles, cached at startup */
    int  lit_hw_min_freq;
    int  lit_hw_max_freq;
    int  big_hw_min_freq;
    int  big_hw_max_freq;
    int  has_sugov_ext;
    int  has_schedutil;
    int  inotify_fd;
    int  netlink_fd;
};

struct core_state {
    profile_t current_profile;
    int manual_profile;               /* -1 = auto (autonomous profiler), 0=Sleep, 1=Interactive, 2=Gaming, 3=Gaming_MOBA */
    int touch_boost_ticks;
    int gaming_hold_ticks;
    int launch_boost_ticks;
    int app_boost_ticks;
    int jitter_rescue_ticks;
    int prev_load;
};

typedef struct {
    int  has_baseline;
    
    /* CPU Frequencies & Governors */
    char gov0[64];
    char gov6[64];
    int  lit_min_freq;
    int  lit_max_freq;
    int  big_min_freq;
    int  big_max_freq;
    char pol0_up_rate[32];
    char pol0_down_rate[32];
    char pol6_up_rate[32];
    char pol6_down_rate[32];

    /* Cgroups & UCLAMP */
    char bg_cpus[32];
    char sys_bg_cpus[32];
    char top_app_cpus[32];
    char bg_shares[32];
    char bg_uclamp_min[32];
    char bg_uclamp_max[32];
    char sys_bg_uclamp_max[32];
    char top_app_shares[32];
    char top_app_uclamp_min[32];
    char top_app_uclamp_max[32];

    /* GPU Devfreq */
    char mali_policy[64];
    char mali_gpu_gov[64];
    char mali_poll_int[64];
    char mali_upthresh[32];
    char mali_downdiff[32];
    char mali_min_freq[32];
    char mali_max_freq[32];

    /* MediaTek GED & FPSGO */
    char boost_gpu_enable[16];
    char ged_smart_boost[16];
    char ged_boost_enable[16];
    char enable_gpu_boost[16];
    char gpu_cust_boost_freq[32];
    char gpu_cust_upbound_freq[32];
    char gpu_bottom_freq[32];
    char g_fb_dvfs_threshold[16];
    char gx_fb_dvfs_margin[16];
    char gx_game_mode[16];
    char fpsgo_force_onoff[16];
    char fpsgo_boost_ta[16];
    char fpsgo_ultra_rescue[16];
    char fpsgo_light_loading[16];
    char fpsgo_idleprefer[16];
    char fpsgo_thrm_enable[16];

    /* Xiaomi Thermal / sconfig */
    char sconfig[16];

    /* Touchscreen stock (captured live; previously assumed 0 on restore) */
    char touch_smooth[16];
    char touch_noise[16];
    char touch_game[16];
    char touch_sens[16];
    char touch_edge[16];

    /* Memory VM */
    char vm_swappiness[16];
    char vm_dirty_ratio[16];
    char vm_dirty_bg_ratio[16];
    char vm_vfs_cache_pressure[16];
    char vm_stat_interval[16];
    char vm_dirty_writeback[16];
    char vm_page_cluster[16];
    char vm_compaction[16];

    /* Storage I/O */
    char io_read_ahead[32];
    char io_nr_requests[32];
    char io_iostats[16];

    /* Scheduler */
    char sched_migration_cost[32];
    char sched_latency[32];
    char sched_nr_migrate[32];

} stock_baseline_t;

extern volatile sig_atomic_t g_running;
extern struct hw_nodes g_nodes;
extern struct core_state g_state;
extern stock_baseline_t g_stock_baseline;

#endif 
