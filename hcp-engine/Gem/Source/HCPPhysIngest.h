#pragma once

#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    class HCPEngineSystemComponent;

    struct PhysIngestResult
    {
        bool ok = false;
        AZStd::string docId;
        int docPk = 0;
        bool usedStub = false;
        bool positionsStored = false;
        bool isWorkingDoc = false;
        int workingDocId = 0;

        AZ::u32 phase1Settled = 0;
        AZ::u32 phase1Total = 0;
        float phase1TimeMs = 0.f;
        AZ::u32 totalRuns = 0;
        AZ::u32 resolved = 0;
        AZ::u32 unresolved = 0;
        float resolveTimeMs = 0.f;
        AZ::u64 bondTypes = 0;
        AZ::u64 totalPairs = 0;
        AZ::u64 uniqueTokens = 0;
        int totalSlots = 0;
        AZ::u64 entityAnnotations = 0;

        AZStd::string errorMessage;
    };

    // Run the full Phase 1 → Phase 2 → scanner → storage pipeline on a text.
    // If isWorkingDoc is true, stores in hcp_var.working_docs instead of PBM.
    // catalog / catalogId used for stub lookup (may be empty).
    PhysIngestResult PhysIngestText(
        const AZStd::string& text,
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const AZStd::string& catalog,
        const AZStd::string& catalogId,
        bool fictionFirst,
        bool isWorkingDoc,
        HCPEngineSystemComponent* engine);

} // namespace HCPEngine
