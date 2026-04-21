#include "HCPPbmReader.h"
#include "HCPDbUtils.h"
#include "HCPResolutionChamber.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <libpq-fe.h>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <cstdlib>

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

    bool HCPPbmReader::LoadPositionsWithModifiers(
        const AZStd::string& docId,
        const HCPVocabulary& vocab,
        AZStd::vector<AZStd::string>& words,
        AZStd::vector<AZ::u32>& modifiers)
    {
        words.clear();
        modifiers.clear();

        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPPbmReader: Not connected");
            return false;
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
                return false;
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

        AZStd::string pkStr = AZStd::to_string(docPk);

        // ALL_CAPS positions for this doc (nominal — author override).
        // FIRST_CAP is positional, computed by TokenIdsToText (capitalize after . ? ! \n).
        AZStd::unordered_set<int> allCapsPositions;
        {
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT all_caps_positions FROM pbm_documents WHERE id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            {
                const char* arr = PQgetvalue(res, 0, 0);
                // Parse "{1,2,3}" libpq array literal
                if (arr && arr[0] == '{')
                {
                    const char* p = arr + 1;
                    while (*p && *p != '}')
                    {
                        char* end = nullptr;
                        long v = strtol(p, &end, 10);
                        if (end == p) break;
                        allCapsPositions.insert(static_cast<int>(v));
                        p = end;
                        if (*p == ',') ++p;
                    }
                }
            }
            PQclear(res);
        }

        // Pull all starters with their position arrays. One row per (doc, token).
        // Per-doc result: ~thousands of rows, each with positions INTEGER[] payload.
        struct PosWord { int pos; AZStd::string word; };
        AZStd::vector<PosWord> entries;
        {
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT token_a_id, positions FROM pbm_starters WHERE doc_id = $1",
                1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                fprintf(stderr, "[HCPPbmReader] starters query failed: %s\n", PQerrorMessage(pg));
                PQclear(res);
                return false;
            }

            // First pass: collect word token_ids needing entries lookup
            AZStd::vector<AZStd::string> wordTokenIds;
            int n = PQntuples(res);
            for (int i = 0; i < n; ++i)
            {
                const char* tid = PQgetvalue(res, i, 0);
                if (strncmp(tid, "00.00.00.", 9) == 0) continue;
                if (strcmp(tid, STREAM_START) == 0 || strcmp(tid, STREAM_END) == 0) continue;
                if (vocab.TokenToChar(tid) != '\0') continue;
                wordTokenIds.emplace_back(tid);
            }

            // Batch-fetch surface forms from hcp_english.entries
            AZStd::unordered_map<AZStd::string, AZStd::string> nameMap;
            if (!wordTokenIds.empty())
            {
                PGconn* eng = PQconnectdb(
                    "dbname=hcp_english user=hcp password=hcp_dev host=192.168.68.60 port=5435 sslmode=disable");
                if (PQstatus(eng) == CONNECTION_OK)
                {
                    std::string arr = "ARRAY[";
                    for (size_t j = 0; j < wordTokenIds.size(); ++j)
                    {
                        if (j) arr += ',';
                        arr += '\'';
                        for (char ch : wordTokenIds[j]) { if (ch == '\'') arr += '\''; arr += ch; }
                        arr += '\'';
                    }
                    arr += "]";
                    std::string q = "SELECT token_id, word FROM entries WHERE token_id = ANY(" + arr + ")";
                    PGresult* nr = PQexec(eng, q.c_str());
                    if (PQresultStatus(nr) == PGRES_TUPLES_OK)
                    {
                        for (int j = 0; j < PQntuples(nr); ++j)
                            nameMap[PQgetvalue(nr, j, 0)] = PQgetvalue(nr, j, 1);
                    }
                    PQclear(nr);
                }
                else
                {
                    fprintf(stderr, "[HCPPbmReader] hcp_english connect failed: %s\n", PQerrorMessage(eng));
                    fflush(stderr);
                }
                PQfinish(eng);
            }

            // Second pass: for each starter, resolve word and unnest positions array
            for (int i = 0; i < n; ++i)
            {
                const char* tid = PQgetvalue(res, i, 0);
                if (strcmp(tid, STREAM_START) == 0 || strcmp(tid, STREAM_END) == 0) continue;

                AZStd::string word;
                if (strncmp(tid, "00.00.00.", 9) == 0)
                {
                    AZStd::string varId(tid + 9);
                    auto vit = varSurfaces.find(varId);
                    if (vit != varSurfaces.end()) word = vit->second;
                }
                else
                {
                    char ch = vocab.TokenToChar(tid);
                    if (ch != '\0') word = AZStd::string(1, ch);
                    else
                    {
                        auto nit = nameMap.find(tid);
                        if (nit != nameMap.end()) word = nit->second;
                    }
                }
                if (word.empty()) continue;

                // Parse positions INTEGER[] literal "{1,2,3}"
                const char* arr = PQgetvalue(res, i, 1);
                if (!arr || arr[0] != '{') continue;
                const char* p = arr + 1;
                while (*p && *p != '}')
                {
                    char* end = nullptr;
                    long v = strtol(p, &end, 10);
                    if (end == p) break;
                    entries.push_back({static_cast<int>(v), word});
                    p = end;
                    if (*p == ',') ++p;
                }
            }
            PQclear(res);
        }

        // Sort by position to produce document-ordered output
        std::sort(entries.begin(), entries.end(),
            [](const PosWord& a, const PosWord& b) { return a.pos < b.pos; });

        words.reserve(entries.size());
        modifiers.reserve(entries.size());
        for (auto& e : entries)
        {
            // modifier bit 1 = ALL_CAPS (only flag still stored). bit 0 (FIRST_CAP)
            // is positional and handled inside TokenIdsToText.
            AZ::u32 mod = allCapsPositions.count(e.pos) ? 2u : 0u;
            words.push_back(AZStd::move(e.word));
            modifiers.push_back(mod);
        }

        fprintf(stderr, "[HCPPbmReader] LoadPositions: %s — %zu words placed, %d total_slots\n",
            docId.c_str(), words.size(), totalSlots);
        fflush(stderr);

        return true;
    }

} // namespace HCPEngine
