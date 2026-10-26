#include "gpu.h"
#include "nodes.h"
#include "log.h"

static char s_max_gpu_freq_hz[32]  = "1003000000";
static char s_max_gpu_freq_khz[32] = "1003000";
static int  s_gpu_freq_detected    = 0;

void detect_max_gpu_freq(void) {
    if (s_gpu_freq_detected) return;

    long max_freq = 0;
    const char *avail = node_first(N_MALI_AVAIL);
    if (avail) {
        FILE *f = fopen(avail, "r");
        if (f) {
            long freq;
            while (fscanf(f, "%ld", &freq) == 1) {
                if (freq > max_freq) max_freq = freq;
            }
            fclose(f);
        }
    }

    if (max_freq <= 0) {
        for (int i = 0; N_MALI_MAX[i]; i++) {
            long val = (long)sysfs_read_int(N_MALI_MAX[i]);
            if (val > max_freq) max_freq = val;
        }
    }

    if (max_freq >= 800000000L && max_freq <= 2500000000L) {
        snprintf(s_max_gpu_freq_hz, sizeof(s_max_gpu_freq_hz), "%ld", max_freq);
        snprintf(s_max_gpu_freq_khz, sizeof(s_max_gpu_freq_khz), "%ld", max_freq / 1000L);
        log_info("GPU", "Detected hardware max GPU frequency: %ld Hz (%ld MHz)", max_freq, max_freq / 1000000L);
    } else if (max_freq >= 800000L && max_freq <= 2500000L) {
        snprintf(s_max_gpu_freq_hz, sizeof(s_max_gpu_freq_hz), "%ld", max_freq * 1000L);
        snprintf(s_max_gpu_freq_khz, sizeof(s_max_gpu_freq_khz), "%ld", max_freq);
        log_info("GPU", "Detected hardware max GPU frequency: %ld kHz (%ld MHz)", max_freq, max_freq / 1000L);
    } else {
        log_info("GPU", "Defaulting max GPU frequency to 1003000000 Hz (1003 MHz)");
    }

    s_gpu_freq_detected = 1;
}

const char *get_max_gpu_freq_hz(void) {
    if (!s_gpu_freq_detected) detect_max_gpu_freq();
    return s_max_gpu_freq_hz;
}

const char *get_max_gpu_freq_khz(void) {
    if (!s_gpu_freq_detected) detect_max_gpu_freq();
    return s_max_gpu_freq_khz;
}

void set_mali_upthreshold(const char *val) {
    sysfs_write_fallback(N_MALI_UPTHRESH, val);
}

void set_mali_downdifferential(const char *val) {
    sysfs_write_fallback(N_MALI_DOWNDIFF, val);
}
