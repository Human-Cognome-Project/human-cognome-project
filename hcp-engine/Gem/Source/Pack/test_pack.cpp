// ============================================================================
// test_pack — deterministic checks for the compact-ID packer + GPU store.
//
// Proves the corrected vocab.lmdb shape: {compact-id -> chars} only, identity
// reattached BY POSITION via a CPU-side ledger. No large ids, no morphology, no
// reverse maps in the store. No GPU needed — this is the oracle.
// ============================================================================
#include "PackKernel.h"
#include "PackStore.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace HCP::Pack;

static int g_fail = 0;
static void check(bool cond, const std::string& msg)
{
    std::printf("%s %s\n", cond ? "  ok:" : "FAIL:", msg.c_str());
    if (!cond) ++g_fail;
}

int main()
{
    // A bounded staging window. Note: "dog"/"dogs" and "run"/"running" are
    // DISTINCT tokens with DISTINCT canonical ids — see-it/mint-it, no morph
    // reconstruction (claim 56). Canonical ids are large and sparse.
    const std::vector<WindowEntry> win = {
        { 500000001ULL, "dog"     },
        { 500000002ULL, "dogs"    },
        { 742000099ULL, "cat"     },
        {   9000123ULL, "the"     },
        { 123456789ULL, "running" },
        {        42ULL, "run"     },
        { 999999999ULL, "cats"    },
    };

    PackResult packed = PackWindow(win);

    // 1. compact ids are dense 0..N-1, each canonical present exactly once
    check(packed.count == win.size(), "window count preserved");
    check(packed.ledger.size() == win.size(), "ledger sized to window");
    {
        std::set<uint64_t> seen(packed.ledger.begin(), packed.ledger.end());
        check(seen.size() == win.size(), "every canonical id present exactly once in ledger");
    }

    // 2. slot order is length-ascending (so position is a stable join key)
    {
        bool ascending = true;
        uint32_t prev = 0;
        for (const auto& b : packed.buckets) { if (b.length < prev) ascending = false; prev = b.length; }
        check(ascending, "buckets/compact-id ranges are length-ascending (slot order)");
    }

    // 3. write the GPU-facing store
    std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp")
                     + "/hcp_pack_test.mdb";
    std::remove(path.c_str());
    std::remove((path + "-lock").c_str());
    check(WriteStore(path.c_str(), packed), "wrote GPU-facing store");

    // 4. the store holds EXACTLY one sub-db — no c2t/t2c/t2w/manifest/entities cruft
    check(CountNamedSubDbs(path.c_str()) == 1, "store has exactly ONE sub-db (chars only)");

    // 5. read back; the store carries NO canonical identity
    PackResult rb = ReadStore(path.c_str());
    check(rb.count == packed.count, "readback window count matches");
    check(rb.ledger.empty(), "store carries NO canonical ids (identity is CPU-side)");

    // 6. round-trip: each compact id -> identical chars, written vs read
    bool charsMatch = true;
    for (uint32_t cid = 0; cid < packed.count; ++cid)
        if (CharsForCompactId(packed, cid) != CharsForCompactId(rb, cid)) charsMatch = false;
    check(charsMatch, "chars round-trip identical by compact-id slot");

    // 7. identity-by-position: simulate the GPU returning "compact id N settled",
    //    reattach the canonical id via the CPU ledger, and confirm it lands on the
    //    correct original entry (matched by its chars).
    std::map<uint64_t, std::string> canonToChars;
    for (const auto& e : win) canonToChars[e.canonicalId] = e.chars;

    bool identityOk = true;
    for (uint32_t cid = 0; cid < rb.count; ++cid)
    {
        std::string chars   = CharsForCompactId(rb, cid);   // what the GPU matched
        uint64_t    canon   = packed.ledger[cid];           // CPU reattaches by slot
        if (canonToChars[canon] != chars) identityOk = false;
    }
    check(identityOk, "compact-id -> canonical (by position) lands on the right entry");

    // 8. minimality: stored payload is EXACTLY the chars, nothing per-entry extra.
    size_t charBytes = 0; for (const auto& e : win) charBytes += e.chars.size();
    size_t storedBytes = 0; for (const auto& b : rb.buckets) storedBytes += b.blob.size();
    check(storedBytes == charBytes, "stored payload == sum of chars (no ids/morph per entry)");
    // The drifted store spent >= 14 bytes/entry on the token-id STRING alone.
    check(storedBytes < win.size() * 14, "store smaller than old 14-byte-id-per-entry format");

    // 9. caps derived, not stored: count recomputed from blob/length matches
    bool capsOk = true;
    for (const auto& b : rb.buckets) if (b.count != b.blob.size() / b.length) capsOk = false;
    check(capsOk, "per-length count is DERIVED from blob/length (claim 91)");

    std::remove(path.c_str());
    std::remove((path + "-lock").c_str());

    std::printf("\n%s (%d failures)\n", g_fail == 0 ? "PASS" : "FAILED", g_fail);
    return g_fail == 0 ? 0 : 1;
}
