#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/base.h>

namespace HCPEngine
{
    class HCPVocabulary;

    //! Result of tokenization: token IDs with their stream positions.
    //! Positions include space slots — any gap in the position sequence
    //! represents whitespace. Gap of N = N spaces. No gap = adjacent (e.g. punctuation).
    struct TokenStream
    {
        AZStd::vector<AZStd::string> tokenIds;
        AZStd::vector<AZ::u32> positions;   // stream position per token (including space gaps)
        AZ::u32 totalSlots = 0;             // total positions in the stream (tokens + spaces)
    };

    //! Tokenize text into a positioned token stream.
    //!
    //! Analysis unit: space-to-space. Everything between whitespace boundaries
    //! is one chunk to look up. The pipeline is staged:
    //!
    //!   1. Full chunk lookup (LMDB) — with continuation walk for boilerplate
    //!   2. Punctuation/separator split — word + punctuation tokens
    //!   3. Greedy word walk — missing space detection
    //!   4. Var DB handoff — unresolved sequences (stub, pending pipeline)
    //!
    //! Whitespace encoding: spaces = gaps in position numbering.
    //! Newlines/tabs = structural tokens with their own positions.
    //!
    //! @param text The input text
    //! @param vocab The loaded vocabulary
    //! @return TokenStream with IDs, positions, and total slot count
    TokenStream Tokenize(const AZStd::string& text, const HCPVocabulary& vocab);

} // namespace HCPEngine
