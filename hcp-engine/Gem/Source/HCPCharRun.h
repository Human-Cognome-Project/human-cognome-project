#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

// CharRun + RunTag — the character-run types shared across the resolution path
// (byte-floor ingest, vocab beds, chambers). Formerly defined in
// HCPWordSuperpositionTrial.h alongside the PhysX trial; that PhysX path was
// removed in the AZSL swap, so the shared types live here on their own.

namespace HCPEngine
{
    //! Transform layer tag — guides resolution routing.
    //! Detected by punctuation context in the P1→P2 boundary.
    enum class RunTag : AZ::u8
    {
        Word = 0,       // Normal word — resolve via the settle
        SingleChar,     // Single-char word (I, a) — pre-assigned at transform, skip settle
        Numeric,        // All digits (possibly with hyphens) — tag, skip settle
        Newline,        // Paragraph break (\n\n) — pre-resolved to newline char token
    };

    //! A character run extracted from the input stream.
    //! Runs are whitespace-delimited, edge-punctuation-stripped, lowercased.
    struct CharRun
    {
        AZStd::string text;       // Lowercase core (no edge punct)
        AZ::u32 startPos;         // Position in original input
        AZ::u32 length;           // Character count

        // Transform layer routing tag
        RunTag tag = RunTag::Word;

        // Pre-assigned token ID (for SingleChar, Numeric — resolved at transform layer)
        AZStd::string preAssignedTokenId;

        // Capitalization metadata — text preserves original case.
        // Resolution tries the word as-is first. Lowercase fallback only when
        // positionalCap is set and the as-is lookup fails.
        bool firstCap = false;                   // First char is uppercase
        bool allCaps = false;                    // All chars uppercase (e.g. "NASA") — try both Label and lowercase
        bool positionalCap = false;              // Caps may be positional (after ., ?, !, \n) — lowercase fallback
        AZStd::vector<AZ::u32> capMask;         // Run-relative positions that were uppercase

        // Source byte span (the byte-floor positional map) — carries the proprioceptive
        // position from raw bytes up to the run, so a resolved word traces to source bytes.
        AZ::u32 byteStart = 0;
        AZ::u32 byteLen = 0;
    };

} // namespace HCPEngine
