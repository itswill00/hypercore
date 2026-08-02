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
    { "module.prop", "f6c65f9a01443d0a8df32d75f8a07536be196eb7acecb69632f5e73d91f8abc7" },
    { "gamelist.txt", "c382088fc5bed225c9726611e90227b0d2f39e3f9b514e48f7102be9934d4a4e" },
    { "update.json", "fbb05d75e3d2b12867634c67128957576679e24c47daa7dada17ebfb51c591e2" },
    { "changelog.md", "1e4d4139a898849998de3d58c8f0a75d0653045d4d91d228ee345f2a24ea07dc" },
    { "uninstall.sh", "d744ee35a8c5a6a7f5f9d727655579aab4035efa698d2a00cfbb4948a6fd8d1f" },
    { "webroot/index.html", "d4c74cc27dc5063443ef1bf8b6def3455849375a3be640ea659332d2cd35569d" },
    { "banner.jpg", "7c529f60727b6cce0736f8df7fe7d14087fb15da15f97ff20cb289456c62def6" },
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
