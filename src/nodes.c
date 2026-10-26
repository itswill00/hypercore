#include "nodes.h"
#include "sysfs.h"
#include "log.h"

/* --- Display -------------------------------------------------------------
 * Ordered so the standard Android backlight class wins before vendor paths. */
const char *const N_BACKLIGHT[] = {
    "/sys/class/backlight/panel0-backlight/brightness",
    "/sys/class/backlight/panel0-backlight/actual_brightness",
    "/sys/class/leds/lcd-backlight/brightness",
    "/sys/class/leds/lcd-backlight/actual_brightness",
    "/sys/class/backlight/lcd-backlight/brightness",
    "/sys/class/backlight/panel/brightness",
    "/sys/devices/platform/soc/soc:mtk_leds/leds/lcd-backlight/brightness",
    "/sys/devices/platform/leds-mt65xx/leds/lcd-backlight/brightness",
    "/sys/class/leds/mtk_leds/brightness",
    NULL
};

/* --- CPU ----------------------------------------------------------------- */
const char *const N_CPU_GOV0[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor",
    NULL
};
const char *const N_CPU_GOV6[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/scaling_governor",
    NULL
};
/* Hardware min must come from cpuinfo_min_freq. scaling_min_freq is a policy
 * value: the vendor thermal/perfmgr stack raises it transiently during boosts
 * (1.8 GHz on MT6789), so reading it here would turn that transient floor into
 * the Gaming floor and effectively pin the cluster. Baseline capture and
 * FREQ_HW both consume these arrays. */
const char *const N_CPU_POL0_MIN[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq",
    "/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq",
    NULL
};
const char *const N_CPU_POL0_MAX[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq",
    "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq",
    NULL
};
const char *const N_CPU_POL6_MIN[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_min_freq",
    "/sys/devices/system/cpu/cpufreq/policy6/scaling_min_freq",
    "/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_min_freq",
    NULL
};
const char *const N_CPU_POL6_MAX[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/cpuinfo_max_freq",
    "/sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq",
    "/sys/devices/system/cpu/cpufreq/policy4/cpuinfo_max_freq",
    NULL
};
const char *const N_RATE_P0_UP[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
    NULL
};
const char *const N_RATE_P0_DOWN[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy0/rate_limit_us",
    NULL
};
const char *const N_RATE_P6_UP[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/schedutil/up_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
    NULL
};
const char *const N_RATE_P6_DOWN[] = {
    "/sys/devices/system/cpu/cpufreq/policy6/sugov_ext/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/schedutil/down_rate_limit_us",
    "/sys/devices/system/cpu/cpufreq/policy6/rate_limit_us",
    NULL
};
const char *const N_CPUAVAIL_GOV[] = {
    "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors",
    NULL
};

