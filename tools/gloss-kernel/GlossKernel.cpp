#include "GlossKernel.h"
#include "md5.h"
#include <cstdio>
#include <cstring>
#include <regex>

namespace hcp {

GlossKernel::~GlossKernel() { if (m_conn) PQfinish(m_conn); }

int GlossKernel::Intern(const std::string& s)
{
    auto it = m_internIdx.find(s);
    if (it != m_internIdx.end()) return it->second;
    int id = static_cast<int>(m_lemmas.size());
    m_lemmas.push_back(s);
    m_internIdx.emplace(s, id);
    return id;
}

bool GlossKernel::Connect()
{
    m_conn = PQconnectdb(m_cfg.conninfo.c_str());
    if (PQstatus(m_conn) != CONNECTION_OK) {
        std::fprintf(stderr, "connect failed: %s\n", PQerrorMessage(m_conn));
        return false;
    }
    return true;
}

// Single-word lowercase common lemmas only (interim ingestion rule, claim 536).
static bool isCommonWord(const char* w)
{
    if (!w || !*w) return false;
    if (w[0] < 'a' || w[0] > 'z') return false;
    for (const char* p = w; *p; ++p) {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || c == '\'' || c == '-')) return false;
    }
    return true;
}

bool GlossKernel::LoadLemmaMap()
{
    // Resolution target must itself be a lowercase single word; entry word likewise
    // (capitalized entries are label-ring, preflight-excluded per claim 536).
    const char* q =
        "SELECT token_id,"
        "       lower(coalesce(form_of_word, alt_of_word, word))"
        "  FROM entries"
        " WHERE token_id IS NOT NULL"
        "   AND word = lower(word)"
        "   AND word ~ '^[a-z][a-z''-]*$'"
        "   AND coalesce(form_of_word, alt_of_word, word) ~ '^[a-z][a-z''-]*$'";
    PGresult* r = PQexec(m_conn, q);
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
        std::fprintf(stderr, "lemma map: %s\n", PQerrorMessage(m_conn));
        PQclear(r); return false;
    }
    int n = PQntuples(r);
    m_tokenLemma.reserve(n * 2);
    for (int i = 0; i < n; ++i) {
        const char* lem = PQgetvalue(r, i, 1);
        if (!isCommonWord(lem)) continue;
        m_tokenLemma.emplace(PQgetvalue(r, i, 0), Intern(lem));
    }
    PQclear(r);
    std::fprintf(stderr, "lemma map: %zu tokens, %zu lemmas\n",
                 m_tokenLemma.size(), m_lemmas.size());
    return true;
}

bool GlossKernel::LoadMapTables()
{
    PGresult* r = PQexec(m_conn, "SELECT exponent, core_token_id FROM cx_coremap");
    if (PQresultStatus(r) != PGRES_TUPLES_OK) { PQclear(r); return false; }
    for (int i = 0; i < PQntuples(r); ++i)
        m_coreMap[Intern(PQgetvalue(r, i, 0))] = PQgetvalue(r, i, 1);
    PQclear(r);

    r = PQexec(m_conn, "SELECT word FROM cx_scaffold");
    if (PQresultStatus(r) != PGRES_TUPLES_OK) { PQclear(r); return false; }
    for (int i = 0; i < PQntuples(r); ++i)
        m_scaffold.insert(Intern(PQgetvalue(r, i, 0)));
    PQclear(r);

    r = PQexec(m_conn, "SELECT bad, good FROM cx_lemma_fix");
    if (PQresultStatus(r) != PGRES_TUPLES_OK) { PQclear(r); return false; }
    for (int i = 0; i < PQntuples(r); ++i)
        m_lemmaFix[Intern(PQgetvalue(r, i, 0))] = Intern(PQgetvalue(r, i, 1));
    PQclear(r);

    std::fprintf(stderr, "maps: %zu core exponents, %zu scaffold, %zu lemma fixes\n",
                 m_coreMap.size(), m_scaffold.size(), m_lemmaFix.size());
    return true;
}

