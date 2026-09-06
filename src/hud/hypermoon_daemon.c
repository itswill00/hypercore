#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

static char state_dir[256] = "/data/adb/hypercore/hud";
static char stats_file[512] = "/data/adb/hypercore/hud/stats.json";

static long long prev_idle = 0;
static long long prev_total = 0;
static long long prev_net_rx = 0;
static long long prev_net_tx = 0;

static long long prev_gpu_busy = 0;
static long long prev_gpu_total = 0;

static char cached_hz[32] = "60Hz";

static void read_file_string(const char *path, char *buf, size_t max_len) {
    buf[0] = '\0';
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    ssize_t n = read(fd, buf, max_len - 1);
    close(fd);
    if (n > 0) {
        buf[n] = '\0';
        char *p = strchr(buf, '\n');
        if (p) *p = '\0';
        p = strchr(buf, '\r');
        if (p) *p = '\0';
    }
}

static void get_cpu_temp(char *out, size_t max_len) {
    glob_t g;
    float max_temp = 0.0f;
    if (glob("/sys/class/thermal/thermal_zone*", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            char type_path[512], temp_path[512], type_buf[128], temp_buf[128];
            snprintf(type_path, sizeof(type_path), "%s/type", g.gl_pathv[i]);
            snprintf(temp_path, sizeof(temp_path), "%s/temp", g.gl_pathv[i]);
            read_file_string(type_path, type_buf, sizeof(type_buf));
            for (char *p = type_buf; *p; p++) *p = (char)tolower(*p);
            if (strstr(type_buf, "soc") || strstr(type_buf, "cpu") || strstr(type_buf, "tsens") || strstr(type_buf, "ap")) {
                read_file_string(temp_path, temp_buf, sizeof(temp_buf));
                float v = atof(temp_buf);
                if (v > 1000.0f) v /= 1000.0f;
                if (v > 0.0f && v < 120.0f && v > max_temp) max_temp = v;
            }
        }
        globfree(&g);
    }
    if (max_temp == 0.0f) snprintf(out, max_len, "42.0");
    else snprintf(out, max_len, "%.1f", max_temp);
}

static void get_cpu_freq_and_load(char *freq_out, size_t freq_len, char *load_out, size_t load_len) {
    snprintf(freq_out, freq_len, "0.00GHz");
    snprintf(load_out, load_len, "0");

    glob_t g;
    if (glob("/sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq", 0, NULL, &g) != 0 || g.gl_pathc == 0) {
        glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq", 0, NULL, &g);
    }
    if (g.gl_pathc > 0) {
        long long sum = 0;
        int count = 0;
        for (size_t i = 0; i < g.gl_pathc; i++) {
            char buf[64];
            read_file_string(g.gl_pathv[i], buf, sizeof(buf));
            long v = atol(buf);
            if (v > 0) { sum += v; count++; }
        }
        globfree(&g);
        if (count > 0) {
            double avg_ghz = (sum / (double)count) / 1000000.0;
            snprintf(freq_out, freq_len, "%.2fGHz", avg_ghz);
        }
    }

    FILE *f = fopen("/proc/stat", "r");
    if (f) {
        char line[256];
        if (fgets(line, sizeof(line), f)) {
            long long user, nice, sys, idle, iowait, irq, softirq, steal;
            if (sscanf(line, "cpu %lld %lld %lld %lld %lld %lld %lld %lld",
                       &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal) >= 4) {
                long long idle_total = idle + iowait;
                long long total = user + nice + sys + idle + iowait + irq + softirq + steal;
                if (prev_total > 0) {
                    long long totald = total - prev_total;
                    long long idled = idle_total - prev_idle;
                    if (totald > 0) {
                        int load = (int)((1.0 - ((double)idled / totald)) * 100.0);
                        if (load < 0) load = 0;
                        if (load > 100) load = 100;
                        snprintf(load_out, load_len, "%d", load);
                    }
                }
                prev_idle = idle_total;
                prev_total = total;
            }
        }
        fclose(f);
    }
}

