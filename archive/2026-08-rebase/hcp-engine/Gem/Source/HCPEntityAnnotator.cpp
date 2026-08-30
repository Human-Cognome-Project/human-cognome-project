#include "HCPEntityAnnotator.h"

#include <AzCore/Console/ILogger.h>
#include <lmdb.h>
#include <cstring>
#include <algorithm>

namespace HCPEngine
{
    bool EntityAnnotator::LoadSubDb(
        MDB_env* env, const char* dbName, const char* metaDbName,
        AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>>& out,
        size_t& totalOut)
    {
        MDB_txn* txn = nullptr;
        int rc = mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);
        if (rc != 0)
        {
            AZLOG_ERROR("EntityAnnotator: mdb_txn_begin failed: %s", mdb_strerror(rc));
            return false;
        }

        // Open data sub-db
        MDB_dbi dataDbi;
        rc = mdb_dbi_open(txn, dbName, 0, &dataDbi);
        if (rc != 0)
        {
            fprintf(stderr, "[EntityAnnotator] Sub-db '%s' not found (rc=%d) — skipping\n",
                dbName, rc);
            fflush(stderr);
            mdb_txn_abort(txn);
            return true;  // Not an error — entity data is optional
        }

        // Open metadata sub-db
        MDB_dbi metaDbi;
        rc = mdb_dbi_open(txn, metaDbName, 0, &metaDbi);
        if (rc != 0)
        {
            fprintf(stderr, "[EntityAnnotator] Metadata '%s' not found — skipping\n", metaDbName);
            fflush(stderr);
            mdb_txn_abort(txn);
            return true;
        }

        // Read metadata
        MDB_val key, val;
        key.mv_data = const_cast<char*>("meta");
        key.mv_size = 4;
        rc = mdb_get(txn, metaDbi, &key, &val);
        if (rc != 0 || val.mv_size < 16)
        {
            fprintf(stderr, "[EntityAnnotator] Could not read metadata from '%s'\n", metaDbName);
            fflush(stderr);
            mdb_txn_abort(txn);
            return true;
        }

        struct EntityMeta { uint32_t totalEntries, maxTokenCount, distinctEntities, distinctStarters; };
        EntityMeta meta;
        memcpy(&meta, val.mv_data, sizeof(EntityMeta));

        // Read data buffer
        key.mv_data = const_cast<char*>("data");
        key.mv_size = 4;
        rc = mdb_get(txn, dataDbi, &key, &val);
        if (rc != 0)
        {
            fprintf(stderr, "[EntityAnnotator] Could not read data from '%s'\n", dbName);
            fflush(stderr);
            mdb_txn_abort(txn);
            return true;
        }

        const uint8_t* buf = static_cast<const uint8_t*>(val.mv_data);
        size_t bufSize = val.mv_size;

        if (bufSize < meta.totalEntries * ENTITY_ENTRY_SIZE)
        {
            fprintf(stderr, "[EntityAnnotator] Buffer too small in '%s': %zu < %u\n",
                dbName, bufSize, meta.totalEntries * ENTITY_ENTRY_SIZE);
            fflush(stderr);
            mdb_txn_abort(txn);
            return false;
        }

        // Parse entries into starter map
        for (uint32_t i = 0; i < meta.totalEntries; ++i)
        {
            const uint8_t* entry = buf + i * ENTITY_ENTRY_SIZE;

            // starter_token_id (14 bytes at offset 0)
            const char* starterPtr = reinterpret_cast<const char*>(entry);
            AZ::u32 starterLen = ENTITY_TOKEN_ID_WIDTH;
            while (starterLen > 0 && starterPtr[starterLen - 1] == '\0') --starterLen;
            AZStd::string starter(starterPtr, starterLen);

            // entity_id (14 bytes at offset 14)
            const char* eidPtr = reinterpret_cast<const char*>(entry + 14);
            AZ::u32 eidLen = ENTITY_TOKEN_ID_WIDTH;
            while (eidLen > 0 && eidPtr[eidLen - 1] == '\0') --eidLen;

            EntitySequence seq;
            seq.entityId = AZStd::string(eidPtr, eidLen);
            seq.nameGroup = entry[28];
            seq.nameType = entry[29];
            uint8_t tokenCount = entry[30];

            // token_ids (14 bytes each, starting at offset 32)
            for (uint8_t j = 0; j < tokenCount && j < ENTITY_MAX_NAME_TOKENS; ++j)
            {
                const char* tidPtr = reinterpret_cast<const char*>(entry + 32 + j * ENTITY_TOKEN_ID_WIDTH);
                AZ::u32 tidLen = ENTITY_TOKEN_ID_WIDTH;
                while (tidLen > 0 && tidPtr[tidLen - 1] == '\0') --tidLen;
                seq.tokenIds.push_back(AZStd::string(tidPtr, tidLen));
            }

            out[starter].push_back(AZStd::move(seq));
        }

