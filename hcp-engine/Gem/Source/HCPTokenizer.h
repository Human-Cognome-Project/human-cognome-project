#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/base.h>

namespace HCPEngine
{
    class HCPVocabulary;

    //! Result of tokenization: ordered token IDs with position data.
    //! Adjacent tokens in the stream form bond pairs for PBM derivation.
    //! Whitespace is implicit — squeezed out during tokenization.
    //! Positions encode spacing: gaps in position numbering = spaces.
    //! Consecutive positions = no space between tokens.
    struct TokenStream
    {
        AZStd::vector<AZStd::string> tokenIds;
        AZStd::vector<int> positions;  // 1:1 with tokenIds — position of each token
        int totalSlots = 0;            // Final position counter (for total_slots in DB)
    };

    //! Tokenize text into an ordered token stream.
    //!
    //! Analysis unit: space-to-space. Everything between whitespace boundaries
    //! is one chunk to look up. The pipeline is staged:
    //!
    //!   1. Full chunk lookup (LMDB) — with continuation walk for boilerplate
    //!   2. Punctuation/separator split — word + punctuation tokens
    //!   3. Greedy word walk — missing space detection
    //!   4. Var DB handoff — unresolved sequences (stub, pending pipeline)
    //!
    //! Spaces are squeezed out — they don't appear as tokens.
    //! Newlines = structural tokens (label "newline").
    //!
    //! @param text The input text
    //! @param vocab The loaded vocabulary
    //! @return TokenStream with ordered token IDs
    TokenStream Tokenize(const AZStd::string& text, const HCPVocabulary& vocab);

    //! Recover surface text from an ordered token stream.
    //!
    //! Inverse of Tokenize. Each token resolves to its surface form via vocab,
    //! then whitespace is interpolated using stickiness rules:
    //!   - Closing punct (.,;:!?)]}"'-*) sticks to preceding token
    //!   - Opening punct (([{"'\n) sticks to following token
    //!   - Everything else gets a space before it
    //!
    //! Stream markers (STREAM_START, STREAM_END) are skipped.
    //!
    //! @param tokenIds Ordered token IDs (from Reassemble or Tokenize)
    //! @param vocab The loaded vocabulary
    //! @return Reconstructed text
    AZStd::string TokenIdsToText(
        const AZStd::vector<AZStd::string>& tokenIds,
        const HCPVocabulary& vocab);

} // namespace HCPEngine
