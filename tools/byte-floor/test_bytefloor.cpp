// Vet harness for the byte-floor resolver: crafted byte streams with known answers.
// Checks self-discrimination (size/endian/table) + decode + residue, and that BOMs are
// reached from CONTENT, not trusted as authority.
#include "bytefloor.h"
#include <cstdio>
#include <vector>
#include <cstdint>

using namespace hcp::bytefloor;

static int g_pass = 0, g_fail = 0;

static std::vector<uint32_t> codepointsOf(const Result& r) {
    std::vector<uint32_t> v;
    for (auto& e : r.elems) if (e.kind == Elem::Codepoint) v.push_back(e.value);
    return v;
}

static void check(const char* label,
                  std::vector<uint8_t> bytes,
                  Size esize, Endian eend, Table etab,
                  std::vector<uint32_t> ecp, size_t eres) {
    Result r = resolve(bytes.data(), bytes.size());
    std::vector<uint32_t> cp = codepointsOf(r);
    bool ok = r.disc.size == esize && r.disc.endian == eend && r.disc.table == etab
              && cp == ecp && r.residue == eres;
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", label);
    printf("      got: %s / %s / %s | %zu cps, %zu residue | %s\n",
           name(r.disc.size), name(r.disc.endian), name(r.disc.table),
           r.codepoints, r.residue, r.disc.evidence.c_str());
    if (!ok) {
        printf("      exp: %s / %s / %s | cps[", name(esize), name(eend), name(etab));
        for (auto c : ecp) printf("%04X ", c); printf("] residue=%zu\n", eres);
        printf("      got cps[");
        for (auto c : cp) printf("%04X ", c); printf("]\n");
        ++g_fail;
    } else ++g_pass;
}

int main() {
    // 1. plain ASCII -> 1-byte, no endian, ASCII (superposition with UTF-8/Latin-1)
    check("ASCII 'Hi'", {0x48,0x69}, Size::One, Endian::None, Table::Ascii, {0x48,0x69}, 0);

    // 2. UTF-8 with a 2-byte char: 'café' (é = C3 A9)
    check("UTF-8 'cafe'+acute", {0x63,0x61,0x66,0xC3,0xA9}, Size::One, Endian::None, Table::Utf8, {0x63,0x61,0x66,0xE9}, 0);

    // 3. UTF-8 3-byte: euro sign U+20AC = E2 82 AC
    check("UTF-8 euro", {0xE2,0x82,0xAC}, Size::One, Endian::None, Table::Utf8, {0x20AC}, 0);

    // 4. UTF-16LE 'Hi'
    check("UTF-16LE 'Hi'", {0x48,0x00,0x69,0x00}, Size::Two, Endian::Little, Table::Utf16, {0x48,0x69}, 0);

    // 5. UTF-16BE 'Hi'
    check("UTF-16BE 'Hi'", {0x00,0x48,0x00,0x69}, Size::Two, Endian::Big, Table::Utf16, {0x48,0x69}, 0);

    // 6. UTF-32LE 'H'
    check("UTF-32LE 'H'", {0x48,0x00,0x00,0x00}, Size::Four, Endian::Little, Table::Utf32, {0x48}, 0);

    // 7. UTF-32BE 'H'
    check("UTF-32BE 'H'", {0x00,0x00,0x00,0x48}, Size::Four, Endian::Big, Table::Utf32, {0x48}, 0);

    // 8. Latin-1: high byte that is NOT valid UTF-8 (E9 'é' followed by ascii) -> 8-bit table
    check("Latin-1 high byte", {0x48,0xE9,0x69}, Size::One, Endian::None, Table::Latin1, {0x48,0xE9,0x69}, 0);

    // 9. UTF-8 with a broken sequence -> classified UTF-8 (valid é present), broken C3 -> residue
    check("UTF-8 + residue", {0xC3,0xA9,0xC3,0x48}, Size::One, Endian::None, Table::Utf8, {0xE9,0x48}, 1);

    // 10. UTF-8 BOM as CONTENT (EF BB BF) — reached as UTF-8 from content, BOM decodes to U+FEFF, not trusted
    check("UTF-8 BOM=content", {0xEF,0xBB,0xBF,0x48,0x69}, Size::One, Endian::None, Table::Utf8, {0xFEFF,0x48,0x69}, 0);

    // 11. UTF-16LE BOM as CONTENT (FF FE) — size/endian from null pattern, BOM decodes to U+FEFF
    check("UTF-16LE BOM=content", {0xFF,0xFE,0x48,0x00,0x69,0x00}, Size::Two, Endian::Little, Table::Utf16, {0xFEFF,0x48,0x69}, 0);

    printf("\n==== %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