static void get_cpu_gov_and_policy(char *gov_out, size_t gov_len, char *pol_out, size_t pol_len) {
    snprintf(gov_out, gov_len, "schedutil");
    snprintf(pol_out, pol_len, "--");

    read_file_string("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", gov_out, gov_len);

    glob_t g_min, g_max;
    int min_mhz = 99999, max_mhz = 0;
    if (glob("/sys/devices/system/cpu/cpufreq/policy*/scaling_min_freq", 0, NULL, &g_min) == 0) {
        for (size_t i = 0; i < g_min.gl_pathc; i++) {
            char buf[64];
            read_file_string(g_min.gl_pathv[i], buf, sizeof(buf));
            int v = atoi(buf) / 1000;
            if (v > 0 && v < min_mhz) min_mhz = v;
        }
        globfree(&g_min);
    }
    if (glob("/sys/devices/system/cpu/cpufreq/policy*/scaling_max_freq", 0, NULL, &g_max) == 0) {
        for (size_t i = 0; i < g_max.gl_pathc; i++) {
            char buf[64];
            read_file_string(g_max.gl_pathv[i], buf, sizeof(buf));
            int v = atoi(buf) / 1000;
            if (v > max_mhz) max_mhz = v;
        }
        globfree(&g_max);
    }
    if (max_mhz > 0) {
        snprintf(pol_out, pol_len, "%d - %d MHz", min_mhz, max_mhz);
    }
}