        totalOut = meta.totalEntries;
        mdb_txn_commit(txn);
        return true;
    }


    bool EntityAnnotator::Initialize(MDB_env* lmdbEnv)
    {
        if (!lmdbEnv)
        {
            AZLOG_ERROR("EntityAnnotator: null LMDB env");
            return false;
        }

        bool ok = true;
        ok = LoadSubDb(lmdbEnv, "entities_fic", "entities_fic_meta", m_ficStarters, m_ficTotal) && ok;
        ok = LoadSubDb(lmdbEnv, "entities_nf", "entities_nf_meta", m_nfStarters, m_nfTotal) && ok;

        m_initialized = (m_ficTotal > 0 || m_nfTotal > 0);

        if (m_initialized)
        {
            fprintf(stderr, "[EntityAnnotator] Loaded: %zu fic sequences (%zu starters), "
                "%zu nf sequences (%zu starters)\n",
                m_ficTotal, m_ficStarters.size(), m_nfTotal, m_nfStarters.size());
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "[EntityAnnotator] No entity data found in LMDB\n");
            fflush(stderr);
        }

        return ok;
    }


    AZ::u32 EntityAnnotator::TryMatch(
        const AZStd::vector<ResolutionResult>& results,
        size_t i,
        const AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>>& starters) const
    {
        const auto& r = results[i];
        auto it = starters.find(r.matchedTokenId);
        if (it == starters.end())
            return 0;

        // Sequences are sorted longest first — greedy match
        for (const auto& seq : it->second)
        {
            AZ::u32 seqLen = static_cast<AZ::u32>(seq.tokenIds.size());
            if (i + seqLen > results.size())
                continue;

            bool match = true;
            for (AZ::u32 j = 0; j < seqLen; ++j)
            {
                const auto& rj = results[i + j];
                if (!rj.resolved)
                {
                    match = false;
                    break;
                }
                if (rj.matchedTokenId != seq.tokenIds[j])
                {
                    match = false;
                    break;
                }
                // All tokens in entity name should be capitalized
                if (j > 0 && !rj.firstCap && !rj.allCaps)
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                // Annotate all tokens in the matched span
                for (AZ::u32 j = 0; j < seqLen; ++j)
                {
                    // const_cast safe here — we own the manifest
                    auto& rj = const_cast<ResolutionResult&>(results[i + j]);
                    rj.entityId = seq.entityId;
                    rj.entityNameGroup = seq.nameGroup;
                }
                return seqLen;
            }
        }
        return 0;
    }


    void EntityAnnotator::AnnotateManifest(ResolutionManifest& manifest, bool fictionFirst) const
    {
        if (!m_initialized || manifest.results.empty())
            return;

        auto& results = manifest.results;
        size_t annotated = 0;

        // First pass: primary corpus
        const auto& primary = fictionFirst ? m_ficStarters : m_nfStarters;
        const auto& secondary = fictionFirst ? m_nfStarters : m_ficStarters;

        for (size_t i = 0; i < results.size(); )
        {
            const auto& r = results[i];

            // Only check resolved, capitalized tokens as entity starters
            if (!r.resolved || (!r.firstCap && !r.allCaps))
            {
                ++i;
                continue;
            }

            // Already annotated (by an earlier match)
            if (!r.entityId.empty())
            {
                ++i;
                continue;
            }

            AZ::u32 consumed = TryMatch(results, i, primary);
            if (consumed > 0)
            {
                annotated += consumed;
                i += consumed;
                continue;
            }

            ++i;
        }

        // Second pass: secondary corpus (for unmatched capitalized tokens)
        if (!secondary.empty())
        {
            for (size_t i = 0; i < results.size(); )
            {
                const auto& r = results[i];
                if (!r.resolved || (!r.firstCap && !r.allCaps) || !r.entityId.empty())
                {
                    ++i;
                    continue;
                }

                AZ::u32 consumed = TryMatch(results, i, secondary);
                if (consumed > 0)
                {
                    annotated += consumed;
                    i += consumed;
                    continue;
                }

                ++i;
            }
        }

        if (annotated > 0)
        {
            fprintf(stderr, "[EntityAnnotator] %zu tokens annotated with entity IDs\n", annotated);
            fflush(stderr);
        }
    }

} // namespace HCPEngine