/* --- Cgroups ------------------------------------------------------------- */
const char *const N_CPUSET_BG[] = {
    "/dev/cpuset/background/cpus",
    "/sys/fs/cgroup/background/cpuset.cpus",
    "/sys/fs/cgroup/background/cpus",
    "/sys/fs/cgroup/system/background/cpuset.cpus",
    "/sys/fs/cgroup/system/background/cpus",
    NULL
};
const char *const N_CPUSET_SYS_BG[] = {
    "/dev/cpuset/system-background/cpus",
    "/sys/fs/cgroup/system-background/cpuset.cpus",
    "/sys/fs/cgroup/system-background/cpus",
    "/sys/fs/cgroup/system/sys-background/cpuset.cpus",
    "/sys/fs/cgroup/system/sys-background/cpus",
    NULL
};
const char *const N_CPUSET_TOP_APP[] = {
    "/dev/cpuset/top-app/cpus",
    "/sys/fs/cgroup/top-app/cpuset.cpus",
    "/sys/fs/cgroup/top-app/cpus",
    "/sys/fs/cgroup/top-app.slice/cpuset.cpus",
    "/sys/fs/cgroup/top-app.slice/cpus",
    NULL
};
const char *const N_CPUCTL_BG_SHARES[] = {
    "/dev/cpuctl/background/cpu.shares",
    "/sys/fs/cgroup/background/cpu.shares",
    "/sys/fs/cgroup/background/cpu.weight",
    "/sys/fs/cgroup/system/background/cpu.weight",
    "/sys/fs/cgroup/system/background/cpu.shares",
    NULL
};
const char *const N_CPUCTL_BG_MIN[] = {
    "/dev/cpuctl/background/cpu.uclamp.min",
    "/sys/fs/cgroup/background/cpu.uclamp.min",
    "/sys/fs/cgroup/system/background/cpu.uclamp.min",
    NULL
};
const char *const N_CPUCTL_BG_MAX[] = {
    "/dev/cpuctl/background/cpu.uclamp.max",
    "/sys/fs/cgroup/background/cpu.uclamp.max",
    "/sys/fs/cgroup/system/background/cpu.uclamp.max",
    NULL
};
const char *const N_CPUCTL_SYS_BG_MAX[] = {
    "/dev/cpuctl/system-background/cpu.uclamp.max",
    "/sys/fs/cgroup/system-background/cpu.uclamp.max",
    "/sys/fs/cgroup/system/sys-background/cpu.uclamp.max",
    NULL
};
const char *const N_CPUCTL_TOP_SHARES[] = {
    "/dev/cpuctl/top-app/cpu.shares",
    "/sys/fs/cgroup/top-app/cpu.shares",
    "/sys/fs/cgroup/top-app/cpu.weight",
    "/sys/fs/cgroup/top-app.slice/cpu.weight",
    "/sys/fs/cgroup/top-app.slice/cpu.shares",
    NULL
};
const char *const N_CPUCTL_TOP_MIN[] = {
    "/dev/cpuctl/top-app/cpu.uclamp.min",
    "/sys/fs/cgroup/top-app/cpu.uclamp.min",
    "/sys/fs/cgroup/top-app.slice/cpu.uclamp.min",
    NULL
};
const char *const N_CPUCTL_TOP_MAX[] = {
    "/dev/cpuctl/top-app/cpu.uclamp.max",
    "/sys/fs/cgroup/top-app/cpu.uclamp.max",
    "/sys/fs/cgroup/top-app.slice/cpu.uclamp.max",
    NULL
};

/* --- Mali ---------------------------------------------------------------- */
const char *const N_MALI_GOV[] = {
    "/sys/class/devfreq/13000000.mali/governor",
    "/sys/class/devfreq/soc:mali/governor",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/governor",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/governor",
    "/sys/devices/platform/13000000.mali/devfreq/13000000.mali/governor",
    "/sys/devices/platform/soc/13fbf000.gpufreq/governor",
    NULL
};
const char *const N_MALI_MIN[] = {
    "/sys/class/devfreq/13000000.mali/min_freq",
    "/sys/class/devfreq/soc:mali/min_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/min_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/min_freq",
    "/sys/devices/platform/soc/13fbf000.gpufreq/min_freq",
    NULL
};
const char *const N_MALI_MAX[] = {
    "/sys/class/devfreq/13000000.mali/max_freq",
    "/sys/class/devfreq/soc:mali/max_freq",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/max_freq",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/max_freq",
    "/sys/devices/platform/soc/13fbf000.gpufreq/max_freq",
    NULL
};
const char *const N_MALI_POLL[] = {
    "/sys/class/devfreq/13000000.mali/polling_interval",
    "/sys/class/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/polling_interval",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/polling_interval",
    "/sys/devices/platform/soc/13fbf000.gpufreq/polling_interval",
    NULL
};
const char *const N_MALI_AVAIL[] = {
    "/sys/class/devfreq/13000000.mali/available_frequencies",
    "/sys/class/devfreq/soc:mali/available_frequencies",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/available_frequencies",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/available_frequencies",
    "/sys/devices/platform/soc/13fbf000.gpufreq/available_frequencies",
    NULL
};
const char *const N_MALI_POLICY[] = {
    "/sys/devices/platform/soc/13000000.mali/power_policy",
    "/sys/devices/platform/soc/soc:mali/power_policy",
    "/sys/class/devfreq/13000000.mali/power_policy",
    "/sys/class/devfreq/soc:mali/power_policy",
    "/sys/devices/platform/13000000.mali/power_policy",
    "/sys/devices/platform/soc/13fbf000.gpufreq/power_policy",
    NULL
};
const char *const N_MALI_UPTHRESH[] = {
    "/sys/class/devfreq/13000000.mali/simple_ondemand/upthreshold",
    "/sys/class/devfreq/soc:mali/simple_ondemand/upthreshold",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/upthreshold",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/upthreshold",
    "/sys/devices/platform/soc/13fbf000.gpufreq/simple_ondemand/upthreshold",
    NULL
};
const char *const N_MALI_DOWNDIFF[] = {
    "/sys/class/devfreq/13000000.mali/simple_ondemand/downdifferential",
    "/sys/class/devfreq/soc:mali/simple_ondemand/downdifferential",
    "/sys/devices/platform/soc/13000000.mali/devfreq/13000000.mali/simple_ondemand/downdifferential",
    "/sys/devices/platform/soc/soc:mali/devfreq/soc:mali/simple_ondemand/downdifferential",
    "/sys/devices/platform/soc/13fbf000.gpufreq/simple_ondemand/downdifferential",
    NULL
};
const char *const N_MALI_CUR[] = {
    "/sys/class/devfreq/13000000.mali/cur_freq",
    "/sys/class/devfreq/soc:mali/cur_freq",
    NULL
};

