#include "HCPStorage.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_set.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    static constexpr const char* DEFAULT_CONNINFO =
        "dbname=hcp_fic_pbm user=hcp password=hcp_dev host=localhost port=5432";

    // Var request token prefix — must match HCPVocabulary.h VAR_REQUEST
    static constexpr const char* VAR_PREFIX = "AA.AE.AF.AA.AC";
    static constexpr size_t VAR_PREFIX_LEN = 14;  // strlen("AA.AE.AF.AA.AC")

    // Check if a token is a var request (prefix + space + surface form)
    static bool IsVarToken(const AZStd::string& token)
    {
        return token.size() > VAR_PREFIX_LEN + 1 &&
               token.starts_with(VAR_PREFIX) &&
               token[VAR_PREFIX_LEN] == ' ';
    }

    // Extract the surface form from a var token (everything after "AA.AE.AF.AA.AC ")
    static AZStd::string VarSurface(const AZStd::string& token)
    {
        return AZStd::string(token.data() + VAR_PREFIX_LEN + 1,
                             token.size() - VAR_PREFIX_LEN - 1);
    }

    // Base-50 pair encoding (value 0-2499 → 2 chars)
    static const char B50[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx";

    static AZStd::string EncodePairStr(int value)
    {
        if (value < 0) value = 0;
        if (value >= 2500) value = 2499;
        char buf[3];
        buf[0] = B50[value / 50];
        buf[1] = B50[value % 50];
        buf[2] = '\0';
        return AZStd::string(buf);
    }

    // Base-50 position encoding: position → 4 chars (two pairs)
    // pair1 = position / 2500, pair2 = position % 2500
    // Max position: 2499 * 2500 + 2499 = 6,249,999
    static void EncodePosition(int position, char out[4])
    {
        int pair1 = position / 2500;
        int pair2 = position % 2500;
        out[0] = B50[pair1 / 50];
        out[1] = B50[pair1 % 50];
        out[2] = B50[pair2 / 50];
        out[3] = B50[pair2 % 50];
    }

    static int DecodeB50Char(char c)
    {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'x') return 26 + (c - 'a');
        return 0;
    }

    static int DecodePosition(const char* p)
    {
        int pair1 = DecodeB50Char(p[0]) * 50 + DecodeB50Char(p[1]);
        int pair2 = DecodeB50Char(p[2]) * 50 + DecodeB50Char(p[3]);
        return pair1 * 2500 + pair2;
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

        m_lastDocPk = docPk;

        fprintf(stderr, "[HCPStorage] StorePBM: doc '%s' -> %s (pk=%d)\n",
            docName.c_str(), docId.c_str(), docPk);
        fflush(stderr);

        // ---- Mint document-local vars (decimal pair IDs) ----
        // Scan all bonds for var tokens, mint a decimal ID for each unique surface form.
        // Map: full var token string → short decimal var_id (e.g. "01.03")
        // Application-side minting: no stored procedure dependency.
        AZStd::unordered_map<AZStd::string, AZStd::string> varToDecimal;
        {
            // Seed decimal counter from any existing docvars for this document
            int nextDecimal = 0;
            {
                AZStd::string pkStr = AZStd::to_string(docPk);
                const char* params[] = { pkStr.c_str() };
                PGresult* res = PQexecParams(m_conn,
                    "SELECT COALESCE(MAX("
                    "  CAST(SPLIT_PART(var_id, '.', 1) AS INTEGER) * 100 + "
                    "  CAST(SPLIT_PART(var_id, '.', 2) AS INTEGER)"
                    "), -1) FROM pbm_docvars WHERE doc_id = $1",
                    1, nullptr, params, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                {
                    nextDecimal = atoi(PQgetvalue(res, 0, 0)) + 1;
                }
                PQclear(res);
            }

            AZStd::string docPkStr2 = AZStd::to_string(docPk);
            AZStd::unordered_map<AZStd::string, AZStd::string> surfaceSeen;  // surface → var_id
            for (const auto& bond : pbmData.bonds)
            {
                for (const AZStd::string* tok : { &bond.tokenA, &bond.tokenB })
                {
                    if (!IsVarToken(*tok) || varToDecimal.count(*tok))
                        continue;

                    AZStd::string surface = VarSurface(*tok);
                    auto it = surfaceSeen.find(surface);
                    if (it != surfaceSeen.end())
                    {
                        varToDecimal[*tok] = it->second;
                        continue;
                    }

                    // Check if this surface already has a docvar for this document
                    const char* checkParams[] = { docPkStr2.c_str(), surface.c_str() };
                    PGresult* res = PQexecParams(m_conn,
                        "SELECT var_id FROM pbm_docvars WHERE doc_id = $1 AND surface = $2",
                        2, nullptr, checkParams, nullptr, nullptr, 0);
                    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                    {
                        AZStd::string varId = PQgetvalue(res, 0, 0);
                        varToDecimal[*tok] = varId;
                        surfaceSeen[surface] = varId;
                        PQclear(res);
                        continue;
                    }
                    PQclear(res);

                    // Mint new decimal var_id
                    char varIdBuf[8];
                    snprintf(varIdBuf, sizeof(varIdBuf), "%02d.%02d",
                        nextDecimal / 100, nextDecimal % 100);
                    AZStd::string varId(varIdBuf);
                    ++nextDecimal;

                    // Insert into pbm_docvars
                    const char* insParams[] = { docPkStr2.c_str(), varIdBuf, surface.c_str() };
                    res = PQexecParams(m_conn,
                        "INSERT INTO pbm_docvars (doc_id, var_id, surface) "
                        "VALUES ($1::integer, $2, $3)",
                        3, nullptr, insParams, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPStorage] docvar INSERT failed for '%s': %s\n",
                            surface.c_str(), PQerrorMessage(m_conn));
                        fflush(stderr);
                    }
                    else
                    {
                        varToDecimal[*tok] = varId;
                        surfaceSeen[surface] = varId;
                    }
                    PQclear(res);
                }
            }
        }

        if (!varToDecimal.empty())
        {
            fprintf(stderr, "[HCPStorage] StorePBM: minted %zu document-local vars\n",
                varToDecimal.size());
            fflush(stderr);
        }

        // Group bonds by A-side token
        AZStd::unordered_map<AZStd::string, AZStd::vector<const Bond*>> bondsByA;
        for (const auto& bond : pbmData.bonds)
        {
            bondsByA[bond.tokenA].push_back(&bond);
        }

        size_t wordBonds = 0, charBonds = 0, markerBonds = 0, varBonds = 0;
        AZStd::string docPkStr = AZStd::to_string(docPk);

        for (const auto& [tokenA, bonds] : bondsByA)
        {
            // Insert starter row — var A-sides use zero-padded decimal decomposition
            int starterId = 0;
            {
                AZStd::string aNs, aP2, aP3, aP4, aP5;
                auto varIt = varToDecimal.find(tokenA);
                if (varIt != varToDecimal.end())
                {
                    // Decimal var_id "XX.YY" → zero-padded 5-part: 00.00.00.XX.YY
                    const AZStd::string& vid = varIt->second;
                    size_t dot = vid.find('.');
                    aNs = "00";
                    aP2 = "00";
                    aP3 = "00";
                    aP4 = AZStd::string(vid.data(), dot);
                    aP5 = AZStd::string(vid.data() + dot + 1, vid.size() - dot - 1);
                }
                else
                {
                    AZStd::string aParts[5];
                    SplitTokenId(tokenA, aParts);
                    aNs = aParts[0]; aP2 = aParts[1]; aP3 = aParts[2];
                    aP4 = aParts[3]; aP5 = aParts[4];
                }

                const char* params[] = {
                    docPkStr.c_str(),
                    aNs.c_str(), aP2.c_str(), aP3.c_str(), aP4.c_str(), aP5.c_str()
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

            // Insert each B-side bond into the correct subtable
            for (const Bond* bond : bonds)
            {
                AZStd::string countStr = AZStd::to_string(bond->count);

                // Check B-side for var first
                auto varIt = varToDecimal.find(bond->tokenB);
                if (varIt != varToDecimal.end())
                {
                    // Var bond → pbm_var_bonds with short decimal var_id
                    const char* params[] = {
                        starterIdStr.c_str(),
                        varIt->second.c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(m_conn,
                        "INSERT INTO pbm_var_bonds (starter_id, b_var_id, count) "
                        "VALUES ($1::integer, $2, $3::integer) "
                        "ON CONFLICT (starter_id, b_var_id) "
                        "DO UPDATE SET count = pbm_var_bonds.count + EXCLUDED.count",
                        3, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPStorage] var bond insert failed: %s\n",
                            PQerrorMessage(m_conn));
                        fflush(stderr);
                    }
                    else
                    {
                        ++varBonds;
                    }
                    PQclear(res);
                    continue;
                }

                AZStd::string bParts[5];
                SplitTokenId(bond->tokenB, bParts);

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
                // else: truly unknown token type — should not happen with var handling above
            }
        }

        PGresult* commitRes = PQexec(m_conn, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        if (ok)
        {
            fprintf(stderr, "[HCPStorage] StorePBM: '%s' -> %s — %zu starters, "
                "%zu word bonds, %zu char bonds, %zu marker bonds, %zu var bonds\n",
                docName.c_str(), docId.c_str(), bondsByA.size(),
                wordBonds, charBonds, markerBonds, varBonds);
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

    bool HCPWriteKernel::StorePositions(
        int docPk,
        const AZStd::vector<AZStd::string>& tokenIds,
        const AZStd::vector<int>& positions,
        int totalSlots)
    {
        if (!m_conn || tokenIds.size() != positions.size())
        {
            AZLOG_ERROR("HCPWriteKernel::StorePositions: not connected or size mismatch");
            return false;
        }

        PQexec(m_conn, "BEGIN");

        // Group positions by token ID
        AZStd::unordered_map<AZStd::string, AZStd::vector<int>> tokenPositions;
        for (size_t i = 0; i < tokenIds.size(); ++i)
        {
            tokenPositions[tokenIds[i]].push_back(positions[i]);
        }

        // Update total_slots and unique_tokens on pbm_documents
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            AZStd::string slotsStr = AZStd::to_string(totalSlots);
            AZStd::string uniqStr = AZStd::to_string(static_cast<int>(tokenPositions.size()));
            const char* params[] = { slotsStr.c_str(), uniqStr.c_str(), pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "UPDATE pbm_documents SET total_slots = $1::integer, unique_tokens = $2::integer "
                "WHERE id = $3::integer",
                3, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        // Build surface→decimal var_id lookup from pbm_docvars
        AZStd::unordered_map<AZStd::string, AZStd::string> surfaceToVarId;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT var_id, surface FROM pbm_docvars WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    surfaceToVarId[PQgetvalue(res, i, 1)] = PQgetvalue(res, i, 0);
                }
            }
            PQclear(res);
        }

        AZStd::string docPkStr = AZStd::to_string(docPk);
        size_t updated = 0;

        for (const auto& [tokenId, posList] : tokenPositions)
        {
            // Encode positions as base-50 packed string (4 chars per position)
            AZStd::string packed;
            packed.resize(posList.size() * 4);
            for (size_t j = 0; j < posList.size(); ++j)
            {
                EncodePosition(posList[j], packed.data() + j * 4);
            }

            // Determine the token_a_id used in pbm_starters
            // Var tokens: "AA.AE.AF.AA.AC surface" → lookup decimal → "00.00.00.XX.YY"
            // Regular tokens: use as-is
            AZStd::string starterTokenId;
            if (tokenId.starts_with(VAR_PREFIX) && tokenId.size() > VAR_PREFIX_LEN + 1)
            {
                AZStd::string surface = VarSurface(tokenId);
                auto it = surfaceToVarId.find(surface);
                if (it != surfaceToVarId.end())
                {
                    // Decimal var_id "XX.YY" → "00.00.00.XX.YY"
                    const AZStd::string& vid = it->second;
                    size_t dot = vid.find('.');
                    starterTokenId = "00.00.00." +
                        AZStd::string(vid.data(), dot) + "." +
                        AZStd::string(vid.data() + dot + 1, vid.size() - dot - 1);
                }
                else
                {
                    fprintf(stderr, "[HCPStorage] StorePositions: no docvar for surface '%s'\n",
                        surface.c_str());
                    fflush(stderr);
                    continue;
                }
            }
            else
            {
                starterTokenId = tokenId;
            }

            // UPDATE the starter row with packed positions
            const char* params[] = { packed.c_str(), docPkStr.c_str(), starterTokenId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "UPDATE pbm_starters SET positions = $1 "
                "WHERE doc_id = $2::integer AND token_a_id = $3",
                3, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_COMMAND_OK)
            {
                int rows = atoi(PQcmdTuples(res));
                if (rows > 0) ++updated;
                else
                {
                    fprintf(stderr, "[HCPStorage] StorePositions: no starter for token '%s'\n",
                        starterTokenId.c_str());
                    fflush(stderr);
                }
            }
            else
            {
                fprintf(stderr, "[HCPStorage] StorePositions UPDATE failed: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
            }
            PQclear(res);
        }

        PGresult* commitRes = PQexec(m_conn, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        fprintf(stderr, "[HCPStorage] StorePositions: pk=%d — %zu/%zu starters updated\n",
            docPk, updated, tokenPositions.size());
        fflush(stderr);

        return ok;
    }

    bool HCPWriteKernel::StoreMetadata(
        int docPk,
        const AZStd::string& key,
        const AZStd::string& value)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str(), key.c_str(), value.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "UPDATE pbm_documents SET metadata = metadata || jsonb_build_object($2, $3::jsonb) "
            "WHERE id = $1::integer",
            3, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPStorage] StoreMetadata failed: %s\n", PQerrorMessage(m_conn));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    bool HCPWriteKernel::StoreDocumentMetadata(
        int docPk,
        const AZStd::string& metadataJson)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str(), metadataJson.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "UPDATE pbm_documents SET metadata = metadata || $2::jsonb "
            "WHERE id = $1::integer",
            2, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPStorage] StoreDocumentMetadata failed: %s\n", PQerrorMessage(m_conn));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    bool HCPWriteKernel::StoreProvenance(
        int docPk,
        const AZStd::string& sourceType,
        const AZStd::string& sourcePath,
        const AZStd::string& sourceFormat,
        const AZStd::string& catalog,
        const AZStd::string& catalogId)
    {
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = {
            pkStr.c_str(), sourceType.c_str(), sourcePath.c_str(),
            sourceFormat.c_str(), catalog.c_str(), catalogId.c_str()
        };
        PGresult* res = PQexecParams(m_conn,
            "INSERT INTO document_provenance "
            "(doc_id, source_type, source_path, source_format, source_catalog, catalog_id) "
            "VALUES ($1::integer, $2, $3, $4, $5, $6) "
            "ON CONFLICT (doc_id) DO UPDATE SET "
            "source_type = EXCLUDED.source_type, "
            "source_path = EXCLUDED.source_path, "
            "source_format = EXCLUDED.source_format, "
            "source_catalog = EXCLUDED.source_catalog, "
            "catalog_id = EXCLUDED.catalog_id",
            6, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPStorage] StoreProvenance failed: %s\n", PQerrorMessage(m_conn));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    AZStd::vector<HCPWriteKernel::DocumentInfo> HCPWriteKernel::ListDocuments()
    {
        AZStd::vector<DocumentInfo> result;
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return result;
        }

        PGresult* res = PQexec(m_conn,
            "SELECT d.doc_id, d.name, "
            "  (SELECT COUNT(*) FROM pbm_starters s WHERE s.doc_id = d.id) AS starters, "
            "  (SELECT COALESCE(SUM(wb.count),0) + COALESCE(SUM(cb.count),0) + "
            "          COALESCE(SUM(mb.count),0) + COALESCE(SUM(vb.count),0) "
            "   FROM pbm_starters s2 "
            "   LEFT JOIN pbm_word_bonds wb ON wb.starter_id = s2.id "
            "   LEFT JOIN pbm_char_bonds cb ON cb.starter_id = s2.id "
            "   LEFT JOIN pbm_marker_bonds mb ON mb.starter_id = s2.id "
            "   LEFT JOIN pbm_var_bonds vb ON vb.starter_id = s2.id "
            "   WHERE s2.doc_id = d.id) AS total_bonds "
            "FROM pbm_documents d ORDER BY d.doc_id");
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                DocumentInfo info;
                info.docId = PQgetvalue(res, i, 0);
                info.name = PQgetvalue(res, i, 1);
                info.starters = atoi(PQgetvalue(res, i, 2));
                info.bonds = atoi(PQgetvalue(res, i, 3));
                result.push_back(AZStd::move(info));
            }
        }
        PQclear(res);
        return result;
    }

    PBMData HCPWriteKernel::LoadPBM(const AZStd::string& docId)
    {
        PBMData result;
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return result;
        }

        // Get document PK and first FPB
        int docPk = 0;
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT id, first_fpb_a, first_fpb_b FROM pbm_documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPWriteKernel: Document %s not found", docId.c_str());
                PQclear(res);
                return result;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            result.firstFpbA = PQgetvalue(res, 0, 1);
            result.firstFpbB = PQgetvalue(res, 0, 2);
            PQclear(res);
        }

        // Build var_id → surface form lookup for this document
        AZStd::unordered_map<AZStd::string, AZStd::string> varSurfaces;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT var_id, surface FROM pbm_docvars WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    varSurfaces[PQgetvalue(res, i, 0)] = PQgetvalue(res, i, 1);
                }
            }
            PQclear(res);
        }

        // Load all starters and their bonds via a single query per bond type
        // First get starters: id, token_a_id
        struct StarterInfo { int id; AZStd::string tokenA; };
        AZStd::vector<StarterInfo> starters;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT id, token_a_id FROM pbm_starters WHERE doc_id = $1 ORDER BY id",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    StarterInfo si;
                    si.id = atoi(PQgetvalue(res, i, 0));
                    si.tokenA = PQgetvalue(res, i, 1);
                    starters.push_back(AZStd::move(si));
                }
            }
            PQclear(res);
        }

        if (starters.empty())
        {
            AZLOG_ERROR("HCPWriteKernel: No starters for doc %s", docId.c_str());
            return result;
        }

        // Resolve var-encoded A-sides: starters with a_ns="00" are var tokens
        // Their token_a_id is "00.00.00.XX.YY" — look up XX.YY in docvars
        for (auto& si : starters)
        {
            if (si.tokenA.starts_with("00.00.00."))
            {
                // Extract decimal var_id: "00.00.00.XX.YY" → "XX.YY"
                AZStd::string varId = si.tokenA.substr(9);  // skip "00.00.00."
                auto it = varSurfaces.find(varId);
                if (it != varSurfaces.end())
                {
                    si.tokenA = AZStd::string(VAR_PREFIX) + " " + it->second;
                }
            }
        }

        // Build starter ID → tokenA lookup
        AZStd::unordered_map<int, AZStd::string> starterTokenA;
        for (const auto& si : starters)
        {
            starterTokenA[si.id] = si.tokenA;
        }

        // Helper: load bonds from a subtable, reconstruct B-side token ID
        auto loadBonds = [&](const char* query, auto reconstructB) {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn, query,
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    int starterId = atoi(PQgetvalue(res, i, 0));
                    auto aIt = starterTokenA.find(starterId);
                    if (aIt == starterTokenA.end()) continue;

                    Bond bond;
                    bond.tokenA = aIt->second;
                    bond.tokenB = reconstructB(res, i);
                    bond.count = atoi(PQgetvalue(res, i, PQnfields(res) - 1));
                    result.bonds.push_back(AZStd::move(bond));
                    result.totalPairs += bond.count;
                }
            }
            PQclear(res);
        };

        // Word bonds: starter_id, b_p3, b_p4, b_p5, count
        loadBonds(
            "SELECT wb.starter_id, wb.b_p3, wb.b_p4, wb.b_p5, wb.count "
            "FROM pbm_word_bonds wb "
            "JOIN pbm_starters s ON s.id = wb.starter_id "
            "WHERE s.doc_id = $1",
            [](PGresult* res, int i) -> AZStd::string {
                return AZStd::string("AB.AB.") + PQgetvalue(res, i, 1) + "." +
                       PQgetvalue(res, i, 2) + "." + PQgetvalue(res, i, 3);
            });

        // Char bonds: starter_id, b_p2, b_p3, b_p4, b_p5, count
        loadBonds(
            "SELECT cb.starter_id, cb.b_p2, cb.b_p3, cb.b_p4, cb.b_p5, cb.count "
            "FROM pbm_char_bonds cb "
            "JOIN pbm_starters s ON s.id = cb.starter_id "
            "WHERE s.doc_id = $1",
            [](PGresult* res, int i) -> AZStd::string {
                return AZStd::string("AA.") + PQgetvalue(res, i, 1) + "." +
                       PQgetvalue(res, i, 2) + "." + PQgetvalue(res, i, 3) + "." +
                       PQgetvalue(res, i, 4);
            });

        // Marker bonds: starter_id, b_p3, b_p4, count
        loadBonds(
            "SELECT mb.starter_id, mb.b_p3, mb.b_p4, mb.count "
            "FROM pbm_marker_bonds mb "
            "JOIN pbm_starters s ON s.id = mb.starter_id "
            "WHERE s.doc_id = $1",
            [](PGresult* res, int i) -> AZStd::string {
                return AZStd::string("AA.AE.") + PQgetvalue(res, i, 1) + "." +
                       PQgetvalue(res, i, 2);
            });

        // Var bonds: starter_id, b_var_id, count
        loadBonds(
            "SELECT vb.starter_id, vb.b_var_id, vb.count "
            "FROM pbm_var_bonds vb "
            "JOIN pbm_starters s ON s.id = vb.starter_id "
            "WHERE s.doc_id = $1",
            [&varSurfaces](PGresult* res, int i) -> AZStd::string {
                AZStd::string varId = PQgetvalue(res, i, 1);
                auto it = varSurfaces.find(varId);
                if (it != varSurfaces.end())
                {
                    return AZStd::string(VAR_PREFIX) + " " + it->second;
                }
                return AZStd::string("var.") + varId;
            });

        // Count unique tokens
        AZStd::unordered_set<AZStd::string> uniqueTokens;
        for (const auto& bond : result.bonds)
        {
            uniqueTokens.insert(bond.tokenA);
            uniqueTokens.insert(bond.tokenB);
        }
        result.uniqueTokens = uniqueTokens.size();

        AZLOG_INFO("HCPWriteKernel: Loaded PBM %s — %zu bonds, %zu total pairs, %zu unique tokens",
            docId.c_str(), result.bonds.size(), result.totalPairs, result.uniqueTokens);
        return result;
    }

    AZStd::vector<AZStd::string> HCPWriteKernel::LoadPositions(const AZStd::string& docId)
    {
        AZStd::vector<AZStd::string> result;
        if (!m_conn)
        {
            AZLOG_ERROR("HCPWriteKernel: Not connected");
            return result;
        }

        // Get document PK and total_slots
        int docPk = 0;
        int totalSlots = 0;
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT id, COALESCE(total_slots, 0) FROM pbm_documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPWriteKernel::LoadPositions: Document %s not found", docId.c_str());
                PQclear(res);
                return result;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            totalSlots = atoi(PQgetvalue(res, 0, 1));
            PQclear(res);
        }

        // Build var_id → surface lookup for resolving var-encoded starters
        AZStd::unordered_map<AZStd::string, AZStd::string> varSurfaces;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT var_id, surface FROM pbm_docvars WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    varSurfaces[PQgetvalue(res, i, 0)] = PQgetvalue(res, i, 1);
                }
            }
            PQclear(res);
        }

        // Single query: all starters with positions for this document
        struct PosToken { int pos; AZStd::string tokenId; };
        AZStd::vector<PosToken> entries;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT token_a_id, positions FROM pbm_starters "
                "WHERE doc_id = $1 AND positions IS NOT NULL",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    AZStd::string tokenAId = PQgetvalue(res, i, 0);

                    // Resolve var-encoded starters: "00.00.00.XX.YY" → VAR_PREFIX + " " + surface
                    if (tokenAId.starts_with("00.00.00."))
                    {
                        AZStd::string varId = tokenAId.substr(9);
                        auto it = varSurfaces.find(varId);
                        if (it != varSurfaces.end())
                        {
                            tokenAId = AZStd::string(VAR_PREFIX) + " " + it->second;
                        }
                    }

                    const char* packed = PQgetvalue(res, i, 1);
                    int packedLen = static_cast<int>(strlen(packed));

                    for (int off = 0; off + 3 < packedLen; off += 4)
                    {
                        int pos = DecodePosition(packed + off);
                        entries.push_back({pos, tokenAId});
                    }
                }
            }
            PQclear(res);
        }

        // Sort by position
        AZStd::sort(entries.begin(), entries.end(),
            [](const PosToken& a, const PosToken& b) { return a.pos < b.pos; });

        // Build result — token IDs in position order
        result.reserve(entries.size());
        for (auto& e : entries)
        {
            result.push_back(AZStd::move(e.tokenId));
        }

        fprintf(stderr, "[HCPStorage] LoadPositions: %s — %zu tokens, %d total_slots\n",
            docId.c_str(), result.size(), totalSlots);
        fflush(stderr);

        return result;
    }

    // ---- Asset Manager: Document Detail ----

    int HCPWriteKernel::GetDocPk(const AZStd::string& docId)
    {
        if (!m_conn) return 0;

        const char* params[] = { docId.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "SELECT id FROM pbm_documents WHERE doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        int pk = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            pk = atoi(PQgetvalue(res, 0, 0));
        }
        PQclear(res);
        return pk;
    }

    HCPWriteKernel::DocumentDetail HCPWriteKernel::GetDocumentDetail(const AZStd::string& docId)
    {
        DocumentDetail detail;
        if (!m_conn) return detail;

        const char* params[] = { docId.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "SELECT d.id, d.doc_id, d.name, "
            "  COALESCE(d.total_slots, 0), COALESCE(d.unique_tokens, 0), "
            "  COALESCE(d.metadata::text, '{}'), "
            "  (SELECT COUNT(*) FROM pbm_starters s WHERE s.doc_id = d.id), "
            "  (SELECT COALESCE(SUM(sub.cnt), 0) FROM ("
            "    SELECT SUM(wb.count) AS cnt FROM pbm_starters s2 "
            "      JOIN pbm_word_bonds wb ON wb.starter_id = s2.id WHERE s2.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(cb.count) FROM pbm_starters s3 "
            "      JOIN pbm_char_bonds cb ON cb.starter_id = s3.id WHERE s3.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(mb.count) FROM pbm_starters s4 "
            "      JOIN pbm_marker_bonds mb ON mb.starter_id = s4.id WHERE s4.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(vb.count) FROM pbm_starters s5 "
            "      JOIN pbm_var_bonds vb ON vb.starter_id = s5.id WHERE s5.doc_id = d.id "
            "  ) sub) "
            "FROM pbm_documents d WHERE d.doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            detail.pk = atoi(PQgetvalue(res, 0, 0));
            detail.docId = PQgetvalue(res, 0, 1);
            detail.name = PQgetvalue(res, 0, 2);
            detail.totalSlots = atoi(PQgetvalue(res, 0, 3));
            detail.uniqueTokens = atoi(PQgetvalue(res, 0, 4));
            detail.metadataJson = PQgetvalue(res, 0, 5);
            detail.starters = atoi(PQgetvalue(res, 0, 6));
            detail.bonds = atoi(PQgetvalue(res, 0, 7));
        }
        PQclear(res);
        return detail;
    }

    HCPWriteKernel::ProvenanceInfo HCPWriteKernel::GetProvenance(int docPk)
    {
        ProvenanceInfo prov;
        if (!m_conn) return prov;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "SELECT source_type, source_path, source_format, source_catalog, catalog_id "
            "FROM document_provenance WHERE doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            prov.sourceType = PQgetvalue(res, 0, 0);
            prov.sourcePath = PQgetvalue(res, 0, 1);
            prov.sourceFormat = PQgetvalue(res, 0, 2);
            prov.catalog = PQgetvalue(res, 0, 3);
            prov.catalogId = PQgetvalue(res, 0, 4);
            prov.found = true;
        }
        PQclear(res);
        return prov;
    }

    AZStd::vector<HCPWriteKernel::DocVar> HCPWriteKernel::GetDocVars(int docPk)
    {
        AZStd::vector<DocVar> vars;
        if (!m_conn) return vars;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(m_conn,
            "SELECT var_id, surface FROM pbm_docvars WHERE doc_id = $1 ORDER BY var_id",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                DocVar v;
                v.varId = PQgetvalue(res, i, 0);
                v.surface = PQgetvalue(res, i, 1);
                vars.push_back(AZStd::move(v));
            }
        }
        PQclear(res);
        return vars;
    }

    bool HCPWriteKernel::UpdateMetadata(
        int docPk,
        const AZStd::string& setJson,
        const AZStd::vector<AZStd::string>& removeKeys)
    {
        if (!m_conn) return false;

        AZStd::string pkStr = AZStd::to_string(docPk);
        bool ok = true;

        // Merge new keys
        if (!setJson.empty() && setJson != "{}")
        {
            const char* params[] = { pkStr.c_str(), setJson.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "UPDATE pbm_documents SET metadata = COALESCE(metadata, '{}'::jsonb) || $2::jsonb "
                "WHERE id = $1::integer",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[HCPStorage] UpdateMetadata merge failed: %s\n",
                    PQerrorMessage(m_conn));
                fflush(stderr);
                ok = false;
            }
            PQclear(res);
        }

        // Remove keys one at a time
        for (const auto& key : removeKeys)
        {
            const char* params[] = { pkStr.c_str(), key.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "UPDATE pbm_documents SET metadata = metadata - $2 "
                "WHERE id = $1::integer",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[HCPStorage] UpdateMetadata remove '%s' failed: %s\n",
                    key.c_str(), PQerrorMessage(m_conn));
                fflush(stderr);
                ok = false;
            }
            PQclear(res);
        }

        return ok;
    }

    AZStd::vector<HCPWriteKernel::BondEntry> HCPWriteKernel::GetBondsForToken(
        int docPk, const AZStd::string& tokenId)
    {
        AZStd::vector<BondEntry> bonds;
        if (!m_conn) return bonds;

        AZStd::string pkStr = AZStd::to_string(docPk);

        if (tokenId.empty())
        {
            // Overview mode: top starters by total bond count
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT s.token_a_id, "
                "  COALESCE((SELECT SUM(wb.count) FROM pbm_word_bonds wb WHERE wb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(cb.count) FROM pbm_char_bonds cb WHERE cb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(mb.count) FROM pbm_marker_bonds mb WHERE mb.starter_id = s.id), 0) + "
                "  COALESCE((SELECT SUM(vb.count) FROM pbm_var_bonds vb WHERE vb.starter_id = s.id), 0) AS total "
                "FROM pbm_starters s WHERE s.doc_id = $1 "
                "ORDER BY total DESC LIMIT 50",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    BondEntry be;
                    be.tokenB = PQgetvalue(res, i, 0);  // reusing tokenB for the starter token
                    be.count = atoi(PQgetvalue(res, i, 1));
                    bonds.push_back(AZStd::move(be));
                }
            }
            PQclear(res);
        }
        else
        {
            // Drill-down: bonds for a specific A-side token
            const char* params[] = { pkStr.c_str(), tokenId.c_str() };
            PGresult* res = PQexecParams(m_conn,
                "SELECT s.id FROM pbm_starters s "
                "WHERE s.doc_id = $1 AND s.token_a_id = $2",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                PQclear(res);
                return bonds;
            }
            int starterId = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);

            AZStd::string sidStr = AZStd::to_string(starterId);

            // Word bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(m_conn,
                    "SELECT 'AB.AB.' || b_p3 || '.' || b_p4 || '.' || b_p5, count "
                    "FROM pbm_word_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Char bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(m_conn,
                    "SELECT 'AA.' || b_p2 || '.' || b_p3 || '.' || b_p4 || '.' || b_p5, count "
                    "FROM pbm_char_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Marker bonds
            {
                const char* p[] = { sidStr.c_str() };
                res = PQexecParams(m_conn,
                    "SELECT 'AA.AE.' || b_p3 || '.' || b_p4, count "
                    "FROM pbm_marker_bonds WHERE starter_id = $1 ORDER BY count DESC",
                    1, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }

            // Var bonds
            {
                const char* p[] = { sidStr.c_str(), pkStr.c_str() };
                res = PQexecParams(m_conn,
                    "SELECT COALESCE(dv.surface, vb.b_var_id), vb.count "
                    "FROM pbm_var_bonds vb "
                    "LEFT JOIN pbm_docvars dv ON dv.doc_id = $2::integer AND dv.var_id = vb.b_var_id "
                    "WHERE vb.starter_id = $1::integer ORDER BY vb.count DESC",
                    2, nullptr, p, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK)
                {
                    for (int i = 0; i < PQntuples(res); ++i)
                    {
                        BondEntry be;
                        be.tokenB = PQgetvalue(res, i, 0);
                        be.count = atoi(PQgetvalue(res, i, 1));
                        bonds.push_back(AZStd::move(be));
                    }
                }
                PQclear(res);
            }
        }

        return bonds;
    }

} // namespace HCPEngine
