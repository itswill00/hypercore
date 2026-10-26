#include "io.h"
#include "log.h"

static int block_is_interesting(const char *name) {
    return strncmp(name, "sd", 2) == 0 ||
           strncmp(name, "mmcblk", 6) == 0 ||
           strncmp(name, "dm-", 3) == 0 ||
           strncmp(name, "ufs", 3) == 0 ||
           strncmp(name, "nvme", 4) == 0;
}

void set_read_ahead(const char *kb_val) {
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!block_is_interesting(ent->d_name)) continue;
        char p[256];
        snprintf(p, sizeof(p), "/sys/block/%s/queue/read_ahead_kb", ent->d_name);
        sysfs_write(p, kb_val);
    }
    closedir(d);
}

void set_io_nr_requests(const char *nr_str) {
    DIR *d = opendir("/sys/block");
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!block_is_interesting(ent->d_name)) continue;
        /* Device-mapper queues reject nr_requests with EINVAL on MT6789; the
         * write can never take effect, so do not spam the log on every
         * profile transition. Read-ahead is still applied to them. */
        if (strncmp(ent->d_name, "dm-", 3) == 0) continue;
        char p[256];
        snprintf(p, sizeof(p), "/sys/block/%s/queue/nr_requests", ent->d_name);
        sysfs_write(p, nr_str);
    }
    closedir(d);
}

/* --- IRQ affinity -------------------------------------------------------- */

#define MAX_TARGET_IRQS 24

static int s_irqs[MAX_TARGET_IRQS];
static int s_irq_count = -1; /* -1 = not probed yet */

static int irq_name_is_relevant(const char *line) {
    static const char *const keys[] = {
        "mali", "ged", "kgsl", "gpu", "dsi", "touch", "tpd", "fts",
        NULL
    };
    for (int i = 0; keys[i]; i++) {
        if (strstr(line, keys[i])) return 1;
    }
    return 0;
}

void init_irq_targets(void) {
    if (s_irq_count >= 0) return;
    s_irq_count = 0;

    FILE *f = fopen("/proc/interrupts", "r");
    if (!f) {
        log_info("Irq", "Cannot read /proc/interrupts - IRQ affinity tuning disabled");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f) && s_irq_count < MAX_TARGET_IRQS) {
        int irq = atoi(line);
        if (irq <= 0) continue;
        if (!irq_name_is_relevant(line)) continue;

        int duplicate = 0;
        for (int i = 0; i < s_irq_count; i++) {
            if (s_irqs[i] == irq) { duplicate = 1; break; }
        }
        if (!duplicate) s_irqs[s_irq_count++] = irq;
    }
    fclose(f);

    if (s_irq_count > 0) {
        log_info("Irq", "Resolved %d GPU/display/touch IRQs for affinity tuning", s_irq_count);
    } else {
        log_info("Irq", "No matching IRQs in /proc/interrupts - affinity tuning disabled");
    }
}

static unsigned long cpu_mask_all(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n <= 0 || n >= 63) return ~0UL;
    return (1UL << n) - 1;
}

static unsigned long cpu_mask_big(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n >= 8) return 0xC0UL;       /* CPUs 6-7 on MT6789 */
    if (n >= 6) return 0x30UL;       /* CPUs 4-5 fallback */
    return cpu_mask_all();
}

void apply_irq_tuning(profile_t prof) {
    if (s_irq_count <= 0) return;

    int gaming = (prof == PROFILE_Gaming || prof == PROFILE_Gaming_MOBA);
    unsigned long mask = gaming ? cpu_mask_big() : cpu_mask_all();

    char hex[32];
    snprintf(hex, sizeof(hex), "%lx", mask);

    for (int i = 0; i < s_irq_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/irq/%d/smp_affinity", s_irqs[i]);
        sysfs_write(path, hex);
    }
}
