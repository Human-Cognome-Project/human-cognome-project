#include "HCPEnvelopeManager.h"

#include <lmdb.h>
#include <libpq-fe.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

namespace HCPEngine
{
    bool HCPEnvelopeManager::Initialize(MDB_env* lmdbEnv, const char* coreConnStr)
    {
        if (!lmdbEnv || !coreConnStr) return false;

        m_lmdbEnv = lmdbEnv;
        m_coreConnStr = coreConnStr;

        // Open _manifest sub-db in LMDB for tracking loaded entries
        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
            return false;

        MDB_dbi dbi;
        if (mdb_dbi_open(txn, "_manifest", MDB_CREATE, &dbi) != 0)
        {
            mdb_txn_abort(txn);
            return false;
        }
        m_manifestDbi = dbi;
        m_manifestOpen = true;
        mdb_txn_commit(txn);

        // Connect to hcp_core for envelope definitions
        m_coreConn = PQconnectdb(coreConnStr);
        if (PQstatus(m_coreConn) != CONNECTION_OK)
        {
            fprintf(stderr, "[EnvelopeManager] Core DB connection failed: %s\n",
                PQerrorMessage(m_coreConn));
            fflush(stderr);
            PQfinish(m_coreConn);
            m_coreConn = nullptr;
            return false;
        }

        fprintf(stderr, "[EnvelopeManager] Initialized (manifest sub-db ready)\n");
        fflush(stderr);
        return true;
    }

    EnvelopeDefinition HCPEnvelopeManager::LoadEnvelope(const AZStd::string& name)
    {
        EnvelopeDefinition def;
        if (!m_coreConn) return def;

        // Load definition
        const char* params[] = { name.c_str() };
        PGresult* res = PQexecParams(m_coreConn,
            "SELECT id, name, description, active FROM envelope_definitions WHERE name = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
        {
            fprintf(stderr, "[EnvelopeManager] Envelope '%s' not found\n", name.c_str());
            fflush(stderr);
            PQclear(res);
            return def;
        }

        def.id = atoi(PQgetvalue(res, 0, 0));
        def.name = PQgetvalue(res, 0, 1);
        def.description = PQgetvalue(res, 0, 2);
        def.active = (strcmp(PQgetvalue(res, 0, 3), "t") == 0);
        PQclear(res);

        // Load queries (priority-ordered)
        AZStd::string idStr = AZStd::string::format("%d",def.id);
        const char* qParams[] = { idStr.c_str() };
        res = PQexecParams(m_coreConn,
            "SELECT id, shard_db, query_text, description, priority, lmdb_subdb "
            "FROM envelope_queries WHERE envelope_id = $1 ORDER BY priority ASC",
            1, nullptr, qParams, nullptr, nullptr, 0);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                EnvelopeQuery q;
                q.id = atoi(PQgetvalue(res, i, 0));
                q.shardDb = PQgetvalue(res, i, 1);
                q.queryText = PQgetvalue(res, i, 2);
                q.description = PQgetvalue(res, i, 3);
                q.priority = atoi(PQgetvalue(res, i, 4));
                q.lmdbSubdb = PQgetvalue(res, i, 5);
                def.queries.push_back(q);
            }
        }
        PQclear(res);

