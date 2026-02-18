#include "HCPTokenizer.h"
#include "HCPVocabulary.h"

#include <AzCore/Console/ILogger.h>
#include <cctype>

namespace HCPEngine
{
    // Classify character types for tokenization splitting
    enum class CharType
    {
        Alpha,
        Digit,
        Whitespace,
        Punctuation,
        Other
    };

    static CharType ClassifyChar(char c)
    {
        if (std::isalpha(static_cast<unsigned char>(c))) return CharType::Alpha;
        if (std::isdigit(static_cast<unsigned char>(c))) return CharType::Digit;
        if (c == ' ' || c == '\t') return CharType::Whitespace;
        if (c == '\n' || c == '\r') return CharType::Whitespace;
        return CharType::Punctuation;
    }

    //! Atomize an unknown word into largest recognized sub-tokens.
    //! Falls back to individual characters if nothing matches.
    static void AtomizeUnknown(
        const AZStd::string& word,
        const HCPVocabulary& vocab,
        AZStd::vector<AZStd::string>& out)
    {
        size_t pos = 0;
        while (pos < word.size())
        {
            // Try longest match first
            bool found = false;
            for (size_t len = word.size() - pos; len > 1; --len)
            {
                AZStd::string sub(word.data() + pos, len);
                AZStd::string tid = vocab.LookupWord(sub);
                if (!tid.empty())
                {
                    out.push_back(tid);
                    pos += len;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // Single character fallback
                AZStd::string cid = vocab.LookupChar(word[pos]);
                if (!cid.empty())
                {
                    out.push_back(cid);
                }
                else
                {
                    // Truly unknown character — use replacement
                    AZLOG_WARN("HCPTokenizer: Unknown character U+%04X", static_cast<unsigned>(word[pos]));
                }
                ++pos;
            }
        }
    }

    AZStd::vector<AZStd::string> Tokenize(const AZStd::string& text, const HCPVocabulary& vocab)
    {
        AZStd::vector<AZStd::string> tokens;
        tokens.reserve(text.size() / 4); // rough estimate

        // Stream start anchor
        tokens.push_back(STREAM_START);

        size_t i = 0;
        while (i < text.size())
        {
            char c = text[i];
            CharType ct = ClassifyChar(c);

            if (ct == CharType::Alpha)
            {
                // Collect contiguous alphabetic word
                size_t start = i;
                while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])))
                {
                    ++i;
                }
                AZStd::string word(text.data() + start, i - start);

                // Hash table hit — the engine's core tokenization operation
                AZStd::string tid = vocab.LookupWord(word);
                if (!tid.empty())
                {
                    tokens.push_back(tid);
                }
                else
                {
                    // Unknown word: atomize to largest recognized sub-tokens
                    AtomizeUnknown(word, vocab, tokens);
                }
            }
            else if (ct == CharType::Digit)
            {
                // Individual digit tokens
                AZStd::string cid = vocab.LookupChar(c);
                if (!cid.empty())
                {
                    tokens.push_back(cid);
                }
                ++i;
            }
            else if (ct == CharType::Whitespace)
            {
                // Structural whitespace: newlines get their own token, spaces are implicit
                if (c == '\n')
                {
                    AZStd::string nid = vocab.LookupLabel("newline");
                    if (!nid.empty())
                    {
                        tokens.push_back(nid);
                    }
                }
                // Spaces are extrapolated during reassembly, not stored as tokens
                ++i;
            }
            else
            {
                // Punctuation and other single characters
                AZStd::string cid = vocab.LookupChar(c);
                if (!cid.empty())
                {
                    tokens.push_back(cid);
                }
                ++i;
            }
        }

        // Stream end anchor
        tokens.push_back(STREAM_END);

        AZLOG_INFO("HCPTokenizer: %zu chars -> %zu tokens", text.size(), tokens.size());
        return tokens;
    }

} // namespace HCPEngine
