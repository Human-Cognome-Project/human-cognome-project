#include "HCPBondCompiler.h"
#include "HCPVocabulary.h"
#include "HCPParticlePipeline.h"

#include <AzCore/Console/ILogger.h>
#include <cstdio>
#include <libpq-fe.h>

namespace HCPEngine
{
    // ---- HCPBondTable ----

    AZStd::string HCPBondTable::MakeKey(const AZStd::string& a, const AZStd::string& b)
    {
        AZStd::string key;
        key.reserve(a.size() + 1 + b.size());
        key += a;
        key += '|';
        key += b;
        return key;
    }

    AZ::u32 HCPBondTable::GetBondStrength(const AZStd::string& a, const AZStd::string& b) const
    {
        auto it = m_bonds.find(MakeKey(a, b));
        return (it != m_bonds.end()) ? it->second : 0;
    }

    void HCPBondTable::AddBond(const AZStd::string& a, const AZStd::string& b, AZ::u32 count)
    {
        AZStd::string key = MakeKey(a, b);
        AZ::u32& existing = m_bonds[key];
        existing += count;
        m_totalCount += count;
        if (existing > m_maxCount)
        {
            m_maxCount = existing;
        }
    }

    void HCPBondTable::LogStats(const char* label) const
    {
        fprintf(stderr, "[HCPBondCompiler] %s: %zu unique pairs, %zu total bonds, max count %u\n",
            label, m_bonds.size(), m_totalCount, m_maxCount);
        fflush(stderr);
    }

    // ---- Helper: extract codepoints from UTF-8 string ----

    static AZStd::vector<AZStd::string> Utf8Codepoints(const char* data, size_t len)
    {
        AZStd::vector<AZStd::string> chars;
        chars.reserve(len);

        size_t i = 0;
        while (i < len)
        {
            unsigned char c = static_cast<unsigned char>(data[i]);
            size_t charLen = 1;
            if ((c & 0x80) == 0)      charLen = 1;
            else if ((c & 0xE0) == 0xC0) charLen = 2;
            else if ((c & 0xF0) == 0xE0) charLen = 3;
            else if ((c & 0xF8) == 0xF0) charLen = 4;

            if (i + charLen > len) charLen = 1;

            chars.push_back(AZStd::string(data + i, charLen));
            i += charLen;
        }
        return chars;
    }

    // ---- Char → Word compilation from Postgres ----

    HCPBondTable CompileCharWordBondsFromPostgres(const char* connInfo)
    {
        HCPBondTable table;

        PGconn* conn = PQconnectdb(connInfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Postgres connect failed: %s\n", PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return table;
        }

        // Query all word forms from hcp_english
        PGresult* res = PQexec(conn, "SELECT name FROM tokens WHERE layer = 'word'");
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Query failed: %s\n", PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            PQfinish(conn);
            return table;
        }

        int nrows = PQntuples(res);
        fprintf(stderr, "[HCPBondCompiler] Processing %d word forms from Postgres...\n", nrows);
        fflush(stderr);

        for (int r = 0; r < nrows; ++r)
        {
            const char* name = PQgetvalue(res, r, 0);
            int nameLen = PQgetlength(res, r, 0);

            if (nameLen < 2) continue;

            auto chars = Utf8Codepoints(name, static_cast<size_t>(nameLen));

            for (size_t j = 0; j + 1 < chars.size(); ++j)
            {
                table.AddBond(chars[j], chars[j + 1]);
            }
        }

        PQclear(res);
        PQfinish(conn);

        fprintf(stderr, "[HCPBondCompiler] Compiled char->word bonds from %d words\n", nrows);
        fflush(stderr);
        table.LogStats("char->word");

