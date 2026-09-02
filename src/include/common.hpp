
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
    char pid_file[256];
    char lock_file[256];
    char cpu_temp[256];
    char bat_temp[256];
    char bat_status[256];
    char backlight[256];
    char touch_thp_smooth[256];
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
    int thermal_tier;
    int thermal_hold_ticks;
    int touch_boost_ticks;
    int gaming_hold_ticks;
    int launch_boost_ticks;
    int app_boost_ticks;
    int jitter_rescue_ticks;
    int prev_load;
    int is_charging;
    int user_charge_mode;             /* CHARGE_MODE_* selected by user (0=OEM 1=Fast 2=Balanced 3=Safe 4=Bypass 5=Violent 6=Custom) */
    int charge_mode;                  /* CHARGE_MODE_* currently active on hardware (effective mode)                      */
    int charge_mode_thermal_override; /* 1 = daemon temporarily overrode user mode due to heat                            */
    int charger_supported;            /* 1 = hardware sysfs charger control nodes detected, 0 = unsupported               */
    int custom_charge_limit;          /* Custom slider hardware limit level (0-15), default 10                            */
};

extern volatile sig_atomic_t g_running;
extern struct hw_nodes g_nodes;
extern struct core_state g_state;

#endif 