/* --- MediaTek extras ----------------------------------------------------- */
const char *const N_DVFSRC_QOS[] = {
    "/sys/kernel/helio-dvfsrc/dvfsrc_qos_mode",
    "/sys/devices/platform/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
    "/sys/module/dvfsrc/parameters/dvfsrc_qos_mode",
    "/sys/devices/platform/soc/10012000.dvfsrc/helio-dvfsrc/dvfsrc_qos_mode",
    NULL
};
const char *const N_GX_GAME_MODE[] = {
    "/sys/module/ged/parameters/gx_game_mode",
    "/sys/module/ged/parameters/gx_boost_on",
    NULL
};
const char *const N_SCONFIG[] = {
    "/sys/class/thermal/thermal_message/sconfig",
    "/sys/devices/virtual/thermal/thermal_message/sconfig",
    "/sys/class/thermal/thermal_message/sport_mode",
    NULL
};

/* --- Touch --------------------------------------------------------------- */
const char *const N_TOUCH_SMOOTH[] = {
    "/sys/class/touch/touch_dev/touch_thp_smooth",
    "/sys/devices/virtual/touch/touch_dev/touch_thp_smooth",
    "/sys/class/touch/touch_dev/thp_smooth",
    "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_thp_smooth",
    NULL
};
const char *const N_TOUCH_NOISE[] = {
    "/sys/class/touch/touch_dev/touch_thp_noisefilter",
    "/sys/devices/virtual/touch/touch_dev/touch_thp_noisefilter",
    "/sys/class/touch/touch_dev/thp_noisefilter",
    "/sys/devices/platform/goodix_ts.0/touch_thp_noisefilter",
    NULL
};
const char *const N_TOUCH_EDGE[] = {
    "/sys/class/touch/touch_dev/touch_edge",
    "/sys/class/touch/touch_dev/edge_mode",
    "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_edge_mode",
    NULL
};
const char *const N_TOUCH_GAME[] = {
    "/sys/class/touch/touch_dev/touch_thp_game",
    "/sys/class/touch/touch_dev/game_mode",
    "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_game_mode",
    NULL
};
const char *const N_TOUCH_SENS[] = {
    "/sys/class/touch/touch_dev/sensitivity",
    "/sys/class/touch/touch_dev/touch_sensitivity",
    "/sys/devices/platform/11007000.i2c/i2c-0/0-0038/fts_sensitivity",
    NULL
};