void GlossKernel::BuildPatterns()
{
    // Lexicographic multiword patterns -> structural markers (CEF-adjacent ops).
    // Folded over the lemma sequence BEFORE scaffold drop (patterns contain scaffold words).
    static const struct { const char* marker; const char* words; } pats[] = {
        { "#ACT_OF",    "the act of" },      { "#ACT_OF",    "an act of" },
        { "#ACT_OF",    "act of" },
        { "#STATE_OF",  "the state of being" }, { "#STATE_OF", "the state of" },
        { "#STATE_OF",  "state of being" },  { "#STATE_OF",  "the condition of" },
        { "#QUAL_OF",   "the quality of being" }, { "#QUAL_OF", "the quality of" },
        { "#PROC_OF",   "the process of" },  { "#RESULT_OF", "the result of" },
        { "#AGENT",     "one who" },         { "#AGENT",     "a person who" },
        { "#AGENT",     "someone who" },     { "#AGENT",     "somebody who" },
        { "#THING_THAT","something that" },  { "#THING_THAT","that which" },
        { "#REL_TO",    "of or relating to" }, { "#REL_TO",  "of or pertaining to" },
        { "#REL_TO",    "relating to" },     { "#REL_TO",    "pertaining to" },
        { "#KIND",      "a kind of" },       { "#KIND",      "a type of" },
        { "#KIND",      "a sort of" },       { "#KIND",      "a species of" },
        { "#PART",      "a part of" },       { "#PART",      "part of" },
        { "#CAN",       "capable of" },      { "#CAN",       "able to" },
        { "#CAN",       "the ability to" },
        { "#LIKE",      "such as" },         { "#LIKE",      "similar to" },
        { "#LIKE",      "resembling" },
        { "#CAUSE",     "to cause to" },     { "#CAUSE",     "cause to" },
        { "#SLOT_ANY",  "someone or something" }, { "#SLOT_ANY", "something or someone" },
    };
    m_patNodes.clear();
    m_patNodes.emplace_back(); // root
    for (auto& p : pats) {
        int markerId;
        auto it = std::find(m_markers.begin(), m_markers.end(), p.marker);
        if (it == m_markers.end()) { markerId = (int)m_markers.size(); m_markers.push_back(p.marker); }
        else markerId = (int)(it - m_markers.begin());

        int node = 0;
        const char* s = p.words;
        while (*s) {
            const char* e = std::strchr(s, ' ');
            std::string w = e ? std::string(s, e - s) : std::string(s);
            int lem = Intern(w);
            auto& nx = m_patNodes[node].next;
            auto f = nx.find(lem);
            if (f == nx.end()) {
                nx.emplace(lem, (int)m_patNodes.size());
                node = (int)m_patNodes.size();
                m_patNodes.emplace_back();
            } else node = f->second;
            s = e ? e + 1 : s + std::strlen(s);
        }
        m_patNodes[node].marker = markerId;
    }
    std::fprintf(stderr, "patterns: %zu markers, %zu trie nodes\n",
                 m_markers.size(), m_patNodes.size());
}