        // Load composed children
        res = PQexecParams(m_coreConn,
            "SELECT child_id FROM envelope_includes WHERE parent_id = $1 ORDER BY priority ASC",
            1, nullptr, qParams, nullptr, nullptr, 0);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                def.childEnvelopeIds.push_back(atoi(PQgetvalue(res, i, 0)));
            }
        }
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] Loaded envelope '%s': %zu queries, %zu children\n",
            name.c_str(), def.queries.size(), def.childEnvelopeIds.size());
        fflush(stderr);

        return def;
    }

    EnvelopeActivation HCPEnvelopeManager::ActivateEnvelope(const AZStd::string& name)
    {
        EnvelopeActivation activation;
        auto t0 = std::chrono::high_resolution_clock::now();

        // Evict previous envelope
        if (!m_activeEnvelope.empty() && m_activeEnvelope != name)
        {
            activation.evictedEntries = EvictManifest(m_activeEnvelope);
            fprintf(stderr, "[EnvelopeManager] Evicted '%s' (%d entries)\n",
                m_activeEnvelope.c_str(), activation.evictedEntries);
            fflush(stderr);
        }

        // Load definition
        EnvelopeDefinition def = LoadEnvelope(name);
        if (def.id == 0)
        {
            fprintf(stderr, "[EnvelopeManager] Cannot activate — envelope not found\n");
            fflush(stderr);
            return activation;
        }
        activation.envelopeId = def.id;

        // Resolve children: load their queries first (depth-first)
        AZStd::vector<EnvelopeQuery> allQueries;
        for (int childId : def.childEnvelopeIds)
        {
            // Load child by ID
            AZStd::string childIdStr = AZStd::string::format("%d",childId);
            const char* params[] = { childIdStr.c_str() };
            PGresult* res = PQexecParams(m_coreConn,
                "SELECT name FROM envelope_definitions WHERE id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                AZStd::string childName = PQgetvalue(res, 0, 0);
                PQclear(res);
                EnvelopeDefinition childDef = LoadEnvelope(childName);
                for (auto& q : childDef.queries)
                    allQueries.push_back(q);
            }
            else
            {
                PQclear(res);
            }
        }

        // Add own queries after children
        for (auto& q : def.queries)
            allQueries.push_back(q);

        // Execute all queries in order (kernel chain, single transaction per shard)
        int totalLoaded = 0;
        for (const auto& q : allQueries)
        {
            int loaded = ExecuteQuery(q);
            totalLoaded += loaded;
            RecordManifest(name, q.lmdbSubdb, loaded);

            fprintf(stderr, "[EnvelopeManager]   Query '%s' -> %s: %d entries\n",
                q.description.c_str(), q.lmdbSubdb.c_str(), loaded);
            fflush(stderr);
        }

        activation.entriesLoaded = totalLoaded;

        auto t1 = std::chrono::high_resolution_clock::now();
        activation.loadTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        m_activeEnvelope = name;

        // Mark active in DB
        {
            const char* params[] = { name.c_str() };
            PQexecParams(m_coreConn,
                "UPDATE envelope_definitions SET active = true WHERE name = $1",
                1, nullptr, params, nullptr, nullptr, 0);
        }

        // Log to audit table
        LogActivation(activation, name);

        fprintf(stderr, "[EnvelopeManager] Activated '%s': %d entries in %.1f ms\n",
            name.c_str(), totalLoaded, activation.loadTimeMs);
        fflush(stderr);

        return activation;
    }

    void HCPEnvelopeManager::DeactivateEnvelope(const AZStd::string& name)
    {
        int evicted = EvictManifest(name);

        if (m_activeEnvelope == name)
            m_activeEnvelope.clear();

        // Mark inactive in DB
        if (m_coreConn)
        {
            const char* params[] = { name.c_str() };
            PQexecParams(m_coreConn,
                "UPDATE envelope_definitions SET active = false WHERE name = $1",
                1, nullptr, params, nullptr, nullptr, 0);

            // Log deactivation
            PQexecParams(m_coreConn,
                "UPDATE envelope_activations SET deactivated_at = now() "
                "WHERE envelope_id = (SELECT id FROM envelope_definitions WHERE name = $1) "
                "AND deactivated_at IS NULL",
                1, nullptr, params, nullptr, nullptr, 0);
        }

        fprintf(stderr, "[EnvelopeManager] Deactivated '%s' (%d entries evicted)\n",
            name.c_str(), evicted);
        fflush(stderr);
    }

    void HCPEnvelopeManager::Prefetch(const AZStd::string& envelopeName, int depth)
    {
        EnvelopeDefinition def = LoadEnvelope(envelopeName);
        if (def.id == 0) return;

        int count = 0;
        for (const auto& q : def.queries)
        {
            if (count >= depth) break;
            int loaded = ExecuteQuery(q);
            RecordManifest(envelopeName, q.lmdbSubdb, loaded);
            ++count;

            fprintf(stderr, "[EnvelopeManager] Prefetch '%s' query %d: %d entries\n",
                envelopeName.c_str(), count, loaded);
            fflush(stderr);
        }
    }

    int HCPEnvelopeManager::ExecuteQuery(const EnvelopeQuery& query)
    {
        PGconn* conn = GetShardConnection(query.shardDb);
        if (!conn) return 0;

        // Execute the stored query
        PGresult* res = PQexec(conn, query.queryText.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[EnvelopeManager] Query failed on %s: %s\n",
                query.shardDb.c_str(), PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            return 0;
        }

        int nrows = PQntuples(res);
        if (nrows == 0)
        {
            PQclear(res);
            return 0;
        }

        // Write results to target LMDB sub-db
        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
        {
            PQclear(res);
            return 0;
        }

        MDB_dbi dbi;
        if (mdb_dbi_open(txn, query.lmdbSubdb.c_str(), MDB_CREATE, &dbi) != 0)
        {
            mdb_txn_abort(txn);
            PQclear(res);
            return 0;
        }

        int written = 0;
        int nfields = PQnfields(res);

        for (int i = 0; i < nrows; ++i)
        {
            // First column = key (word), second column = value (token_id)
            if (nfields < 2) continue;

            const char* key = PQgetvalue(res, i, 0);
            const char* val = PQgetvalue(res, i, 1);
            int keyLen = PQgetlength(res, i, 0);
            int valLen = PQgetlength(res, i, 1);

            MDB_val mdbKey = { static_cast<size_t>(keyLen), const_cast<char*>(key) };
            MDB_val mdbVal = { static_cast<size_t>(valLen), const_cast<char*>(val) };

            if (mdb_put(txn, dbi, &mdbKey, &mdbVal, 0) == 0)
                ++written;
        }

        mdb_txn_commit(txn);
        PQclear(res);

        return written;
    }

    int HCPEnvelopeManager::EvictManifest(const AZStd::string& envelopeName)
    {
        if (!m_manifestOpen || !m_lmdbEnv) return 0;

        // Read manifest entries for this envelope
        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
            return 0;

        // Look up manifest key: "env:<name>" → list of "subdb:count" entries
        AZStd::string manifestKey = "env:" + envelopeName;
        MDB_val mdbKey = { manifestKey.size(), const_cast<char*>(manifestKey.c_str()) };
        MDB_val mdbVal;

        int evicted = 0;
        if (mdb_get(txn, m_manifestDbi, &mdbKey, &mdbVal) == 0)
        {
            // Parse: "subdb1\nsubdb2\n..." — drop entire sub-dbs
            AZStd::string manifest(static_cast<const char*>(mdbVal.mv_data), mdbVal.mv_size);
            size_t pos = 0;
            while (pos < manifest.size())
            {
                size_t nl = manifest.find('\n', pos);
                if (nl == AZStd::string::npos) nl = manifest.size();
                AZStd::string subdb = manifest.substr(pos, nl - pos);
                pos = nl + 1;

                if (!subdb.empty())
                {
                    MDB_dbi dbi;
                    if (mdb_dbi_open(txn, subdb.c_str(), 0, &dbi) == 0)
                    {
                        // Drop all entries (empty the sub-db, don't delete it)
                        mdb_drop(txn, dbi, 0);
                        ++evicted;
                    }
                }
            }

            // Remove manifest entry
            mdb_del(txn, m_manifestDbi, &mdbKey, nullptr);
        }

        mdb_txn_commit(txn);
        return evicted;
    }

    void HCPEnvelopeManager::RecordManifest(
        const AZStd::string& envelopeName,
        const AZStd::string& subdb,
        int entryCount)
    {
        if (!m_manifestOpen || !m_lmdbEnv || entryCount == 0) return;

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
            return;

        AZStd::string manifestKey = "env:" + envelopeName;
        MDB_val mdbKey = { manifestKey.size(), const_cast<char*>(manifestKey.c_str()) };
        MDB_val mdbVal;

        // Read existing manifest (append)
        AZStd::string manifest;
        if (mdb_get(txn, m_manifestDbi, &mdbKey, &mdbVal) == 0)
        {
            manifest = AZStd::string(static_cast<const char*>(mdbVal.mv_data), mdbVal.mv_size);
        }

        // Append sub-db name if not already present
        if (manifest.find(subdb) == AZStd::string::npos)
        {
            manifest += subdb + "\n";
        }

        MDB_val newVal = { manifest.size(), const_cast<char*>(manifest.c_str()) };
        mdb_put(txn, m_manifestDbi, &mdbKey, &newVal, 0);
        mdb_txn_commit(txn);
    }

    void HCPEnvelopeManager::LogActivation(
        const EnvelopeActivation& activation,
        const AZStd::string& envelopeName)
    {
        if (!m_coreConn) return;

        AZStd::string envIdStr = AZStd::string::format("%d",activation.envelopeId);
        AZStd::string loadedStr = AZStd::string::format("%d",activation.entriesLoaded);
        AZStd::string timeStr = AZStd::string::format("%.1f",activation.loadTimeMs);
        AZStd::string evictedStr = AZStd::string::format("%d",activation.evictedEntries);

        const char* params[] = {
            envIdStr.c_str(), loadedStr.c_str(),
            timeStr.c_str(), evictedStr.c_str()
        };
        PGresult* res = PQexecParams(m_coreConn,
            "INSERT INTO envelope_activations (envelope_id, entries_loaded, load_time_ms, evicted_entries) "
            "VALUES ($1::integer, $2::integer, $3::double precision, $4::integer)",
            4, nullptr, params, nullptr, nullptr, 0);
        PQclear(res);
    }

    PGconn* HCPEnvelopeManager::GetShardConnection(const AZStd::string& shardDb)
    {
        auto it = m_shardConns.find(shardDb);
        if (it != m_shardConns.end() && PQstatus(it->second) == CONNECTION_OK)
            return it->second;

        // Build connection string: same host/user/password, different dbname
        // Parse base conn string to extract host/user/password
        AZStd::string connStr = "host=localhost dbname=" + shardDb + " user=hcp password=hcp_dev";

        PGconn* conn = PQconnectdb(connStr.c_str());
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[EnvelopeManager] Failed to connect to shard '%s': %s\n",
                shardDb.c_str(), PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return nullptr;
        }

        m_shardConns[shardDb] = conn;
        return conn;
    }

    void HCPEnvelopeManager::Shutdown()
    {
        for (auto& [_, conn] : m_shardConns)
        {
            if (conn) PQfinish(conn);
        }
        m_shardConns.clear();

        if (m_coreConn)
        {
            PQfinish(m_coreConn);
            m_coreConn = nullptr;
        }

        m_activeEnvelope.clear();
        m_manifestOpen = false;
    }

} // namespace HCPEngine
