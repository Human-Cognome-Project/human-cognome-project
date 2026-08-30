#pragma once

#include "HCPDbConnection.h"
#include "HCPParticlePipeline.h"
#include "HCPVocabulary.h"
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

        //! Load a document's positional token sequence, pre-resolved to words.
        //!
        //! Each unique token (starter row) is resolved to its word string ONCE,
        //! then placed at every position from its packed position list.
        //! Result is a position-sorted parallel pair of pre-resolved words and
        //! per-position modifier bits — no further vocab lookup needed.
        //!
        //! Tokens that cannot be resolved (stream markers, unknown IDs) are
        //! silently dropped.  Var tokens are resolved to their surface form.
        //!
        //! modifier encoding: bit0=firstCap, bit1=allCaps, bits2+=morphBits.
        //! @return true on success, false if document not found
        bool LoadPositionsWithModifiers(const AZStd::string& docId,
                                        const HCPVocabulary& vocab,
                                        AZStd::vector<AZStd::string>& words,
                                        AZStd::vector<AZ::u32>& modifiers);

    private:
        HCPDbConnection& m_conn;
    };

} // namespace HCPEngine
