// bf — run the byte-floor resolver on a real file and report what the bytes argued.
// No mocking: it reads actual bytes, discriminates, decodes, renders, and verifies the
// positional map reverse-walks losslessly. This is the "bytes in -> tokens out" demo.
#include "bytefloor.h"
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

static void appendUtf8(uint32_t cp, std::string& out) {
    if      (cp < 0x80)    out += char(cp);
    else if (cp < 0x800)   { out += char(0xC0|(cp>>6));  out += char(0x80|(cp&0x3F)); }
    else if (cp < 0x10000) { out += char(0xE0|(cp>>12)); out += char(0x80|((cp>>6)&0x3F)); out += char(0x80|(cp&0x3F)); }
    else                   { out += char(0xF0|(cp>>18)); out += char(0x80|((cp>>12)&0x3F)); out += char(0x80|((cp>>6)&0x3F)); out += char(0x80|(cp&0x3F)); }
}

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

    printf("file        : %s (%ld bytes)\n", argv[1], sz);
    printf("discriminate: %s / %s / %s  (confidence %.2f, sampled %zu of %ld bytes)\n",
           name(r.disc.size), name(r.disc.endian), name(r.disc.table),
           r.disc.confidence, r.disc.sampledBytes, sz);
    printf("evidence    : %s\n", r.disc.evidence.c_str());
    printf("decoded     : %zu objects (%zu codepoints, %zu residue)\n",
           r.elems.size(), r.codepoints, r.residue);

    // Render the first stretch back to text — the "rendered set of tokens out the other end".
    std::string text;
    for (auto& e : r.elems) {
        if (e.kind == Elem::Codepoint) appendUtf8(e.value, text);
        else                           text += '.';   // residue placeholder
        if (text.size() > 200) break;
    }
    printf("rendered[:200]: %s%s\n", text.c_str(), text.size() > 200 ? "..." : "");

    // Show a few multibyte / residue objects with their source spans (the positional map).
    printf("positional map (first multibyte/residue objects):\n");
    int shown = 0;
    for (size_t k = 0; k < r.elems.size() && shown < 6; ++k) {
        const Elem& e = r.elems[k];
        if (e.kind == Elem::Residue || e.value >= 0x80) {
            printf("  obj %zu: %-9s U+%04X  <- source bytes [%u..%u)\n", k,
                   e.kind == Elem::Residue ? "residue" : "codepoint",
                   e.value, e.srcOffset, e.srcOffset + e.srcLen);
            ++shown;
        }
    }
    if (shown == 0) printf("  (pure ASCII — every object is one byte)\n");

    // Verify the positional map is lossless: reverse-walk the spans back to the source.
    std::vector<uint8_t> rebuilt; rebuilt.reserve(buf.size());
    bool ordered = true; uint32_t expect = 0;
    for (auto& e : r.elems) {
        if (e.srcOffset != expect) ordered = false;
        expect += e.srcLen;
        for (uint32_t b = 0; b < e.srcLen; ++b) rebuilt.push_back(buf[e.srcOffset + b]);
    }
    const bool lossless = ordered && rebuilt == buf;
    printf("round-trip  : spans %s, reverse-walk %s original (%s)\n",
           ordered ? "tile exactly" : "DO NOT TILE",
           lossless ? "==" : "!=",
           lossless ? "LOSSLESS" : "LOSSY");
    return lossless ? 0 : 1;
}
