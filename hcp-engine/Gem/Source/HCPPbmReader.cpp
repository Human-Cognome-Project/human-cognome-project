#include "HCPPbmReader.h"
#include "HCPDbUtils.h"
#include "HCPResolutionChamber.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <libpq-fe.h>
#include <cstring>
#include <cctype>
#include <unordered_map>

namespace HCPEngine
{
    // -------------------------------------------------------------------------
    // Suffix re-application rules (inverse of strip rules used during ingestion).
    // Returns the inflected surface form of `base` given `morphBits`.
    // Irregular forms (from variantMap) take priority over all rules.
    //
    // variantMap: token_id → { MorphBit → surface_form }
    // -------------------------------------------------------------------------
    static std::string ApplyMorphBits(
        const std::string& base,
        AZ::u16 morphBits,
        const std::string& tokenId,
        const std::unordered_map<std::string,
              std::unordered_map<AZ::u16, std::string>>& variantMap)
    {
        if (morphBits == 0) return base;

        // Helper: look up an irregular form for a given bit
        auto irr = [&](AZ::u16 bit) -> std::string {
            auto tit = variantMap.find(tokenId);
            if (tit != variantMap.end()) {
                auto bit_it = tit->second.find(bit);
                if (bit_it != tit->second.end()) return bit_it->second;
            }
            return {};
        };

        // Helper: last char of base
        auto back = [&]() -> char { return base.empty() ? '\0' : base.back(); };

        // Helper: is consonant
        auto isCons = [](char c) -> bool {
            static const char* v = "aeiou";
            c = static_cast<char>(tolower(c));
            return isalpha(c) && !strchr(v, c);
        };

        // Helper: CVC pattern (short doubling trigger)
        auto cvc = [&]() -> bool {
            if (base.size() < 3) return false;
            char c1 = base[base.size()-3], c2 = base[base.size()-2], c3 = base[base.size()-1];
            static const char* v = "aeiou";
            return isCons(c1) && strchr(v, c2) && isCons(c3) && c3 != 'w' && c3 != 'x' && c3 != 'y';
        };

        // Contractions — always regular, just append
        if (morphBits & MorphBit::NEG)  return base + "n't";
        if (morphBits & MorphBit::WILL) return base + "'ll";
        if (morphBits & MorphBit::HAVE) return base + "'ve";
        if (morphBits & MorphBit::BE)   return base + "'re";
        if (morphBits & MorphBit::AM)   return base + "'m";
        if (morphBits & MorphBit::COND) return base + "'d";
        if (morphBits & MorphBit::POSS_PL) return base + "'";
        if (morphBits & MorphBit::POSS) return base + "'s";

        // PROG (-ing): irregular lookup, then suffix rules
        if (morphBits & MorphBit::PROG) {
            std::string s = irr(MorphBit::PROG);
            if (!s.empty()) return s;
            // silent-e drop: base ends in 'e' (but not 'ee') → drop e + ing
            if (back() == 'e' && base.size() >= 2 && base[base.size()-2] != 'e')
                return base.substr(0, base.size()-1) + "ing";
            // CVC → double final consonant
            if (cvc()) return base + std::string(1, back()) + "ing";
            return base + "ing";
        }

        // PAST (-ed)
        if (morphBits & MorphBit::PAST) {
            std::string s = irr(MorphBit::PAST);
            if (!s.empty()) return s;
            if (back() == 'e') return base + "d";
            if (back() == 'y' && base.size() >= 2 && !strchr("aeiou", base[base.size()-2]))
                return base.substr(0, base.size()-1) + "ied";
            if (cvc()) return base + std::string(1, back()) + "ed";
            return base + "ed";
        }

        // PLURAL and/or THIRD (-s/-es/-ies)
        if (morphBits & (MorphBit::PLURAL | MorphBit::THIRD)) {
            AZ::u16 bit = (morphBits & MorphBit::PLURAL) ? MorphBit::PLURAL : MorphBit::THIRD;
            std::string s = irr(bit);
            if (s.empty() && (morphBits & MorphBit::PLURAL) && (morphBits & MorphBit::THIRD))
                s = irr(MorphBit::THIRD);  // try alternate bit
            if (!s.empty()) return s;
            if (back() == 'y' && base.size() >= 2 && !strchr("aeiou", base[base.size()-2]))
                return base.substr(0, base.size()-1) + "ies";
            std::string b2 = base.size() >= 2 ? base.substr(base.size()-2) : base;
            if (back()=='s'||back()=='x'||back()=='z'||b2=="ch"||b2=="sh")
                return base + "es";
            return base + "s";
        }

        return base;  // unhandled bits — return base unchanged
    }

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

