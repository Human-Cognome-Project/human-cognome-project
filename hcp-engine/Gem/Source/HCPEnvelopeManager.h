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

    //! In-memory vocab entry returned by QueryEnvelopeEntries.
    //! Used for mid-resolve tense pre-fetch injection into BedManager.
    struct VocabEntry
    {
        AZStd::string word;
        AZStd::string tokenId;
    };

    //! Manages the three-layer cache lifecycle:
    //!
    //!   Cold (hcp_english/hcp_core) → Warm (hcp_var.envelope_working_set) → Hot (LMDB)
    //!
    //! Assembly (cold → warm) happens ONCE per activation. After that, the warm db
    //! is the working layer — adjustments (feedback, priority_delta) happen there.
    //! LMDB is populated from warm in ordered slices and evicted on envelope change.
    //!
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

        //! Activate an envelope:
        //!   1. Evict previous envelope from LMDB + clear warm working set
        //!   2. Assemble cold → warm (one-shot: run queries against cold shard,
        //!      write enriched rows into hcp_var.envelope_working_set)
        //!   3. Flush warm → LMDB (ordered by effective_priority)
        //! Returns activation stats.
        EnvelopeActivation ActivateEnvelope(const AZStd::string& name);

        //! Deactivate: evict LMDB entries and clear warm working set for this envelope.
        void DeactivateEnvelope(const AZStd::string& name);

        //! Prefetch: assemble + flush the next N queries for a not-yet-active envelope.
        //! No-op if envelope is already active (already fully assembled).
        void Prefetch(const AZStd::string& envelopeName, int depth = 2);

        //! Return entries for an envelope from the warm db (hcp_var.envelope_working_set).
        //! Does NOT query the cold shard — warm must be assembled first.
        //! Used for mid-resolve pre-fetch injection into BedManager.
        AZStd::vector<VocabEntry> QueryEnvelopeEntries(const AZStd::string& envelopeName);

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
        //! Returns number of rows assembled.
        int AssembleQuery(const EnvelopeQuery& query, int envelopeId, int startOffset);

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
    };

} // namespace HCPEngine
