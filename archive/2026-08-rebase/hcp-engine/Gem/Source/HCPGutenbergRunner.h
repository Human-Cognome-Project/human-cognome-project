#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <libpq-fe.h>

namespace HCPEngine
{
    class HCPEngineSystemComponent;

    struct GutenbergRunResult
    {
        int processed = 0;  // text files found and attempted
        int succeeded = 0;  // successfully stored in PBM
        int failed    = 0;  // pipeline errors
        int skipped   = 0;  // catalog_id had no matching .txt file
        AZStd::vector<AZStd::string> errors;
    };

    // Walk the entries in a working_doc (hcp_var.working_docs row identified by workingDocId),
    // find the corresponding Gutenberg .txt files in textsDir, and run PhysIngestText on each.
    // Files are matched by prefix: catalog_id zero-padded to 5 digits + "_".
    GutenbergRunResult RunGutenbergTexts(
        int workingDocId,
        const AZStd::string& textsDir,
        const AZStd::string& centuryCode,
        bool fictionFirst,
        HCPEngineSystemComponent* engine,
        PGconn* varConn);

} // namespace HCPEngine