        // Load all starters with positions — resolve word ONCE per starter row,
        // then place at every position in its packed list.  No per-position lookup.
        struct PosWord { int pos; AZStd::string word; AZ::u32 modifier; };
        AZStd::vector<PosWord> entries;
        {
            AZStd::string pkStr = AZStd::to_string(docPk);
            const char* params[] = { pkStr.c_str() };
            PGresult* res = PQexecParams(pg,
                "SELECT token_a_id, positions FROM pbm_starters "
                "WHERE doc_id = $1 AND positions IS NOT NULL",
                1, nullptr, params, nullptr, nullptr, 0);

            // --- Batch-fetch canonical names from hcp_english for all word tokens ---
            // This avoids LMDB t2w entirely — Postgres is authoritative for token names.
            AZStd::unordered_map<AZStd::string, AZStd::string> nameMap;
            std::unordered_map<std::string, std::unordered_map<AZ::u16, std::string>> variantMap;
            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                // First pass: collect unique word token_ids (not var, not stream marker).
                // Char tokens (TokenToChar succeeds) are handled via LMDB c2t below.
                AZStd::vector<AZStd::string> wordTokenIds;
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    const char* tid = PQgetvalue(res, i, 0);
                    if (strncmp(tid, "00.00.00.", 9) == 0) continue;  // var
                    if (strcmp(tid, STREAM_START) == 0 || strcmp(tid, STREAM_END) == 0) continue;
                    if (vocab.TokenToChar(tid) != '\0') continue;  // char token
                    wordTokenIds.emplace_back(tid);
                }

                // Batch query: SELECT token_id, name FROM hcp_english.tokens WHERE token_id = ANY(...)
                // Uses a temporary connection — reconstruction is infrequent CPU work.
                if (!wordTokenIds.empty())
                {
                    PGconn* eng = PQconnectdb(
                        "dbname=hcp_english user=hcp password=hcp_dev host=localhost port=5432 sslmode=disable");
                    if (PQstatus(eng) == CONNECTION_OK)
                    {
                        // Build $1 = ARRAY['id1','id2',...] literal for the IN list
                        std::string arr = "ARRAY[";
                        for (size_t j = 0; j < wordTokenIds.size(); ++j)
                        {
                            if (j) arr += ',';
                            arr += '\'';
                            // Escape single quotes in token_id (shouldn't occur, but safe)
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
                        // Fetch irregular forms for morph re-inflection (variantMap).
                        // Only use modern irregular forms: IRREGULAR bit (1<<21) set AND
                        // ARCHAIC (1<<8) + DIALECT (1<<12) bits clear.
                        // Archaic forms (laugheth, goeth) must not override modern -s/-ed rules.
                        std::string q2 = "SELECT canonical_id, morpheme, name FROM token_variants WHERE canonical_id = ANY(" + arr + ") AND morpheme IS NOT NULL AND (characteristics & 2097152) != 0 AND (characteristics & 4352) = 0";
                        PGresult* vr = PQexec(eng, q2.c_str());
                        if (PQresultStatus(vr) == PGRES_TUPLES_OK)
                        {
                            for (int j = 0; j < PQntuples(vr); ++j)
                            {
                                const char* morphStr = PQgetvalue(vr, j, 1);
                                AZ::u16 bit = 0;
                                if      (strcmp(morphStr, "PAST")        == 0) bit = MorphBit::PAST;
                                else if (strcmp(morphStr, "PLURAL")      == 0) bit = MorphBit::PLURAL;
                                else if (strcmp(morphStr, "3RD_SING")    == 0) bit = MorphBit::THIRD;
                                else if (strcmp(morphStr, "PROGRESSIVE") == 0) bit = MorphBit::PROG;
                                if (bit == 0) continue;
                                variantMap[PQgetvalue(vr, j, 0)][bit] = PQgetvalue(vr, j, 2);
                            }
                        }
                        PQclear(vr);
                        PQfinish(eng);
                    }
                    else
                    {
                        fprintf(stderr, "[HCPPbmReader] hcp_english connect failed: %s\n", PQerrorMessage(eng));
                        fflush(stderr);
                        PQfinish(eng);
                    }
                }
            }

