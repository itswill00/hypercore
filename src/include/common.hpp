
#ifndef HYPERCORE_COMMON_HPP
#define HYPERCORE_COMMON_HPP

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

#define LOG_PATH        "/sdcard/Android/hypercore.log"
#define FREQ_LITTLE_MAX 2000000
#define FREQ_BIG_MAX    2200000

typedef enum {
    PROFILE_Sleep,
    PROFILE_Interactive,
    PROFILE_Gaming,
    PROFILE_Gaming_MOBA
} profile_t;

typedef enum {
    CHARGE_MODE_OEM      = 0,  /* Hands-off — 100% stock kernel/ROM driver control */
    CHARGE_MODE_FAST     = 1,  /* charge_control_limit=0  (~3.9A / ~16W) */
    CHARGE_MODE_BALANCED = 2,  /* charge_control_limit=10 (~2.3A / ~9.3W) */
    CHARGE_MODE_SAFE     = 3,  /* charge_control_limit=14 (~0.73A / ~2.8W) */
    CHARGE_MODE_BYPASS   = 4,  /* input_suspend=1 — cell disconnected (0 mA) */
    CHARGE_MODE_VIOLENT  = 5,  /* limit=0 + sconfig=10 + smart_chg=0 (19-33W) */
    CHARGE_MODE_CUSTOM   = 6,  /* Custom user-defined limit via slider (level 0-15) */
} charge_mode_t;

extern const char *g_profile_names[];

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
    int is_charging;
    int user_charge_mode;             /* CHARGE_MODE_* selected by user (0=OEM 1=Fast 2=Balanced 3=Safe 4=Bypass 5=Violent 6=Custom) */
    int charge_mode;                  /* CHARGE_MODE_* currently active on hardware (effective mode)                      */
    int charger_supported;            /* 1 = hardware sysfs charger control nodes detected, 0 = unsupported               */
    int custom_charge_limit;          /* Custom slider hardware limit level (0-15), default 10                            */
    int night_charging;               /* 1 = night charging protection enabled (pauses at 80% overnight)                   */
    int smart_chg;                    /* 1 = smart charging curve enabled                                                  */
    int protect_80;                   /* 1 = stop charging at 80% capacity limit                                           */
    int thermal_tier;                 /* 0 = Optimal, 1 = Warm mitigation, 2 = Hot safety protection                       */
    int charge_override;              /* 1 = safety / low battery / thermal override currently active                     */
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

    /* Memory VM */
    char vm_swappiness[16];
    char vm_dirty_ratio[16];
    char vm_dirty_bg_ratio[16];
    char vm_vfs_cache_pressure[16];
    char vm_stat_interval[16];
    char vm_dirty_writeback[16];
    char vm_page_cluster[16];

    /* Storage I/O */
    char io_read_ahead[32];
    char io_nr_requests[32];
    char io_iostats[16];

    /* Scheduler */
    char sched_migration_cost[32];
    char sched_latency[32];
    char sched_nr_migrate[32];

    /* Charger */
    char charge_limit[64];
} stock_baseline_t;

extern volatile sig_atomic_t g_running;
extern struct hw_nodes g_nodes;
extern struct core_state g_state;
extern stock_baseline_t g_stock_baseline;

#endif 