/* --- GED / FPSGO --------------------------------------------------------- */
const char *const N_GED_BOOST_GPU[] = { "/sys/module/ged/parameters/boost_gpu_enable", NULL };
const char *const N_GED_SMART_BOOST[] = { "/sys/module/ged/parameters/ged_smart_boost", NULL };
const char *const N_GED_BOOST_ENABLE[] = { "/sys/module/ged/parameters/ged_boost_enable", NULL };
const char *const N_GED_ENABLE_GPU_BOOST[] = { "/sys/module/ged/parameters/enable_gpu_boost", NULL };
const char *const N_GED_CUST_BOOST_FREQ[] = { "/sys/module/ged/parameters/gpu_cust_boost_freq", NULL };
const char *const N_GED_CUST_UPBOUND[] = { "/sys/module/ged/parameters/gpu_cust_upbound_freq", NULL };
const char *const N_GED_GPU_BOTTOM[] = { "/sys/module/ged/parameters/gpu_bottom_freq", NULL };
const char *const N_GED_FB_DVFS_THRESHOLD[] = { "/sys/module/ged/parameters/g_fb_dvfs_threshold", NULL };
const char *const N_GED_GX_MARGIN[] = { "/sys/module/ged/parameters/gx_fb_dvfs_margin", NULL };
const char *const N_GED_GX_GAME_MODE[] = { "/sys/module/ged/parameters/gx_game_mode", NULL };
const char *const N_GED_GPU_LOAD[] = { "/sys/module/ged/parameters/gpu_loading", NULL };
const char *const N_FPSGO_FORCE[] = { "/sys/kernel/fpsgo/common/force_onoff", NULL };
const char *const N_FPSGO_BOOST_TA[] = { "/sys/kernel/fpsgo/fbt/boost_ta", NULL };
const char *const N_FPSGO_ULTRA[] = { "/sys/kernel/fpsgo/fbt/ultra_rescue", NULL };
const char *const N_FPSGO_LIGHT[] = { "/sys/kernel/fpsgo/fbt/light_loading_policy", NULL };
const char *const N_FPSGO_IDLEPREFER[] = { "/sys/kernel/fpsgo/fbt/switch_idleprefer", NULL };
const char *const N_FPSGO_THRM[] = { "/sys/kernel/fpsgo/fbt/thrm_enable", NULL };

/* --- VM / scheduler ------------------------------------------------------ */
const char *const N_VM_SWAPPINESS[] = { "/proc/sys/vm/swappiness", NULL };
const char *const N_VM_DIRTY_RATIO[] = { "/proc/sys/vm/dirty_ratio", NULL };
const char *const N_VM_DIRTY_BG_RATIO[] = { "/proc/sys/vm/dirty_background_ratio", NULL };
const char *const N_VM_VFS_CACHE[] = { "/proc/sys/vm/vfs_cache_pressure", NULL };
const char *const N_VM_STAT_INTERVAL[] = { "/proc/sys/vm/stat_interval", NULL };
const char *const N_VM_DIRTY_WRITEBACK[] = { "/proc/sys/vm/dirty_writeback_centisecs", NULL };
const char *const N_VM_PAGE_CLUSTER[] = { "/proc/sys/vm/page-cluster", NULL };
const char *const N_VM_COMPACTION[] = { "/proc/sys/vm/compaction_proactiveness", NULL };
const char *const N_NET_READ_AHEAD[] = {
    "/sys/block/mmcblk0/queue/read_ahead_kb",
    "/sys/block/sda/queue/read_ahead_kb",
    "/sys/block/dm-0/queue/read_ahead_kb",
    NULL
};
const char *const N_NET_NR_REQUESTS[] = {
    "/sys/block/mmcblk0/queue/nr_requests",
    "/sys/block/sda/queue/nr_requests",
    "/sys/block/dm-0/queue/nr_requests",
    NULL
};
const char *const N_SCHED_MIGRATION[] = { "/proc/sys/kernel/sched_migration_cost_ns", NULL };
const char *const N_SCHED_LATENCY[] = { "/proc/sys/kernel/sched_latency_ns", NULL };
const char *const N_SCHED_NR_MIGRATE[] = { "/proc/sys/kernel/sched_nr_migrate", NULL };