static void get_gpu_stats(char *load_out, size_t load_len, char *freq_out, size_t freq_len, char *temp_out, size_t temp_len, char *gov_out, size_t gov_len, char *pol_out, size_t pol_len) {
    snprintf(load_out, load_len, "0");
    snprintf(freq_out, freq_len, "--");
    snprintf(temp_out, temp_len, "--");
    snprintf(gov_out, gov_len, "default");
    snprintf(pol_out, pol_len, "--");

    char buf[128];
    int found_gpu_load = 0;

    // 1. MediaTek GED
    read_file_string("/sys/kernel/ged/hal/gpu_utilization", buf, sizeof(buf));
    if (buf[0]) {
        int v = atoi(buf);
        if (v >= 0) {
            if (v > 100) v = 100;
            snprintf(load_out, load_len, "%d", v);
            found_gpu_load = 1;
        }
    }
    read_file_string("/sys/kernel/ged/hal/current_frequency", buf, sizeof(buf));
    if (!buf[0]) read_file_string("/sys/kernel/ged/hal/current_freqency", buf, sizeof(buf));
    if (buf[0]) {
        char *p = strchr(buf, ' ');
        long khz = atol(p ? p + 1 : buf);
        if (khz > 0) snprintf(freq_out, freq_len, "%ldMHz", khz / 1000);
    }

    // Read actual GPU Governor from sysfs
    glob_t g_gov;
    const char *gov_patterns[] = {
        "/sys/devices/platform/soc/*.mali/devfreq/*.mali/governor",
        "/sys/devices/platform/soc/*mali*/devfreq/*mali*/governor",
        "/sys/devices/platform/soc/*gpu*/devfreq/*gpu*/governor",
        "/sys/devices/platform/*mali*/devfreq/*mali*/governor",
        "/sys/class/devfreq/*mali*/governor",
        "/sys/class/devfreq/*gpu*/governor",
        "/sys/class/devfreq/gpufreq/governor",
        "/sys/class/devfreq/mtk-dvfsrc-devfreq/governor",
        "/sys/class/kgsl/kgsl-3d0/devfreq/governor",
        NULL
    };
    for (int i = 0; gov_patterns[i] != NULL; i++) {
        if (glob(gov_patterns[i], 0, NULL, &g_gov) == 0) {
            if (g_gov.gl_pathc > 0) {
                for (size_t k = 0; k < g_gov.gl_pathc; k++) {
                    read_file_string(g_gov.gl_pathv[k], buf, sizeof(buf));
                    if (buf[0]) {
                        snprintf(gov_out, gov_len, "%s", buf);
                        break;
                    }
                }
            }
            globfree(&g_gov);
            if (buf[0]) break;
        }
    }

    // 2. Qualcomm Adreno (kgsl)
    if (!found_gpu_load) {
        read_file_string("/sys/class/kgsl/kgsl-3d0/gpubusy", buf, sizeof(buf));
        if (buf[0]) {
            long long busy = 0, total = 0;
            if (sscanf(buf, "%lld %lld", &busy, &total) == 2 && total > 0) {
                if (prev_gpu_total > 0 && total > prev_gpu_total) {
                    long long busyd = busy - prev_gpu_busy;
                    long long totald = total - prev_gpu_total;
                    if (totald > 0) {
                        int v = (int)((busyd * 100) / totald);
                        if (v < 0) v = 0; if (v > 100) v = 100;
                        snprintf(load_out, load_len, "%d", v);
                        found_gpu_load = 1;
                    }
                } else {
                    int v = (int)((busy * 100) / total);
                    if (v < 0) v = 0; if (v > 100) v = 100;
                    snprintf(load_out, load_len, "%d", v);
                    found_gpu_load = 1;
                }
                prev_gpu_busy = busy;
                prev_gpu_total = total;
            }
        }
        read_file_string("/sys/class/kgsl/kgsl-3d0/gpuclk", buf, sizeof(buf));
        if (!buf[0]) read_file_string("/sys/class/kgsl/kgsl-3d0/devfreq/cur_freq", buf, sizeof(buf));
        if (buf[0]) {
            long hz = atol(buf);
            if (hz > 1000000) snprintf(freq_out, freq_len, "%ldMHz", hz / 1000000);
            else if (hz > 1000) snprintf(freq_out, freq_len, "%ldMHz", hz / 1000);
        }
    }

    // 3. Mali / Devfreq Fallback
    if (!found_gpu_load || strcmp(freq_out, "0MHz") == 0) {
        glob_t g_devfreq;
        const char *gpu_load_patterns[] = {
            "/sys/class/devfreq/*gpu*/load",
            "/sys/class/devfreq/*mali*/load",
            "/sys/devices/platform/*mali*/gpu_busy",
            "/sys/devices/platform/soc/*mali*/gpu_busy",
            NULL
        };
        for (int i = 0; !found_gpu_load && gpu_load_patterns[i] != NULL; i++) {
            if (glob(gpu_load_patterns[i], 0, NULL, &g_devfreq) == 0) {
                if (g_devfreq.gl_pathc > 0) {
                    read_file_string(g_devfreq.gl_pathv[0], buf, sizeof(buf));
                    if (buf[0]) {
                        int v = atoi(buf);
                        if (v >= 0) {
                            if (v > 100) v = 100;
                            snprintf(load_out, load_len, "%d", v);
                            found_gpu_load = 1;
                        }
                    }
                }
                globfree(&g_devfreq);
            }
        }

        const char *gpu_freq_patterns[] = {
            "/sys/class/devfreq/*gpu*/cur_freq",
            "/sys/class/devfreq/*mali*/cur_freq",
            "/sys/devices/platform/*mali*/devfreq/*mali*/cur_freq",
            "/sys/devices/platform/soc/*mali*/devfreq/*mali*/cur_freq",
            NULL
        };
        for (int i = 0; (strcmp(freq_out, "0MHz") == 0 || strcmp(freq_out, "") == 0) && gpu_freq_patterns[i] != NULL; i++) {
            if (glob(gpu_freq_patterns[i], 0, NULL, &g_devfreq) == 0) {
                if (g_devfreq.gl_pathc > 0) {
                    read_file_string(g_devfreq.gl_pathv[0], buf, sizeof(buf));
                    if (buf[0]) {
                        long hz = atol(buf);
                        if (hz > 1000000) snprintf(freq_out, freq_len, "%ldMHz", hz / 1000000);
                        else if (hz > 1000) snprintf(freq_out, freq_len, "%ldMHz", hz / 1000);
                    }
                }
                globfree(&g_devfreq);
            }
        }
    }

    // GPU Temp
    glob_t g;
    if (glob("/sys/class/thermal/thermal_zone*", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            char type_path[512], temp_path[512], type_buf[128], temp_buf[128];
            snprintf(type_path, sizeof(type_path), "%s/type", g.gl_pathv[i]);
            snprintf(temp_path, sizeof(temp_path), "%s/temp", g.gl_pathv[i]);
            read_file_string(type_path, type_buf, sizeof(type_buf));
            for (char *p = type_buf; *p; p++) *p = (char)tolower(*p);
            if (strstr(type_buf, "gpu") || strstr(type_buf, "g3d") || strstr(type_buf, "mali") || strstr(type_buf, "adreno") || strstr(type_buf, "kgsl")) {
                read_file_string(temp_path, temp_buf, sizeof(temp_buf));
                float v = atof(temp_buf);
                if (v > 1000.0f) v /= 1000.0f;
                if (v > 0.0f && v < 120.0f) {
                    snprintf(temp_out, temp_len, "%.1f", v);
                    break;
                }
            }
        }
        globfree(&g);
    }
}

