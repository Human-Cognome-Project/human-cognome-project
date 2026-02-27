#include "HCPCacheMissResolver.h"

#include <lmdb.h>
#include <libpq-fe.h>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace HCPEngine
{
    // Var request token prefix — keys starting with this route to VarHandler
    static constexpr const char* VAR_PREFIX = "AA.AE.AF.AA.AC";
    static constexpr size_t VAR_PREFIX_LEN = 14; // strlen("AA.AE.AF.AA.AC")

    // ---- CacheMissResolver ----

    CacheMissResolver::~CacheMissResolver()
    {
        Shutdown();
    }

    void CacheMissResolver::Shutdown()
    {
        for (auto& [name, conn] : m_pgConns)
        {
            if (conn)
            {
                PQfinish(conn);
            }
        }
        m_pgConns.clear();
        m_handlers.clear();
    }

    void CacheMissResolver::RegisterHandler(AZStd::unique_ptr<ICacheMissHandler> handler)
    {
        AZStd::string name(handler->GetSubDbName());
        m_handlers[name] = AZStd::move(handler);
    }

    void CacheMissResolver::SetLmdbEnv(MDB_env* env)
    {
        m_env = env;
    }

    void CacheMissResolver::SetLmdbDbi(const char* subDbName, MDB_dbi dbi)
    {
        m_dbis[AZStd::string(subDbName)] = dbi;
    }

    PGconn* CacheMissResolver::GetConnection(const char* dbname)
    {
        AZStd::string key(dbname);
        auto it = m_pgConns.find(key);
        if (it != m_pgConns.end() && it->second)
        {
            // Check if connection is still alive
            if (PQstatus(it->second) == CONNECTION_OK)
                return it->second;
            PQfinish(it->second);
        }

        char conninfo[256];
        snprintf(conninfo, sizeof(conninfo),
            "host=localhost dbname=%s user=hcp password=hcp_dev", dbname);

        PGconn* conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[CacheMissResolver] Postgres connect(%s): %s\n",
                dbname, PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            m_pgConns[key] = nullptr;
            return nullptr;
        }

        m_pgConns[key] = conn;
        return conn;
    }

    AZStd::string CacheMissResolver::HandleMiss(
        const char* subDbName,
        const char* key, size_t keyLen,
        const ResolveContext& context)
    {
        // Check for var_request prefix — routes to VarHandler
        AZStd::string handlerName;
        if (keyLen > VAR_PREFIX_LEN &&
            memcmp(key, VAR_PREFIX, VAR_PREFIX_LEN) == 0)
        {
            handlerName = "var";
        }
        else
        {
            handlerName = subDbName;
        }

        auto it = m_handlers.find(handlerName);
        if (it == m_handlers.end())
        {
            return {};
        }

        ResolveResult result;
        if (!it->second->Resolve(key, keyLen, context, result))
        {
            return {};
        }

        // Execute all LMDB writes in a single transaction
        if (!result.writes.empty() && m_env)
        {
            MDB_txn* txn;
            int rc = mdb_txn_begin(m_env, nullptr, 0, &txn);
            if (rc == 0)
            {
                bool allOk = true;
                for (const auto& write : result.writes)
                {
                    auto dbiIt = m_dbis.find(write.subDbName);
                    if (dbiIt == m_dbis.end()) continue;

                    MDB_val k, v;
                    k.mv_data = const_cast<char*>(write.key.c_str());
                    k.mv_size = write.key.size();
                    v.mv_data = const_cast<char*>(write.value.c_str());
                    v.mv_size = write.value.size();

                    rc = mdb_put(txn, dbiIt->second, &k, &v, 0);
                    if (rc != 0)
                    {
                        fprintf(stderr, "[CacheMissResolver] mdb_put(%s): %s\n",
                            write.subDbName.c_str(), mdb_strerror(rc));
                        fflush(stderr);
                        allOk = false;
                        break;
                    }
                }

                if (allOk)
                    mdb_txn_commit(txn);
                else
                    mdb_txn_abort(txn);
            }
        }

        return result.value;
    }

    bool CacheMissResolver::LmdbPut(
        const char* subDbName,
        const void* key, size_t keyLen,
        const void* value, size_t valueLen)
    {
        if (!m_env) return false;

        auto it = m_dbis.find(AZStd::string(subDbName));
        if (it == m_dbis.end()) return false;

        MDB_txn* txn;
        int rc = mdb_txn_begin(m_env, nullptr, 0, &txn);
        if (rc != 0)
        {
            fprintf(stderr, "[CacheMissResolver] mdb_txn_begin: %s\n", mdb_strerror(rc));
            fflush(stderr);
            return false;
        }

        MDB_val k, v;
        k.mv_data = const_cast<void*>(key);
        k.mv_size = keyLen;
        v.mv_data = const_cast<void*>(value);
        v.mv_size = valueLen;

        rc = mdb_put(txn, it->second, &k, &v, 0);
        if (rc != 0)
        {
            fprintf(stderr, "[CacheMissResolver] mdb_put(%s): %s\n",
                subDbName, mdb_strerror(rc));
            fflush(stderr);
            mdb_txn_abort(txn);
            return false;
        }

        rc = mdb_txn_commit(txn);
        if (rc != 0)
        {
            fprintf(stderr, "[CacheMissResolver] mdb_txn_commit: %s\n", mdb_strerror(rc));
            fflush(stderr);
            return false;
        }

        return true;
    }

    // ---- WordHandler ----

    bool WordHandler::Resolve(
        const char* key, size_t keyLen,
        const ResolveContext& /*ctx*/, ResolveResult& result)
    {
        PGconn* conn = m_resolver->GetConnection("hcp_english");
        if (!conn) return false;

        // Build lowercase version
        AZStd::string word(key, keyLen);
        AZStd::string lower(word);
        for (size_t i = 0; i < lower.size(); ++i)
        {
            unsigned char uc = static_cast<unsigned char>(lower[i]);
            if (std::isupper(uc))
            {
                lower[i] = static_cast<char>(std::tolower(uc));
            }
        }

        // Try exact case first — labels and mixed-case forms (eBook, November)
        // carry their case as the surface form.
        const char* paramValues[1] = { word.c_str() };
        int paramLengths[1] = { static_cast<int>(word.size()) };
        int paramFormats[1] = { 0 }; // text

        PGresult* res = PQexecParams(conn,
            "SELECT token_id FROM tokens WHERE name = $1 LIMIT 1",
            1, nullptr, paramValues, paramLengths, paramFormats, 0);

        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            result.value = PQgetvalue(res, 0, 0);
            PQclear(res);

            result.writes.push_back({ "w2t", word, result.value });
            result.writes.push_back({ "t2w", result.value, word });
            return true;
        }
        PQclear(res);

        // Lowercase fallback (standard words — "the", "and", etc.)
        if (word != lower)
        {
            paramValues[0] = lower.c_str();
            paramLengths[0] = static_cast<int>(lower.size());

            res = PQexecParams(conn,
                "SELECT token_id FROM tokens WHERE name = $1 LIMIT 1",
                1, nullptr, paramValues, paramLengths, paramFormats, 0);

            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                result.value = PQgetvalue(res, 0, 0);
                PQclear(res);

                // Cache lowercase form + reverse lookup
                result.writes.push_back({ "w2t", lower, result.value });
                result.writes.push_back({ "t2w", result.value, lower });

                // Also cache the original case variant to avoid repeated misses
                result.writes.push_back({ "w2t", word, result.value });
                return true;
            }
            PQclear(res);
        }

        return false;
    }

    // ---- CharHandler ----

    bool CharHandler::Resolve(
        const char* key, size_t keyLen,
        const ResolveContext& /*ctx*/, ResolveResult& result)
    {
        if (keyLen != 4) return false;

        uint32_t cp;
        memcpy(&cp, key, 4);

        // Deterministic token_id using 2-pair codepoint encoding:
        //   p4 = EncodePairB50(cp / 2500)  — high pair
        //   p5 = EncodePairB50(cp % 2500)  — low pair
        // For ASCII (cp < 256): p4 = "AA", p5 = same as old byte encoding
        // Capacity: 2500 × 2500 = 6,250,000 — covers all Unicode codepoints
        AZStd::string tokenId = "AA.AA.AA.";
        tokenId += EncodePairB50(static_cast<int>(cp / 2500));
        tokenId += ".";
        tokenId += EncodePairB50(static_cast<int>(cp % 2500));

        result.value = tokenId;
        result.writes.push_back({ "c2t", AZStd::string(key, 4), tokenId });
        result.writes.push_back({ "t2c", tokenId, AZStd::string(key, 4) });
        return true;
    }

    // ---- LabelHandler ----

    bool LabelHandler::Resolve(
        const char* key, size_t keyLen,
        const ResolveContext& /*ctx*/, ResolveResult& result)
    {
        AZStd::string label(key, keyLen);
        const char* paramValues[1] = { label.c_str() };
        int paramLengths[1] = { static_cast<int>(label.size()) };
        int paramFormats[1] = { 0 };

        // Try hcp_english first (language-specific labels)
        PGconn* conn = m_resolver->GetConnection("hcp_english");
        if (conn)
        {
            PGresult* res = PQexecParams(conn,
                "SELECT token_id FROM tokens WHERE name = $1 AND layer = 'label' LIMIT 1",
                1, nullptr, paramValues, paramLengths, paramFormats, 0);

            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                result.value = PQgetvalue(res, 0, 0);
                PQclear(res);

                result.writes.push_back({ "l2t", label, result.value });
                return true;
            }
            PQclear(res);
        }

        // Fallback to hcp_core (structural markers: pbm_marker category)
        PGconn* coreConn = m_resolver->GetConnection("hcp_core");
        if (!coreConn) return false;

        PGresult* res = PQexecParams(coreConn,
            "SELECT token_id FROM tokens WHERE name = $1 AND category = 'pbm_marker' LIMIT 1",
            1, nullptr, paramValues, paramLengths, paramFormats, 0);

        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            result.value = PQgetvalue(res, 0, 0);
            PQclear(res);

            result.writes.push_back({ "l2t", label, result.value });
            return true;
        }
        PQclear(res);

        return false;
    }

    // ---- VarHandler ----

    bool VarHandler::Resolve(
        const char* key, size_t keyLen,
        const ResolveContext& ctx, ResolveResult& result)
    {
        // Key format: "AA.AE.AF.AA.AC chunk" — extract chunk after prefix
        if (keyLen <= VAR_PREFIX_LEN) return false;

        // Skip prefix and whitespace
        size_t chunkStart = VAR_PREFIX_LEN;
        while (chunkStart < keyLen && (key[chunkStart] == ' ' || key[chunkStart] == '\t'))
            ++chunkStart;
        if (chunkStart >= keyLen) return false;

        AZStd::string chunk(key + chunkStart, keyLen - chunkStart);

        PGconn* conn = m_resolver->GetConnection("hcp_var");
        if (!conn) return false;

        // Application-side mint: check existing, then mint if needed
        const char* paramValues[1] = { chunk.c_str() };
        int paramLengths[1] = { static_cast<int>(chunk.size()) };
        int paramFormats[1] = { 0 };

        // Step 1: Check for existing active var with this surface form
        PGresult* res = PQexecParams(conn,
            "SELECT var_id FROM var_tokens WHERE form = $1 AND status = 'active' LIMIT 1",
            1, nullptr, paramValues, paramLengths, paramFormats, 0);

        bool minted = false;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            result.value = PQgetvalue(res, 0, 0);
            PQclear(res);
        }
        else
        {
            PQclear(res);

            // Step 2: Get next var ID from max existing
            res = PQexec(conn,
                "SELECT COALESCE(MAX(CAST(SUBSTR(var_id, 5) AS INTEGER)), 0) FROM var_tokens");
            int nextId = 1;
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                nextId = atoi(PQgetvalue(res, 0, 0)) + 1;
            }
            PQclear(res);

            // Step 3: Format var_id and insert
            char varIdBuf[32];
            snprintf(varIdBuf, sizeof(varIdBuf), "var.%d", nextId);
            result.value = varIdBuf;

            const char* insParams[2] = { varIdBuf, chunk.c_str() };
            int insLens[2] = { static_cast<int>(strlen(varIdBuf)), static_cast<int>(chunk.size()) };
            int insFmts[2] = { 0, 0 };

            res = PQexecParams(conn,
                "INSERT INTO var_tokens (var_id, form) VALUES ($1, $2)",
                2, nullptr, insParams, insLens, insFmts, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[VarHandler] var token INSERT failed: %s\n",
                    PQerrorMessage(conn));
                fflush(stderr);
                PQclear(res);
                return false;
            }
            PQclear(res);
            minted = true;
        }

        // Var tokens go in w2t (they occupy word positions)
        result.writes.push_back({ "w2t", chunk, result.value });
        result.writes.push_back({ "t2w", result.value, chunk });

        // Log source location for librarian promotion workflow (new mints only)
        if (minted && ctx.docId && ctx.position >= 0)
        {
            const char* srcParams[3];
            srcParams[0] = result.value.c_str();
            srcParams[1] = ctx.docId;
            char posStr[16];
            snprintf(posStr, sizeof(posStr), "%d", ctx.position);
            srcParams[2] = posStr;
            int srcLens[3] = {
                static_cast<int>(result.value.size()),
                static_cast<int>(strlen(ctx.docId)),
                static_cast<int>(strlen(posStr))
            };
            int srcFmts[3] = { 0, 0, 0 };

            PGresult* srcRes = PQexecParams(conn,
                "INSERT INTO var_sources (var_id, doc_id, position) "
                "VALUES ($1, $2, $3)",
                3, nullptr, srcParams, srcLens, srcFmts, 0);
            PQclear(srcRes);
        }

        return true;
    }

} // namespace HCPEngine