const char *node_first(const char *const paths[]) {
    if (!paths) return NULL;
    for (int i = 0; paths[i]; i++) {
        if (paths[i][0] != '\0' && access(paths[i], F_OK) == 0) return paths[i];
    }
    return NULL;
}

void node_resolve(char *dst, size_t cap, const char *const paths[]) {
    if (!dst || cap == 0) return;
    dst[0] = '\0';
    const char *p = node_first(paths);
    if (p) snprintf(dst, cap, "%s", p);
}

int node_read_int(const char *const paths[]) {
    const char *p = node_first(paths);
    return p ? sysfs_read_int(p) : 0;
}

/* Thermal zone names differ per vendor. Rather than hardcoding zone indices
 * (which moved between ROM drops), probe the stable `type` attribute once and
 * remember the matching `temp` path. */
static void lowercase(char *s) {
    for (; s && *s; s++) if (*s >= 'A' && *s <= 'Z') *s = (char)(*s - 'A' + 'a');
}

static int zone_type_matches(const char *type_path, const char *const keywords[]) {
    char type[64];
    if (!sysfs_read_str(type_path, type, sizeof(type))) return 0;
    lowercase(type);
    for (int i = 0; keywords[i]; i++) {
        if (strstr(type, keywords[i])) return 1;
    }
    return 0;
}

static void resolve_thermal_zone(const char *const keywords[], char *dst, size_t cap, int fallback_zone) {
    DIR *d = opendir("/sys/class/thermal");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strncmp(ent->d_name, "thermal_zone", 12) != 0) continue;
            char type_path[256], temp_path[256];
            snprintf(type_path, sizeof(type_path), "/sys/class/thermal/%s/type", ent->d_name);
            if (!zone_type_matches(type_path, keywords)) continue;
            snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/%s/temp", ent->d_name);
            if (access(temp_path, F_OK) == 0) {
                snprintf(dst, cap, "%s", temp_path);
                closedir(d);
                return;
            }
        }
        closedir(d);
    }
    if (fallback_zone >= 0) {
        char temp_path[128];
        snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/thermal_zone%d/temp", fallback_zone);
        if (access(temp_path, F_OK) == 0) snprintf(dst, cap, "%s", temp_path);
    }
}

void nodes_resolve_backlight(void) {
    node_resolve(g_nodes.backlight, sizeof(g_nodes.backlight), N_BACKLIGHT);
}

void nodes_rediscover_touch(void) {
    static struct {
        char *slot;
        size_t cap;
        const char *const *cands;
        const char *name;
    } const maps[] = {
        { g_nodes.touch_thp_smooth,     sizeof(g_nodes.touch_thp_smooth),     N_TOUCH_SMOOTH, "touch smoothing" },
        { g_nodes.touch_thp_noisefilter,sizeof(g_nodes.touch_thp_noisefilter),N_TOUCH_NOISE,  "touch noise filter" },
        { g_nodes.touch_edge,           sizeof(g_nodes.touch_edge),           N_TOUCH_EDGE,   "touch edge" },
        { g_nodes.touch_game_mode,      sizeof(g_nodes.touch_game_mode),      N_TOUCH_GAME,   "touch game mode" },
        { g_nodes.touch_sensitivity,    sizeof(g_nodes.touch_sensitivity),    N_TOUCH_SENS,   "touch sensitivity" },
    };

    for (size_t i = 0; i < sizeof(maps) / sizeof(maps[0]); i++) {
        if (maps[i].slot[0] != '\0') continue;
        const char *p = node_first(maps[i].cands);
        if (!p) continue;
        snprintf(maps[i].slot, maps[i].cap, "%s", p);
        log_info("Hardware", "Late touch node discovered: %s -> %s", maps[i].name, p);
    }
}