static void get_ram_stats(char *r_used, size_t ru_len, char *r_tot, size_t rt_len, char *s_used, size_t su_len, char *s_tot, size_t st_len) {
    snprintf(r_used, ru_len, "0.0"); snprintf(r_tot, rt_len, "0.0");
    snprintf(s_used, su_len, "0.0"); snprintf(s_tot, st_len, "0.0");

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[256];
    long long total = 0, free_m = 0, buf_m = 0, cache_m = 0, sreclaim = 0, swap_tot = 0, swap_free = 0;
    while (fgets(line, sizeof(line), f)) {
        long long val;
        if (sscanf(line, "MemTotal: %lld kB", &val) == 1) total = val;
        else if (sscanf(line, "MemFree: %lld kB", &val) == 1) free_m = val;
        else if (sscanf(line, "Buffers: %lld kB", &val) == 1) buf_m = val;
        else if (sscanf(line, "Cached: %lld kB", &val) == 1) cache_m = val;
        else if (sscanf(line, "SReclaimable: %lld kB", &val) == 1) sreclaim = val;
        else if (sscanf(line, "SwapTotal: %lld kB", &val) == 1) swap_tot = val;
        else if (sscanf(line, "SwapFree: %lld kB", &val) == 1) swap_free = val;
    }
    fclose(f);

    long long actual_free = free_m + buf_m + cache_m + sreclaim;
    long long used = total - actual_free;
    if (used < 0) used = 0;
    long long swap_used = swap_tot - swap_free;
    if (swap_used < 0) swap_used = 0;

    snprintf(r_tot, rt_len, "%.1f", total / 1048576.0);
    snprintf(r_used, ru_len, "%.1f", used / 1048576.0);
    snprintf(s_tot, st_len, "%.1f", swap_tot / 1048576.0);
    snprintf(s_used, su_len, "%.1f", swap_used / 1048576.0);
}

static void get_battery_stats(char *watt, size_t w_len, char *btemp, size_t bt_len, char *ma, size_t ma_len, char *mv, size_t mv_len, char *level, size_t lvl_len) {
    snprintf(watt, w_len, "0.0"); snprintf(btemp, bt_len, "--");
    snprintf(ma, ma_len, "0mA"); snprintf(mv, mv_len, "0mV");
    snprintf(level, lvl_len, "100");

    glob_t g;
    if (glob("/sys/class/power_supply/battery*", 0, NULL, &g) == 0 && g.gl_pathc > 0) {
        char path[512], buf[128];
        snprintf(path, sizeof(path), "%s/current_now", g.gl_pathv[0]);
        read_file_string(path, buf, sizeof(buf));
        long curr_ua = labs(atol(buf));
        snprintf(ma, ma_len, "%ldmA", curr_ua / 1000);

        snprintf(path, sizeof(path), "%s/voltage_now", g.gl_pathv[0]);
        read_file_string(path, buf, sizeof(buf));
        long volt_uv = labs(atol(buf));
        snprintf(mv, mv_len, "%ldmV", volt_uv / 1000);

        if (curr_ua > 0 && volt_uv > 0) {
            double w = (curr_ua / 1000000.0) * (volt_uv / 1000000.0);
            snprintf(watt, w_len, "%.2f", w);
        }

        snprintf(path, sizeof(path), "%s/temp", g.gl_pathv[0]);
        read_file_string(path, buf, sizeof(buf));
        float t = atof(buf);
        if (t > 100.0f) t /= 10.0f;
        snprintf(btemp, bt_len, "%.1f", t);

        snprintf(path, sizeof(path), "%s/capacity", g.gl_pathv[0]);
        read_file_string(path, buf, sizeof(buf));
        if (buf[0]) snprintf(level, lvl_len, "%s", buf);

        globfree(&g);
    }
}