            // Load morpheme/cap overlay maps from pbm_morpheme_positions.
            // Builds two maps: pos→morphBits, pos→capFlags (bits 0=firstCap, 1=allCaps).
            AZStd::unordered_map<int, AZ::u16> posMorphBits;
            AZStd::unordered_map<int, AZ::u32> posCapBits;
            {
                const char* mParams[] = { pkStr.c_str() };
                PGresult* mr = PQexecParams(pg,
                    "SELECT morpheme, positions FROM pbm_morpheme_positions WHERE doc_id = $1",
                    1, nullptr, mParams, nullptr, nullptr, 0);
                if (PQresultStatus(mr) == PGRES_TUPLES_OK)
                {
                    static const struct { const char* name; AZ::u16 bit; } kMorphMap[] = {
                        {"PLURAL",  MorphBit::PLURAL},  {"POSS",    MorphBit::POSS},
                        {"POSS_PL", MorphBit::POSS_PL}, {"PAST",    MorphBit::PAST},
                        {"PROG",    MorphBit::PROG},    {"3RD",     MorphBit::THIRD},
                        {"NEG",     MorphBit::NEG},     {"COND",    MorphBit::COND},
                        {"WILL",    MorphBit::WILL},    {"HAVE",    MorphBit::HAVE},
                        {"BE",      MorphBit::BE},      {"AM",      MorphBit::AM},
                        {nullptr, 0}
                    };
                    for (int mi = 0; mi < PQntuples(mr); ++mi)
                    {
                        const char* morpheme = PQgetvalue(mr, mi, 0);
                        const char* packed   = PQgetvalue(mr, mi, 1);
                        int plen = static_cast<int>(strlen(packed));

                        // Cap flags stored as morpheme rows
                        if (strcmp(morpheme, "FIRST_CAP") == 0)
                        {
                            for (int off = 0; off + 3 < plen; off += 4)
                                posCapBits[DecodePosition(packed + off)] |= 1u;
                            continue;
                        }
                        if (strcmp(morpheme, "ALL_CAPS") == 0)
                        {
                            for (int off = 0; off + 3 < plen; off += 4)
                                posCapBits[DecodePosition(packed + off)] |= 2u;
                            continue;
                        }
                        // Morpheme bits
                        AZ::u16 bit = 0;
                        for (int k = 0; kMorphMap[k].name; ++k)
                            if (strcmp(morpheme, kMorphMap[k].name) == 0) { bit = kMorphMap[k].bit; break; }
                        if (bit == 0) continue;
                        for (int off = 0; off + 3 < plen; off += 4)
                            posMorphBits[DecodePosition(packed + off)] |= bit;
                    }
                }
                PQclear(mr);
            }

            if (PQresultStatus(res) == PGRES_TUPLES_OK)
            {
                for (int i = 0; i < PQntuples(res); ++i)
                {
                    AZStd::string tokenAId = PQgetvalue(res, i, 0);

                    // Skip stream markers
                    if (tokenAId == STREAM_START || tokenAId == STREAM_END)
                        continue;

                    AZStd::string word;
                    char charTok = '\0';

                    if (tokenAId.starts_with("00.00.00."))
                    {
                        AZStd::string varId = tokenAId.substr(9);
                        auto it = varSurfaces.find(varId);
                        if (it != varSurfaces.end())
                            word = it->second;
                    }
                    else
                    {
                        charTok = vocab.TokenToChar(tokenAId);
                        if (charTok != '\0')
                        {
                            word = AZStd::string(1, charTok);
                        }
                        else
                        {
                            auto nit = nameMap.find(tokenAId);
                            if (nit != nameMap.end())
                                word = nit->second;
                        }
                    }

                    if (word.empty())
                        continue;

                    // Place word at every position in this row's packed list
                    const char* packed = PQgetvalue(res, i, 1);
                    int packedLen = static_cast<int>(strlen(packed));

                    for (int off = 0; off + 3 < packedLen; off += 4)
                    {
                        int pos = DecodePosition(packed + off);

                        // Look up morpheme and cap overlays for this position
                        AZ::u16 morphBits = 0;
                        {
                            auto mit = posMorphBits.find(pos);
                            if (mit != posMorphBits.end()) morphBits = mit->second;
                        }
                        AZ::u32 capFlags = 0;
                        {
                            auto cit = posCapBits.find(pos);
                            if (cit != posCapBits.end()) capFlags = cit->second;
                        }

                        AZStd::string surfaceWord = word;
                        if (charTok == '\0' && !tokenAId.starts_with("00.00.00.") && morphBits != 0)
                        {
                            std::string surface = ApplyMorphBits(
                                std::string(word.c_str(), word.size()),
                                morphBits,
                                std::string(tokenAId.c_str(), tokenAId.size()),
                                variantMap);
                            surfaceWord = AZStd::string(surface.c_str(), surface.size());
                        }

                        // Pack cap flags into low 2 bits of modifier (morphBits now stored separately)
                        entries.push_back({pos, surfaceWord, capFlags});
                    }
                }
            }
            PQclear(res);
        }

        // Sort by position
        AZStd::sort(entries.begin(), entries.end(),
            [](const PosWord& a, const PosWord& b) { return a.pos < b.pos; });

        words.reserve(entries.size());
        modifiers.reserve(entries.size());
        for (auto& e : entries)
        {
            words.push_back(AZStd::move(e.word));
            modifiers.push_back(e.modifier);
        }

        fprintf(stderr, "[HCPPbmReader] LoadPositions: %s — %zu words placed, %d total_slots\n",
            docId.c_str(), words.size(), totalSlots);
        fflush(stderr);

        return true;
    }

} // namespace HCPEngine
