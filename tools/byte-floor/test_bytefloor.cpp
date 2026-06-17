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

    // ---- Adaptive sampling (claim 617): narrow by structure on a growing probe ----
    auto checkb = [](const char* label, bool ok, const char* detail) {
        printf("[%s] %s  (%s)\n", ok ? "PASS" : "FAIL", label, detail);
        if (ok) ++g_pass; else ++g_fail;
    };
    auto hasCp = [](const Result& r, uint32_t cp) {
        for (auto& e : r.elems) if (e.kind == Elem::Codepoint && e.value == cp) return true;
        return false;
    };

    // 12. Type shows fast -> stop early. UTF-8 multibyte up front, then 1000 ASCII bytes.
    {
        std::vector<uint8_t> b = {0xC3,0xA9};          // é
        b.insert(b.end(), 1000, 'a');
        Result r = resolve(b.data(), b.size());
        char d[128]; snprintf(d, sizeof d, "table=%s sampled=%zu of %zu", name(r.disc.table), r.disc.sampledBytes, b.size());
        checkb("adaptive: clean UTF-8 stops early (<=256B)", r.disc.table == Table::Utf8 && r.disc.sampledBytes <= 256, d);
    }

    // 13. Uninformative ASCII prefix -> probe GROWS past the small window to find the multibyte.
    {
        std::vector<uint8_t> b(2000, 'a');
        b.push_back(0xC3); b.push_back(0xA9);          // é at offset 2000
        b.insert(b.end(), 10, 'b');
        Result r = resolve(b.data(), b.size());
        char d[128]; snprintf(d, sizeof d, "table=%s sampled=%zu, decoded e9=%d", name(r.disc.table), r.disc.sampledBytes, (int)hasCp(r, 0xE9));
        checkb("adaptive: grows past ASCII prefix to resolve UTF-8", r.disc.table == Table::Utf8 && r.disc.sampledBytes > 1024 && hasCp(r, 0xE9), d);
    }

    // 14. All-ASCII -> grows to the whole buffer, settles the ASCII superposition.
    {
        std::vector<uint8_t> b(5000, 'x');
        Result r = resolve(b.data(), b.size());
        char d[128]; snprintf(d, sizeof d, "table=%s sampled=%zu of %zu", name(r.disc.table), r.disc.sampledBytes, b.size());
        checkb("adaptive: all-ASCII grows to full buffer", r.disc.table == Table::Ascii && r.disc.sampledBytes == b.size(), d);
    }

    // 15. Determinism: same input, identical discrimination + decode (pure function).
    {
        std::vector<uint8_t> b(2000, 'a'); b.push_back(0xC3); b.push_back(0xA9);
        Result a = resolve(b.data(), b.size());
        Result c = resolve(b.data(), b.size());
        bool same = a.disc.size == c.disc.size && a.disc.table == c.disc.table
                 && a.disc.sampledBytes == c.disc.sampledBytes
                 && a.codepoints == c.codepoints && a.residue == c.residue;
        checkb("adaptive: deterministic across runs", same, same ? "identical" : "DIVERGED");
    }

    // ---- Positional map (claim 569 output 2): objects carry source spans that tile the
    //      stream exactly and reverse-walk losslessly. ----
    auto tiles = [](const Result& r, size_t len) {        // spans contiguous, ordered, full cover
        uint32_t expect = 0;
        for (auto& e : r.elems) { if (e.srcOffset != expect) return false; expect += e.srcLen; }
        return expect == len;
    };
    auto reconstructs = [](const Result& r, const std::vector<uint8_t>& src) {  // reverse-walk == original
        std::vector<uint8_t> out;
        for (auto& e : r.elems) for (uint32_t k = 0; k < e.srcLen; ++k) out.push_back(src[e.srcOffset + k]);
        return out == src;
    };
    auto posCheck = [&](const char* label, std::vector<uint8_t> b) {
        Result r = resolve(b.data(), b.size());
        bool ok = tiles(r, b.size()) && reconstructs(r, b);
        printf("[%s] %s  (%s, %zu elems)\n", ok ? "PASS" : "FAIL", label,
               name(r.disc.table), r.elems.size());
        if (ok) ++g_pass; else ++g_fail;
    };

    // 16-20. spans tile + reverse-walk losslessly across every decode path.
    posCheck("pos: UTF-8 mixed tiles+reconstructs", {0x63,0x61,0x66,0xC3,0xA9,0xE2,0x82,0xAC,0x21});
    posCheck("pos: Latin-1 tiles+reconstructs",     {0x48,0xE9,0x69,0xFF});
    posCheck("pos: UTF-16LE tiles+reconstructs",    {0x48,0x00,0x69,0x00,0x3D,0xD8,0x00,0xDE}); // incl surrogate pair
    posCheck("pos: UTF-32BE tiles+reconstructs",    {0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x69}); // "Hi" UTF-32BE (75% null -> 4-byte)
    posCheck("pos: UTF-8 with residue tiles+reconstructs", {0xC3,0xA9,0xC3,0x48});

    // 21. spot-check a specific span: the é in "caf"+é sits at byte 3, spans 2 bytes.
    {
        std::vector<uint8_t> b = {0x63,0x61,0x66,0xC3,0xA9};
        Result r = resolve(b.data(), b.size());
        bool ok = r.elems.size() == 4 && r.elems[3].kind == Elem::Codepoint
                  && r.elems[3].value == 0xE9 && r.elems[3].srcOffset == 3 && r.elems[3].srcLen == 2;
        printf("[%s] pos: é object maps to source bytes [3,2)\n", ok ? "PASS" : "FAIL");
        if (ok) ++g_pass; else ++g_fail;
    }

    printf("\n==== %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
