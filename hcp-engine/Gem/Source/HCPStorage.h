#pragma once

#include "HCPParticlePipeline.h"
#include <AzCore/std/string/string.h>

// Forward declare libpq types
struct pg_conn;
typedef pg_conn PGconn;

namespace HCPEngine
{
    //! Lean Postgres write kernel.
    //! All database writes flow through this single class — positions, metadata,
    //! provenance, everything. Designed for commonized write operations that can
    //! be parallelized as kernel combinations.
    //!
    //! Position-based document storage:
    //!   - Each unique token maps to a list of positions (base-50 encoded)
    //!   - Whitespace is implicit (gaps in position numbering)
    //!   - PBM bonds derived at aggregation time, not stored per-document
    class HCPWriteKernel
    {
    public:
        HCPWriteKernel() = default;
        ~HCPWriteKernel();

        //! Connect to the database. Returns true on success.
        bool Connect(const char* connInfo = nullptr);

        //! Disconnect and release resources.
        void Disconnect();

        bool IsConnected() const { return m_conn != nullptr; }

        // ---- Document position storage ----

        //! Store a document's position map to the prefix tree schema.
        //! @param docName Human-readable document name
        //! @param centuryCode Century code (e.g., "AS")
        //! @param posMap Position-based document data from DisassemblePositions()
        //! @param stream Original token stream (for total_slots)
        //! @return Document address string, or empty on failure
        AZStd::string StorePositionMap(
            const AZStd::string& docName,
            const AZStd::string& centuryCode,
            const PositionMap& posMap,
            const TokenStream& stream);

        //! Load a document's position map from the prefix tree schema.
        //! @param docId Document address string
        //! @param outStream Receives totalSlots and reconstructed token/position data
        //! @return The loaded PositionMap (empty entries on failure)
        PositionMap LoadPositionMap(
            const AZStd::string& docId,
            TokenStream& outStream);

        // ---- PBM storage (hcp_fic_pbm) ----

        //! Store a document's PBM bonds into hcp_fic_pbm.
        //! Inserts into pbm_documents, pbm_starters, and the appropriate bond tables.
        //! @param docName Human-readable document name
        //! @param centuryCode Century code for namespace (e.g., "AS" for 19th century)
        //! @param pbmData PBM bond data from DerivePBM()
        //! @return Document ID string (ns.p2.p3.p4.p5), or empty on failure
        AZStd::string StorePBM(
            const AZStd::string& docName,
            const AZStd::string& centuryCode,
            const PBMData& pbmData);

        // ---- Metadata (future — all writes flow through this kernel) ----

        //! Store a key-value metadata entry for a document.
        bool StoreMetadata(
            const AZStd::string& docId,
            const AZStd::string& key,
            const AZStd::string& value);

    private:
        PGconn* m_conn = nullptr;

        //! Get next document sequence number for the given namespace path.
        int NextDocSequence(
            const AZStd::string& ns,
            const AZStd::string& p2,
            const AZStd::string& p3);

        //! Insert a document row and return (pk, doc_id).
        bool InsertDocument(
            const AZStd::string& ns,
            const AZStd::string& p2,
            const AZStd::string& p3,
            const AZStd::string& p4,
            const AZStd::string& p5,
            const AZStd::string& docName,
            AZ::u32 totalSlots,
            size_t uniqueTokens,
            int& outPk,
            AZStd::string& outDocId);

        //! Route a token's positions to the correct subtable.
        bool InsertTokenPositions(
            const AZStd::string& docId,
            const AZStd::string& tokenId,
            const AZStd::vector<AZ::u32>& positions);
    };

    // ---- Base-50 position encoding utilities ----

    //! Encode a position list to base-50 text (4 chars per position).
    AZStd::string EncodePositions(const AZStd::vector<AZ::u32>& positions);

    //! Decode base-50 text back to position list.
    AZStd::vector<AZ::u32> DecodePositions(const AZStd::string& encoded);

} // namespace HCPEngine
