#pragma once

#include "HCPDbConnection.h"
#include "HCPParticlePipeline.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    //! PBM write kernel — bond insertion and position storage.
    //! Single-purpose: writes PBM data to hcp_fic_pbm.
    //! Takes a HCPDbConnection reference — does not own the connection.
    class HCPPbmWriter
    {
    public:
        explicit HCPPbmWriter(HCPDbConnection& conn) : m_conn(conn) {}

        //! Store a document's PBM bonds.
        //! Inserts into pbm_documents, pbm_starters, and bond subtables.
        //! Mints document-local vars for any var tokens encountered.
        //! @return Document ID string (ns.p2.p3.p4.p5), or empty on failure
        AZStd::string StorePBM(
            const AZStd::string& docName,
            const AZStd::string& centuryCode,
            const PBMData& pbmData);

        //! Integer PK of the last document inserted by StorePBM.
        int LastDocPk() const { return m_lastDocPk; }

        //! Store position data alongside PBM bonds.
        //! Each unique token gets a base-50 encoded position list on its starter row.
        //! @param modifiers Per-token (morphBits<<2 | capFlags). Empty = bare document.
        bool StorePositions(
            int docPk,
            const AZStd::vector<AZStd::string>& tokenIds,
            const AZStd::vector<int>& positions,
            int totalSlots,
            const AZStd::vector<AZ::u32>& modifiers = {});

    private:
        HCPDbConnection& m_conn;
        int m_lastDocPk = 0;
    };

} // namespace HCPEngine
