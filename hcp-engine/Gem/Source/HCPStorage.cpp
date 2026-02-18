#include "HCPStorage.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    static constexpr const char* PBM_DB_CONNINFO = "dbname=hcp_fic_pbm user=hcp password=hcp_dev host=localhost port=5432";

    // Base-50 encoding for PBM address segments
    static const char BASE50_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx";

    static AZStd::string EncodePair(int value)
    {
        if (value < 0 || value >= 2500)
        {
            return "AA";
        }
        char hi = BASE50_CHARS[value / 50];
        char lo = BASE50_CHARS[value % 50];
        char buf[3] = {hi, lo, '\0'};
        return AZStd::string(buf);
    }

    static void SplitTokenId(const AZStd::string& tokenId, AZStd::string parts[5])
    {
        int idx = 0;
        size_t start = 0;
        for (size_t i = 0; i <= tokenId.size() && idx < 5; ++i)
        {
            if (i == tokenId.size() || tokenId[i] == '.')
            {
                parts[idx++] = AZStd::string(tokenId.data() + start, i - start);
                start = i + 1;
            }
        }
    }

    AZStd::string StorePBM(
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const PBMData& pbmData)
    {
        PGconn* conn = PQconnectdb(PBM_DB_CONNINFO);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            AZLOG_ERROR("HCPStorage: Failed to connect to hcp_fic_pbm: %s", PQerrorMessage(conn));
            PQfinish(conn);
            return {};
        }

        PQexec(conn, "BEGIN");

        AZStd::string ns = "vA";
        AZStd::string p2 = "AB";
        AZStd::string p3 = centuryCode;

        // Get next PBM address (auto-increment)
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PQexecParams(conn,
                "INSERT INTO pbm_counters (ns, p2, p3, next_value) "
                "VALUES ($1, $2, $3, 1) ON CONFLICT (ns, p2, p3) DO NOTHING",
                3, nullptr, params, nullptr, nullptr, 0);
        }

        int seq = 0;
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PGresult* res = PQexecParams(conn,
                "UPDATE pbm_counters SET next_value = next_value + 1 "
                "WHERE ns = $1 AND p2 = $2 AND p3 = $3 RETURNING next_value - 1",
                3, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                seq = atoi(PQgetvalue(res, 0, 0));
            }
            PQclear(res);
        }

        AZStd::string p4 = EncodePair(seq / 2500);
        AZStd::string p5 = EncodePair(seq % 2500);

        // Insert document
        int docPk = 0;
        AZStd::string docId;
        {
            const char* params[] = {
                ns.c_str(), p2.c_str(), p3.c_str(), p4.c_str(), p5.c_str(),
                docName.c_str(), "book", "novel",
                pbmData.firstFpbA.c_str(), pbmData.firstFpbB.c_str(), "{}"
            };
            PGresult* res = PQexecParams(conn,
                "INSERT INTO pbm_documents (ns, p2, p3, p4, p5, name, category, "
                "subcategory, first_fpb_a, first_fpb_b, metadata) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11::jsonb) "
                "RETURNING id, doc_id",
                11, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                docPk = atoi(PQgetvalue(res, 0, 0));
                docId = PQgetvalue(res, 0, 1);
            }
            PQclear(res);
        }

        if (docId.empty())
        {
            AZLOG_ERROR("HCPStorage: Failed to insert document");
            PQexec(conn, "ROLLBACK");
            PQfinish(conn);
            return {};
        }

        // Insert starters and bonds
        // Build starter lookup: tokenA -> starter_pk
        AZStd::unordered_map<AZStd::string, int> starterPks;
        AZStd::string docPkStr = AZStd::to_string(docPk);

        for (const auto& bond : pbmData.bonds)
        {
            if (starterPks.find(bond.tokenA) != starterPks.end())
            {
                continue;
            }

            AZStd::string aParts[5];
            SplitTokenId(bond.tokenA, aParts);

            const char* params[] = {
                docPkStr.c_str(),
                aParts[0].c_str(), aParts[1].c_str(), aParts[2].c_str(),
                aParts[3].c_str(), aParts[4].empty() ? nullptr : aParts[4].c_str()
            };
            PGresult* res = PQexecParams(conn,
                "INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5) "
                "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
                6, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                starterPks[bond.tokenA] = atoi(PQgetvalue(res, 0, 0));
            }
            PQclear(res);
        }

        // Route bonds to correct subtables
        for (const auto& bond : pbmData.bonds)
        {
            auto starterIt = starterPks.find(bond.tokenA);
            if (starterIt == starterPks.end()) continue;

            AZStd::string starterId = AZStd::to_string(starterIt->second);
            AZStd::string countStr = AZStd::to_string(bond.count);

            AZStd::string bParts[5];
            SplitTokenId(bond.tokenB, bParts);

            if (bParts[0] == "AB" && bParts[1] == "AB")
            {
                // Word bond
                const char* params[] = {
                    starterId.c_str(), bParts[2].c_str(), bParts[3].c_str(),
                    bParts[4].empty() ? nullptr : bParts[4].c_str(), countStr.c_str()
                };
                PGresult* res = PQexecParams(conn,
                    "INSERT INTO pbm_word_bonds (starter_id, b_p3, b_p4, b_p5, count) "
                    "VALUES ($1, $2, $3, $4, $5)",
                    5, nullptr, params, nullptr, nullptr, 0);
                PQclear(res);
            }
            else if (bParts[0] == "AA" && bParts[1] == "AE" && bParts[4].empty())
            {
                // Marker bond (no p5)
                const char* params[] = {
                    starterId.c_str(), bParts[2].c_str(), bParts[3].c_str(), countStr.c_str()
                };
                PGresult* res = PQexecParams(conn,
                    "INSERT INTO pbm_marker_bonds (starter_id, b_p3, b_p4, count) "
                    "VALUES ($1, $2, $3, $4)",
                    4, nullptr, params, nullptr, nullptr, 0);
                PQclear(res);
            }
            else
            {
                // Character bond (or fallback)
                const char* params[] = {
                    starterId.c_str(), bParts[1].c_str(), bParts[2].c_str(),
                    bParts[3].c_str(), bParts[4].empty() ? nullptr : bParts[4].c_str(),
                    countStr.c_str()
                };
                PGresult* res = PQexecParams(conn,
                    "INSERT INTO pbm_char_bonds (starter_id, b_p2, b_p3, b_p4, b_p5, count) "
                    "VALUES ($1, $2, $3, $4, $5, $6)",
                    6, nullptr, params, nullptr, nullptr, 0);
                PQclear(res);
            }
        }

        PQexec(conn, "COMMIT");
        PQfinish(conn);

        AZLOG_INFO("HCPStorage: Stored PBM %s with %zu bonds", docId.c_str(), pbmData.bonds.size());
        return docId;
    }

    PBMData LoadPBM(const AZStd::string& docId)
    {
        PBMData result;

        PGconn* conn = PQconnectdb(PBM_DB_CONNINFO);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            AZLOG_ERROR("HCPStorage: Failed to connect to hcp_fic_pbm: %s", PQerrorMessage(conn));
            PQfinish(conn);
            return result;
        }

        // Get document
        int docPk = 0;
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(conn,
                "SELECT id, first_fpb_a, first_fpb_b FROM pbm_documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPStorage: Document %s not found", docId.c_str());
                PQclear(res);
                PQfinish(conn);
                return result;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            result.firstFpbA = PQgetvalue(res, 0, 1);
            result.firstFpbB = PQgetvalue(res, 0, 2);
            PQclear(res);
        }

        // Load all bonds via UNION ALL
        AZStd::string docPkStr = AZStd::to_string(docPk);
        {
            const char* params[] = { docPkStr.c_str(), docPkStr.c_str(), docPkStr.c_str() };
            PGresult* res = PQexecParams(conn,
                "SELECT s.token_a_id, "
                "  'AB.AB.' || wb.b_p3 || '.' || wb.b_p4 || COALESCE('.' || wb.b_p5, '') AS token_b_id, "
                "  wb.count "
                "FROM pbm_word_bonds wb "
                "JOIN pbm_starters s ON s.id = wb.starter_id "
                "WHERE s.doc_id = $1 "
                "UNION ALL "
                "SELECT s.token_a_id, "
                "  'AA.' || cb.b_p2 || '.' || cb.b_p3 || '.' || cb.b_p4 || COALESCE('.' || cb.b_p5, '') AS token_b_id, "
                "  cb.count "
                "FROM pbm_char_bonds cb "
                "JOIN pbm_starters s ON s.id = cb.starter_id "
                "WHERE s.doc_id = $2 "
                "UNION ALL "
                "SELECT s.token_a_id, "
                "  'AA.AE.' || mb.b_p3 || '.' || mb.b_p4 AS token_b_id, "
                "  mb.count "
                "FROM pbm_marker_bonds mb "
                "JOIN pbm_starters s ON s.id = mb.starter_id "
                "WHERE s.doc_id = $3",
                3, nullptr, params, nullptr, nullptr, 0);

            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                int rows = PQntuples(res);
                result.bonds.reserve(rows);
                for (int i = 0; i < rows; ++i)
                {
                    Bond bond;
                    bond.tokenA = PQgetvalue(res, i, 0);
                    bond.tokenB = PQgetvalue(res, i, 1);
                    bond.count = atoi(PQgetvalue(res, i, 2));
                    result.bonds.push_back(bond);
                    result.totalPairs += bond.count;
                }
            }
            PQclear(res);
        }

        PQfinish(conn);

        AZLOG_INFO("HCPStorage: Loaded PBM %s with %zu bonds", docId.c_str(), result.bonds.size());
        return result;
    }

} // namespace HCPEngine