static void get_net_speed(double dt, char *dl_out, size_t dl_len, char *ul_out, size_t ul_len) {
    long long rx = 0, tx = 0;
    FILE *f = fopen("/proc/net/dev", "r");
    if (f) {
        char line[256];
        int line_num = 0;
        while (fgets(line, sizeof(line), f)) {
            if (++line_num <= 2) continue;
            char iface[64];
            long long r, t, dummy;
            if (sscanf(line, "%s %lld %lld %lld %lld %lld %lld %lld %lld %lld",
                       iface, &r, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &t) >= 10) {
                if (strncmp(iface, "lo", 2) != 0) {
                    rx += r; tx += t;
                }
            }
        }
        fclose(f);
    }
    if (dt <= 0) dt = 0.25;
    double dl_kbs = (rx - prev_net_rx) / dt / 1024.0;
    double ul_kbs = (tx - prev_net_tx) / dt / 1024.0;
    if (dl_kbs < 0) dl_kbs = 0;
    if (ul_kbs < 0) ul_kbs = 0;
    prev_net_rx = rx; prev_net_tx = tx;

    if (dl_kbs >= 1024.0) snprintf(dl_out, dl_len, "%.1f MB/s", dl_kbs / 1024.0);
    else snprintf(dl_out, dl_len, "%d KB/s", (int)dl_kbs);

    if (ul_kbs >= 1024.0) snprintf(ul_out, ul_len, "%.1f MB/s", ul_kbs / 1024.0);
    else snprintf(ul_out, ul_len, "%d KB/s", (int)ul_kbs);
}

static void get_fps_and_hz(char *fps_out, size_t fps_len, char *ft_out, size_t ft_len) {
    snprintf(fps_out, fps_len, "60");
    snprintf(ft_out, ft_len, "16.6");

    char buf[128];

    // 1. Detect Screen Refresh Rate (Hz)
    glob_t g_hz;
    const char *hz_paths[] = {
        "/sys/class/graphics/fb0/dynamic_fps",
        "/sys/class/graphics/fb0/fps",
        "/sys/class/drm/sde-crtc-0/fps",
        "/sys/class/drm/card0-DSI-1/fps",
        "/sys/devices/platform/soc/*display*/fps",
        "/sys/devices/platform/soc/*display*/fps_select",
        "/sys/class/graphics/fb0/mode_name",
        "/sys/class/drm/card0-DP-1/mode",
        NULL
    };
    for (int i = 0; hz_paths[i] != NULL; i++) {
        if (strchr(hz_paths[i], '*')) {
            if (glob(hz_paths[i], 0, NULL, &g_hz) == 0) {
                for (size_t k = 0; k < g_hz.gl_pathc; k++) {
                    read_file_string(g_hz.gl_pathv[k], buf, sizeof(buf));
                    if (buf[0]) {
                        char *p = strstr(buf, "@");
                        int val = p ? atoi(p + 1) : atoi(buf);
                        if (val > 0) {
                            snprintf(cached_hz, sizeof(cached_hz), "%dHz", val);
                            globfree(&g_hz);
                            goto hz_found;
                        }
                    }
                }
                globfree(&g_hz);
            }
        } else {
            read_file_string(hz_paths[i], buf, sizeof(buf));
            if (buf[0]) {
                char *p = strstr(buf, "@");
                int val = p ? atoi(p + 1) : atoi(buf);
                if (val > 0) {
                    snprintf(cached_hz, sizeof(cached_hz), "%dHz", val);
                    break;
                }
            }
        }
    }
hz_found:
    ;

    // 2. Multi-backend FPS Detection

    // Backend A: MediaTek FPSGO
    glob_t g;
    if (glob("/sys/kernel/fpsgo*/fstb/fpsgo_status", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            FILE *f = fopen(g.gl_pathv[i], "r");
            if (f) {
                char line[256];
                int line_cnt = 0;
                while (fgets(line, sizeof(line), f)) {
                    if (++line_cnt == 1) continue;
                    int tid, curr_fps;
                    char bufid[64], name[128];
                    if (sscanf(line, "%d %s %s %d", &tid, bufid, name, &curr_fps) >= 4) {
                        if (curr_fps > 0 && !strstr(name, "systemui") && !strstr(name, "launcher") && !strstr(name, "inputmethod")) {
                            snprintf(fps_out, fps_len, "%d", curr_fps);
                            snprintf(ft_out, ft_len, "%.1f", 1000.0 / curr_fps);
                            fclose(f);
                            globfree(&g);
                            return;
                        }
                    }
                }
                fclose(f);
            }
        }
        globfree(&g);
    }

    // Backend B: Snapdragon Adreno / SDE Measured FPS
    read_file_string("/sys/class/drm/sde-crtc-0/measured_fps", buf, sizeof(buf));
    if (!buf[0]) read_file_string("/sys/class/graphics/fb0/measured_fps", buf, sizeof(buf));
    if (buf[0]) {
        int curr_fps = atoi(buf);
        if (curr_fps > 0) {
            snprintf(fps_out, fps_len, "%d", curr_fps);
            snprintf(ft_out, ft_len, "%.1f", 1000.0 / curr_fps);
            return;
        }
    }
}

