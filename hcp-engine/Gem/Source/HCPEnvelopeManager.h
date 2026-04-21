#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include "HCPResolutionChamber.h"  // InflectionRule

// Forward declarations
typedef struct pg_conn PGconn;
struct MDB_env;
typedef unsigned int MDB_dbi;

namespace HCPEngine
{
    class HCPVocabulary;

    //! Envelope query descriptor — mirrors envelope_queries table row.
    struct EnvelopeQuery
    {
        int id = 0;
        AZStd::string shardDb;     // Source Postgres DB name (cold shard)
        AZStd::string queryText;   // SQL to execute against cold shard
        AZStd::string description;
        int priority = 0;
        AZStd::string lmdbSubdb;   // Target LMDB sub-database
    };

    //! Envelope definition — mirrors envelope_definitions table row.
    struct EnvelopeDefinition
    {
        int id = 0;
        AZStd::string name;
        AZStd::string description;
        bool active = false;
        AZStd::vector<EnvelopeQuery> queries;   // Priority-ordered
        AZStd::vector<int> childEnvelopeIds;     // Composed children
    };

    //! Activation record for audit log.
    struct EnvelopeActivation
    {
        int envelopeId = 0;
        int entriesLoaded = 0;
        double loadTimeMs = 0.0;
        int evictedEntries = 0;
    };

    //! Map a morpheme name string (e.g. "PAST", "PLURAL") to the corresponding MorphBit.
    //! Returns 0 for morphemes not yet mapped to a bit.
    inline AZ::u16 MorphemeNameToBit(const char* morpheme) noexcept
    {
        if (!morpheme || !morpheme[0]) return 0;
        if (morpheme[0] == 'P')
        {
            if (morpheme[1] == 'A') return MorphBit::PAST;        // PAST
            if (morpheme[1] == 'L') return MorphBit::PLURAL;      // PLURAL
            if (morpheme[1] == 'R') return MorphBit::PROG;        // PROGRESSIVE
        }
        if (morpheme[0] == '3') return MorphBit::THIRD;           // 3RD_SING
        return 0;
    }

    //! In-memory vocab entry returned by QueryEnvelopeEntries / QueryWarmBatch.
    //! Used for mid-resolve tense pre-fetch injection into BedManager.
    struct VocabEntry
    {
        AZStd::string word;
        AZStd::string tokenId;
        AZStd::string morpheme;  // Morpheme name (e.g. "PAST", "PLURAL") or empty for canonical forms
    };

    //! Manages the three-layer cache lifecycle:
    //!
    //!   Cold (hcp_english/hcp_core) → Warm (hcp_var.envelope_working_set) → Hot (LMDB)
    //!
    //! Envelopes are composable query plans — each defines the set of queries needed
    //! to assemble its working vocabulary. Multiple envelopes can be active simultaneously
    //! (e.g. english_resolve + french_resolve). Working set rows are tagged by envelope_id.
    //!
    //! Assembly is demand-driven: activating an envelope loads the DEFINITION only.
    //! Actual cold→warm query execution happens per word-length when the resolution
    //! loop first requests candidates at that length. Short lengths are cheap (small
    //! candidate pools). Long lengths only assemble if the text needs them.
    //!
    //! LMDB is ephemeral hot cache — populated from warm on demand, always clearable.
    //! NAPIER decides policy (what goes in which envelope, when to activate).
    class HCPEnvelopeManager
    {
    public:
        //! Initialize with LMDB env and Postgres connection string for envelope definitions.
        //! hcp_var (warm layer) is connected lazily on first use.
        bool Initialize(MDB_env* lmdbEnv, const char* envelopeConnStr);

        //! Load envelope definition from Postgres by name.
        //! Resolves composed children recursively.
        EnvelopeDefinition LoadEnvelope(const AZStd::string& name);

        //! Activate an envelope: load definition, store query plan, clear LMDB hot cache.
        //! NO assembly — cold→warm queries are deferred to EnsureLengthAssembled().
        //! Multiple envelopes can be active; working set rows coexist by envelope_id.
        //! Returns activation stats (entriesLoaded = 0 for deferred assembly).
        EnvelopeActivation ActivateEnvelope(const AZStd::string& name);

        //! Ensure the warm working set has been assembled for a given word length.
        //! Checks for existing rows first (reuses data from previous sessions).
        //! If missing, runs all stored queries filtered to targetLength, inserts results.
        //! Called transparently from QueryWarmBatch — BedManager doesn't need to know.
        void EnsureLengthAssembled(int wordLength);

        //! Feed rows [offset, offset+count) from warm set into LMDB w2t (and t2w).
        //! Used by BedManager to advance the sliding hot-cache window.
        //! Returns number of entries written.
        int FeedSlice(int envelopeId, int offset, int count);

        //! Clear the w2t hot-cache sub-db (empty it, do not delete).
        //! Call before FeedSlice(envelopeId, 0, ...) to reset the window to the beginning
        //! of the warm set for a new document. t2w is intentionally left intact.
        void ClearHotWindow();

        //! Evict rows [offset, offset+count) from LMDB w2t.
        //! t2w is NOT evicted — reverse map accumulates across the document for reconstruction.
        void EvictSlice(int envelopeId, int offset, int count);

        //! Return total rows in warm set for this envelope.
        int GetWorkingSetSize(int envelopeId);

        //! Deactivate: evict LMDB entries and clear warm working set for this envelope.
        void DeactivateEnvelope(const AZStd::string& name);

