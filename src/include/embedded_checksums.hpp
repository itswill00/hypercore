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
    { "module.prop", "d768d4b3489ba39218bd2902072c1e93fc6b3c194c4f5c44d1692470d752d445" },
    { "gamelist.txt", "c382088fc5bed225c9726611e90227b0d2f39e3f9b514e48f7102be9934d4a4e" },
    { "update.json", "c117e794956398539d333f89557919c30bf13f6b7cc3a74223e8bf2f03df3d8c" },
    { "changelog.md", "8e22fd4aa269e946133a0f40f5dabaa1eb7fb983e2372586013d87c63dd4f414" },
    { "uninstall.sh", "d744ee35a8c5a6a7f5f9d727655579aab4035efa698d2a00cfbb4948a6fd8d1f" },
    { "webroot/index.html", "3a8d17f713642c71aabc38eaa57ca7f0b5d848b2ddc4bc433a5483ad848dda96" },
    { "banner.jpg", "7c529f60727b6cce0736f8df7fe7d14087fb15da15f97ff20cb289456c62def6" },
};

#endif /* EMBEDDED_CHECKSUMS_HPP */
