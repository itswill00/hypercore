#ifndef EMBEDDED_CHECKSUMS_H
#define EMBEDDED_CHECKSUMS_H
typedef struct { const char *rel_path; const char *expected_sha256; } file_checksum_t;
static const file_checksum_t g_embedded_checksums[] = { { "x", "y" } };
#endif
