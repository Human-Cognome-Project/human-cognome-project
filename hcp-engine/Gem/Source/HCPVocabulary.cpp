#include "HCPVocabulary.h"
#include "HCPCacheMissResolver.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <libpq-fe.h>
#include <cstdio>
#include <cstring>

namespace HCPEngine
{
    HCPVocabulary::~HCPVocabulary()
    {
        if (m_env)
        {
            mdb_env_close(m_env);
            m_env = nullptr;
        }
    }

    bool HCPVocabulary::Load(const char* lmdbPath)
    {
        int rc;

        rc = mdb_env_create(&m_env);
        if (rc != 0)
        {
            fprintf(stderr, "[HCPVocabulary] mdb_env_create: %s\n", mdb_strerror(rc));
            return false;
        }

        mdb_env_set_maxdbs(m_env, 10);
        mdb_env_set_mapsize(m_env, 1ULL * 1024 * 1024 * 1024); // 1 GB virtual

        rc = mdb_env_open(m_env, lmdbPath, 0, 0644);
        if (rc != 0)
        {
            fprintf(stderr, "[HCPVocabulary] mdb_env_open(%s): %s\n", lmdbPath, mdb_strerror(rc));
            mdb_env_close(m_env);
            m_env = nullptr;
            return false;
        }

        // Open named sub-databases — write txn so DBI handles persist
        MDB_txn* txn;
        rc = mdb_txn_begin(m_env, nullptr, 0, &txn);
        if (rc != 0)
        {
            fprintf(stderr, "[HCPVocabulary] mdb_txn_begin: %s\n", mdb_strerror(rc));
            mdb_env_close(m_env);
            m_env = nullptr;
            return false;
        }

        auto openDb = [&](const char* name, MDB_dbi* dbi) -> bool
        {
            int r = mdb_dbi_open(txn, name, MDB_CREATE, dbi);
            if (r != 0)
            {
                fprintf(stderr, "[HCPVocabulary] mdb_dbi_open(%s): %s\n", name, mdb_strerror(r));
                return false;
            }
            return true;
        };

        bool ok = openDb("w2t", &m_w2t)
                && openDb("c2t", &m_c2t)
                && openDb("l2t", &m_l2t)
                && openDb("t2w", &m_t2w)
                && openDb("t2c", &m_t2c)
                && openDb("forward", &m_fwd);

        if (!ok)
        {
            mdb_txn_abort(txn);
            mdb_env_close(m_env);
            m_env = nullptr;
            return false;
        }

        // Count entries
        MDB_stat stat;
        mdb_stat(txn, m_w2t, &stat);
        m_wordCount = stat.ms_entries;
        mdb_stat(txn, m_c2t, &stat);
        m_charCount = stat.ms_entries;
        mdb_stat(txn, m_l2t, &stat);
        m_labelCount = stat.ms_entries;

        mdb_txn_commit(txn);

        m_loaded = true;

        fprintf(stderr, "[HCPVocabulary] LMDB ready — %zu words, %zu labels, %zu chars\n",
            m_wordCount, m_labelCount, m_charCount);

        return true;
    }

    void HCPVocabulary::SetResolver(CacheMissResolver* resolver)
    {
        m_resolver = resolver;
    }

    MDB_dbi HCPVocabulary::GetDbi(const char* name) const
    {
        if (strcmp(name, "w2t") == 0) return m_w2t;
        if (strcmp(name, "c2t") == 0) return m_c2t;
        if (strcmp(name, "l2t") == 0) return m_l2t;
        if (strcmp(name, "t2w") == 0) return m_t2w;
        if (strcmp(name, "t2c") == 0) return m_t2c;
        if (strcmp(name, "forward") == 0) return m_fwd;
        return 0;
    }

    const char* HCPVocabulary::SubDbName(MDB_dbi dbi) const
    {
        if (dbi == m_w2t) return "w2t";
        if (dbi == m_c2t) return "c2t";
        if (dbi == m_l2t) return "l2t";
        if (dbi == m_t2w) return "t2w";
        if (dbi == m_t2c) return "t2c";
        if (dbi == m_fwd) return "forward";
        return nullptr;
    }

    AZStd::string HCPVocabulary::LmdbGet(MDB_dbi dbi, const char* key, size_t keyLen) const
    {
        MDB_txn* txn;
        int rc = mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn);
        if (rc != 0)
        {
            if (m_debugCount < 5)
            {
                fprintf(stderr, "[HCPVocabulary] txn_begin failed: %s\n", mdb_strerror(rc));
                fflush(stderr);
            }
            return {};
        }

        MDB_val k, v;
        k.mv_data = const_cast<char*>(key);
        k.mv_size = keyLen;

