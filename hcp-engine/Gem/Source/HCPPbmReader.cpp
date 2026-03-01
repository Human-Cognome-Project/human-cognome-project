#include "HCPPbmReader.h"
#include "HCPDbUtils.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <libpq-fe.h>
#include <cstring>

namespace HCPEngine
{
    PBMData HCPPbmReader::LoadPBM(const AZStd::string& docId)
    {
        PBMData result;
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmReader: Not connected");
            return result;
        }

        // Get document PK and first FPB
        int docPk = 0;
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT id, first_fpb_a, first_fpb_b FROM pbm_documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPPbmReader: Document %s not found", docId.c_str());
                PQclear(res);
                return result;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            result.firstFpbA = PQgetvalue(res, 0, 1);
            result.firstFpbB = PQgetvalue(res, 0, 2);
            PQclear(res);
        }

        // Build var_id → surface form lookup
        AZStd::unordered_map<AZStd::string, AZStd::string> varSurfaces;
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
                    varSurfaces[PQgetvalue(res, i, 0)] = PQgetvalue(res, i, 1);
                }
            }
            PQclear(res);
        }

        // Load starters
        struct StarterInfo { int id; AZStd::string tokenA; };
        AZStd::vector<StarterInfo> starters;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
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
            AZLOG_ERROR("HCPPbmReader: No starters for doc %s", docId.c_str());
            return result;
        }

        // Resolve var-encoded A-sides
        for (auto& si : starters)
        {
            if (si.tokenA.starts_with("00.00.00."))
            {
                AZStd::string varId = si.tokenA.substr(9);
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

        // Helper: load bonds from a subtable
        auto loadBonds = [&](const char* query, auto reconstructB) {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg, query,
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

        // Word bonds
        loadBonds(
            "SELECT wb.starter_id, wb.b_p3, wb.b_p4, wb.b_p5, wb.count "
            "FROM pbm_word_bonds wb "
            "JOIN pbm_starters s ON s.id = wb.starter_id "
            "WHERE s.doc_id = $1",
            [](PGresult* res, int i) -> AZStd::string {
                return AZStd::string("AB.AB.") + PQgetvalue(res, i, 1) + "." +
                       PQgetvalue(res, i, 2) + "." + PQgetvalue(res, i, 3);
            });

        // Char bonds
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

        // Marker bonds
        loadBonds(
            "SELECT mb.starter_id, mb.b_p3, mb.b_p4, mb.count "
            "FROM pbm_marker_bonds mb "
            "JOIN pbm_starters s ON s.id = mb.starter_id "
            "WHERE s.doc_id = $1",
            [](PGresult* res, int i) -> AZStd::string {
                return AZStd::string("AA.AE.") + PQgetvalue(res, i, 1) + "." +
                       PQgetvalue(res, i, 2);
            });

        // Var bonds
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

        AZLOG_INFO("HCPPbmReader: Loaded PBM %s — %zu bonds, %zu total pairs, %zu unique tokens",
            docId.c_str(), result.bonds.size(), result.totalPairs, result.uniqueTokens);
        return result;
    }

    AZStd::vector<AZStd::string> HCPPbmReader::LoadPositions(const AZStd::string& docId)
    {
        AZStd::vector<AZStd::string> result;
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmReader: Not connected");
            return result;
        }

        // Get document PK and total_slots
        int docPk = 0;
        int totalSlots = 0;
        {
            const char* params[] = { docId.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT id, COALESCE(total_slots, 0) FROM pbm_documents WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
            {
                AZLOG_ERROR("HCPPbmReader::LoadPositions: Document %s not found", docId.c_str());
                PQclear(res);
                return result;
            }
            docPk = atoi(PQgetvalue(res, 0, 0));
            totalSlots = atoi(PQgetvalue(res, 0, 1));
            PQclear(res);
        }

        // Build var_id → surface lookup
        AZStd::unordered_map<AZStd::string, AZStd::string> varSurfaces;
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
                    varSurfaces[PQgetvalue(res, i, 0)] = PQgetvalue(res, i, 1);
                }
            }
            PQclear(res);
        }

        // Load all starters with positions and modifiers
        struct PosToken { int pos; AZStd::string tokenId; AZ::u32 modifier; };
        AZStd::vector<PosToken> entries;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT token_a_id, positions, modifiers FROM pbm_starters "
                "WHERE doc_id = $1 AND positions IS NOT NULL",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    AZStd::string tokenAId = PQgetvalue(res, i, 0);

                    // Resolve var-encoded starters
                    if (tokenAId.starts_with("00.00.00."))
                    {
                        AZStd::string varId = tokenAId.substr(9);
                        auto it = varSurfaces.find(varId);
                        if (it != varSurfaces.end())
                        {
                            tokenAId = AZStd::string(VAR_PREFIX) + " " + it->second;
                        }
                    }

                    // Decode sparse modifiers
                    AZStd::unordered_map<int, AZ::u32> posModMap;
                    if (!PQgetisnull(res, i, 2))
                    {
                        const char* modPacked = PQgetvalue(res, i, 2);
                        int modLen = static_cast<int>(strlen(modPacked));
                        for (int off = 0; off + 7 < modLen; off += 8)
                        {
                            int modPos = DecodePosition(modPacked + off);
                            int modVal = DecodePosition(modPacked + off + 4);
                            posModMap[modPos] = static_cast<AZ::u32>(modVal);
                        }
                    }

                    // Decode positions
                    const char* packed = PQgetvalue(res, i, 1);
                    int packedLen = static_cast<int>(strlen(packed));

                    for (int off = 0; off + 3 < packedLen; off += 4)
                    {
                        int pos = DecodePosition(packed + off);
                        AZ::u32 mod = 0;
                        auto mit = posModMap.find(pos);
                        if (mit != posModMap.end()) mod = mit->second;
                        entries.push_back({pos, tokenAId, mod});
                    }
                }
            }
            PQclear(res);
        }

        // Sort by position
        AZStd::sort(entries.begin(), entries.end(),
            [](const PosToken& a, const PosToken& b) { return a.pos < b.pos; });

        // Build result
        result.reserve(entries.size());
        for (auto& e : entries)
        {
            result.push_back(AZStd::move(e.tokenId));
        }

        fprintf(stderr, "[HCPPbmReader] LoadPositions: %s — %zu tokens, %d total_slots\n",
            docId.c_str(), result.size(), totalSlots);
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
