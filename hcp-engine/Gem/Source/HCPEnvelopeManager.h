#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

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
        AZStd::string shardDb;     // Target Postgres DB name
        AZStd::string queryText;   // SQL to execute
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

    //! Manages the envelope→LMDB cache lifecycle.
    //!
    //! Mechanism only — provides plumbing for:
    //!   - Loading envelope definitions from Postgres
    //!   - Executing envelope queries (kernel chain) into LMDB
    //!   - Evicting previous envelope data
    //!   - Prefetching ahead in query chain
    //!
    //! NAPIER decides policy (what goes in which envelope, when to activate).
    class HCPEnvelopeManager
    {
    public:
        //! Initialize with LMDB env and Postgres connection string.
        //! Sets up the _manifest sub-db for tracking loaded keys.
        bool Initialize(MDB_env* lmdbEnv, const char* coreConnStr);

        //! Load envelope definition from Postgres by name.
        //! Resolves composed children recursively.
        EnvelopeDefinition LoadEnvelope(const AZStd::string& name);

        //! Activate an envelope: execute its queries, write results to LMDB.
        //! Evicts previous envelope first. Returns activation stats.
        EnvelopeActivation ActivateEnvelope(const AZStd::string& name);

        //! Deactivate: evict entries loaded by this envelope from LMDB.
        void DeactivateEnvelope(const AZStd::string& name);

        //! Prefetch: load the next N queries ahead of current progress.
        //! For pipeline parallelism — engine never stalls on cold read.
        //! @param depth Number of queries to prefetch (default 2)
        void Prefetch(const AZStd::string& envelopeName, int depth = 2);

        //! Get the currently active envelope name (empty if none).
        const AZStd::string& ActiveEnvelope() const { return m_activeEnvelope; }

        void Shutdown();

    private:
        //! Execute a single envelope query and write results to LMDB.
        //! Returns number of entries written.
        int ExecuteQuery(const EnvelopeQuery& query);

        //! Evict all entries tracked in _manifest for a given envelope.
        int EvictManifest(const AZStd::string& envelopeName);

        //! Record loaded keys in _manifest for clean eviction.
        void RecordManifest(const AZStd::string& envelopeName,
                            const AZStd::string& subdb,
                            int entryCount);

        //! Log activation to Postgres audit table.
        void LogActivation(const EnvelopeActivation& activation,
                           const AZStd::string& envelopeName);

        //! Get a Postgres connection for a shard DB.
        PGconn* GetShardConnection(const AZStd::string& shardDb);

        MDB_env* m_lmdbEnv = nullptr;
        AZStd::string m_coreConnStr;
        PGconn* m_coreConn = nullptr;

        // Per-shard Postgres connections (lazy-opened)
        AZStd::unordered_map<AZStd::string, PGconn*> m_shardConns;

        // LMDB _manifest sub-db handle
        unsigned int m_manifestDbi = 0;
        bool m_manifestOpen = false;

        AZStd::string m_activeEnvelope;
    };

} // namespace HCPEngine
