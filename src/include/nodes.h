#ifndef HYPERCORE_NODES_H
#define HYPERCORE_NODES_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Single source of truth for every device path HyperCore reads or writes.
 *
 * Every list is NULL-terminated and ordered most-specific first. Adding a
 * device variant means editing exactly one array here, not the five
 * hand-maintained copies (cpu.c, gpu.c, sysfs.c, customize.sh, uninstall.sh)
 * the previous layout required.
 *
 * Resolution is deliberately lazy: node_first() probes only when called, so
 * driver nodes that probe after boot_completed can be re-resolved later. */

/* Display */
extern const char *const N_BACKLIGHT[];

/* CPU policy nodes. MT6789 is a strict bi-cluster part (policy0 + policy6),
 * so policy4/policy7 are not probed: writing them was always a no-op. */
extern const char *const N_CPU_GOV0[];
extern const char *const N_CPU_GOV6[];
extern const char *const N_CPU_POL0_MIN[];
extern const char *const N_CPU_POL0_MAX[];
extern const char *const N_CPU_POL6_MIN[];
extern const char *const N_CPU_POL6_MAX[];
extern const char *const N_RATE_P0_UP[];
extern const char *const N_RATE_P0_DOWN[];
extern const char *const N_RATE_P6_UP[];
extern const char *const N_RATE_P6_DOWN[];
extern const char *const N_CPUAVAIL_GOV[];

/* Cgroups / uclamp (cgroup v1 and v2 paths, same list for apply and restore) */
extern const char *const N_CPUSET_BG[];
extern const char *const N_CPUSET_SYS_BG[];
extern const char *const N_CPUSET_TOP_APP[];
extern const char *const N_CPUCTL_BG_SHARES[];
extern const char *const N_CPUCTL_BG_MIN[];
extern const char *const N_CPUCTL_BG_MAX[];
extern const char *const N_CPUCTL_SYS_BG_MAX[];
extern const char *const N_CPUCTL_TOP_SHARES[];
extern const char *const N_CPUCTL_TOP_MIN[];
extern const char *const N_CPUCTL_TOP_MAX[];

/* Mali devfreq */
extern const char *const N_MALI_GOV[];
extern const char *const N_MALI_MIN[];
extern const char *const N_MALI_MAX[];
extern const char *const N_MALI_POLL[];
extern const char *const N_MALI_AVAIL[];
extern const char *const N_MALI_POLICY[];
extern const char *const N_MALI_UPTHRESH[];
extern const char *const N_MALI_DOWNDIFF[];
extern const char *const N_MALI_CUR[];

/* MediaTek extras */
extern const char *const N_DVFSRC_QOS[];
extern const char *const N_GX_GAME_MODE[];
extern const char *const N_SCONFIG[];

/* Touch */
extern const char *const N_TOUCH_SMOOTH[];
extern const char *const N_TOUCH_NOISE[];
extern const char *const N_TOUCH_EDGE[];
extern const char *const N_TOUCH_GAME[];
extern const char *const N_TOUCH_SENS[];

/* Fixed GED / FPSGO / VM / scheduler nodes (single path lists so the same
 * table can drive both capture and restore). */
extern const char *const N_GED_BOOST_GPU[];
extern const char *const N_GED_SMART_BOOST[];
extern const char *const N_GED_BOOST_ENABLE[];
extern const char *const N_GED_ENABLE_GPU_BOOST[];
extern const char *const N_GED_CUST_BOOST_FREQ[];
extern const char *const N_GED_CUST_UPBOUND[];
extern const char *const N_GED_GPU_BOTTOM[];
extern const char *const N_GED_FB_DVFS_THRESHOLD[];
extern const char *const N_GED_GX_MARGIN[];
extern const char *const N_GED_GX_GAME_MODE[];
extern const char *const N_GED_GPU_LOAD[];
extern const char *const N_FPSGO_FORCE[];
extern const char *const N_FPSGO_BOOST_TA[];
extern const char *const N_FPSGO_ULTRA[];
extern const char *const N_FPSGO_LIGHT[];
extern const char *const N_FPSGO_IDLEPREFER[];
extern const char *const N_FPSGO_THRM[];
extern const char *const N_VM_SWAPPINESS[];
extern const char *const N_VM_DIRTY_RATIO[];
extern const char *const N_VM_DIRTY_BG_RATIO[];
extern const char *const N_VM_VFS_CACHE[];
extern const char *const N_VM_STAT_INTERVAL[];
extern const char *const N_VM_DIRTY_WRITEBACK[];
extern const char *const N_VM_PAGE_CLUSTER[];
extern const char *const N_VM_COMPACTION[];
extern const char *const N_NET_READ_AHEAD[];
extern const char *const N_NET_NR_REQUESTS[];
extern const char *const N_SCHED_MIGRATION[];
extern const char *const N_SCHED_LATENCY[];
extern const char *const N_SCHED_NR_MIGRATE[];

/* First existing candidate, or NULL. */
const char *node_first(const char *const paths[]);
/* Copy the first existing candidate into dst, or leave dst empty. */
void node_resolve(char *dst, size_t cap, const char *const paths[]);
/* Read an int from the first existing candidate (0 when none). */
int node_read_int(const char *const paths[]);

/* Resolve g_nodes paths, caches and event descriptors. */
void nodes_init(void);
/* Re-probe touch nodes that were missing at startup (late IC probe). */
void nodes_rediscover_touch(void);
/* Re-probe the backlight node. */
void nodes_resolve_backlight(void);

#ifdef __cplusplus
}
#endif

#endif
