// HCPGlossKernel — gloss → concept-formula parser kernel.
//
// Standalone for now (Patrick 2026-06-11); class is shaped for later lift into the
// HCPEngine O3DE Gem (kernel-takes-connection pattern, cf. Gem/Source/HCPDbConnection.h).
//
// Pipeline per sense (doctrine: claims 531-541):
//   tokenized_gloss (address sequence, english shard)
//     -> lemma resolve (entries.form_of/alt_of; lemma-fix remaps)
//     -> pattern fold  (multiword lexicographic patterns -> structural markers)
//     -> classify      (core concept | scaffold-drop | content)
//     -> sectioned ordered structure ([;] = section boundary) -> md5 collapse key
//   fixpoint ladder: mint complete explications (residue == 0); minted words become
//   skins; repeat until no gloss newly resolves ("chew through everything resolvable").
#pragma once
#include <libpq-fe.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hcp {

struct KernelConfig
{
    std::string conninfo;        // libpq conninfo for hcp_english
    bool includeDated = false;   // archaic/obsolete/dated senses (deferred per Patrick)
    int  maxPasses    = 50;      // fixpoint safety bound
    long limitSenses  = 0;       // 0 = all (smoke-test knob)
    int  maxResidue   = 0;       // mint threshold: 0 = complete explications only
};

struct KernelStats
{
    long sensesLoaded = 0, sensesParsed = 0, sensesEmpty = 0;
    long unknownTokens = 0, totalTokens = 0;
    long concepts = 0, links = 0, passes = 0;
    long complete = 0, incomplete = 0;
};

class GlossKernel
{
public:
    explicit GlossKernel(const KernelConfig& cfg) : m_cfg(cfg) {}
    ~GlossKernel();

    bool Run();                          // full pipeline; returns false on hard error
    const KernelStats& Stats() const { return m_stats; }

private:
    // --- setup ---
    bool Connect();
    bool LoadLemmaMap();                 // entries: token_id -> interned lemma
    bool LoadMapTables();                // cx_coremap / cx_scaffold / cx_lemma_fix
    bool LoadSenses();                   // eligible senses + owning word

    // --- parse ---
    void BuildPatterns();
    void ParseAll();

    // --- ladder ---
    void Fixpoint();

    // --- output ---
    bool WriteResults();                 // TRUNCATE + COPY kx_* tables

    int Intern(const std::string& s);
    const std::string& Lemma(int id) const { return m_lemmas[id]; }

    struct Unit { int lemma; uint8_t cls; };          // cls: 0 core, 1 content, 2 marker
    struct Parsed
    {
        long sense_id = 0;
        int  word = -1;                               // owning word (lemma id)
        std::vector<std::vector<Unit>> sections;      // ordered
        std::vector<int> residueWords;                // content lemmas, unresolved cache
        std::string structure, ckey;
        int  status = 0;                              // 0 pending, 1 complete(minted), 2 empty
        int  mintedPass = 0;
        int  residueFinal = 0;                        // unresolved content words at fixpoint end
    };

    KernelConfig m_cfg;
    KernelStats  m_stats;
    PGconn*      m_conn = nullptr;

    std::vector<std::string>             m_lemmas;        // intern pool
    std::unordered_map<std::string,int>  m_internIdx;
    std::unordered_map<std::string,int>  m_tokenLemma;    // token_id -> lemma id
    std::unordered_map<int,std::string>  m_coreMap;       // lemma id -> core token_id
    std::unordered_set<int>              m_scaffold;      // lemma ids to drop
    std::unordered_map<int,int>          m_lemmaFix;      // bad lemma id -> good lemma id
    std::vector<Parsed>                  m_parsed;

    // pattern trie over lemma ids (greedy longest match), folded to marker names
    struct PatNode { std::unordered_map<int,int> next; int marker = -1; };
    std::vector<PatNode>      m_patNodes;
    std::vector<std::string>  m_markers;
};

} // namespace hcp