void nodes_init(void) {
    memset(&g_nodes, 0, sizeof(g_nodes));
    g_nodes.inotify_fd = -1;
    g_nodes.netlink_fd = -1;

    if (access("/data/adb/modules/hypercore", F_OK) == 0) {
        snprintf(g_nodes.mod_dir, sizeof(g_nodes.mod_dir), "%s", "/data/adb/modules/hypercore");
    } else if (access("/data/adb", F_OK) == 0) {
        snprintf(g_nodes.mod_dir, sizeof(g_nodes.mod_dir), "%s", "/data/adb/hypercore");
    } else {
        snprintf(g_nodes.mod_dir, sizeof(g_nodes.mod_dir), "%s", "/data/local/tmp");
    }

    if (access("/data/adb", F_OK) == 0) {
        mkdir("/data/adb/hypercore", 0700);
        chmod("/data/adb/hypercore", 0700);
        snprintf(g_nodes.data_dir, sizeof(g_nodes.data_dir), "%s", "/data/adb/hypercore");
    } else {
        mkdir("/data/local/tmp/hypercore", 0700);
        chmod("/data/local/tmp/hypercore", 0700);
        snprintf(g_nodes.data_dir, sizeof(g_nodes.data_dir), "%s", "/data/local/tmp/hypercore");
    }
    snprintf(g_nodes.pid_file, sizeof(g_nodes.pid_file), "%s/hypercore.pid", g_nodes.data_dir);

    /* Legacy runtime files used to live in mod_dir. */
    if (strcmp(g_nodes.mod_dir, g_nodes.data_dir) != 0) {
        const char *legacy[] = { "hypercore.sock", "hypercore.pid", "status.json", NULL };
        char path[300];
        for (int i = 0; legacy[i]; i++) {
            snprintf(path, sizeof(path), "%s/%s", g_nodes.mod_dir, legacy[i]);
            unlink(path);
        }
    }

    nodes_resolve_backlight();
    nodes_rediscover_touch();

    g_nodes.has_sugov_ext = (access("/sys/devices/system/cpu/cpufreq/policy0/sugov_ext", F_OK) == 0);
    g_nodes.has_schedutil = (access("/sys/devices/system/cpu/cpufreq/policy0/schedutil", F_OK) == 0);

    /* Thermal telemetry: never resolved before, so cpu_temp/bat_temp/gpu_temp
     * stayed empty strings and status.json always reported 0. */
    {
        static const char *const cpu_kw[]  = { "cpu", "soc", "tscpu", "cluster", NULL };
        static const char *const gpu_kw[]  = { "gpu", "mali", NULL };
        static const char *const batt_kw[] = { "batt", "battery", NULL };
        resolve_thermal_zone(cpu_kw, g_nodes.cpu_temp, sizeof(g_nodes.cpu_temp), 0);
        resolve_thermal_zone(gpu_kw, g_nodes.gpu_temp, sizeof(g_nodes.gpu_temp), 1);
        resolve_thermal_zone(batt_kw, g_nodes.bat_temp, sizeof(g_nodes.bat_temp), 2);
    }

    g_nodes.inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (g_nodes.inotify_fd >= 0 && g_nodes.backlight[0] != '\0') {
        inotify_add_watch(g_nodes.inotify_fd, g_nodes.backlight, IN_MODIFY | IN_ATTRIB);
    }

    g_nodes.netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (g_nodes.netlink_fd >= 0) {
        struct sockaddr_nl nladdr;
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_pid = (unsigned int)getpid();
        nladdr.nl_groups = 1;
        bind(g_nodes.netlink_fd, (struct sockaddr *)&nladdr, sizeof(nladdr));
        int flags = fcntl(g_nodes.netlink_fd, F_GETFL, 0);
        if (flags != -1) fcntl(g_nodes.netlink_fd, F_SETFL, flags | O_NONBLOCK);
    }

    log_info("Hardware", "Nodes: backlight='%s' cpu_temp='%s' bat_temp='%s' gpu_temp='%s'",
             g_nodes.backlight[0] ? g_nodes.backlight : "none",
             g_nodes.cpu_temp[0] ? g_nodes.cpu_temp : "none",
             g_nodes.bat_temp[0] ? g_nodes.bat_temp : "none",
             g_nodes.gpu_temp[0] ? g_nodes.gpu_temp : "none");
}