        //! Prefetch: assemble + flush the next N queries for a not-yet-active envelope.
        //! No-op if envelope is already active (already fully assembled).
        void Prefetch(const AZStd::string& envelopeName, int depth = 2);

        //! Return entries for an envelope from the warm db (hcp_var.envelope_working_set).
        //! Does NOT query the cold shard — warm must be assembled first.
        //! Used for mid-resolve pre-fetch injection into BedManager.
        AZStd::vector<VocabEntry> QueryEnvelopeEntries(const AZStd::string& envelopeName);

        //! Query a single batch of vocab for one word length from the warm set.
        //! Uses per-length OFFSET so offsets stay small (max ~40K) instead of scanning
        //! through the global 535K ordering. Called from ResolveLengthCycle multi-slice loop.
        //! Returns entries in effective_priority order (highest-priority first, offset=0).
        AZStd::vector<VocabEntry> QueryWarmBatch(int envelopeId, int wordLength, int offset, int count);

        //! Query a priority-ordered phase from the warm set across ALL word lengths.
        //! Returns entries in effective_priority order (Labels first, then special chars,
        //! then common vocab ascending, etc.). Triggers incremental child-envelope
        //! assembly when the current warm set is exhausted.
        //! BedManager distributes results into m_vocabByLength by word length.
        AZStd::vector<VocabEntry> QueryWarmPhase(int envelopeId, int offset, int count);

        //! Load targeted p3 buckets from the shard directly into LMDB.
        //! Called after Phase 1 classification — the engine knows exactly which
        //! (first_letter, word_length) buckets it needs. Postgres queries only those
        //! buckets, writes results to LMDB w2t + t2w. No warm set needed.
        //! Labels (ns=AD) are loaded first, then common vocab (ns=AB).
        //! @param neededP3 Set of p3 values (2-char encoded first letter)
        //! @param neededLengths Set of word lengths present in the text
        //! @return Total entries written to LMDB
        int LoadBucketsToLmdb(const AZStd::vector<AZStd::string>& neededP3,
                              const AZStd::vector<int>& neededLengths);

        //! Load inflection rules from the named Postgres database.
        //! Queries inflection_rules table ordered by (morpheme, priority).
        //! dbName defaults to "hcp_english" but is flaggable for other language shards.
        AZStd::vector<InflectionRule> LoadInflectionRules(const char* dbName = "hcp_english");

        //! Get the currently active envelope name (empty if none).
        const AZStd::string& ActiveEnvelope() const { return m_activeEnvelope; }

        void Shutdown();

    private:
        //! Cold → warm: execute stored query against cold shard, write enriched rows
        //! into hcp_var.envelope_working_set. Wraps query with characteristics/category
        //! enrichment join. startOffset drives base_priority ordering.
        //! If targetLength > 0, filters base query results to that word length only.
        //! Returns number of rows assembled.
        int AssembleQuery(const EnvelopeQuery& query, int envelopeId, int startOffset,
                          int targetLength = 0);

        //! Warm → LMDB: read envelope_working_set for envelopeId ordered by
        //! effective_priority, write to LMDB sub-dbs. Also writes t2w reverse for w2t.
        //! Returns number of entries written.
        int FlushWorkingSetToLmdb(int envelopeId);

        //! Clear warm working set rows for a given envelope (before re-assembly or eviction).
        void ClearWorkingSet(int envelopeId);

        //! Evict all LMDB entries tracked in _manifest for a given envelope.
        int EvictManifest(const AZStd::string& envelopeName);

        //! Record loaded sub-dbs in _manifest for clean eviction.
        void RecordManifest(const AZStd::string& envelopeName,
                            const AZStd::string& subdb,
                            int entryCount);

        //! Log activation to Postgres audit table.
        void LogActivation(const EnvelopeActivation& activation,
                           const AZStd::string& envelopeName);

        //! Get (or lazily open) a Postgres connection for a named DB.
        PGconn* GetShardConnection(const AZStd::string& dbName);

        MDB_env*      m_lmdbEnv     = nullptr;
        AZStd::string m_coreConnStr;
        PGconn*       m_coreConn    = nullptr;

        // Per-DB Postgres connections (lazy-opened) — includes shards and hcp_var
        AZStd::unordered_map<AZStd::string, PGconn*> m_shardConns;

        // LMDB _manifest sub-db handle
        unsigned int m_manifestDbi  = 0;
        bool         m_manifestOpen = false;

        AZStd::string m_activeEnvelope;
        int           m_activeEnvelopeId = 0;

        // Stored query plan for the active envelope (all children + own, cascade order).
        // Populated by ActivateEnvelope, consumed by incremental assembly.
        AZStd::vector<EnvelopeQuery> m_activeQueries;

        // Track which word lengths have been assembled into the working set.
        // Avoids redundant DB checks once a length is confirmed present.
        AZStd::unordered_map<int, bool> m_assembledLengths;

        // Incremental child-envelope assembly state.
        // Each AdvancePhase assembles the next child's queries into the warm set.
        // m_childQueryRanges[i] = {startIdx, count} into m_activeQueries for child i.
        struct ChildQueryRange { int startIdx; int count; };
        AZStd::vector<ChildQueryRange> m_childQueryRanges;
        int m_nextChildToAssemble = 0;   // Next child index to assemble on demand
        int m_totalAssembled = 0;        // Total rows assembled so far (drives base_priority)
    };

} // namespace HCPEngine
