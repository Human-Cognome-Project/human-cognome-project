#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "HCPWordSuperpositionTrial.h"  // CharRun

namespace HCPEngine
{
    // ---- Morphological bit field (16-bit) ----
    // Each bit records one inflection/contraction applied during resolution.
    // Stored as positional modifiers alongside token_id — zero for bare tokens.
    namespace MorphBit
    {
        static constexpr AZ::u16 PLURAL   = 1 << 0;   // -s (plural)
        static constexpr AZ::u16 POSS     = 1 << 1;   // 's (possessive)
        static constexpr AZ::u16 POSS_PL  = 1 << 2;   // s' (plural possessive)
        static constexpr AZ::u16 PAST     = 1 << 3;   // -ed
        static constexpr AZ::u16 PROG     = 1 << 4;   // -ing
        static constexpr AZ::u16 THIRD    = 1 << 5;   // -s (3rd person singular)
        static constexpr AZ::u16 NEG      = 1 << 6;   // n't
        static constexpr AZ::u16 COND     = 1 << 7;   // 'd (would/had)
        static constexpr AZ::u16 WILL     = 1 << 8;   // 'll
        static constexpr AZ::u16 HAVE     = 1 << 9;   // 've
        static constexpr AZ::u16 BE       = 1 << 10;  // 're
        static constexpr AZ::u16 AM       = 1 << 11;  // 'm
        // Variant surface form flags (bits 12-15)
        static constexpr AZ::u16 VARIANT         = 1 << 12;  // Any variant surface form
        static constexpr AZ::u16 VARIANT_ARCHAIC = 1 << 13;  // Archaic/obsolete form
        static constexpr AZ::u16 VARIANT_DIALECT = 1 << 14;  // Dialectal form
        static constexpr AZ::u16 VARIANT_CASUAL  = 1 << 15;  // Casual/informal form
    }

    // ---- Constants (empirical, tunable) ----

    static constexpr float RC_Z_SCALE = 10.0f;
    static constexpr float RC_Y_OFFSET = 1.5f;
    static constexpr float RC_SETTLE_THRESHOLD = 0.5f;
    static constexpr float RC_VELOCITY_THRESHOLD = 3.0f;
    static constexpr float RC_CONTACT_OFFSET = 0.4f;
    static constexpr float RC_REST_OFFSET = 0.1f;
    static constexpr float RC_RUN_X_GAP = 2.0f;
    static constexpr float RC_DT = 1.0f / 60.0f;
    static constexpr int RC_SETTLE_STEPS = 60;
    static constexpr AZ::u32 RC_VOCAB_PER_PHASE = 2000;  // Vocab entries per phase slice (larger = fewer phases, fewer simulate() rounds)

    //! Tracking slot for a stream run loaded into a workspace buffer.
    struct StreamRunSlot
    {
        AZ::u32 runIndex;          // Index into the original runs array
        AZ::u32 bufferStart;       // First particle index in the buffer
        AZ::u32 charCount;         // Number of characters (= particles)
        AZStd::string runText;     // Lowercase run text (for match lookup)
        bool resolved = false;
        AZStd::string matchedWord;
        AZStd::string matchedTokenId;
        AZ::u32 tierResolved = 0xFF;  // Which tier resolved it (0xFF = unresolved)
        bool firstCap = false;     // Positional cap data from original CharRun
        bool allCaps = false;
    };

    //! Result for a single run's resolution.
    //! The manifest is the train manifest — each position carries its payload:
    //! token_id + morph bits + cap flags. runIndex ties back to document order.
    struct ResolutionResult
    {
        AZStd::string runText;
        AZStd::string matchedWord;
        AZStd::string matchedTokenId;
        AZ::u32 tierResolved = 0xFF;  // 0xFF = unresolved
        AZ::u32 runIndex = 0;          // Index into original CharRun array (document order)
        bool resolved = false;

        // Morphological + capitalization modifiers (positional, zero for bare tokens)
        AZ::u16 morphBits = 0;     // MorphBit flags (inflection/contraction applied)
        bool firstCap = false;      // First char was uppercase (Label pattern)
        bool allCaps = false;       // All chars were uppercase (e.g. "NASA")

        // Entity annotation (non-empty = part of a recognized multi-word entity)
        AZStd::string entityId;         // Entity token_id (e.g. "uA.AA.AA.AA.AA")
        AZ::u8 entityNameGroup = 0;     // Which name variant matched (0=primary)
    };

    //! Full manifest from a resolution pass.
    struct ResolutionManifest
    {
        AZStd::vector<ResolutionResult> results;
        AZ::u32 totalRuns = 0;
        AZ::u32 resolvedRuns = 0;
        AZ::u32 unresolvedRuns = 0;
        float totalTimeMs = 0.0f;
    };

} // namespace HCPEngine
