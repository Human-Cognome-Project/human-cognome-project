// ============================================================================
// pack_vocab — produce the corrected GPU-facing vocab store from the REAL warm
// set, replacing the stale 186 MB relic.
//
// Reads hcp_var.envelope_working_set (the 1.39M-row warm set, localhost:5432 —
// physics-side, claim 120), takes a bounded priority-ordered slice, runs the
// packer, and writes a single-sub-db {compact-id -> chars} store. Verifies the
// result against the warm set (round-trip chars + identity-by-position).
//
// Usage: pack_vocab [slice_size|all] [out_path]
//   slice_size default 20000 (LMDB_SLICE_SIZE, claim 628 bounded window)
//   out_path   default ./vocab.lmdb.new
// ============================================================================
#include "PackKernel.h"
#include "PackStore.h"

#include <libpq-fe.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace HCP::Pack;

int main(int argc, char** argv)
{
    long sliceSize = 20000;                 // bounded window default
    bool all = false;
    if (argc > 1) { if (std::strcmp(argv[1], "all") == 0) all = true; else sliceSize = std::atol(argv[1]); }
    const char* outPath = (argc > 2) ? argv[2] : "vocab.lmdb.new";
    // mode: "dir" = engine-format subdir store (data.mdb/lock.mdb in a directory);
    // "file" (default) = single-file store. The live engine opens subdir mode.
    bool subdir = (argc > 3 && std::strcmp(argv[3], "dir") == 0);
    if (subdir) mkdir(outPath, 0755);       // ensure the directory exists (ok if present)

    PGconn* c = PQconnectdb("host=localhost port=5432 dbname=hcp_var user=hcp password=hcp_dev");
    if (PQstatus(c) != CONNECTION_OK) { std::fprintf(stderr, "connect: %s\n", PQerrorMessage(c)); return 2; }

    std::string q = "SELECT word, token_id FROM envelope_working_set ORDER BY effective_priority";
    if (!all) q += " LIMIT " + std::to_string(sliceSize);

    PGresult* r = PQexec(c, q.c_str());
    if (PQresultStatus(r) != PGRES_TUPLES_OK) { std::fprintf(stderr, "query: %s\n", PQerrorMessage(c)); return 2; }

    int n = PQntuples(r);
    std::vector<WindowEntry> win; win.reserve(n);
    std::vector<std::string> tokenIds; tokenIds.reserve(n);   // parallel to feed order
    for (int i = 0; i < n; ++i)
    {
        win.push_back({ static_cast<uint64_t>(i), std::string(PQgetvalue(r, i, 0), PQgetlength(r, i, 0)) });
        tokenIds.emplace_back(PQgetvalue(r, i, 1), PQgetlength(r, i, 1));
    }
    PQclear(r);
    PQfinish(c);

    std::printf("warm-set rows read: %d  (%s)\n", n, all ? "ALL" : ("slice=" + std::to_string(sliceSize)).c_str());

    PackResult packed = PackWindow(win);

    if (subdir)
    {
        std::remove((std::string(outPath) + "/data.mdb").c_str());
        std::remove((std::string(outPath) + "/lock.mdb").c_str());
    }
    else
    {
        std::remove(outPath);
        std::remove((std::string(outPath) + "-lock").c_str());
    }
    if (!WriteStore(outPath, packed, subdir)) { std::fprintf(stderr, "WriteStore failed\n"); return 2; }

    // ---- verify against the warm set ----
    int fail = 0;
    int subdbs = CountNamedSubDbs(outPath, subdir);
    if (subdbs != 1) { std::printf("FAIL: store has %d sub-dbs (want 1)\n", subdbs); ++fail; }

    PackResult rb = ReadStore(outPath, subdir);
    if (rb.count != packed.count) { std::printf("FAIL: readback count %u != %u\n", rb.count, packed.count); ++fail; }
    if (!rb.ledger.empty()) { std::printf("FAIL: store carries canonical ids\n"); ++fail; }

    // round-trip chars + identity-by-position on real data (spot-check every entry)
    size_t storedBytes = 0; for (const auto& b : rb.buckets) storedBytes += b.blob.size();
    size_t charBytes   = 0; for (const auto& e : win) charBytes += e.chars.size();
    bool chars = true, ident = true;
    for (uint32_t cid = 0; cid < packed.count; ++cid)
    {
        if (CharsForCompactId(packed, cid) != CharsForCompactId(rb, cid)) chars = false;
        // compact-id -> feed-index (ledger) -> real token_id; chars must match that row's word
        uint64_t feedIdx = packed.ledger[cid];
        if (CharsForCompactId(rb, cid) != win[feedIdx].chars) ident = false;
        (void)tokenIds;  // tokenIds[feedIdx] is the canonical id the engine reattaches in RAM
    }
    if (!chars) { std::printf("FAIL: chars round-trip mismatch\n"); ++fail; }
    if (!ident) { std::printf("FAIL: identity-by-position mismatch\n"); ++fail; }
    if (storedBytes != charBytes) { std::printf("FAIL: stored %zu != char bytes %zu\n", storedBytes, charBytes); ++fail; }

    std::printf("entries packed:     %u across %zu length buckets\n", packed.count, packed.buckets.size());
    std::printf("store payload:      %zu bytes (chars only)\n", storedBytes);
    std::printf("sample (compact-id -> chars : canonical token_id):\n");
    for (uint32_t cid = 0; cid < packed.count && cid < 5; ++cid)
        std::printf("  %u -> \"%s\" : %s\n", cid, CharsForCompactId(rb, cid).c_str(),
                    tokenIds[packed.ledger[cid]].c_str());

    std::printf("\n%s (%d failures)  ->  %s\n", fail == 0 ? "OK" : "FAILED", fail, outPath);
    return fail == 0 ? 0 : 1;
}
