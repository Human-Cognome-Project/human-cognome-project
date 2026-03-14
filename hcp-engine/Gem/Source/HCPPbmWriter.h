#pragma once

#include "HCPDbConnection.h"
#include "HCPParticlePipeline.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

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

        //! Create a stub document row (name + namespace only, no bonds).
        //! Used when JSON metadata arrives before the text is ingested.
        //! @return Integer PK of the new stub row, or 0 on failure.
        int CreateDocumentStub(
            const AZStd::string& docName,
            const AZStd::string& centuryCode);

        //! Write PBM bonds into an existing document row (stub-fill path).
        //! Used when text is ingested after JSON metadata created a stub.
        //! @return doc_id string of the filled document, or empty on failure.
        AZStd::string FillPBMData(int existingDocPk, const PBMData& pbmData);

        //! Integer PK of the last document touched by StorePBM / CreateDocumentStub.
        int LastDocPk() const { return m_lastDocPk; }

        //! Store position data alongside PBM bonds.
        //! Each unique token gets a base-50 encoded position list on its starter row.
        //! Sparse morpheme/cap overlays written to pbm_morpheme_positions.
        //! @param morphemePositions Map of morpheme name → position list (from ScanManifestToPBM).
        bool StorePositions(
            int docPk,
            const AZStd::vector<AZStd::string>& tokenIds,
            const AZStd::vector<int>& positions,
            int totalSlots,
            const AZStd::unordered_map<AZStd::string, AZStd::vector<int>>& morphemePositions = {});

    private:
        struct BondWriteSummary
        {
            size_t starters = 0;
            size_t wordBonds = 0;
            size_t charBonds = 0;
            size_t markerBonds = 0;
            size_t varBonds = 0;
        };

        //! Shared bond-writing logic used by StorePBM and FillPBMData.
        //! Runs inside the caller's BEGIN transaction — does not commit.
        BondWriteSummary WritePBMBonds(PGconn* pg, int docPk, const PBMData& pbmData);

        HCPDbConnection& m_conn;
        int m_lastDocPk = 0;
    };

} // namespace HCPEngine
