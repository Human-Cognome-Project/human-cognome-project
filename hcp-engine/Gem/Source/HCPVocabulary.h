#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/function/function_template.h>
#include <lmdb.h>

// Forward declarations
namespace HCPEngine { class CacheMissResolver; }
typedef struct pg_conn PGconn;

namespace HCPEngine
{
    // Stream boundary and control token IDs
    static constexpr const char* STREAM_START  = "AA.AE.AF.AA.AA";
    static constexpr const char* STREAM_END    = "AA.AE.AF.AA.AB";
    static constexpr const char* VAR_REQUEST   = "AA.AE.AF.AA.AC";

    //! Affix entry — suffix or prefix morpheme for decomposition.
    //! Stripped form has positional hyphen removed (e.g., "-ing" → "ing").
    //! Token ID is pre-resolved from Postgres at load time.
    struct AffixEntry
    {
        AZStd::string stripped;  // Match form (no positional hyphen)
        AZStd::string tokenId;   // Pre-resolved token_id
    };

    //! Result of a vocabulary lookup — token_id only.
    struct LookupResult
    {
        AZStd::string tokenId;  // Resolved token (empty = miss)

        bool IsHit() const { return !tokenId.empty(); }
    };

    //! Result of a continuation check — three-state: miss / continue / complete.
    //! Postgres/LMDB detects end tokens and returns the sequence ID directly.
    struct ContinuationResult
    {
        enum Status { Miss, Continue, Complete };
        Status status = Miss;
        AZStd::string sequenceId;   // Only populated when status == Complete

        bool IsMiss() const { return status == Miss; }
        bool IsContinue() const { return status == Continue; }
        bool IsComplete() const { return status == Complete; }
    };

    //! Vocabulary reader — zero-copy LMDB lookups with forward walk.
    //!
    //! LMDB is not pre-populated. It fills from Postgres via the cache miss
    //! pipeline (resolver writes to LMDB, engine re-reads on hit).
    //!
    //! The C++ engine is a pure reader — it never writes to LMDB.
    //! Cache miss handling is done by the resolver pipeline.
    class HCPVocabulary
    {
    public:
        HCPVocabulary() = default;
        ~HCPVocabulary();

        //! Open the LMDB vocabulary environment (read-only).
        //! @param lmdbPath Directory containing data.mdb + lock.mdb
        //! @return true on success
        bool Load(const char* lmdbPath = "/opt/project/repo/data/vocab.lmdb");

        // ---- Primary lookup ----

        //! Look up a space-to-space chunk → token_id.
        //! Tries w2t first, then c2t for single chars.
        LookupResult LookupChunk(const AZStd::string& chunk) const;

        //! Check continuation: is "accumulated + next" a valid boilerplate prefix?
        //! Returns three-state result:
        //!   Miss     — not a valid continuation, stop walking
        //!   Continue — valid prefix, keep accumulating
        //!   Complete — end token hit, sequenceId contains the boilerplate token_id
        //!
        //! @param accumulated The string built so far (may be multi-word)
        //! @param nextChunk The next space-to-space chunk to test
        ContinuationResult CheckContinuation(
            const AZStd::string& accumulated,
            const AZStd::string& nextChunk) const;

        // ---- Granular lookups (used by punctuation split and reconstruction) ----

        //! Look up a single word form → token_id.
        AZStd::string LookupWord(const AZStd::string& wordForm) const;

        //! Look up a word form in LMDB only — no resolver/Postgres on miss.
        //! Used by affix stem checks where misses are expected and frequent.
        AZStd::string LookupWordLocal(const AZStd::string& wordForm) const;

        //! Look up a single character → token_id.
        AZStd::string LookupChar(char c) const;

        //! Look up a label → token_id.
        AZStd::string LookupLabel(const AZStd::string& label) const;

        // ---- Reverse lookups (used by reconstruction) ----

        //! Reverse lookup: token_id → word form.
        AZStd::string TokenToWord(const AZStd::string& tokenId) const;

        //! Reverse lookup: token_id → character.
        char TokenToChar(const AZStd::string& tokenId) const;

        // ---- Iteration (used by bond compiler) ----

        //! Iterate all word forms in the vocabulary.
        //! Callback receives (wordForm, tokenId). Return true to continue, false to stop.
        void IterateWords(const AZStd::function<bool(const AZStd::string&, const AZStd::string&)>& callback) const;

        //! Iterate all characters in the vocabulary.
        //! Callback receives (character, tokenId). Return true to continue, false to stop.
        void IterateChars(const AZStd::function<bool(const AZStd::string&, const AZStd::string&)>& callback) const;

        // ---- Affix morpheme data (loaded from Postgres at startup) ----

        //! Bulk-load all affixes from Postgres into suffix/prefix vectors.
        //! Sorted by stripped length descending (longest match first).
        //! @param conn Active Postgres connection to hcp_english
        //! @return true if any affixes were loaded
        bool LoadAffixes(PGconn* conn);

        //! Get suffixes whose stripped form ends with the given char.
        //! Returns nullptr if no suffixes match that terminal char.
        const AZStd::vector<AffixEntry>* GetSuffixesForChar(char lastChar) const;

        //! Get prefixes whose stripped form starts with the given char.
        //! Returns nullptr if no prefixes match that initial char.
        const AZStd::vector<AffixEntry>* GetPrefixesForChar(char firstChar) const;

        size_t AffixCount() const { return m_suffixCount + m_prefixCount; }

        bool IsLoaded() const { return m_loaded; }
        size_t WordCount() const { return m_wordCount; }
        size_t CharCount() const { return m_charCount; }
        size_t LabelCount() const { return m_labelCount; }

        // ---- Resolver integration ----

        //! Set the cache miss resolver. The resolver writes to LMDB on miss.
        //! Must be called after Load() (resolver needs DBI handles).
        void SetResolver(CacheMissResolver* resolver);

        //! Get the LMDB env handle (for resolver setup).
        MDB_env* GetLmdbEnv() const { return m_env; }

        //! Get a named DBI handle (for resolver setup).
        MDB_dbi GetDbi(const char* name) const;

    private:
        AZStd::string LmdbGet(MDB_dbi dbi, const char* key, size_t keyLen) const;

        //! Map DBI handle back to sub-db name (for resolver routing).
        const char* SubDbName(MDB_dbi dbi) const;

        bool m_loaded = false;
        size_t m_wordCount = 0;
        size_t m_charCount = 0;
        size_t m_labelCount = 0;

        MDB_env* m_env = nullptr;
        MDB_dbi m_w2t = 0;   // word form -> token_id
        MDB_dbi m_c2t = 0;   // char -> token_id
        MDB_dbi m_l2t = 0;   // label -> token_id
        MDB_dbi m_t2w = 0;   // token_id -> word form
        MDB_dbi m_t2c = 0;   // token_id -> char
        MDB_dbi m_fwd = 0;   // forward walk (boilerplate prefix checks)
        mutable size_t m_debugCount = 0;
        mutable CacheMissResolver* m_resolver = nullptr;  // Cache fill on miss

        // Affix index: char → bucket of entries (sorted by length desc within bucket)
        AZStd::unordered_map<char, AZStd::vector<AffixEntry>> m_suffixByLastChar;
        AZStd::unordered_map<char, AZStd::vector<AffixEntry>> m_prefixByFirstChar;
        size_t m_suffixCount = 0;
        size_t m_prefixCount = 0;
    };

} // namespace HCPEngine