        return table;
    }

    // ---- Byte → Char compilation from Postgres ----

    HCPBondTable CompileByteCharBondsFromPostgres(const char* connInfo)
    {
        HCPBondTable table;

        PGconn* conn = PQconnectdb(connInfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Postgres connect failed: %s\n", PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return table;
        }

        // Query all characters with their UTF-8 byte sequences from atomization.
        // Single-byte chars (ASCII): hex in metadata, 1 byte = no pairs.
        // Multi-byte chars: atomization->'UTF-8'->'raw' has the byte array.
        PGresult* res = PQexec(conn,
            "SELECT token_id, "
            "       metadata->'atomization'->'UTF-8'->'raw' AS raw_bytes "
            "FROM tokens "
            "WHERE category = 'character' "
            "  AND metadata->'atomization'->'UTF-8'->'raw' IS NOT NULL");
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Query failed: %s\n", PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            PQfinish(conn);
            return table;
        }

        int nrows = PQntuples(res);
        size_t singleByteCount = 0;
        size_t multiByteCount = 0;

        for (int r = 0; r < nrows; ++r)
        {
            const char* rawJson = PQgetvalue(res, r, 1);
            if (!rawJson || rawJson[0] == '\0') continue;

            // Parse the JSON byte array: e.g., "[239, 191, 166]"
            AZStd::vector<unsigned char> bytes;
            const char* p = rawJson;
            while (*p)
            {
                if (*p >= '0' && *p <= '9')
                {
                    int val = 0;
                    while (*p >= '0' && *p <= '9')
                    {
                        val = val * 10 + (*p - '0');
                        ++p;
                    }
                    if (val >= 0 && val <= 255)
                    {
                        bytes.push_back(static_cast<unsigned char>(val));
                    }
                }
                else
                {
                    ++p;
                }
            }

            if (bytes.size() < 2)
            {
                ++singleByteCount;
                continue;
            }

            ++multiByteCount;

            // Extract adjacent byte pairs
            for (size_t i = 0; i + 1 < bytes.size(); ++i)
            {
                char hexA[3], hexB[3];
                snprintf(hexA, sizeof(hexA), "%02X", bytes[i]);
                snprintf(hexB, sizeof(hexB), "%02X", bytes[i + 1]);
                table.AddBond(AZStd::string(hexA, 2), AZStd::string(hexB, 2));
            }
        }

        PQclear(res);
        PQfinish(conn);

        fprintf(stderr, "[HCPBondCompiler] Compiled byte->char bonds: %d chars total, "
            "%zu single-byte, %zu multi-byte\n", nrows, singleByteCount, multiByteCount);
        fflush(stderr);
        table.LogStats("byte->char");

        return table;
    }

    // ---- LMDB-based compilation (for when cache is populated) ----

    HCPBondTable CompileCharWordBonds(const HCPVocabulary& vocab)
    {
        HCPBondTable table;
        size_t wordCount = 0;

        vocab.IterateWords([&](const AZStd::string& wordForm, [[maybe_unused]] const AZStd::string& tokenId) -> bool
        {
            if (wordForm.size() < 2)
            {
                ++wordCount;
                return true;
            }

            auto chars = Utf8Codepoints(wordForm.c_str(), wordForm.size());

            for (size_t j = 0; j + 1 < chars.size(); ++j)
            {
                table.AddBond(chars[j], chars[j + 1]);
            }

            ++wordCount;
            return true;
        });

        fprintf(stderr, "[HCPBondCompiler] Compiled char->word bonds from %zu words (LMDB)\n", wordCount);
        fflush(stderr);
        table.LogStats("char->word");
        return table;
    }

    HCPBondTable CompileByteCharBonds(const HCPVocabulary& vocab)
    {
        HCPBondTable table;
        // ASCII is 1:1 byte→char. Only relevant for multi-byte encodings.
        // Placeholder until atomization data is accessible.
        (void)vocab;
        table.LogStats("byte->char");
        return table;
    }

    // ---- Temp Postgres persistence (hcp_temp.bond_aggregates) ----

    bool SaveBondTable(const HCPBondTable& table, const char* level, const char* connInfo)
    {
        PGconn* conn = PQconnectdb(connInfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Save: Postgres connect failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return false;
        }

        PQexec(conn, "BEGIN");

        // Clear existing rows for this level
        {
            const char* params[] = { level };
            PGresult* res = PQexecParams(conn,
                "DELETE FROM bond_aggregates WHERE level = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        // Batch insert via COPY for speed
        PGresult* copyRes = PQexec(conn,
            "COPY bond_aggregates (level, elem_a, elem_b, count) FROM STDIN");
        if (PQresultStatus(copyRes) != PGRES_COPY_IN)
        {
            fprintf(stderr, "[HCPBondCompiler] Save: COPY failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQclear(copyRes);
            PQexec(conn, "ROLLBACK");
            PQfinish(conn);
            return false;
        }
        PQclear(copyRes);

        size_t written = 0;
        for (const auto& [key, count] : table.GetAllBonds())
        {
            // Key format is "a|b"
            size_t sep = key.find('|');
            if (sep == AZStd::string::npos) continue;

            AZStd::string a(key.c_str(), sep);
            AZStd::string b(key.c_str() + sep + 1, key.size() - sep - 1);

            char line[256];
            int len = snprintf(line, sizeof(line), "%s\t%s\t%s\t%u\n",
                level, a.c_str(), b.c_str(), count);
            PQputCopyData(conn, line, len);
            ++written;
        }

        PQputCopyEnd(conn, nullptr);

        // Consume result
        PGresult* endRes = PQgetResult(conn);
        bool ok = (PQresultStatus(endRes) == PGRES_COMMAND_OK);
        PQclear(endRes);

        if (ok)
        {
            PQexec(conn, "COMMIT");
            fprintf(stderr, "[HCPBondCompiler] Saved %zu %s bonds to hcp_temp\n",
                written, level);
        }
        else
        {
            fprintf(stderr, "[HCPBondCompiler] Save COPY failed: %s\n",
                PQerrorMessage(conn));
            PQexec(conn, "ROLLBACK");
        }
        fflush(stderr);

        PQfinish(conn);
        return ok;
    }

    HCPBondTable LoadBondTable(const char* level, const char* connInfo)
    {
        HCPBondTable table;

        PGconn* conn = PQconnectdb(connInfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Load: Postgres connect failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return table;
        }

        const char* params[] = { level };
        PGresult* res = PQexecParams(conn,
            "SELECT elem_a, elem_b, count FROM bond_aggregates WHERE level = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] Load: query failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQclear(res);
            PQfinish(conn);
            return table;
        }

        int nrows = PQntuples(res);
        for (int r = 0; r < nrows; ++r)
        {
            AZStd::string a(PQgetvalue(res, r, 0));
            AZStd::string b(PQgetvalue(res, r, 1));
            AZ::u32 count = static_cast<AZ::u32>(atoi(PQgetvalue(res, r, 2)));
            table.AddBond(a, b, count);
        }

        PQclear(res);
        PQfinish(conn);

        if (nrows > 0)
        {
            fprintf(stderr, "[HCPBondCompiler] Loaded %d %s bonds from hcp_temp\n",
                nrows, level);
            fflush(stderr);
            table.LogStats(level);
        }

        return table;
    }

    // ---- Document PBM persistence ----

    bool SaveDocPBM(const AZStd::string& docName, const PBMData& pbm, const char* connInfo)
    {
        PGconn* conn = PQconnectdb(connInfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[HCPBondCompiler] SaveDocPBM: connect failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQfinish(conn);
            return false;
        }

        PQexec(conn, "BEGIN");

        // Clear existing PBM for this doc
        {
            const char* params[] = { docName.c_str() };
            PGresult* res = PQexecParams(conn,
                "DELETE FROM doc_pbm WHERE doc_name = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        // COPY in all bonds
        PGresult* copyRes = PQexec(conn,
            "COPY doc_pbm (doc_name, token_a, token_b, count) FROM STDIN");
        if (PQresultStatus(copyRes) != PGRES_COPY_IN)
        {
            fprintf(stderr, "[HCPBondCompiler] SaveDocPBM: COPY failed: %s\n",
                PQerrorMessage(conn));
            fflush(stderr);
            PQclear(copyRes);
            PQexec(conn, "ROLLBACK");
            PQfinish(conn);
            return false;
        }
        PQclear(copyRes);

        for (const auto& bond : pbm.bonds)
        {
            char line[512];
            int len = snprintf(line, sizeof(line), "%s\t%s\t%s\t%d\n",
                docName.c_str(), bond.tokenA.c_str(), bond.tokenB.c_str(), bond.count);
            PQputCopyData(conn, line, len);
        }

        PQputCopyEnd(conn, nullptr);
        PGresult* endRes = PQgetResult(conn);
        bool ok = (PQresultStatus(endRes) == PGRES_COMMAND_OK);
        PQclear(endRes);

        if (ok)
        {
            PQexec(conn, "COMMIT");
            fprintf(stderr, "[HCPBondCompiler] Saved %zu PBM bonds for '%s' to hcp_temp\n",
                pbm.bonds.size(), docName.c_str());
        }
        else
        {
            fprintf(stderr, "[HCPBondCompiler] SaveDocPBM COPY failed: %s\n",
                PQerrorMessage(conn));
            PQexec(conn, "ROLLBACK");
        }
        fflush(stderr);

        PQfinish(conn);
        return ok;
    }

} // namespace HCPEngine
