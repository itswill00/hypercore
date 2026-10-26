#include "module.h"
#include "nodes.h"

void update_module_prop_status(const char *status) {
    static char s_last_status[128] = "";
    static time_t s_last_write_time = 0;
    time_t now = time(NULL);

    if (status && strcmp(s_last_status, status) == 0 && (now - s_last_write_time) < 30) {
        return;
    }

    if (status) {
        snprintf(s_last_status, sizeof(s_last_status), "%s", status);
    }
    s_last_write_time = now;

    char prop_path[300], tmp_path[310];
    snprintf(prop_path, sizeof(prop_path), "%s/module.prop", g_nodes.mod_dir);
    snprintf(tmp_path, sizeof(tmp_path), "%s/module.prop.tmp", g_nodes.mod_dir);

    FILE *fin = fopen(prop_path, "r");
    if (!fin) {
        s_last_status[0] = '\0';
        return;
    }

    FILE *fout = fopen(tmp_path, "w");
    if (!fout) {
        fclose(fin);
        return;
    }

    char line[512];
    int write_ok = 1;
    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, "description=", 12) == 0) {
            if (fprintf(fout, "description=[Active: %s] Smart kernel optimizer & gaming daemon for MT6789 Family.\n",
                        status ? status : "Idle") < 0) {
                write_ok = 0;
            }
        } else if (fputs(line, fout) == EOF) {
            write_ok = 0;
        }
    }
    fclose(fin);
    if (fclose(fout) != 0) write_ok = 0;

    if (write_ok) rename(tmp_path, prop_path);
    else unlink(tmp_path);
}