        rc = mdb_get(txn, dbi, &k, &v);
        AZStd::string result;
        if (rc == 0)
        {
            result.assign(static_cast<const char*>(v.mv_data), v.mv_size);
        }
        else if (rc != MDB_NOTFOUND && m_debugCount < 5)
        {
            fprintf(stderr, "[HCPVocabulary] mdb_get(dbi=%u, key='%.*s' len=%zu): %s\n",
                dbi, (int)keyLen, key, keyLen, mdb_strerror(rc));
            fflush(stderr);
            ++m_debugCount;
        }
        mdb_txn_abort(txn);

        // On cache miss, try resolver (forward lookups only: w2t, c2t, l2t)
        if (result.empty() && rc == MDB_NOTFOUND && m_resolver)
        {
            const char* subDb = SubDbName(dbi);
            if (subDb &&
                (strcmp(subDb, "w2t") == 0 ||
                 strcmp(subDb, "c2t") == 0 ||
                 strcmp(subDb, "l2t") == 0))
            {
                ResolveContext ctx;
                result = m_resolver->HandleMiss(subDb, key, keyLen, ctx);
            }
        }

        return result;
    }

    // ---- Primary lookup: chunk → token_id ----

    LookupResult HCPVocabulary::LookupChunk(const AZStd::string& chunk) const
    {
        LookupResult result;
        if (!m_loaded) return result;

        // Lowercase first — most common return path
        AZStd::string lower(chunk);
        for (size_t i = 0; i < lower.size(); ++i)
        {
            unsigned char uc = static_cast<unsigned char>(lower[i]);
            if (std::isupper(uc))
                lower[i] = static_cast<char>(std::tolower(uc));
        }

        result.tokenId = LmdbGet(m_w2t, lower.c_str(), lower.size());

        // Single character fallback
        if (result.tokenId.empty() && chunk.size() == 1)
        {
            result.tokenId = LmdbGet(m_c2t, chunk.c_str(), 1);
        }

        return result;
    }

    // ---- Forward walk: 3-state boilerplate prefix check ----

    ContinuationResult HCPVocabulary::CheckContinuation(
        const AZStd::string& accumulated,
        const AZStd::string& nextChunk) const
    {
        ContinuationResult result;
        if (!m_loaded) return result;

        // Key = "accumulated next_chunk" (space-separated prefix)
        AZStd::string extended = accumulated;
        extended += ' ';
        extended += nextChunk;

        // LMDB forward db returns:
        //   not found → uncached (miss, signal resolver for backfill)
        //   "0"       → cached negative (no match)
        //   "1"       → partial match (keep walking)
        //   token_id  → complete match (sequence token_id)
        AZStd::string val = LmdbGet(m_fwd, extended.c_str(), extended.size());
        if (val.empty() || val == "0")
        {
            return result; // Miss — uncached or cached negative
        }

        if (val == "1")
        {
            result.status = ContinuationResult::Continue;
        }
        else
        {
            result.status = ContinuationResult::Complete;
            result.sequenceId = val;
        }
        return result;
    }

    // ---- Granular lookups ----

    AZStd::string HCPVocabulary::LookupWord(const AZStd::string& wordForm) const
    {
        if (!m_loaded) return {};
        return LmdbGet(m_w2t, wordForm.c_str(), wordForm.size());
    }

    AZStd::string HCPVocabulary::LookupWordLocal(const AZStd::string& wordForm) const
    {
        if (!m_loaded) return {};

        MDB_txn* txn;
        int rc = mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn);
        if (rc != 0) return {};

        MDB_val k, v;
        k.mv_data = const_cast<char*>(wordForm.c_str());
        k.mv_size = wordForm.size();

        rc = mdb_get(txn, m_w2t, &k, &v);
        AZStd::string result;
        if (rc == 0)
            result.assign(static_cast<const char*>(v.mv_data), v.mv_size);
        mdb_txn_abort(txn);
        return result;
    }

    AZStd::string HCPVocabulary::LookupChar(char c) const
    {
        if (!m_loaded) return {};
        return LmdbGet(m_c2t, &c, 1);
    }

    AZStd::string HCPVocabulary::LookupLabel(const AZStd::string& label) const
    {
        if (!m_loaded) return {};
        return LmdbGet(m_l2t, label.c_str(), label.size());
    }

    AZStd::string HCPVocabulary::TokenToWord(const AZStd::string& tokenId) const
    {
        if (!m_loaded) return {};
        return LmdbGet(m_t2w, tokenId.c_str(), tokenId.size());
    }

    char HCPVocabulary::TokenToChar(const AZStd::string& tokenId) const
    {
        if (!m_loaded) return '\0';
        AZStd::string result = LmdbGet(m_t2c, tokenId.c_str(), tokenId.size());
        if (!result.empty()) return result[0];
        return '\0';
    }

    // ---- Affix loading (bulk from Postgres) ----

    bool HCPVocabulary::LoadAffixes(PGconn* conn)
    {
        if (!conn || PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPVocabulary] LoadAffixes: no Postgres connection\n");
            fflush(stderr);
            return false;
        }

        PGresult* res = PQexec(conn,
            "SELECT name, token_id FROM tokens WHERE layer = 'affix'");

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[HCPVocabulary] LoadAffixes query failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            return false;
        }

        int rows = PQntuples(res);
        m_suffixByLastChar.clear();
        m_prefixByFirstChar.clear();
        m_suffixCount = 0;
        m_prefixCount = 0;

        for (int i = 0; i < rows; ++i)
        {
            const char* name = PQgetvalue(res, i, 0);
            const char* tokenId = PQgetvalue(res, i, 1);
            size_t nameLen = strlen(name);

            if (nameLen < 2) continue;  // Need at least hyphen + one char

            bool leadingHyphen = (name[0] == '-');
            bool trailingHyphen = (name[nameLen - 1] == '-');

            if (leadingHyphen && !trailingHyphen)
            {
                // Suffix: "-ing" → stripped = "ing", index by last char 'g'
                AZStd::string stripped(name + 1, nameLen - 1);
                char key = stripped.back();
                m_suffixByLastChar[key].push_back(
                    { AZStd::move(stripped), AZStd::string(tokenId) });
                ++m_suffixCount;
            }
            else if (trailingHyphen && !leadingHyphen)
            {
                // Prefix: "un-" → stripped = "un", index by first char 'u'
                AZStd::string stripped(name, nameLen - 1);
                char key = stripped[0];
                m_prefixByFirstChar[key].push_back(
                    { AZStd::move(stripped), AZStd::string(tokenId) });
                ++m_prefixCount;
            }
            // Infixes and others skipped for now
        }

        PQclear(res);

        // Sort each bucket by stripped length descending (longest match first)
        auto byLenDesc = [](const AffixEntry& a, const AffixEntry& b)
        {
            return a.stripped.size() > b.stripped.size();
        };
        for (auto& [ch, bucket] : m_suffixByLastChar)
            AZStd::sort(bucket.begin(), bucket.end(), byLenDesc);
        for (auto& [ch, bucket] : m_prefixByFirstChar)
            AZStd::sort(bucket.begin(), bucket.end(), byLenDesc);

        fprintf(stderr, "[HCPVocabulary] Loaded %zu suffixes (%zu buckets), %zu prefixes (%zu buckets) from Postgres\n",
            m_suffixCount, m_suffixByLastChar.size(),
            m_prefixCount, m_prefixByFirstChar.size());
        fflush(stderr);

        return m_suffixCount > 0 || m_prefixCount > 0;
    }

    const AZStd::vector<AffixEntry>* HCPVocabulary::GetSuffixesForChar(char lastChar) const
    {
        auto it = m_suffixByLastChar.find(lastChar);
        if (it != m_suffixByLastChar.end())
            return &it->second;
        return nullptr;
    }

    const AZStd::vector<AffixEntry>* HCPVocabulary::GetPrefixesForChar(char firstChar) const
    {
        auto it = m_prefixByFirstChar.find(firstChar);
        if (it != m_prefixByFirstChar.end())
            return &it->second;
        return nullptr;
    }

    // ---- Iteration (for bond compiler) ----

    void HCPVocabulary::IterateWords(
        const AZStd::function<bool(const AZStd::string&, const AZStd::string&)>& callback) const
    {
        if (!m_loaded) return;

        MDB_txn* txn;
        if (mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn) != 0) return;

        MDB_cursor* cursor;
        if (mdb_cursor_open(txn, m_t2w, &cursor) != 0)
        {
            mdb_txn_abort(txn);
            return;
        }

        MDB_val key, val;
        while (mdb_cursor_get(cursor, &key, &val, MDB_NEXT) == 0)
        {
            AZStd::string tokenId(static_cast<const char*>(key.mv_data), key.mv_size);
            AZStd::string wordForm(static_cast<const char*>(val.mv_data), val.mv_size);
            if (!callback(wordForm, tokenId))
            {
                break;
            }
        }

        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
    }

    void HCPVocabulary::IterateChars(
        const AZStd::function<bool(const AZStd::string&, const AZStd::string&)>& callback) const
    {
        if (!m_loaded) return;

        MDB_txn* txn;
        if (mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn) != 0) return;

        MDB_cursor* cursor;
        if (mdb_cursor_open(txn, m_t2c, &cursor) != 0)
        {
            mdb_txn_abort(txn);
            return;
        }

        MDB_val key, val;
        while (mdb_cursor_get(cursor, &key, &val, MDB_NEXT) == 0)
        {
            AZStd::string tokenId(static_cast<const char*>(key.mv_data), key.mv_size);
            AZStd::string charVal(static_cast<const char*>(val.mv_data), val.mv_size);
            if (!callback(charVal, tokenId))
            {
                break;
            }
        }

        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
    }

} // namespace HCPEngine
