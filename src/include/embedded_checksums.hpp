#ifndef EMBEDDED_CHECKSUMS_HPP
#define EMBEDDED_CHECKSUMS_HPP

typedef struct {
    const char *rel_path;
    const char *expected_sha256;
} file_checksum_t;

static const file_checksum_t g_embedded_checksums[] = {
    { "customize.sh", "26f1258c320fa6ec8c457caa135c78f77b4ddb640aed248c74734b1caead0845" },
    { "post-fs-data.sh", "08f4818bacf22eb061098631930934e5e5822ad9dc5f99fe7875af48ba1e7d40" },
    { "service.sh", "f22365b95de7bacfc5708daa6e7257d100d8c06bd3e53f7da6d6867329a27504" },
    { "system.prop", "01526be9e2dc85b5d3dff88cf8ee28fd4cd05186714cf8efd8f7c7d45d010155" },
    { "module.prop", "6fd2496e87fb28681c73a4d81b3dd6299bec4ace43be7fcefb50a386ddc71d3e" },
    { "gamelist.txt", "c382088fc5bed225c9726611e90227b0d2f39e3f9b514e48f7102be9934d4a4e" },
    { "update.json", "fbb05d75e3d2b12867634c67128957576679e24c47daa7dada17ebfb51c591e2" },
    { "changelog.md", "1e4d4139a898849998de3d58c8f0a75d0653045d4d91d228ee345f2a24ea07dc" },
    { "uninstall.sh", "d744ee35a8c5a6a7f5f9d727655579aab4035efa698d2a00cfbb4948a6fd8d1f" },
    { "webroot/index.html", "81cdaa4265de48b3b550587becdc86fc0cb16ccbe7dea89b5c8534450531c04b" },
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
