#pragma once

#include "HCPDbConnection.h"
#include "HCPParticlePipeline.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    //! PBM read kernel — bond and position retrieval.
    //! Single-purpose: reads PBM data from hcp_fic_pbm.
    class HCPPbmReader
    {
    public:
        explicit HCPPbmReader(HCPDbConnection& conn) : m_conn(conn) {}

        //! Load a document's PBM bond data.
        //! Reconstructs PBMData from pbm_starters and bond subtables.
        PBMData LoadPBM(const AZStd::string& docId);

        //! Load a document's positional token sequence.
        //! Decodes base-50 positions, resolves var starters, sorts by position.
        //! @return Ordered token IDs (ready for TokenIdsToText), or empty on failure
        AZStd::vector<AZStd::string> LoadPositions(const AZStd::string& docId);

    private:
        HCPDbConnection& m_conn;
    };

} // namespace HCPEngine
