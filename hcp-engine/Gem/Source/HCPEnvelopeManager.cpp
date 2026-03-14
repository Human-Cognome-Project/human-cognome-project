#include "HCPEnvelopeManager.h"
#include "HCPResolutionChamber.h"  // MorphBit namespace

#include <lmdb.h>
#include <libpq-fe.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

namespace HCPEngine
{
    // ---------------------------------------------------------------------------
    // COPY-format escaping for bulk insert into hcp_var
    // ---------------------------------------------------------------------------
    static void AppendCopyField(std::string& line, const char* s, int len, bool addTab)
    {
        if (!s || len == 0)
        {
            line += "\\N";
        }
        else
        {
            for (int i = 0; i < len; ++i)
            {
                char c = s[i];
                if      (c == '\\') line += "\\\\";
                else if (c == '\t') line += "\\t";
                else if (c == '\n') line += "\\n";
                else if (c == '\r') line += "\\r";
                else                line += c;
            }
        }
        if (addTab) line += '\t';
    }

    // ---------------------------------------------------------------------------
    bool HCPEnvelopeManager::Initialize(MDB_env* lmdbEnv, const char* envelopeConnStr)
    {
        if (!lmdbEnv || !envelopeConnStr) return false;

        m_lmdbEnv     = lmdbEnv;
        m_coreConnStr = envelopeConnStr;

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
        m_manifestDbi  = dbi;
        m_manifestOpen = true;
        mdb_txn_commit(txn);

        // Connect to envelope definitions DB (hcp_core for now, hcp_envelope when ready)
        m_coreConn = PQconnectdb(envelopeConnStr);
        if (PQstatus(m_coreConn) != CONNECTION_OK)
        {
            fprintf(stderr, "[EnvelopeManager] Envelope DB connection failed: %s\n",
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

    // ---------------------------------------------------------------------------
    EnvelopeDefinition HCPEnvelopeManager::LoadEnvelope(const AZStd::string& name)
    {
        EnvelopeDefinition def;
        if (!m_coreConn) return def;

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

        def.id          = atoi(PQgetvalue(res, 0, 0));
        def.name        = PQgetvalue(res, 0, 1);
        def.description = PQgetvalue(res, 0, 2);
        def.active      = (strcmp(PQgetvalue(res, 0, 3), "t") == 0);
        PQclear(res);

        AZStd::string idStr = AZStd::string::format("%d", def.id);
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
                q.id          = atoi(PQgetvalue(res, i, 0));
                q.shardDb     = PQgetvalue(res, i, 1);
                q.queryText   = PQgetvalue(res, i, 2);
                q.description = PQgetvalue(res, i, 3);
                q.priority    = atoi(PQgetvalue(res, i, 4));
                q.lmdbSubdb   = PQgetvalue(res, i, 5);
                def.queries.push_back(q);
            }
        }
        PQclear(res);

        res = PQexecParams(m_coreConn,
            "SELECT child_id FROM envelope_includes WHERE parent_id = $1 ORDER BY priority ASC",
            1, nullptr, qParams, nullptr, nullptr, 0);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
                def.childEnvelopeIds.push_back(atoi(PQgetvalue(res, i, 0)));
        }
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] Loaded envelope '%s': %zu queries, %zu children\n",
            name.c_str(), def.queries.size(), def.childEnvelopeIds.size());
        fflush(stderr);

        return def;
    }

    // ---------------------------------------------------------------------------
    EnvelopeActivation HCPEnvelopeManager::ActivateEnvelope(const AZStd::string& name)
    {
        EnvelopeActivation activation;
        auto t0 = std::chrono::high_resolution_clock::now();

        // Always evict current envelope from LMDB before re-assembling so stale
        // entries never accumulate across activations of the same envelope.
        if (!m_activeEnvelope.empty())
        {
            activation.evictedEntries = EvictManifest(m_activeEnvelope);
            if (m_activeEnvelopeId != 0)
                ClearWorkingSet(m_activeEnvelopeId);
            fprintf(stderr, "[EnvelopeManager] Evicted '%s' (%d LMDB sub-dbs)\n",
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

        // Clear warm working set for this envelope before re-assembly
        ClearWorkingSet(def.id);

        // Collect all queries: children first (depth-first), then own
        AZStd::vector<EnvelopeQuery> allQueries;
        for (int childId : def.childEnvelopeIds)
        {
            AZStd::string childIdStr = AZStd::string::format("%d", childId);
            const char* params[]     = { childIdStr.c_str() };
            PGresult* res = PQexecParams(m_coreConn,
                "SELECT name FROM envelope_definitions WHERE id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                AZStd::string childName = PQgetvalue(res, 0, 0);
                PQclear(res);
                EnvelopeDefinition childDef = LoadEnvelope(childName);
                for (auto& q : childDef.queries) allQueries.push_back(q);
            }
            else PQclear(res);
        }
        for (auto& q : def.queries) allQueries.push_back(q);

        // Step 1: Assemble cold → warm (one-shot)
        // Each query's rows land in hcp_var.envelope_working_set, enriched and ordered.
        int rowOffset = 0;
        for (const auto& q : allQueries)
        {
            int assembled = AssembleQuery(q, def.id, rowOffset);
            rowOffset += assembled;
            fprintf(stderr, "[EnvelopeManager]   Assembled '%s': %d rows\n",
                q.description.c_str(), assembled);
            fflush(stderr);
        }
        fprintf(stderr, "[EnvelopeManager] Assembly complete: %d rows in warm db\n", rowOffset);
        fflush(stderr);

        // Clear hot cache — w2t and t2w are rebuilt from warm via FeedSlice().
        // Caller feeds the initial 3-slice window after activation completes.
        {
            MDB_txn* dropTxn = nullptr;
            if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &dropTxn) == 0)
            {
                MDB_dbi dbiW2t = 0, dbiT2w = 0;
                if (mdb_dbi_open(dropTxn, "w2t", 0, &dbiW2t) == 0) mdb_drop(dropTxn, dbiW2t, 0);
                if (mdb_dbi_open(dropTxn, "t2w", 0, &dbiT2w) == 0) mdb_drop(dropTxn, dbiT2w, 0);
                mdb_txn_commit(dropTxn);
            }
        }

        // entriesLoaded reflects warm assembly count; LMDB is populated via FeedSlice().
        activation.entriesLoaded = rowOffset;
        RecordManifest(name, "w2t", rowOffset);

        auto t1 = std::chrono::high_resolution_clock::now();
        activation.loadTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        m_activeEnvelope   = name;
        m_activeEnvelopeId = def.id;

        // Mark active in DB
        {
            const char* params[] = { name.c_str() };
            PQexecParams(m_coreConn,
                "UPDATE envelope_definitions SET active = true WHERE name = $1",
                1, nullptr, params, nullptr, nullptr, 0);
        }

        LogActivation(activation, name);

        fprintf(stderr, "[EnvelopeManager] Activated '%s': %d warm rows in %.1f ms\n",
            name.c_str(), rowOffset, activation.loadTimeMs);
        fflush(stderr);

        return activation;
    }

    // ---------------------------------------------------------------------------
    void HCPEnvelopeManager::DeactivateEnvelope(const AZStd::string& name)
    {
        int evicted = EvictManifest(name);

        if (m_activeEnvelope == name)
        {
            if (m_activeEnvelopeId != 0)
                ClearWorkingSet(m_activeEnvelopeId);
            m_activeEnvelope.clear();
            m_activeEnvelopeId = 0;
        }

        if (m_coreConn)
        {
            const char* params[] = { name.c_str() };
            PQexecParams(m_coreConn,
                "UPDATE envelope_definitions SET active = false WHERE name = $1",
                1, nullptr, params, nullptr, nullptr, 0);

            PQexecParams(m_coreConn,
                "UPDATE envelope_activations SET deactivated_at = now() "
                "WHERE envelope_id = (SELECT id FROM envelope_definitions WHERE name = $1) "
                "AND deactivated_at IS NULL",
                1, nullptr, params, nullptr, nullptr, 0);
        }

        fprintf(stderr, "[EnvelopeManager] Deactivated '%s' (%d LMDB sub-dbs evicted)\n",
            name.c_str(), evicted);
        fflush(stderr);
    }

    // ---------------------------------------------------------------------------
    void HCPEnvelopeManager::Prefetch(const AZStd::string& envelopeName, int depth)
    {
        // If already active and fully assembled, nothing to do
        if (m_activeEnvelope == envelopeName) return;

        EnvelopeDefinition def = LoadEnvelope(envelopeName);
        if (def.id == 0) return;

        int rowOffset = 0;
        int count     = 0;
        for (const auto& q : def.queries)
        {
            if (count >= depth) break;
            rowOffset += AssembleQuery(q, def.id, rowOffset);
            ++count;
        }

        fprintf(stderr, "[EnvelopeManager] Prefetch '%s' (%d queries, %d rows warm)\n",
            envelopeName.c_str(), count, rowOffset);
        fflush(stderr);
    }

    // ---------------------------------------------------------------------------
    // Cold → warm: run stored query against cold shard, insert enriched rows into
    // hcp_var.envelope_working_set. Uses COPY FROM STDIN for bulk efficiency.
    // Wraps stored query with a CTE to join back characteristics + category.
    // ---------------------------------------------------------------------------
    int HCPEnvelopeManager::AssembleQuery(
        const EnvelopeQuery& query, int envelopeId, int startOffset)
    {
        PGconn* shardConn = GetShardConnection(query.shardDb);
        PGconn* varConn   = GetShardConnection("hcp_var");
        if (!shardConn || !varConn) return 0;

        // Strip trailing semicolons/whitespace from stored query
        std::string baseQuery(query.queryText.c_str(), query.queryText.size());
        while (!baseQuery.empty() &&
               (baseQuery.back() == ';' || baseQuery.back() == ' ' ||
                baseQuery.back() == '\n' || baseQuery.back() == '\r'))
            baseQuery.pop_back();

        // Wrap with enrichment join to get characteristics + category from tokens table.
        // Stored queries return at minimum (name, token_id); the wrap adds the rest.
        // No explicit type casts — PostgreSQL coerces from text on COPY insert.
        std::string enrichedQuery =
            "WITH _base AS (" + baseQuery + ") "
            "SELECT _b.name, "
            "       _b.token_id, "
            "       length(_b.name), "
            "       split_part(_b.token_id, '.', 1), "
            "       COALESCE(_t.characteristics, 0), "
            "       _t.category "
            "FROM _base _b "
            "LEFT JOIN tokens _t ON _t.token_id = _b.token_id";

        PGresult* res = PQexec(shardConn, enrichedQuery.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[EnvelopeManager] AssembleQuery failed on %s: %s\n",
                query.shardDb.c_str(), PQerrorMessage(shardConn));
            fflush(stderr);
            PQclear(res);
            return 0;
        }

        int nrows = PQntuples(res);
        if (nrows == 0) { PQclear(res); return 0; }

        // Bulk insert via COPY to a temp staging table, then merge with ON CONFLICT DO NOTHING.
        // This avoids the UNIQUE (envelope_id, lmdb_subdb, word) constraint killing the whole batch
        // when later queries overlap with earlier ones (e.g. irregular forms sharing names with Labels).
        PGresult* tmpRes = PQexec(varConn,
            "CREATE TEMP TABLE IF NOT EXISTS _ews_stage "
            "(envelope_id INT, shard_db TEXT, lmdb_subdb TEXT, word TEXT, token_id TEXT, "
            " word_length SMALLINT, ns TEXT, characteristics INT, category TEXT, base_priority INT) "
            "ON COMMIT DELETE ROWS");
        PQclear(tmpRes);
        PQexec(varConn, "BEGIN");
        PGresult* copyRes = PQexec(varConn,
            "COPY _ews_stage "
            "(envelope_id, shard_db, lmdb_subdb, word, token_id, "
            " word_length, ns, characteristics, category, base_priority) "
            "FROM STDIN");

        if (PQresultStatus(copyRes) != PGRES_COPY_IN)
        {
            fprintf(stderr, "[EnvelopeManager] COPY start failed: %s\n",
                PQerrorMessage(varConn));
            fflush(stderr);
            PQclear(copyRes);
            PQclear(res);
            PQexec(varConn, "ROLLBACK");
            return 0;
        }
        PQclear(copyRes);

        std::string envIdStr   = std::to_string(envelopeId);
        std::string shardDbStr(query.shardDb.c_str(), query.shardDb.size());
        std::string subdbStr(query.lmdbSubdb.c_str(), query.lmdbSubdb.size());

        for (int i = 0; i < nrows; ++i)
        {
            const char* word     = PQgetvalue(res, i, 0);
            const char* tokenId  = PQgetvalue(res, i, 1);
            const char* wordLen  = PQgetvalue(res, i, 2);
            const char* ns       = PQgetvalue(res, i, 3);
            const char* charStr  = PQgetvalue(res, i, 4);
            const char* category = PQgetlength(res, i, 5) > 0 ? PQgetvalue(res, i, 5) : nullptr;

            std::string line;
            line.reserve(64);

            // envelope_id
            line += envIdStr; line += '\t';
            // shard_db
            AppendCopyField(line, shardDbStr.c_str(), (int)shardDbStr.size(), true);
            // lmdb_subdb
            AppendCopyField(line, subdbStr.c_str(), (int)subdbStr.size(), true);
            // word
            AppendCopyField(line, word, PQgetlength(res, i, 0), true);
            // token_id
            AppendCopyField(line, tokenId, PQgetlength(res, i, 1), true);
            // word_length
            AppendCopyField(line, wordLen, PQgetlength(res, i, 2), true);
            // ns
            AppendCopyField(line, ns, PQgetlength(res, i, 3), true);
            // characteristics
            AppendCopyField(line, charStr, PQgetlength(res, i, 4), true);
            // category (nullable)
            AppendCopyField(line, category, category ? (int)strlen(category) : 0, true);
            // base_priority
            std::string priorityStr = std::to_string(startOffset + i);
            line += priorityStr;
            line += '\n';

            if (PQputCopyData(varConn, line.c_str(), (int)line.size()) != 1)
                break;
        }

        PQputCopyEnd(varConn, nullptr);

        // Drain all results — COPY may produce multiple result objects
        PGresult* endRes;
        while ((endRes = PQgetResult(varConn)) != nullptr)
        {
            ExecStatusType st = PQresultStatus(endRes);
            if (st != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[EnvelopeManager] COPY end error (%s): %s\n",
                    PQresStatus(st), PQerrorMessage(varConn));
                fflush(stderr);
            }
            PQclear(endRes);
        }

        // Merge staged rows → envelope_working_set, skipping any UNIQUE conflicts.
        // This allows later queries (e.g. irregular forms) to coexist with earlier
        // Label rows that share the same surface form.
        PGresult* mergeRes = PQexec(varConn,
            "INSERT INTO envelope_working_set "
            "(envelope_id, shard_db, lmdb_subdb, word, token_id, word_length, ns, characteristics, category, base_priority) "
            "SELECT envelope_id, shard_db, lmdb_subdb, word, token_id, word_length, ns, characteristics, category, base_priority "
            "FROM _ews_stage "
            "ON CONFLICT (envelope_id, lmdb_subdb, word) DO NOTHING");

        int merged = 0;
        if (PQresultStatus(mergeRes) == PGRES_COMMAND_OK)
            merged = atoi(PQcmdTuples(mergeRes));
        else
        {
            fprintf(stderr, "[EnvelopeManager] Merge from stage failed: %s\n",
                PQerrorMessage(varConn));
            fflush(stderr);
        }
        PQclear(mergeRes);

        PQexec(varConn, "COMMIT");
        PQclear(res);

        return merged;
    }

    // ---------------------------------------------------------------------------
    // Warm → LMDB: read envelope_working_set ordered by effective_priority,
    // write to LMDB sub-dbs. Also populates t2w reverse for w2t writes.
    // ---------------------------------------------------------------------------
    int HCPEnvelopeManager::FlushWorkingSetToLmdb(int envelopeId)
    {
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn || !m_lmdbEnv) return 0;

        AZStd::string envIdStr    = AZStd::string::format("%d", envelopeId);
        const char*   params[]    = { envIdStr.c_str() };

        PGresult* res = PQexecParams(varConn,
            "SELECT word, token_id, lmdb_subdb "
            "FROM envelope_working_set "
            "WHERE envelope_id = $1 "
            "ORDER BY lmdb_subdb, word_length, effective_priority",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[EnvelopeManager] FlushWorkingSetToLmdb query failed: %s\n",
                PQerrorMessage(varConn));
            fflush(stderr);
            PQclear(res);
            return 0;
        }

        int nrows = PQntuples(res);
        if (nrows == 0) { PQclear(res); return 0; }

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
        {
            PQclear(res);
            return 0;
        }

        std::string currentSubdb;
        MDB_dbi     dbi      = 0;
        MDB_dbi     dbiT2w   = 0;
        bool        t2wOpen  = false;
        int         written  = 0;

        for (int i = 0; i < nrows; ++i)
        {
            const char* word    = PQgetvalue(res, i, 0);
            const char* tokenId = PQgetvalue(res, i, 1);
            const char* subdb   = PQgetvalue(res, i, 2);
            int wordLen   = PQgetlength(res, i, 0);
            int tokenLen  = PQgetlength(res, i, 1);

            // Open new sub-db handle when target changes
            if (currentSubdb != subdb)
            {
                currentSubdb = subdb;
                if (mdb_dbi_open(txn, subdb, MDB_CREATE, &dbi) != 0)
                    continue;

                t2wOpen = false;
                if (currentSubdb == "w2t")
                    t2wOpen = (mdb_dbi_open(txn, "t2w", MDB_CREATE, &dbiT2w) == 0);
            }

            MDB_val mKey = { static_cast<size_t>(wordLen),  const_cast<char*>(word) };
            MDB_val mVal = { static_cast<size_t>(tokenLen), const_cast<char*>(tokenId) };

            if (mdb_put(txn, dbi, &mKey, &mVal, 0) == 0)
            {
                ++written;
                if (t2wOpen)
                {
                    MDB_val rKey = { static_cast<size_t>(tokenLen), const_cast<char*>(tokenId) };
                    MDB_val rVal = { static_cast<size_t>(wordLen),  const_cast<char*>(word) };
                    // MDB_NOOVERWRITE: canonical surface form (first write, lowest priority) wins.
                    // Variant/irregular forms loaded later must NOT overwrite token → base word.
                    mdb_put(txn, dbiT2w, &rKey, &rVal, MDB_NOOVERWRITE);
                }
            }
        }

        mdb_txn_commit(txn);
        PQclear(res);

        return written;
    }

    // ---------------------------------------------------------------------------
    // Map morpheme name string to the corresponding MorphBit constant.
    // Returns 0 for morphemes without a current bit (COMPARATIVE, SUPERLATIVE, ADVERB_LY).
    static AZ::u16 MorphemeNameToBit(const char* morpheme)
    {
        if (strcmp(morpheme, "PAST")        == 0) return MorphBit::PAST;
        if (strcmp(morpheme, "PROGRESSIVE") == 0) return MorphBit::PROG;
        if (strcmp(morpheme, "PLURAL")      == 0) return MorphBit::PLURAL;
        if (strcmp(morpheme, "3RD_SING")    == 0) return MorphBit::THIRD;
        return 0;
    }

    // ---------------------------------------------------------------------------
    AZStd::vector<InflectionRule> HCPEnvelopeManager::LoadInflectionRules(const char* dbName)
    {
        AZStd::vector<InflectionRule> rules;
        PGconn* conn = GetShardConnection(dbName);
        if (!conn) return rules;

        PGresult* res = PQexec(conn,
            "SELECT morpheme, priority, rule_type, condition, "
            "       strip_suffix, add_suffix, strip_prefix, add_prefix "
            "FROM inflection_rules "
            "ORDER BY morpheme, priority");

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[EnvelopeManager] LoadInflectionRules failed on '%s': %s\n",
                dbName, PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            return rules;
        }

        int n = PQntuples(res);
        rules.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            InflectionRule r;
            r.morpheme    = PQgetvalue(res, i, 0);
            r.priority    = atoi(PQgetvalue(res, i, 1));
            r.ruleType    = PQgetvalue(res, i, 2);
            r.condition   = PQgetvalue(res, i, 3);
            r.stripSuffix = PQgetvalue(res, i, 4);
            r.addSuffix   = PQgetvalue(res, i, 5);
            r.stripPrefix = PQgetvalue(res, i, 6);
            r.addPrefix   = PQgetvalue(res, i, 7);
            r.morphBit    = MorphemeNameToBit(r.morpheme.c_str());
            rules.push_back(AZStd::move(r));
        }
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] LoadInflectionRules('%s'): %d rules loaded\n",
            dbName, n);
        fflush(stderr);
        return rules;
    }

    // ---------------------------------------------------------------------------
    // Hot-cache window management
    // ---------------------------------------------------------------------------

    int HCPEnvelopeManager::FeedSlice(int envelopeId, int offset, int count)
    {
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn || !m_lmdbEnv) return 0;

        AZStd::string envIdStr  = AZStd::string::format("%d", envelopeId);
        AZStd::string offsetStr = AZStd::string::format("%d", offset);
        AZStd::string countStr  = AZStd::string::format("%d", count);
        const char* params[] = { envIdStr.c_str(), offsetStr.c_str(), countStr.c_str() };

        PGresult* res = PQexecParams(varConn,
            "SELECT word, token_id, lmdb_subdb "
            "FROM envelope_working_set "
            "WHERE envelope_id = $1 "
            "ORDER BY lmdb_subdb, word_length, effective_priority "
            "OFFSET $2 LIMIT $3",
            3, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[EnvelopeManager] FeedSlice query failed: %s\n",
                PQerrorMessage(varConn));
            fflush(stderr);
            PQclear(res);
            return 0;
        }

        int nrows = PQntuples(res);
        if (nrows == 0) { PQclear(res); return 0; }

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0) { PQclear(res); return 0; }

        std::string currentSubdb;
        MDB_dbi dbi    = 0;
        MDB_dbi dbiT2w = 0;
        bool t2wOpen   = false;
        int written    = 0;

        for (int i = 0; i < nrows; ++i)
        {
            const char* word    = PQgetvalue(res, i, 0);
            const char* tokenId = PQgetvalue(res, i, 1);
            const char* subdb   = PQgetvalue(res, i, 2);
            int wordLen  = PQgetlength(res, i, 0);
            int tokenLen = PQgetlength(res, i, 1);

            if (currentSubdb != subdb)
            {
                currentSubdb = subdb;
                if (mdb_dbi_open(txn, subdb, MDB_CREATE, &dbi) != 0) { dbi = 0; continue; }
                t2wOpen = false;
                if (currentSubdb == "w2t")
                    t2wOpen = (mdb_dbi_open(txn, "t2w", MDB_CREATE, &dbiT2w) == 0);
            }

            if (dbi == 0) continue;

            MDB_val mKey = { static_cast<size_t>(wordLen),  const_cast<char*>(word) };
            MDB_val mVal = { static_cast<size_t>(tokenLen), const_cast<char*>(tokenId) };

            if (mdb_put(txn, dbi, &mKey, &mVal, 0) == 0)
            {
                ++written;
                if (t2wOpen)
                {
                    MDB_val rKey = { static_cast<size_t>(tokenLen), const_cast<char*>(tokenId) };
                    MDB_val rVal = { static_cast<size_t>(wordLen),  const_cast<char*>(word) };
                    mdb_put(txn, dbiT2w, &rKey, &rVal, MDB_NOOVERWRITE);
                }
            }
        }

        mdb_txn_commit(txn);
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] FeedSlice(offset=%d, count=%d): %d written\n",
            offset, count, written);
        fflush(stderr);
        return written;
    }

    // ---------------------------------------------------------------------------
    void HCPEnvelopeManager::EvictSlice(int envelopeId, int offset, int count)
    {
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn || !m_lmdbEnv) return;

        AZStd::string envIdStr  = AZStd::string::format("%d", envelopeId);
        AZStd::string offsetStr = AZStd::string::format("%d", offset);
        AZStd::string countStr  = AZStd::string::format("%d", count);
        const char* params[] = { envIdStr.c_str(), offsetStr.c_str(), countStr.c_str() };

        // Query only word + subdb — t2w is NOT evicted (accumulates for reconstruction).
        PGresult* res = PQexecParams(varConn,
            "SELECT word, lmdb_subdb "
            "FROM envelope_working_set "
            "WHERE envelope_id = $1 "
            "ORDER BY lmdb_subdb, word_length, effective_priority "
            "OFFSET $2 LIMIT $3",
            3, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
        {
            PQclear(res);
            return;
        }

        int nrows = PQntuples(res);

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0) { PQclear(res); return; }

        std::string currentSubdb;
        MDB_dbi dbi = 0;
        int evicted = 0;

        for (int i = 0; i < nrows; ++i)
        {
            const char* word  = PQgetvalue(res, i, 0);
            const char* subdb = PQgetvalue(res, i, 1);
            int wordLen = PQgetlength(res, i, 0);

            if (currentSubdb != subdb)
            {
                currentSubdb = subdb;
                dbi = 0;
                mdb_dbi_open(txn, subdb, 0, &dbi);
            }

            if (dbi == 0) continue;

            MDB_val mKey = { static_cast<size_t>(wordLen), const_cast<char*>(word) };
            if (mdb_del(txn, dbi, &mKey, nullptr) == 0)
                ++evicted;
        }

        mdb_txn_commit(txn);
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] EvictSlice(offset=%d, count=%d): %d evicted\n",
            offset, count, evicted);
        fflush(stderr);
    }

    // ---------------------------------------------------------------------------
    int HCPEnvelopeManager::GetWorkingSetSize(int envelopeId)
    {
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn) return 0;

        AZStd::string envIdStr = AZStd::string::format("%d", envelopeId);
        const char* params[] = { envIdStr.c_str() };

        PGresult* res = PQexecParams(varConn,
            "SELECT COUNT(*) FROM envelope_working_set WHERE envelope_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        int count = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            count = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return count;
    }

    // ---------------------------------------------------------------------------
    void HCPEnvelopeManager::ClearWorkingSet(int envelopeId)
    {
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn) return;

        AZStd::string idStr   = AZStd::string::format("%d", envelopeId);
        const char*   params[] = { idStr.c_str() };

        PGresult* res = PQexecParams(varConn,
            "DELETE FROM envelope_working_set WHERE envelope_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        PQclear(res);
    }

    // ---------------------------------------------------------------------------
    // Read from warm db (envelope_working_set) — NOT the cold shard.
    // Used for mid-resolve tense pre-fetch injection into BedManager.
    // ---------------------------------------------------------------------------
    AZStd::vector<VocabEntry> HCPEnvelopeManager::QueryEnvelopeEntries(
        const AZStd::string& name)
    {
        AZStd::vector<VocabEntry> out;
        if (!m_coreConn) return out;

        // Look up envelope id
        const char* params[] = { name.c_str() };
        PGresult* res = PQexecParams(m_coreConn,
            "SELECT id FROM envelope_definitions WHERE name = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
        {
            PQclear(res);
            return out;
        }
        int envId = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);

        // Read from warm db
        PGconn* varConn = GetShardConnection("hcp_var");
        if (!varConn) return out;

        AZStd::string envIdStr    = AZStd::string::format("%d", envId);
        const char*   qParams[]   = { envIdStr.c_str() };

        res = PQexecParams(varConn,
            "SELECT word, token_id "
            "FROM envelope_working_set "
            "WHERE envelope_id = $1 "
            "ORDER BY word_length, effective_priority",
            1, nullptr, qParams, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            return out;
        }

        int nrows = PQntuples(res);
        out.reserve(static_cast<size_t>(nrows));
        for (int i = 0; i < nrows; ++i)
        {
            VocabEntry e;
            e.word    = PQgetvalue(res, i, 0);
            e.tokenId = PQgetvalue(res, i, 1);
            out.push_back(AZStd::move(e));
        }
        PQclear(res);

        fprintf(stderr, "[EnvelopeManager] QueryEnvelopeEntries('%s'): %zu entries from warm db\n",
            name.c_str(), out.size());
        fflush(stderr);
        return out;
    }

    // ---------------------------------------------------------------------------
    int HCPEnvelopeManager::EvictManifest(const AZStd::string& envelopeName)
    {
        if (!m_manifestOpen || !m_lmdbEnv) return 0;

        MDB_txn* txn = nullptr;
        if (mdb_txn_begin(m_lmdbEnv, nullptr, 0, &txn) != 0)
            return 0;

        AZStd::string manifestKey = "env:" + envelopeName;
        MDB_val mdbKey = { manifestKey.size(), const_cast<char*>(manifestKey.c_str()) };
        MDB_val mdbVal;

        int evicted = 0;
        if (mdb_get(txn, m_manifestDbi, &mdbKey, &mdbVal) == 0)
        {
            AZStd::string manifest(
                static_cast<const char*>(mdbVal.mv_data), mdbVal.mv_size);
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
                        mdb_drop(txn, dbi, 0);
                        ++evicted;
                    }
                }
            }
            mdb_del(txn, m_manifestDbi, &mdbKey, nullptr);
        }

        mdb_txn_commit(txn);
        return evicted;
    }

    // ---------------------------------------------------------------------------
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

        AZStd::string manifest;
        if (mdb_get(txn, m_manifestDbi, &mdbKey, &mdbVal) == 0)
            manifest = AZStd::string(static_cast<const char*>(mdbVal.mv_data), mdbVal.mv_size);

        if (manifest.find(subdb) == AZStd::string::npos)
            manifest += subdb + "\n";

        MDB_val newVal = { manifest.size(), const_cast<char*>(manifest.c_str()) };
        mdb_put(txn, m_manifestDbi, &mdbKey, &newVal, 0);
        mdb_txn_commit(txn);
    }

    // ---------------------------------------------------------------------------
    void HCPEnvelopeManager::LogActivation(
        const EnvelopeActivation& activation,
        const AZStd::string& envelopeName)
    {
        if (!m_coreConn) return;

        AZStd::string envIdStr   = AZStd::string::format("%d",  activation.envelopeId);
        AZStd::string loadedStr  = AZStd::string::format("%d",  activation.entriesLoaded);
        AZStd::string timeStr    = AZStd::string::format("%.1f",activation.loadTimeMs);
        AZStd::string evictedStr = AZStd::string::format("%d",  activation.evictedEntries);

        const char* params[] = {
            envIdStr.c_str(), loadedStr.c_str(),
            timeStr.c_str(),  evictedStr.c_str()
        };
        PGresult* res = PQexecParams(m_coreConn,
            "INSERT INTO envelope_activations "
            "(envelope_id, entries_loaded, load_time_ms, evicted_entries) "
            "VALUES ($1::integer, $2::integer, $3::double precision, $4::integer)",
            4, nullptr, params, nullptr, nullptr, 0);
        PQclear(res);
    }

    // ---------------------------------------------------------------------------
    PGconn* HCPEnvelopeManager::GetShardConnection(const AZStd::string& dbName)
    {
        auto it = m_shardConns.find(dbName);
        if (it != m_shardConns.end() && PQstatus(it->second) == CONNECTION_OK)
            return it->second;

        AZStd::string connStr =
            "host=localhost dbname=" + dbName + " user=hcp password=hcp_dev";

        PGconn* conn = PQconnectdb(connStr.c_str());
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[EnvelopeManager] Failed to connect to '%s': %s\n",
                dbName.c_str(), PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return nullptr;
        }

        m_shardConns[dbName] = conn;
        return conn;
    }

    // ---------------------------------------------------------------------------
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
        m_activeEnvelopeId = 0;
        m_manifestOpen     = false;
    }

} // namespace HCPEngine