bool GlossKernel::LoadSenses()
{
    std::string q =
        "DECLARE sc CURSOR FOR "
        "SELECT s.id, lower(e.word), array_to_string(s.tokenized_gloss, chr(31)) "
        "  FROM senses s JOIN entries e ON e.id = s.entry_id "
        " WHERE e.is_deprecated = false AND e.etymology_number <= 1 "
        "   AND e.pos <> 'name' AND e.word = lower(e.word) "
        "   AND e.word ~ '^[a-z][a-z''-]*$' "
        "   AND s.tokenized_gloss IS NOT NULL "
        "   AND NOT (coalesce(s.tags,'{}') && ARRAY['form-of','alt-of'])";
    if (!m_cfg.includeDated)
        q += " AND NOT (coalesce(s.tags,'{}') && ARRAY['obsolete','archaic','dated'])";
    if (m_cfg.limitSenses > 0)
        q += " LIMIT " + std::to_string(m_cfg.limitSenses);

    PGresult* r = PQexec(m_conn, "BEGIN");
    PQclear(r);
    r = PQexec(m_conn, q.c_str());
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::fprintf(stderr, "cursor: %s\n", PQerrorMessage(m_conn));
        PQclear(r); return false;
    }
    PQclear(r);

    for (;;) {
        r = PQexec(m_conn, "FETCH 50000 FROM sc");
        if (PQresultStatus(r) != PGRES_TUPLES_OK) { PQclear(r); return false; }
        int n = PQntuples(r);
        if (n == 0) { PQclear(r); break; }
        for (int i = 0; i < n; ++i) {
            Parsed p;
            p.sense_id = std::atol(PQgetvalue(r, i, 0));
            p.word = Intern(PQgetvalue(r, i, 1));

            // tokenize the joined gloss and parse inline (single pass, no raw storage)
            const char* g = PQgetvalue(r, i, 2);
            int depth = 0;
            std::vector<int> lemmaSeq;   // current section, lemma ids (pre-classification)
            auto flushSection = [&]() {
                if (lemmaSeq.empty()) return;
                // pattern fold (greedy longest match), then classify
                std::vector<Unit> out;
                size_t k = 0;
                while (k < lemmaSeq.size()) {
                    int node = 0; size_t best = 0; int bestMarker = -1;
                    size_t j = k;
                    while (j < lemmaSeq.size()) {
                        auto f = m_patNodes[node].next.find(lemmaSeq[j]);
                        if (f == m_patNodes[node].next.end()) break;
                        node = f->second; ++j;
                        if (m_patNodes[node].marker >= 0) { best = j - k; bestMarker = m_patNodes[node].marker; }
                    }
                    if (bestMarker >= 0) {
                        out.push_back({ -1 - bestMarker, 2 });   // marker encoded negative
                        k += best;
                        continue;
                    }
                    int lem = lemmaSeq[k++];
                    auto fx = m_lemmaFix.find(lem);
                    if (fx != m_lemmaFix.end()) lem = fx->second;
                    if (m_coreMap.count(lem))        out.push_back({ lem, 0 });
                    else if (m_scaffold.count(lem))  { /* drop */ }
                    else                             out.push_back({ lem, 1 });
                }
                if (!out.empty()) p.sections.push_back(std::move(out));
                lemmaSeq.clear();
            };

            const char* tok = g;
            while (tok && *tok) {
                const char* end = std::strchr(tok, 0x1f);
                std::string t = end ? std::string(tok, end - tok) : std::string(tok);
                tok = end ? end + 1 : nullptr;
                ++m_stats.totalTokens;
                if (t == "[(]") { ++depth; continue; }
                if (t == "[)]") { if (depth > 0) --depth; continue; }
                if (depth > 0) continue;
                if (t == "[;]") { flushSection(); continue; }
                if (t.size() > 2 && t[0] == '[') continue;       // other punctuation/junk
                auto m = m_tokenLemma.find(t);
                if (m == m_tokenLemma.end()) { ++m_stats.unknownTokens; continue; }
                lemmaSeq.push_back(m->second);
            }
            flushSection();

            if (p.sections.empty()) { p.status = 2; ++m_stats.sensesEmpty; }
            m_parsed.push_back(std::move(p));
        }
        m_stats.sensesLoaded += n;
        PQclear(r);
        std::fprintf(stderr, "\rloaded+parsed %ld senses", m_stats.sensesLoaded);
    }
    std::fprintf(stderr, "\n");
    PQclear(PQexec(m_conn, "CLOSE sc"));
    PQclear(PQexec(m_conn, "COMMIT"));
    m_stats.sensesParsed = (long)m_parsed.size() - m_stats.sensesEmpty;
    return true;
}

void GlossKernel::ParseAll()
{
    // structure string + key + residue cache (units already classified during load)
    for (auto& p : m_parsed) {
        if (p.status == 2) continue;
        std::string s;
        for (size_t si = 0; si < p.sections.size(); ++si) {
            if (si) s += ';';
            for (size_t ui = 0; ui < p.sections[si].size(); ++ui) {
                if (ui) s += ',';
                const Unit& u = p.sections[si][ui];
                if (u.cls == 2)       s += m_markers[-1 - u.lemma];
                else if (u.cls == 0)  s += m_coreMap[u.lemma];
                else                  s += Lemma(u.lemma);
                if (u.cls == 1) p.residueWords.push_back(u.lemma);
            }
        }
        p.structure = std::move(s);
        p.ckey = md5::hex(p.structure);
    }
}

void GlossKernel::Fixpoint()
{
    // skins: words that name at least one concept. Seed = core exponents.
    std::unordered_set<int> skins;
    for (auto& kv : m_coreMap) skins.insert(kv.first);

    std::vector<char> done(m_parsed.size(), 0);
    for (int pass = 1; pass <= m_cfg.maxPasses; ++pass) {
        std::vector<int> newWords;
        long minted = 0;
        for (size_t i = 0; i < m_parsed.size(); ++i) {
            if (done[i]) continue;
            Parsed& p = m_parsed[i];
            if (p.status == 2) { done[i] = 1; continue; }
            int residue = 0;
            for (int w : p.residueWords)
                if (!skins.count(w)) ++residue;
            if (residue <= m_cfg.maxResidue) {
                p.status = 1; p.mintedPass = pass;
                done[i] = 1; ++minted;
                newWords.push_back(p.word);
            }
        }
        std::fprintf(stderr, "pass %d: minted %ld senses\n", pass, minted);
        m_stats.passes = pass;
        if (minted == 0) break;
        for (int w : newWords) skins.insert(w);
    }
    for (auto& p : m_parsed) {
        if (p.status == 1) ++m_stats.complete;
        else if (p.status == 0) {
            ++m_stats.incomplete;
            for (int w : p.residueWords)
                if (!skins.count(w)) ++p.residueFinal;
        }
    }
}