static time_t last_cfg_mtime = 0;
static int cached_refresh_interval = 1500;

static int get_refresh_interval() {
    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/config.json", state_dir);
    struct stat st;
    if (stat(cfg_path, &st) == 0) {
        if (st.st_mtime == last_cfg_mtime) {
            return cached_refresh_interval;
        }
        last_cfg_mtime = st.st_mtime;
        FILE *f = fopen(cfg_path, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                char *p = strstr(line, "\"refresh_interval\"");
                if (p) {
                    char *colon = strchr(p, ':');
                    if (colon) {
                        int v = atoi(colon + 1);
                        if (v >= 50 && v <= 5000) cached_refresh_interval = v;
                    }
                }
            }
            fclose(f);
        }
    }
    return cached_refresh_interval;
}

int main() {
    char *env_state = getenv("HYPERMOON_STATE_DIR");
    if (!env_state || strlen(env_state) == 0) {
        env_state = getenv("FPSMOON_STATE_DIR");
    }
    if (env_state && strlen(env_state) > 0) {
        snprintf(state_dir, sizeof(state_dir), "%s", env_state);
    }
    snprintf(stats_file, sizeof(stats_file), "%s/stats.json", state_dir);

    mkdir(state_dir, 0777);
    printf("[HyperMoon C-Daemon] Performance monitor active on %s...\n", state_dir);

    struct timespec ts_last;
    clock_gettime(CLOCK_MONOTONIC, &ts_last);

    while (1) {
        struct timespec ts_now;
        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double dt = (ts_now.tv_sec - ts_last.tv_sec) + (ts_now.tv_nsec - ts_last.tv_nsec) / 1e9;
        ts_last = ts_now;

        char c_temp[32], c_freq[32], c_load[32], c_gov[32], c_pol[64];
        char g_load[32], g_freq[32], g_temp[32], g_gov[32], g_pol[64];
        char r_used[32], r_tot[32], s_used[32], s_tot[32];
        char b_watt[32], b_temp[32], b_ma[32], b_mv[32], b_lvl[32];
        char net_dl[32], net_ul[32], fps[32], ft[32];

        get_cpu_temp(c_temp, sizeof(c_temp));
        get_cpu_freq_and_load(c_freq, sizeof(c_freq), c_load, sizeof(c_load));
        get_cpu_gov_and_policy(c_gov, sizeof(c_gov), c_pol, sizeof(c_pol));
        get_gpu_stats(g_load, sizeof(g_load), g_freq, sizeof(g_freq), g_temp, sizeof(g_temp), g_gov, sizeof(g_gov), g_pol, sizeof(g_pol));
        get_ram_stats(r_used, sizeof(r_used), r_tot, sizeof(r_tot), s_used, sizeof(s_used), s_tot, sizeof(s_tot));
        get_battery_stats(b_watt, sizeof(b_watt), b_temp, sizeof(b_temp), b_ma, sizeof(b_ma), b_mv, sizeof(b_mv), b_lvl, sizeof(b_lvl));
        get_net_speed(dt, net_dl, sizeof(net_dl), net_ul, sizeof(net_ul));
        get_fps_and_hz(fps, sizeof(fps), ft, sizeof(ft));

        time_t now_sec = time(NULL);

        char tmp_path[512];
        snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", stats_file);

        FILE *f = fopen(tmp_path, "w");
        if (f) {
            fprintf(f, "{\n"
                       "  \"fps\": \"%s\",\n"
                       "  \"frametime\": \"%s\",\n"
                       "  \"cpu_load\": \"%s\",\n"
                       "  \"cpu_freq\": \"%s\",\n"
                       "  \"cpu_temp\": \"%s\",\n"
                       "  \"cpu_gov\": \"%s\",\n"
                       "  \"cpu_policy\": \"%s\",\n"
                       "  \"gpu_load\": \"%s\",\n"
                       "  \"gpu_freq\": \"%s\",\n"
                       "  \"gpu_temp\": \"%s\",\n"
                       "  \"gpu_gov\": \"%s\",\n"
                       "  \"gpu_policy\": \"%s\",\n"
                       "  \"ram_used\": \"%s\",\n"
                       "  \"ram_total\": \"%s\",\n"
                       "  \"swap_used\": \"%s\",\n"
                       "  \"swap_total\": \"%s\",\n"
                       "  \"bat_watt\": \"%s\",\n"
                       "  \"bat_temp\": \"%s\",\n"
                       "  \"bat_curr\": \"%s\",\n"
                       "  \"bat_volt\": \"%s\",\n"
                       "  \"bat_level\": \"%s\",\n"
                       "  \"net_dl\": \"%s\",\n"
                       "  \"net_ul\": \"%s\",\n"
                       "  \"screen_hz\": \"%s\",\n"
                       "  \"timestamp\": %ld\n"
                       "}\n",
                    fps, ft, c_load, c_freq, c_temp, c_gov, c_pol,
                    g_load, g_freq, g_temp, g_gov, g_pol,
                    r_used, r_tot, s_used, s_tot,
                    b_watt, b_temp, b_ma, b_mv, b_lvl,
                    net_dl, net_ul, cached_hz, (long)now_sec);
            fclose(f);
            rename(tmp_path, stats_file);
        }

        // Auto Keep-Alive Watchdog: Only respawn if visible is not set to false in config.json
        static int watchdog_cnt = 0;
        if (++watchdog_cnt >= 20) {
            watchdog_cnt = 0;
            int is_vis = 1;
            char cfg_path[512];
            snprintf(cfg_path, sizeof(cfg_path), "%s/config.json", state_dir);
            FILE *fc = fopen(cfg_path, "r");
            if (fc) {
                char cline[256];
                while (fgets(cline, sizeof(cline), fc)) {
                    if (strstr(cline, "\"visible\"") && strstr(cline, "false")) {
                        is_vis = 0;
                        break;
                    }
                }
                fclose(fc);
            }

            if (is_vis && system("pgrep -f com.hypermoon.HyperMoonOverlay >/dev/null 2>&1") != 0 &&
                system("pgrep -f com.fpsmoon.FPSMoonOverlay >/dev/null 2>&1") != 0) {
                char dex_path[512] = "/data/adb/modules/hypercore/system/bin/hypermoon.dex";
                if (access(dex_path, F_OK) != 0) {
                    snprintf(dex_path, sizeof(dex_path), "%s/../bin/hypermoon.dex", state_dir);
                }
                char respawn_cmd[1024];
                snprintf(respawn_cmd, sizeof(respawn_cmd),
                         "( CLASSPATH=\"%s\" /system/bin/app_process /system/bin com.hypermoon.HyperMoonOverlay \"%s\" > \"%s/overlay.log\" 2>&1 & )",
                         dex_path, state_dir, state_dir);
                system(respawn_cmd);
            }
        }

        int interval_ms = get_refresh_interval();
        usleep(interval_ms * 1000);
    }
    return 0;
}

