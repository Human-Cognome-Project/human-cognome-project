#include "HCPPhysIngest.h"
#include "HCPEngineSystemComponent.h"
#include "HCPVocabBed.h"
#include "HCPByteIngest.h"
#include "HCPPbmWriter.h"
#include "HCPDocumentQuery.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace HCPEngine
{
    PhysIngestResult PhysIngestText(
        const AZStd::string& text,
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const AZStd::string& catalog,
        const AZStd::string& catalogId,
        bool fictionFirst,
        bool isWorkingDoc,
        HCPEngineSystemComponent* engine)
    {
        PhysIngestResult result;

        // Prerequisites
        BedManager& bedManager = engine->GetBedManager();
        if (!bedManager.IsInitialized())
        { result.errorMessage = "BedManager not initialized"; return result; }

        HCPDbConnection& dbConn = engine->GetDbConnection();
        if (!dbConn.IsConnected()) dbConn.Connect();
        if (!dbConn.IsConnected())
        { result.errorMessage = "No database connection"; return result; }

        fprintf(stderr, "[PhysIngest] Starting: '%s' (%zu bytes)\n",
            docName.c_str(), text.size());
        fflush(stderr);

        // Normalize CRLF → LF (Gutenberg hard wraps).
        AZStd::string normalizedText;
        normalizedText.reserve(text.size());
        for (char c : text)
            if (c != '\r') normalizedText.push_back(c);

        // bytes → characters → runs, via the byte-floor (replaces the PhysX Phase-1 collapse).
        AZStd::vector<CharRun> runs = IngestBytes(
            reinterpret_cast<const uint8_t*>(normalizedText.data()), normalizedText.size());

        result.phase1Total   = static_cast<AZ::u32>(normalizedText.size());
        result.phase1Settled = result.phase1Total;   // byte-floor decodes deterministically
        result.phase1TimeMs  = 0.0f;

        fprintf(stderr, "[PhysIngest] byte-floor: %zu bytes → %zu runs\n",
            normalizedText.size(), runs.size());
        fflush(stderr);

        if (runs.empty())
        { result.errorMessage = "No runs from byte-floor"; return result; }

        // Phase 2: word resolution
        ResolutionManifest manifest = bedManager.Resolve(runs);

        result.totalRuns    = manifest.totalRuns;
        result.resolved     = manifest.resolvedRuns;
        result.unresolved   = manifest.unresolvedRuns;
        result.resolveTimeMs = manifest.totalTimeMs;

        fprintf(stderr, "[PhysIngest] Phase 2: %u/%u resolved in %.1f ms\n",
            manifest.resolvedRuns, manifest.totalRuns, manifest.totalTimeMs);
        fflush(stderr);

        // Entity annotation
        auto& entityAnnotator = engine->GetEntityAnnotator();
        if (entityAnnotator.IsInitialized())
            entityAnnotator.AnnotateManifest(manifest, fictionFirst);

        // Scanner: manifest → bonds + positions
        ManifestScanResult scan = ScanManifestToPBM(manifest);

        result.bondTypes        = scan.bonds.size();
        result.totalPairs       = scan.totalPairs;
        result.uniqueTokens     = scan.uniqueTokens;
        result.totalSlots       = scan.totalSlots;
        result.entityAnnotations = scan.entityAnnotations;

        if (isWorkingDoc)
        {
            // Serialize resolved tokens and store in hcp_var
            rapidjson::StringBuffer resolvedSb;
            rapidjson::Writer<rapidjson::StringBuffer> rw(resolvedSb);
            rw.StartArray();
            for (const auto& r : manifest.results)
            {
                rw.StartObject();
                rw.Key("run");      rw.String(r.runText.c_str());
                rw.Key("word");     rw.String(r.matchedWord.c_str());
                rw.Key("token_id"); rw.String(r.matchedTokenId.c_str());
                if (r.morphBits) { rw.Key("morph"); rw.Uint(r.morphBits); }
                rw.EndObject();
            }
            rw.EndArray();
            AZStd::string resolvedJson(resolvedSb.GetString(), resolvedSb.GetSize());

            result.workingDocId = engine->StoreWorkingDoc(docName, text, resolvedJson);
            result.isWorkingDoc = true;
            result.ok = (result.workingDocId > 0);

            fprintf(stderr, "[PhysIngest] Working doc '%s' → hcp_var id=%d\n",
                docName.c_str(), result.workingDocId);
            fflush(stderr);
            return result;
        }

        // Physical text — store in PBM
        PBMData pbmData;
        pbmData.bonds      = AZStd::move(scan.bonds);
        pbmData.firstFpbA  = scan.firstFpbA;
        pbmData.firstFpbB  = scan.firstFpbB;
        pbmData.totalPairs = scan.totalPairs;
        pbmData.uniqueTokens = scan.uniqueTokens;

        HCPPbmWriter& pbmWriter = engine->GetPbmWriter();

        if (!catalog.empty() && !catalogId.empty())
        {
            int stubPk = engine->GetDocumentQuery().GetDocPkByCatalogId(catalog, catalogId);
            if (stubPk != 0)
            {
                result.docId   = pbmWriter.FillPBMData(stubPk, pbmData);
                result.usedStub = true;
            }
        }
        if (result.docId.empty())
            result.docId = pbmWriter.StorePBM(docName, centuryCode, pbmData);

        if (result.docId.empty())
        { result.errorMessage = "StorePBM/FillPBMData failed"; return result; }

        result.docPk = pbmWriter.LastDocPk();
        result.positionsStored = pbmWriter.StorePositions(
            result.docPk, scan.tokenIds, scan.positions,
            scan.totalSlots, scan.morphemePositions);

        result.ok = true;

        fprintf(stderr, "[PhysIngest] Complete: %s → %llu bond types, %llu pairs, %d slots\n",
            result.docId.c_str(), (unsigned long long)result.bondTypes,
            (unsigned long long)result.totalPairs, result.totalSlots);
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