static std::string copyEscape(const std::string& s)
{
    std::string o; o.reserve(s.size());
    for (char c : s) {
        if (c == '\\') o += "\\\\";
        else if (c == '\t') o += "\\t";
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

bool GlossKernel::WriteResults()
{
    PGresult* r;
    auto exec = [&](const char* q) {
        r = PQexec(m_conn, q);
        bool ok = PQresultStatus(r) == PGRES_COMMAND_OK || PQresultStatus(r) == PGRES_COPY_IN;
        if (!ok) std::fprintf(stderr, "write: %s — %s\n", q, PQerrorMessage(m_conn));
        PQclear(r);
        return ok;
    };
    if (!exec("BEGIN")) return false;
    exec("CREATE TABLE IF NOT EXISTS kx_concept(ckey text PRIMARY KEY, structure text,"
         " first_sense bigint, pass int)");
    exec("CREATE TABLE IF NOT EXISTS kx_word_concept(word text, sense_id bigint, ckey text, pass int)");
    exec("CREATE TABLE IF NOT EXISTS kx_status(sense_id bigint PRIMARY KEY, word text,"
         " status text, residue int, pass int)");
    exec("TRUNCATE kx_concept, kx_word_concept, kx_status");

    // kx_concept (dedup by key, first sense wins)
    std::unordered_set<std::string> seen;
    if (!exec("COPY kx_concept FROM STDIN")) return false;
    for (auto& p : m_parsed) {
        if (p.status != 1 || !seen.insert(p.ckey).second) continue;
        std::string line = p.ckey + "\t" + copyEscape(p.structure) + "\t"
            + std::to_string(p.sense_id) + "\t" + std::to_string(p.mintedPass) + "\n";
        PQputCopyData(m_conn, line.c_str(), (int)line.size());
    }
    PQputCopyEnd(m_conn, nullptr);
    PQclear(PQgetResult(m_conn));
    m_stats.concepts = (long)seen.size();

    if (!exec("COPY kx_word_concept FROM STDIN")) return false;
    for (auto& p : m_parsed) {
        if (p.status != 1) continue;
        std::string line = Lemma(p.word) + "\t" + std::to_string(p.sense_id) + "\t"
            + p.ckey + "\t" + std::to_string(p.mintedPass) + "\n";
        PQputCopyData(m_conn, line.c_str(), (int)line.size());
        ++m_stats.links;
    }
    PQputCopyEnd(m_conn, nullptr);
    PQclear(PQgetResult(m_conn));

    if (!exec("COPY kx_status FROM STDIN")) return false;
    for (auto& p : m_parsed) {
        const char* st = p.status == 1 ? "complete" : p.status == 2 ? "empty" : "incomplete";
        std::string line = std::to_string(p.sense_id) + "\t" + Lemma(p.word) + "\t" + st
            + "\t" + std::to_string(p.residueFinal) + "\t" + std::to_string(p.mintedPass) + "\n";
        PQputCopyData(m_conn, line.c_str(), (int)line.size());
    }
    PQputCopyEnd(m_conn, nullptr);
    PQclear(PQgetResult(m_conn));

    return exec("COMMIT");
}

bool GlossKernel::Run()
{
    if (!Connect() || !LoadLemmaMap() || !LoadMapTables()) return false;
    BuildPatterns();
    if (!LoadSenses()) return false;
    ParseAll();
    Fixpoint();
    if (!WriteResults()) return false;

    std::fprintf(stderr,
        "done: %ld senses (%ld parsed, %ld empty) | tokens %ld (%ld unknown, %.1f%%)\n"
        "      %ld passes | %ld complete -> %ld concepts, %ld links | %ld incomplete\n",
        m_stats.sensesLoaded, m_stats.sensesParsed, m_stats.sensesEmpty,
        m_stats.totalTokens, m_stats.unknownTokens,
        100.0 * m_stats.unknownTokens / (m_stats.totalTokens ? m_stats.totalTokens : 1),
        m_stats.passes, m_stats.complete, m_stats.concepts, m_stats.links,
        m_stats.incomplete);
    return true;
}

} // namespace hcp
