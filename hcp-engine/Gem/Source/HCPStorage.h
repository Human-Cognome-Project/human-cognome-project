#pragma once

#include "HCPParticlePipeline.h"
#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    //! Store a PBM to the hcp_fic_pbm prefix tree schema.
    //! @param docName Human-readable document name
    //! @param centuryCode Century code (e.g., "AS")
    //! @param pbmData The disassembly result
    //! @return The PBM document address, or empty on failure
    AZStd::string StorePBM(
        const AZStd::string& docName,
        const AZStd::string& centuryCode,
        const PBMData& pbmData);

    //! Load a PBM from hcp_fic_pbm prefix tree schema.
    //! @param docId The PBM document address
    //! @return The loaded PBM data (empty bonds on failure)
    PBMData LoadPBM(const AZStd::string& docId);

} // namespace HCPEngine
