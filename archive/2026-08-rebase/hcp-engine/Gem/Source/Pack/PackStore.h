#pragma once
// ============================================================================
// HCP Pack slice — the GPU-facing LMDB store (compact-id -> chars, NOTHING else).
//
// This is the corrected vocab.lmdb shape. Contrast with the drifted file it
// replaces, which carried w2t/c2t/t2c/t2w/l2t/forward/entities/_manifest/vbed_*
// and stored a 14-byte token-id STRING + morpheme byte per entry. All of that
// is CPU-side bookkeeping or superseded morph-bit design (claim 56). The GPU
// reads ONE sub-db: per-length fixed-stride char blobs, addressed by compact id.
//
//   * No canonical/large ids in the store  (606: identity is CPU-side ledger)
//   * No morphology                        (56/533: lives in the definition)
//   * No reverse maps / indexes            (91: engine-shaped, not relational)
//   * count is DERIVED (blob/length)       (91: caps derived, not stored)
// ============================================================================
#include "PackKernel.h"
#include <lmdb.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace HCP {
namespace Pack {

// The single sub-db name. Exactly one sub-db exists in a correct store.
inline const char* CharsDbName() { return "chars"; }

// Open flag for the store. NOSUBDIR (single file, path + path-lock) is simplest
// for tests; the live engine opens subdir mode (a directory holding data.mdb +
// lock.mdb), so the producer can emit a drop-in store with subdir=true.
inline unsigned int StoreEnvFlags(bool subdir) { return subdir ? 0u : MDB_NOSUBDIR; }

// Write the GPU-facing store: one MDB_INTEGERKEY sub-db "chars", keyed by word
// length -> contiguous fixed-stride blob. The ledger is intentionally NOT
// written — canonical identity stays CPU-side.
inline bool WriteStore(const char* path, const PackResult& r,
                       bool subdir = false,
                       size_t mapSizeBytes = static_cast<size_t>(256) * 1024 * 1024)
{
    MDB_env* env = nullptr;
    if (mdb_env_create(&env) != 0) return false;
    mdb_env_set_mapsize(env, mapSizeBytes);
    mdb_env_set_maxdbs(env, 1);   // bounded by design: a correct store has ONE sub-db

    if (mdb_env_open(env, path, StoreEnvFlags(subdir), 0644) != 0) { mdb_env_close(env); return false; }

    MDB_txn* txn = nullptr;
    if (mdb_txn_begin(env, nullptr, 0, &txn) != 0) { mdb_env_close(env); return false; }

    MDB_dbi dbi;
    if (mdb_dbi_open(txn, CharsDbName(), MDB_CREATE | MDB_INTEGERKEY, &dbi) != 0)
    { mdb_txn_abort(txn); mdb_env_close(env); return false; }

    bool ok = true;
    for (const auto& b : r.buckets)
    {
        uint32_t key = b.length;
        MDB_val k{ sizeof(key), &key };
        MDB_val v{ b.blob.size(), const_cast<char*>(b.blob.data()) };
        if (mdb_put(txn, dbi, &k, &v, 0) != 0) { ok = false; break; }
    }

    if (ok && mdb_txn_commit(txn) != 0) ok = false;
    else if (!ok) mdb_txn_abort(txn);
    mdb_env_close(env);
    return ok;
}

// Read the store back. Reconstructs the per-length buckets and re-derives the
// identical compact-id assignment (lengths ascending == write order). The
// returned PackResult has an EMPTY ledger by construction: the store carries no
// canonical identity — that is the point.
inline PackResult ReadStore(const char* path, bool subdir = false)
{
    PackResult r;
    MDB_env* env = nullptr;
    if (mdb_env_create(&env) != 0) return r;
    mdb_env_set_maxdbs(env, 1);
    if (mdb_env_open(env, path, StoreEnvFlags(subdir) | MDB_RDONLY, 0644) != 0) { mdb_env_close(env); return r; }

    MDB_txn* txn = nullptr;
    if (mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) != 0) { mdb_env_close(env); return r; }

    MDB_dbi dbi;
    if (mdb_dbi_open(txn, CharsDbName(), MDB_INTEGERKEY, &dbi) != 0)
    { mdb_txn_abort(txn); mdb_env_close(env); return r; }

    MDB_cursor* cur = nullptr;
    if (mdb_cursor_open(txn, dbi, &cur) != 0)
    { mdb_txn_abort(txn); mdb_env_close(env); return r; }

    MDB_val k, v;
    uint32_t compactId = 0;
    while (mdb_cursor_get(cur, &k, &v, MDB_NEXT) == 0)   // MDB_INTEGERKEY => ascending length
    {
        uint32_t len = 0;
        memcpy(&len, k.mv_data, sizeof(len));
        if (len == 0) continue;

        LengthBucket b;
        b.length        = len;
        b.baseCompactId = compactId;
        b.blob.assign(static_cast<const char*>(v.mv_data), v.mv_size);
        b.count         = static_cast<uint32_t>(v.mv_size / len);   // DERIVED, not stored
        compactId      += b.count;
        r.buckets.push_back(std::move(b));
    }
    r.count = compactId;

    mdb_cursor_close(cur);
    mdb_txn_abort(txn);
    mdb_env_close(env);
    return r;
}

// Diagnostic: count named sub-dbs in the store. A correct store returns 1.
// (The unnamed/main DB holds one entry per named sub-db.)
inline int CountNamedSubDbs(const char* path, bool subdir = false)
{
    MDB_env* env = nullptr;
    if (mdb_env_create(&env) != 0) return -1;
    mdb_env_set_maxdbs(env, 64);
    if (mdb_env_open(env, path, StoreEnvFlags(subdir) | MDB_RDONLY, 0644) != 0) { mdb_env_close(env); return -1; }

    MDB_txn* txn = nullptr;
    if (mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) != 0) { mdb_env_close(env); return -1; }

    MDB_dbi main;
    if (mdb_dbi_open(txn, nullptr, 0, &main) != 0) { mdb_txn_abort(txn); mdb_env_close(env); return -1; }

    MDB_stat st;
    mdb_stat(txn, main, &st);
    int n = static_cast<int>(st.ms_entries);

    mdb_txn_abort(txn);
    mdb_env_close(env);
    return n;
}

} // namespace Pack
} // namespace HCP
