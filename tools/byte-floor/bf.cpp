// bf — run the byte-floor resolver on a real file and report what the bytes argued.
// No mocking: it reads actual bytes, discriminates, decodes, reports real counts.
#include "bytefloor.h"
#include <cstdio>
#include <vector>
#include <cstdint>

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: bf <file>\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror("open"); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(sz > 0 ? size_t(sz) : 0);
    if (sz > 0 && fread(buf.data(), 1, size_t(sz), f) != size_t(sz)) { perror("read"); fclose(f); return 1; }
    fclose(f);

    using namespace hcp::bytefloor;
    Result r = resolve(buf.data(), buf.size());
    printf("file       : %s (%ld bytes)\n", argv[1], sz);
    printf("discriminate: %s / %s / %s  (confidence %.2f)\n",
           name(r.disc.size), name(r.disc.endian), name(r.disc.table), r.disc.confidence);
    printf("evidence   : %s\n", r.disc.evidence.c_str());
    printf("decoded    : %zu codepoints, %zu residue bytes\n", r.codepoints, r.residue);
    return 0;
}
