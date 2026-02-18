#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    // Stream boundary anchor token IDs
    static constexpr const char* STREAM_START = "AA.AE.AF.AA.AA";
    static constexpr const char* STREAM_END = "AA.AE.AF.AA.AB";

    //! Vocabulary cache — loads word/char/label token mappings from hcp_english.
    //! This IS the engine's tokenizer lookup table.
    class HCPVocabulary
    {
    public:
        HCPVocabulary() = default;
        ~HCPVocabulary() = default;

        //! Load vocabulary from PostgreSQL (hcp_english database).
        //! Populates word, label, and character hash tables.
        //! @return true on success
        bool Load();

        //! Look up a word form and return its token_id, or empty string if not found.
        AZStd::string LookupWord(const AZStd::string& wordForm) const;

        //! Look up a single character and return its token_id, or empty string if not found.
        AZStd::string LookupChar(char c) const;

        //! Look up a label and return its token_id, or empty string if not found.
        AZStd::string LookupLabel(const AZStd::string& label) const;

        //! Reverse lookup: token_id -> word form (for reassembly)
        AZStd::string TokenToWord(const AZStd::string& tokenId) const;

        //! Reverse lookup: token_id -> character
        char TokenToChar(const AZStd::string& tokenId) const;

        bool IsLoaded() const { return m_loaded; }
        size_t WordCount() const { return m_wordToToken.size(); }
        size_t CharCount() const { return m_charToToken.size(); }
        size_t LabelCount() const { return m_labelToToken.size(); }

    private:
        bool m_loaded = false;

        // Forward lookups: form -> token_id
        AZStd::unordered_map<AZStd::string, AZStd::string> m_wordToToken;
        AZStd::unordered_map<char, AZStd::string> m_charToToken;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_labelToToken;

        // Reverse lookups: token_id -> form
        AZStd::unordered_map<AZStd::string, AZStd::string> m_tokenToWord;
        AZStd::unordered_map<AZStd::string, char> m_tokenToChar;
    };

} // namespace HCPEngine
