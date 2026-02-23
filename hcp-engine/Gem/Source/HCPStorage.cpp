#include "HCPStorage.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    static constexpr const char* DEFAULT_CONNINFO =
        "dbname=hcp_fic_pbm user=hcp password=hcp_dev host=localhost port=5432";

    // Base-50 alphabet for position encoding
    static const char B50[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx";

    static int DecodeB50Char(char c)
    {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'x') return 26 + (c - 'a');
        return 0;
    }

    // Base-50 pair encoding (value 0-2499 → 2 chars)
    static void EncodePair(int value, char out[2])
    {
        if (value < 0) value = 0;
        if (value >= 2500) value = 2499;
        out[0] = B50[value / 50];
        out[1] = B50[value % 50];
    }

    static AZStd::string EncodePairStr(int value)
    {
        char buf[3];
        EncodePair(value, buf);
        buf[2] = '\0';
        return AZStd::string(buf);
    }

    // ---- Position encoding: 4 chars per position (2 composable base-50 pairs) ----
    // Position = hi_pair * 2500 + lo_pair
    // Max: 2499 * 2500 + 2499 = 6,249,999

    AZStd::string EncodePositions(const AZStd::vector<AZ::u32>& positions)
    {
        AZStd::string result;
        result.reserve(positions.size() * 4);
        for (AZ::u32 pos : positions)
        {
            AZ::u32 hi = pos / 2500;
            AZ::u32 lo = pos % 2500;
            result += B50[hi / 50];
            result += B50[hi % 50];
            result += B50[lo / 50];
            result += B50[lo % 50];
        }
        return result;
    }

    AZStd::vector<AZ::u32> DecodePositions(const AZStd::string& encoded)
    {
        AZStd::vector<AZ::u32> result;
        result.reserve(encoded.size() / 4);
        for (size_t i = 0; i + 3 < encoded.size(); i += 4)
        {
            int hh = DecodeB50Char(encoded[i]);
            int hl = DecodeB50Char(encoded[i + 1]);
            int lh = DecodeB50Char(encoded[i + 2]);
            int ll = DecodeB50Char(encoded[i + 3]);
            AZ::u32 hi = static_cast<AZ::u32>(hh * 50 + hl);
            AZ::u32 lo = static_cast<AZ::u32>(lh * 50 + ll);
            result.push_back(hi * 2500 + lo);
        }
        return result;
    }

    // Split "AB.AB.CD.AH.xN" → parts[0]="AB", parts[1]="AB", parts[2]="CD", parts[3]="AH", parts[4]="xN"
    // Only splits on the first 4 dots — everything after the 4th dot goes into parts[4].
    // This handles var tokens like "AA.AE.AF.AA.AC 1.E.8" where form text contains dots.
    static void SplitTokenId(const AZStd::string& tokenId, AZStd::string parts[5])
    {
        int idx = 0;
        size_t start = 0;
        for (size_t i = 0; i < tokenId.size() && idx < 4; ++i)
        {
            if (tokenId[i] == '.')
            {
                parts[idx++] = AZStd::string(tokenId.data() + start, i - start);
                start = i + 1;
            }
        }
        // Remainder (including any further dots) goes into the last part
        if (start <= tokenId.size())
        {
            parts[idx] = AZStd::string(tokenId.data() + start, tokenId.size() - start);
        }
    }

    // ---- HCPWriteKernel ----

    HCPWriteKernel::~HCPWriteKernel()
    {
        Disconnect();
    }

    bool HCPWriteKernel::Connect(const char* connInfo)
    {
        if (m_conn)
        {
            Disconnect();
        }

        m_conn = PQconnectdb(connInfo ? connInfo : DEFAULT_CONNINFO);
        if (PQstatus(m_conn) != CONNECTION_OK)
        {
            AZLOG_ERROR("HCPWriteKernel: Connection failed: %s", PQerrorMessage(m_conn));
            PQfinish(m_conn);
            m_conn = nullptr;
            return false;
        }

        AZLOG_INFO("HCPWriteKernel: Connected to %s", connInfo ? connInfo : DEFAULT_CONNINFO);
        return true;
    }

    void HCPWriteKernel::Disconnect()
    {
        if (m_conn)
        {
            PQfinish(m_conn);
            m_conn = nullptr;
        }
    }

    int HCPWriteKernel::NextDocSequence(
        const AZStd::string& ns,
        const AZStd::string& p2,
        const AZStd::string& p3)
    {
        // Ensure counter row exists
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "INSERT INTO doc_counters (ns, p2, p3, next_value) "
                "VALUES ($1, $2, $3, 1) ON CONFLICT (ns, p2, p3) DO NOTHING",
                3, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        // Atomic increment and return
        const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "UPDATE doc_counters SET next_value = next_value + 1 "
            "WHERE ns = $1 AND p2 = $2 AND p3 = $3 RETURNING next_value - 1",
            3, nullptr, params, nullptr, nullptr, 0);

        int seq = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            seq = atoi(PQgetvalue(res, 0, 0));
        }
        PQclear(res);
        return seq;
    }

    bool HCPWriteKernel::InsertDocument(
        const AZStd::string& ns,
        const AZStd::string& p2,
        const AZStd::string& p3,
        const AZStd::string& p4,
        const AZStd::string& p5,
        const AZStd::string& docName,
        AZ::u32 totalSlots,
        size_t uniqueTokens,
        int& outPk,
        AZStd::string& outDocId)
    {
        AZStd::string slotsStr = AZStd::to_string(totalSlots);
        AZStd::string uniqueStr = AZStd::to_string(uniqueTokens);

        const char* params[] = {
            ns.c_str(), p2.c_str(), p3.c_str(), p4.c_str(), p5.c_str(),
            docName.c_str(),
            slotsStr.c_str(), uniqueStr.c_str(), "{}"
        };
        PGresult* res = PQexecParams(m_conn,
            "INSERT INTO documents (ns, p2, p3, p4, p5, name, "
            "total_slots, unique_tokens, metadata) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7::integer, $8::integer, $9::jsonb) "
            "RETURNING doc_id",
            9, nullptr, params, nullptr, nullptr, 0);

        bool ok = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
        if (ok)
        {
            outPk = 0; // No integer PK in new schema — doc_id is the key
            outDocId = PQgetvalue(res, 0, 0);
        }
        else
        {
            AZLOG_ERROR("HCPWriteKernel: InsertDocument failed: %s", PQerrorMessage(m_conn));
        }
        PQclear(res);
        return ok;
    }

    bool HCPWriteKernel::InsertTokenPositions(
        const AZStd::string& docId,
        const AZStd::string& tokenId,
        const AZStd::vector<AZ::u32>& positions)
    {
        AZStd::string parts[5];
        SplitTokenId(tokenId, parts);

        AZStd::string posEncoded = EncodePositions(positions);

        if (parts[0] == "AB" && parts[1] == "AB")
        {
            // Word token → doc_word_positions (store p3, p4, p5)
            const char* params[] = {
                docId.c_str(),
                parts[2].c_str(), parts[3].c_str(),
                parts[4].empty() ? nullptr : parts[4].c_str(),
                posEncoded.c_str()
            };
            PGresult* res = PQexecParams(m_conn,
                "INSERT INTO doc_word_positions (doc_id, t_p3, t_p4, t_p5, positions) "
                "VALUES ($1, $2, $3, $4, $5)",
                5, nullptr, params, nullptr, nullptr, 0);
            bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
            if (!ok) AZLOG_ERROR("HCPWriteKernel: word insert failed: %s", PQerrorMessage(m_conn));
            PQclear(res);
            return ok;
        }
        else if (parts[0] == "AA" && parts[1] == "AE")
        {
            // Marker token → doc_marker_positions (store p3, p4)
            const char* params[] = {
                docId.c_str(),
                parts[2].c_str(), parts[3].c_str(),
                posEncoded.c_str()
            };
            PGresult* res = PQexecParams(m_conn,
                "INSERT INTO doc_marker_positions (doc_id, t_p3, t_p4, positions) "
                "VALUES ($1, $2, $3, $4)",
                4, nullptr, params, nullptr, nullptr, 0);
            bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
            if (!ok) AZLOG_ERROR("HCPWriteKernel: marker insert failed: %s", PQerrorMessage(m_conn));
            PQclear(res);
            return ok;
        }
        else
        {
            // Character/punctuation token → doc_char_positions (store p2, p3, p4, p5)
            const char* params[] = {
                docId.c_str(),
                parts[1].c_str(), parts[2].c_str(), parts[3].c_str(),
                parts[4].empty() ? nullptr : parts[4].c_str(),
                posEncoded.c_str()
            };
            PGresult* res = PQexecParams(m_conn,
                "INSERT INTO doc_char_positions (doc_id, t_p2, t_p3, t_p4, t_p5, positions) "
                "VALUES ($1, $2, $3, $4, $5, $6)",
                6, nullptr, params, nullptr, nullptr, 0);
            bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
            if (!ok) AZLOG_ERROR("HCPWriteKernel: char insert failed: %s", PQerrorMessage(m_conn));
            PQclear(res);
            return ok;
        }
    }

    AZStd::string HCPWriteKernel::StorePositionMap(
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const PositionMap& posMap,
        const TokenStream& stream)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return {};
        }

        PQexec(m_conn, "BEGIN");

        // Allocate document address
        AZStd::string ns = "vA";    // fiction namespace
        AZStd::string p2 = "AB";    // English
        AZStd::string p3 = centuryCode;

        int seq = NextDocSequence(ns, p2, p3);
        AZStd::string p4 = EncodePairStr(seq / 2500);
        AZStd::string p5 = EncodePairStr(seq % 2500);

        // Insert document
        int docPk = 0;
        AZStd::string docId;
        if (!InsertDocument(ns, p2, p3, p4, p5, docName,
            stream.totalSlots, posMap.uniqueTokens, docPk, docId))
        {
            AZLOG_ERROR("HCPWriteKernel: Failed to insert document");
            PQexec(m_conn, "ROLLBACK");
            return {};
        }

        // Insert all token positions
        for (const auto& entry : posMap.entries)
        {
            if (!InsertTokenPositions(docId, entry.tokenId, entry.positions))
            {
                PQexec(m_conn, "ROLLBACK");
                return {};
            }
        }

        PQexec(m_conn, "COMMIT");

        AZLOG_INFO("HCPWriteKernel: Stored %s — %zu unique tokens, %u total slots",
            docId.c_str(), posMap.uniqueTokens, stream.totalSlots);
        return docId;
    }

    PositionMap HCPWriteKernel::LoadPositionMap(
        const AZStd::string& docId,
        TokenStream& outStream)
    {
        PositionMap result;

        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return result;
        }

        // Get document metadata
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT total_slots, unique_tokens FROM documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPWriteKernel: Document %s not found", docId.c_str());
                PQclear(res);
                return result;
            }
            outStream.totalSlots = static_cast<AZ::u32>(atoi(PQgetvalue(res, 0, 0)));
            result.totalTokens = outStream.totalSlots;
            PQclear(res);
        }

        // Load word positions — reconstruct token_id as AB.AB.t_p3.t_p4[.t_p5]
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT 'AB.AB.' || t_p3 || '.' || t_p4 || COALESCE('.' || t_p5, ''), positions "
                "FROM doc_word_positions WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    TokenPositions tp;
                    tp.tokenId = PQgetvalue(res, i, 0);
                    tp.positions = DecodePositions(AZStd::string(PQgetvalue(res, i, 1)));
                    result.entries.push_back(AZStd::move(tp));
                }
            }
            PQclear(res);
        }

        // Load char positions — reconstruct token_id as AA.t_p2.t_p3.t_p4[.t_p5]
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT 'AA.' || t_p2 || '.' || t_p3 || '.' || t_p4 || COALESCE('.' || t_p5, ''), positions "
                "FROM doc_char_positions WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    TokenPositions tp;
                    tp.tokenId = PQgetvalue(res, i, 0);
                    tp.positions = DecodePositions(AZStd::string(PQgetvalue(res, i, 1)));
                    result.entries.push_back(AZStd::move(tp));
                }
            }
            PQclear(res);
        }

        // Load marker positions — reconstruct token_id as AA.AE.t_p3.t_p4
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT 'AA.AE.' || t_p3 || '.' || t_p4, positions "
                "FROM doc_marker_positions WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    TokenPositions tp;
                    tp.tokenId = PQgetvalue(res, i, 0);
                    tp.positions = DecodePositions(AZStd::string(PQgetvalue(res, i, 1)));
                    result.entries.push_back(AZStd::move(tp));
                }
            }
            PQclear(res);
        }

        // Load var positions
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT var_id, positions FROM doc_var_positions WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    TokenPositions tp;
                    tp.tokenId = PQgetvalue(res, i, 0);
                    tp.positions = DecodePositions(AZStd::string(PQgetvalue(res, i, 1)));
                    result.entries.push_back(AZStd::move(tp));
                }
            }
            PQclear(res);
        }

        result.uniqueTokens = result.entries.size();

        AZLOG_INFO("HCPWriteKernel: Loaded %s — %zu unique tokens, %u total slots",
            docId.c_str(), result.uniqueTokens, outStream.totalSlots);
        return result;
    }

    AZStd::string HCPWriteKernel::StorePBM(
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const PBMData& pbmData)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return {};
        }

        if (pbmData.bonds.empty())
        {
            AZLOG_ERROR("HCPWriteKernel: Empty PBM data");
            return {};
        }

        PQexec(m_conn, "BEGIN");

        // Document namespace: vA.AB.<century>.<seq_hi>.<seq_lo>
        AZStd::string ns = "vA";
        AZStd::string p2 = "AB";
        AZStd::string p3 = centuryCode;

        // Next sequence number for this namespace path
        int seq = 0;
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT COUNT(*) FROM pbm_documents WHERE ns = $1 AND p2 = $2 AND p3 = $3",
                3, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                seq = atoi(PQgetvalue(res, 0, 0));
            }
            PQclear(res);
        }

        AZStd::string p4 = EncodePairStr(seq / 2500);
        AZStd::string p5 = EncodePairStr(seq % 2500);

        // Insert document
        int docPk = 0;
        AZStd::string docId;
        {
            const char* params[] = {
                ns.c_str(), p2.c_str(), p3.c_str(), p4.c_str(), p5.c_str(),
                docName.c_str(),
                pbmData.firstFpbA.c_str(), pbmData.firstFpbB.c_str()
            };
            PGresult* res = PQexecParams(m_conn,
                "INSERT INTO pbm_documents (ns, p2, p3, p4, p5, name, first_fpb_a, first_fpb_b) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
                "RETURNING id, doc_id",
                8, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                fprintf(stderr, "[HCPStorage] StorePBM: insert doc failed: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
                PQclear(res);
                PQexec(m_conn, "ROLLBACK");
                return {};
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            docId = PQgetvalue(res, 0, 1);
            PQclear(res);
        }

        fprintf(stderr, "[HCPStorage] StorePBM: doc '%s' -> %s (pk=%d)\n",
            docName.c_str(), docId.c_str(), docPk);
        fflush(stderr);

        // Group bonds by A-side token
        AZStd::unordered_map<AZStd::string, AZStd::vector<const Bond*>> bondsByA;
        for (const auto& bond : pbmData.bonds)
        {
            bondsByA[bond.tokenA].push_back(&bond);
        }

        size_t wordBonds = 0, charBonds = 0, markerBonds = 0, skippedBonds = 0;
        AZStd::string docPkStr = AZStd::to_string(docPk);

        for (const auto& [tokenA, bonds] : bondsByA)
        {
            // Parse A-side token ID
            AZStd::string aParts[5];
            SplitTokenId(tokenA, aParts);

            // Insert starter row
            int starterId = 0;
            {
                const char* params[] = {
                    docPkStr.c_str(),
                    aParts[0].c_str(), aParts[1].c_str(),
                    aParts[2].c_str(), aParts[3].c_str(), aParts[4].c_str()
                };
                PGresult* res = PQexecParams(m_conn,
                    "INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5) "
                    "VALUES ($1::integer, $2, $3, $4, $5, $6) "
                    "RETURNING id",
                    6, nullptr, params, nullptr, nullptr, 0);
                if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
                {
                    fprintf(stderr, "[HCPStorage] StorePBM: insert starter failed for '%s': %s\n",
                        tokenA.c_str(), PQerrorMessage(m_conn));
                    fflush(stderr);
                    PQclear(res);
                    continue;
                }
                starterId = atoi(PQgetvalue(res, 0, 0));
                PQclear(res);
            }

            AZStd::string starterIdStr = AZStd::to_string(starterId);

            // Insert each B-side bond into the correct table
            for (const Bond* bond : bonds)
            {
                AZStd::string bParts[5];
                SplitTokenId(bond->tokenB, bParts);
                AZStd::string countStr = AZStd::to_string(bond->count);

                if (bParts[0] == "AB" && bParts[1] == "AB")
                {
                    // Word bond: b_p3, b_p4, b_p5
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[2].c_str(), bParts[3].c_str(), bParts[4].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(m_conn,
                        "INSERT INTO pbm_word_bonds (starter_id, b_p3, b_p4, b_p5, count) "
                        "VALUES ($1::integer, $2, $3, $4, $5::integer) "
                        "ON CONFLICT (starter_id, b_p3, b_p4, b_p5) "
                        "DO UPDATE SET count = pbm_word_bonds.count + EXCLUDED.count",
                        5, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPStorage] word bond insert failed: %s\n",
                            PQerrorMessage(m_conn));
                        fflush(stderr);
                    }
                    else
                    {
                        ++wordBonds;
                    }
                    PQclear(res);
                }
                else if (bParts[0] == "AA" && bParts[1] != "AE")
                {
                    // Char bond: b_p2, b_p3, b_p4, b_p5
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[1].c_str(), bParts[2].c_str(), bParts[3].c_str(), bParts[4].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(m_conn,
                        "INSERT INTO pbm_char_bonds (starter_id, b_p2, b_p3, b_p4, b_p5, count) "
                        "VALUES ($1::integer, $2, $3, $4, $5, $6::integer) "
                        "ON CONFLICT (starter_id, b_p2, b_p3, b_p4, b_p5) "
                        "DO UPDATE SET count = pbm_char_bonds.count + EXCLUDED.count",
                        6, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPStorage] char bond insert failed: %s\n",
                            PQerrorMessage(m_conn));
                        fflush(stderr);
                    }
                    else
                    {
                        ++charBonds;
                    }
                    PQclear(res);
                }
                else if (bParts[0] == "AA" && bParts[1] == "AE" && bParts[4].empty())
                {
                    // Marker bond (4-part token): b_p3, b_p4
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[2].c_str(), bParts[3].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(m_conn,
                        "INSERT INTO pbm_marker_bonds (starter_id, b_p3, b_p4, count) "
                        "VALUES ($1::integer, $2, $3, $4::integer) "
                        "ON CONFLICT (starter_id, b_p3, b_p4) "
                        "DO UPDATE SET count = pbm_marker_bonds.count + EXCLUDED.count",
                        4, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPStorage] marker bond insert failed: %s\n",
                            PQerrorMessage(m_conn));
                        fflush(stderr);
                    }
                    else
                    {
                        ++markerBonds;
                    }
                    PQclear(res);
                }
                else
                {
                    // Unresolved var or unknown token type — skip for now
                    ++skippedBonds;
                }
            }
        }

        PGresult* commitRes = PQexec(m_conn, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        if (ok)
        {
            fprintf(stderr, "[HCPStorage] StorePBM: '%s' -> %s — %zu starters, "
                "%zu word bonds, %zu char bonds, %zu marker bonds",
                docName.c_str(), docId.c_str(), bondsByA.size(),
                wordBonds, charBonds, markerBonds);
            if (skippedBonds > 0)
            {
                fprintf(stderr, ", %zu skipped (unresolved vars)", skippedBonds);
            }
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "[HCPStorage] StorePBM: COMMIT failed: %s\n",
                PQerrorMessage(m_conn));
            fflush(stderr);
            return {};
        }

        return docId;
    }

    bool HCPWriteKernel::StoreMetadata(
        const AZStd::string& docId,
        const AZStd::string& key,
        const AZStd::string& value)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return false;
        }

        const char* params[] = { docId.c_str(), key.c_str(), value.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "UPDATE documents SET metadata = metadata || jsonb_build_object($2, $3) "
            "WHERE doc_id = $1",
            3, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
        return ok;
    }

} // namespace HCPEngine
