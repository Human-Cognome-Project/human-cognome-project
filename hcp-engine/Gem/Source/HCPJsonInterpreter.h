#pragma once

#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    class HCPWriteKernel;
    class HCPVocabulary;

    //! JSON metadata interpretation layer.
    //!
    //! JSON files enter as byte streams like any other text — the punctuation
    //! tokens ({, }, [, ], :, ,) are already in hcp_core. This interpreter
    //! reads parsed JSON structure and maps key-value pairs to DB operations.
    //!
    //! Known fields route to specific targets (metadata JSONB, provenance table).
    //! Unknown fields are stored under metadata.unreviewed for human review.
    //! Text values are tokenized through the existing Tokenize() pipeline.
    struct JsonInterpretResult
    {
        int knownFields = 0;
        int unreviewedFields = 0;
        bool provenanceStored = false;
    };

    //! Process a JSON metadata entry for a document that has already been stored.
    //!
    //! @param jsonText  Raw JSON text for ONE metadata entry (a single object, not an array)
    //! @param docPk     Integer PK of the target pbm_documents row
    //! @param catalog   Source catalog name (e.g., "gutenberg") — used for provenance
    //! @param writeKernel  Connected write kernel for DB operations
    //! @param vocab     Loaded vocabulary (for tokenizing text values)
    //! @return Summary of what was processed
    JsonInterpretResult ProcessJsonMetadata(
        const AZStd::string& jsonText,
        int docPk,
        const AZStd::string& catalog,
        HCPWriteKernel& writeKernel,
        const HCPVocabulary& vocab);

} // namespace HCPEngine
