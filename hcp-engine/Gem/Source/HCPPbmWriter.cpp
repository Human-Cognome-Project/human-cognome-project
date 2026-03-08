#include "HCPPbmWriter.h"
#include "HCPDbUtils.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    AZStd::string HCPPbmWriter::StorePBM(
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const PBMData& pbmData)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmWriter: Not connected");
            return {};
        }

        if (pbmData.bonds.empty())
        {
            AZLOG_ERROR("HCPPbmWriter: Empty PBM data");
            return {};
        }

        PQexec(pg, "BEGIN");

        // Document namespace: vA.AB.<century>.<seq_hi>.<seq_lo>
        AZStd::string ns = "vA";
        AZStd::string p2 = "AB";
        AZStd::string p3 = centuryCode;

        // Next sequence number for this namespace path
        int seq = 0;
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PGresult* res = PQexecParams(pg,
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
            PGresult* res = PQexecParams(pg,
                "INSERT INTO pbm_documents (ns, p2, p3, p4, p5, name, first_fpb_a, first_fpb_b) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
                "RETURNING id, doc_id",
                8, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                fprintf(stderr, "[HCPPbmWriter] StorePBM: insert doc failed: %s\n",
                    PQerrorMessage(pg));
                fflush(stderr);
                PQclear(res);
                PQexec(pg, "ROLLBACK");
                return {};
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            docId = PQgetvalue(res, 0, 1);
            PQclear(res);
        }

        m_lastDocPk = docPk;

        fprintf(stderr, "[HCPPbmWriter] StorePBM: doc '%s' -> %s (pk=%d)\n",
            docName.c_str(), docId.c_str(), docPk);
        fflush(stderr);

        BondWriteSummary summary = WritePBMBonds(pg, docPk, pbmData);

        PGresult* commitRes = PQexec(pg, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        if (ok)
        {
            fprintf(stderr, "[HCPPbmWriter] StorePBM: '%s' -> %s — %zu starters, "
                "%zu word, %zu char, %zu marker, %zu var bonds\n",
                docName.c_str(), docId.c_str(), summary.starters,
                summary.wordBonds, summary.charBonds, summary.markerBonds, summary.varBonds);
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "[HCPPbmWriter] StorePBM: COMMIT failed: %s\n",
                PQerrorMessage(pg));
            fflush(stderr);
            return {};
        }

        return docId;
    }

    int HCPPbmWriter::CreateDocumentStub(
        const AZStd::string& docName,
        const AZStd::string& centuryCode)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmWriter: Not connected");
            return 0;
        }

        AZStd::string ns = "vA";
        AZStd::string p2 = "AB";
        AZStd::string p3 = centuryCode;

        PQexec(pg, "BEGIN");

        int seq = 0;
        {
            const char* params[] = { ns.c_str(), p2.c_str(), p3.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT COUNT(*) FROM pbm_documents WHERE ns = $1 AND p2 = $2 AND p3 = $3",
                3, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                seq = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);
        }

        AZStd::string p4 = EncodePairStr(seq / 2500);
        AZStd::string p5 = EncodePairStr(seq % 2500);

        int docPk = 0;
        {
            const char* params[] = {
                ns.c_str(), p2.c_str(), p3.c_str(), p4.c_str(), p5.c_str(),
                docName.c_str()
            };
            PGresult* res = PQexecParams(pg,
                "INSERT INTO pbm_documents (ns, p2, p3, p4, p5, name) "
                "VALUES ($1, $2, $3, $4, $5, $6) "
                "RETURNING id",
                6, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                fprintf(stderr, "[HCPPbmWriter] CreateDocumentStub: insert failed: %s\n",
                    PQerrorMessage(pg));
                fflush(stderr);
                PQclear(res);
                PQexec(pg, "ROLLBACK");
                return 0;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            PQclear(res);
        }

        PGresult* commitRes = PQexec(pg, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        if (!ok)
        {
            fprintf(stderr, "[HCPPbmWriter] CreateDocumentStub: COMMIT failed\n");
            fflush(stderr);
            return 0;
        }

        m_lastDocPk = docPk;
        fprintf(stderr, "[HCPPbmWriter] CreateDocumentStub: '%s' pk=%d\n",
            docName.c_str(), docPk);
        fflush(stderr);
        return docPk;
    }

    AZStd::string HCPPbmWriter::FillPBMData(int existingDocPk, const PBMData& pbmData)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmWriter: Not connected");
            return {};
        }

        if (pbmData.bonds.empty())
        {
            AZLOG_ERROR("HCPPbmWriter: FillPBMData: empty PBM data");
            return {};
        }

        PQexec(pg, "BEGIN");

        // Fetch doc_id string for this stub
        AZStd::string docId;
        {
            AZStd::string pkStr = AZStd::to_string(existingDocPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT doc_id FROM pbm_documents WHERE id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                docId = PQgetvalue(res, 0, 0);
            PQclear(res);
        }

        if (docId.empty())
        {
            fprintf(stderr, "[HCPPbmWriter] FillPBMData: pk=%d not found\n", existingDocPk);
            fflush(stderr);
            PQexec(pg, "ROLLBACK");
            return {};
        }

        // Stamp crystallization seeds on the stub row
        {
            AZStd::string pkStr = AZStd::to_string(existingDocPk);
            const char* params[] = {
                pbmData.firstFpbA.c_str(), pbmData.firstFpbB.c_str(), pkStr.c_str()
            };
            PGresult* res = PQexecParams(pg,
                "UPDATE pbm_documents SET first_fpb_a = $1, first_fpb_b = $2 "
                "WHERE id = $3::integer",
                3, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        m_lastDocPk = existingDocPk;

        BondWriteSummary summary = WritePBMBonds(pg, existingDocPk, pbmData);

        PGresult* commitRes = PQexec(pg, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        if (ok)
        {
            fprintf(stderr, "[HCPPbmWriter] FillPBMData: %s (pk=%d) — %zu starters, "
                "%zu word, %zu char, %zu marker, %zu var bonds\n",
                docId.c_str(), existingDocPk, summary.starters,
                summary.wordBonds, summary.charBonds, summary.markerBonds, summary.varBonds);
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "[HCPPbmWriter] FillPBMData: COMMIT failed: %s\n",
                PQerrorMessage(pg));
            fflush(stderr);
            return {};
        }

        return docId;
    }

    HCPPbmWriter::BondWriteSummary HCPPbmWriter::WritePBMBonds(
        PGconn* pg, int docPk, const PBMData& pbmData)
    {
        BondWriteSummary summary;

        // Mint document-local vars (decimal pair IDs)
        AZStd::unordered_map<AZStd::string, AZStd::string> varToDecimal;
        {
            int nextDecimal = 0;
            {
                AZStd::string pkStr = AZStd::to_string(docPk);
                const char* params[] = { pkStr.c_str() };
                PGresult* res = PQexecParams(pg,
                    "SELECT COALESCE(MAX("
                    "  CAST(SPLIT_PART(var_id, '.', 1) AS INTEGER) * 100 + "
                    "  CAST(SPLIT_PART(var_id, '.', 2) AS INTEGER)"
                    "), -1) FROM pbm_docvars WHERE doc_id = $1",
                    1, nullptr, params, nullptr, nullptr, 0);
                if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                    nextDecimal = atoi(PQgetvalue(res, 0, 0)) + 1;
                PQclear(res);
            }

            AZStd::string docPkStr2 = AZStd::to_string(docPk);
            AZStd::unordered_map<AZStd::string, AZStd::string> surfaceSeen;
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

                    // Check existing docvar for this surface
                    const char* checkParams[] = { docPkStr2.c_str(), surface.c_str() };
                    PGresult* res = PQexecParams(pg,
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

                    const char* category = ClassifyVar(surface);
                    const char* insParams[] = { docPkStr2.c_str(), varIdBuf, surface.c_str(), category };
                    res = PQexecParams(pg,
                        "INSERT INTO pbm_docvars (doc_id, var_id, surface, var_category) "
                        "VALUES ($1::integer, $2, $3, $4)",
                        4, nullptr, insParams, nullptr, nullptr, 0);
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPPbmWriter] docvar INSERT failed for '%s': %s\n",
                            surface.c_str(), PQerrorMessage(pg));
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
            fprintf(stderr, "[HCPPbmWriter] WritePBMBonds: minted %zu document-local vars\n",
                varToDecimal.size());
            fflush(stderr);
        }

        // Group bonds by A-side token
        AZStd::unordered_map<AZStd::string, AZStd::vector<const Bond*>> bondsByA;
        for (const auto& bond : pbmData.bonds)
            bondsByA[bond.tokenA].push_back(&bond);

        AZStd::string docPkStr = AZStd::to_string(docPk);

        for (const auto& [tokenA, bonds] : bondsByA)
        {
            // Insert starter row
            int starterId = 0;
            {
                AZStd::string aNs, aP2, aP3, aP4, aP5;
                auto varIt = varToDecimal.find(tokenA);
                if (varIt != varToDecimal.end())
                {
                    const AZStd::string& vid = varIt->second;
                    size_t dot = vid.find('.');
                    aNs = "00"; aP2 = "00"; aP3 = "00";
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
                PGresult* res = PQexecParams(pg,
                    "INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5) "
                    "VALUES ($1::integer, $2, $3, $4, $5, $6) "
                    "RETURNING id",
                    6, nullptr, params, nullptr, nullptr, 0);
                if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
                {
                    fprintf(stderr, "[HCPPbmWriter] insert starter failed for '%s': %s\n",
                        tokenA.c_str(), PQerrorMessage(pg));
                    fflush(stderr);
                    PQclear(res);
                    continue;
                }
                starterId = atoi(PQgetvalue(res, 0, 0));
                PQclear(res);
            }

            ++summary.starters;
            AZStd::string starterIdStr = AZStd::to_string(starterId);

            // Insert B-side bonds into correct subtable
            for (const Bond* bond : bonds)
            {
                AZStd::string countStr = AZStd::to_string(bond->count);

                // Var B-side
                auto varIt = varToDecimal.find(bond->tokenB);
                if (varIt != varToDecimal.end())
                {
                    const char* params[] = {
                        starterIdStr.c_str(), varIt->second.c_str(), countStr.c_str()
                    };
                    PGresult* res = PQexecParams(pg,
                        "INSERT INTO pbm_var_bonds (starter_id, b_var_id, count) "
                        "VALUES ($1::integer, $2, $3::integer) "
                        "ON CONFLICT (starter_id, b_var_id) "
                        "DO UPDATE SET count = pbm_var_bonds.count + EXCLUDED.count",
                        3, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) == PGRES_COMMAND_OK) ++summary.varBonds;
                    PQclear(res);
                    continue;
                }

                AZStd::string bParts[5];
                SplitTokenId(bond->tokenB, bParts);

                if (bParts[0] == "AB" && bParts[1] == "AB")
                {
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[2].c_str(), bParts[3].c_str(), bParts[4].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(pg,
                        "INSERT INTO pbm_word_bonds (starter_id, b_p3, b_p4, b_p5, count) "
                        "VALUES ($1::integer, $2, $3, $4, $5::integer) "
                        "ON CONFLICT (starter_id, b_p3, b_p4, b_p5) "
                        "DO UPDATE SET count = pbm_word_bonds.count + EXCLUDED.count",
                        5, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) == PGRES_COMMAND_OK) ++summary.wordBonds;
                    PQclear(res);
                }
                else if (bParts[0] == "AA" && bParts[1] != "AE")
                {
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[1].c_str(), bParts[2].c_str(), bParts[3].c_str(), bParts[4].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(pg,
                        "INSERT INTO pbm_char_bonds (starter_id, b_p2, b_p3, b_p4, b_p5, count) "
                        "VALUES ($1::integer, $2, $3, $4, $5, $6::integer) "
                        "ON CONFLICT (starter_id, b_p2, b_p3, b_p4, b_p5) "
                        "DO UPDATE SET count = pbm_char_bonds.count + EXCLUDED.count",
                        6, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) == PGRES_COMMAND_OK) ++summary.charBonds;
                    PQclear(res);
                }
                else if (bParts[0] == "AA" && bParts[1] == "AE" && bParts[4].empty())
                {
                    const char* params[] = {
                        starterIdStr.c_str(),
                        bParts[2].c_str(), bParts[3].c_str(),
                        countStr.c_str()
                    };
                    PGresult* res = PQexecParams(pg,
                        "INSERT INTO pbm_marker_bonds (starter_id, b_p3, b_p4, count) "
                        "VALUES ($1::integer, $2, $3, $4::integer) "
                        "ON CONFLICT (starter_id, b_p3, b_p4) "
                        "DO UPDATE SET count = pbm_marker_bonds.count + EXCLUDED.count",
                        4, nullptr, params, nullptr, nullptr, 0);
                    if (PQresultStatus(res) == PGRES_COMMAND_OK) ++summary.markerBonds;
                    PQclear(res);
                }
            }
        }

        return summary;
    }

    bool HCPPbmWriter::StorePositions(
        int docPk,
        const AZStd::vector<AZStd::string>& tokenIds,
        const AZStd::vector<int>& positions,
        int totalSlots,
        const AZStd::vector<AZ::u32>& modifiers)
    {
        PGconn* pg = m_conn.Get();
        if (!pg || tokenIds.size() != positions.size())
        {
            AZLOG_ERROR("HCPPbmWriter::StorePositions: not connected or size mismatch");
            return false;
        }

        bool hasModifiers = !modifiers.empty() && modifiers.size() == tokenIds.size();

        PQexec(pg, "BEGIN");

        // Group positions by token ID + build position→modifier lookup
        AZStd::unordered_map<AZStd::string, AZStd::vector<int>> tokenPositions;
        AZStd::unordered_map<int, AZ::u32> positionModifiers;  // position → modifier (non-zero only)
        for (size_t i = 0; i < tokenIds.size(); ++i)
        {
            tokenPositions[tokenIds[i]].push_back(positions[i]);
            if (hasModifiers && modifiers[i] != 0)
            {
                positionModifiers[positions[i]] = modifiers[i];
            }
        }

        // Update total_slots and unique_tokens
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            AZStd::string slotsStr = AZStd::to_string(totalSlots);
            AZStd::string uniqStr = AZStd::to_string(static_cast<int>(tokenPositions.size()));
            const char* params[] = { slotsStr.c_str(), uniqStr.c_str(), pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "UPDATE pbm_documents SET total_slots = $1::integer, unique_tokens = $2::integer "
                "WHERE id = $3::integer",
                3, nullptr, params, nullptr, nullptr, 0);
            PQclear(res);
        }

        // Build surface→decimal var_id lookup
        AZStd::unordered_map<AZStd::string, AZStd::string> surfaceToVarId;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
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
            // Encode positions as base-50
            AZStd::string packed;
            packed.resize(posList.size() * 4);
            for (size_t j = 0; j < posList.size(); ++j)
            {
                EncodePosition(posList[j], packed.data() + j * 4);
            }

            // Resolve var tokens to decimal starter IDs
            AZStd::string starterTokenId;
            if (tokenId.starts_with(VAR_PREFIX) && tokenId.size() > VAR_PREFIX_LEN + 1)
            {
                AZStd::string surface = VarSurface(tokenId);
                auto it = surfaceToVarId.find(surface);
                if (it != surfaceToVarId.end())
                {
                    const AZStd::string& vid = it->second;
                    size_t dot = vid.find('.');
                    starterTokenId = "00.00.00." +
                        AZStd::string(vid.data(), dot) + "." +
                        AZStd::string(vid.data() + dot + 1, vid.size() - dot - 1);
                }
                else
                {
                    fprintf(stderr, "[HCPPbmWriter] StorePositions: no docvar for surface '%s'\n",
                        surface.c_str());
                    fflush(stderr);
                    continue;
                }
            }
            else
            {
                starterTokenId = tokenId;
            }

            // Encode sparse modifiers via O(1) lookup per position
            AZStd::string modEncoded;
            if (hasModifiers && !positionModifiers.empty())
            {
                for (int pos : posList)
                {
                    auto it = positionModifiers.find(pos);
                    if (it != positionModifiers.end())
                    {
                        char posBuf[4];
                        EncodePosition(pos, posBuf);
                        char modBuf[4];
                        EncodePosition(static_cast<int>(it->second), modBuf);
                        modEncoded.append(posBuf, 4);
                        modEncoded.append(modBuf, 4);
                    }
                }
            }

            // Split token into component parts for matching (avoids generated-column format mismatch)
            AZStd::string aParts[5];
            SplitTokenId(starterTokenId, aParts);

            // UPDATE starter row with positions (+ modifiers), matching on component parts
            PGresult* res;
            if (!modEncoded.empty())
            {
                const char* params[] = { packed.c_str(), modEncoded.c_str(),
                                          docPkStr.c_str(),
                                          aParts[0].c_str(), aParts[1].c_str(), aParts[2].c_str(),
                                          aParts[3].c_str(), aParts[4].c_str() };
                res = PQexecParams(pg,
                    "UPDATE pbm_starters SET positions = $1, modifiers = $2 "
                    "WHERE doc_id = $3::integer AND a_ns = $4 AND a_p2 = $5 AND a_p3 = $6 AND a_p4 = $7 AND a_p5 = $8",
                    8, nullptr, params, nullptr, nullptr, 0);
            }
            else
            {
                const char* params[] = { packed.c_str(), docPkStr.c_str(),
                                          aParts[0].c_str(), aParts[1].c_str(), aParts[2].c_str(),
                                          aParts[3].c_str(), aParts[4].c_str() };
                res = PQexecParams(pg,
                    "UPDATE pbm_starters SET positions = $1 "
                    "WHERE doc_id = $2::integer AND a_ns = $3 AND a_p2 = $4 AND a_p3 = $5 AND a_p4 = $6 AND a_p5 = $7",
                    7, nullptr, params, nullptr, nullptr, 0);
            }

            if (PQresultStatus(res) == PGRES_COMMAND_OK)
            {
                int rows = atoi(PQcmdTuples(res));
                if (rows > 0)
                {
                    ++updated;
                }
                else
                {
                    // No existing starter — INSERT a new position-only row
                    PQclear(res);

                    if (!modEncoded.empty())
                    {
                        const char* insParams[] = {
                            docPkStr.c_str(),
                            aParts[0].c_str(), aParts[1].c_str(), aParts[2].c_str(),
                            aParts[3].c_str(), aParts[4].c_str(),
                            packed.c_str(), modEncoded.c_str()
                        };
                        res = PQexecParams(pg,
                            "INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5, positions, modifiers) "
                            "VALUES ($1::integer, $2, $3, $4, $5, $6, $7, $8)",
                            8, nullptr, insParams, nullptr, nullptr, 0);
                    }
                    else
                    {
                        const char* insParams[] = {
                            docPkStr.c_str(),
                            aParts[0].c_str(), aParts[1].c_str(), aParts[2].c_str(),
                            aParts[3].c_str(), aParts[4].c_str(),
                            packed.c_str()
                        };
                        res = PQexecParams(pg,
                            "INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5, positions) "
                            "VALUES ($1::integer, $2, $3, $4, $5, $6, $7)",
                            7, nullptr, insParams, nullptr, nullptr, 0);
                    }
                    if (PQresultStatus(res) != PGRES_COMMAND_OK)
                    {
                        fprintf(stderr, "[HCPPbmWriter] StorePositions: INSERT failed for '%s': %s\n",
                            starterTokenId.c_str(), PQerrorMessage(pg));
                        fflush(stderr);
                    }
                    else
                    {
                        ++updated;
                    }
                }
            }
            else
            {
                fprintf(stderr, "[HCPPbmWriter] StorePositions: UPDATE failed for '%s': %s\n",
                    starterTokenId.c_str(), PQerrorMessage(pg));
                fflush(stderr);
            }
            PQclear(res);
        }

        PGresult* commitRes = PQexec(pg, "COMMIT");
        bool ok = (PQresultStatus(commitRes) == PGRES_COMMAND_OK);
        PQclear(commitRes);

        fprintf(stderr, "[HCPPbmWriter] StorePositions: pk=%d — %zu/%zu starters updated\n",
            docPk, updated, tokenPositions.size());
        fflush(stderr);

        return ok;
    }

} // namespace HCPEngine
