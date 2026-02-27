#pragma once

#include "HCPParticlePipeline.h"
#include <AzCore/std/string/string.h>

// Forward declare libpq types
struct pg_conn;
typedef pg_conn PGconn;

namespace HCPEngine
{
    //! Lean Postgres write kernel.
    //! All database writes flow through this single class — PBM storage,
    //! metadata, provenance, everything.
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

        //! Return the integer PK of the last document inserted by StorePBM.
        //! Valid only after a successful StorePBM call.
        int LastDocPk() const { return m_lastDocPk; }

        //! Store position data for a document alongside its PBM bonds.
        //! Each unique token gets a base-50 encoded position list.
        //! Gaps in position numbering encode spaces.
        //! @param docPk Integer PK from StorePBM
        //! @param tokenIds Ordered token IDs (from Tokenize)
        //! @param positions Matching position numbers (1:1 with tokenIds)
        //! @param totalSlots Total position counter (document length)
        bool StorePositions(
            int docPk,
            const AZStd::vector<AZStd::string>& tokenIds,
            const AZStd::vector<int>& positions,
            int totalSlots);

        // ---- PBM retrieval ----

        //! Load a document's PBM bond data from the database.
        //! Reconstructs PBMData from pbm_starters and bond subtables.
        //! @param docId Document address string (e.g., "vA.AB.AS.AA.AA")
        //! @return PBMData with bonds, or empty on failure
        PBMData LoadPBM(const AZStd::string& docId);

        //! Load a document's positional token sequence.
        //! Reads position tables, decodes base-50, sorts by position,
        //! returns ordered token IDs with spaces at gaps.
        //! @param docId Document address string
        //! @return Ordered token IDs (ready for TokenIdsToText), or empty on failure
        AZStd::vector<AZStd::string> LoadPositions(const AZStd::string& docId);

        // ---- Metadata ----

        //! Merge a key-value pair into pbm_documents.metadata JSONB for a document.
        bool StoreMetadata(int docPk, const AZStd::string& key, const AZStd::string& value);

        //! Replace the full metadata JSONB blob for a document.
        bool StoreDocumentMetadata(int docPk, const AZStd::string& metadataJson);

        //! Insert a row into document_provenance.
        bool StoreProvenance(
            int docPk,
            const AZStd::string& sourceType,
            const AZStd::string& sourcePath,
            const AZStd::string& sourceFormat,
            const AZStd::string& catalog,
            const AZStd::string& catalogId);

        // ---- Document listing ----

        struct DocumentInfo
        {
            AZStd::string docId;
            AZStd::string name;
            int starters = 0;
            int bonds = 0;
        };

        //! List all stored documents with basic stats.
        AZStd::vector<DocumentInfo> ListDocuments();

        // ---- Document detail (asset manager) ----

        struct DocumentDetail
        {
            int pk = 0;
            AZStd::string docId;
            AZStd::string name;
            int totalSlots = 0;
            int uniqueTokens = 0;
            int starters = 0;
            int bonds = 0;
            AZStd::string metadataJson;  // raw JSONB from pbm_documents
        };

        struct ProvenanceInfo
        {
            AZStd::string sourceType;
            AZStd::string sourcePath;
            AZStd::string sourceFormat;
            AZStd::string catalog;
            AZStd::string catalogId;
            bool found = false;
        };

        struct DocVar
        {
            AZStd::string varId;
            AZStd::string surface;
        };

        struct DocVarExtended
        {
            AZStd::string varId;
            AZStd::string surface;
            AZStd::string category;    // uri_metadata, sic, proper, lingo
            int groupId = 0;           // 0 = ungrouped
            AZStd::string suggestedId; // entity ID from docvar_groups
            AZStd::string groupStatus; // pending, confirmed, rejected
        };

        struct BondEntry
        {
            AZStd::string tokenB;
            int count = 0;
        };

        //! Resolve a doc_id string to its integer PK. Returns 0 on failure.
        int GetDocPk(const AZStd::string& docId);

        //! Get full document detail including metadata JSONB.
        DocumentDetail GetDocumentDetail(const AZStd::string& docId);

        //! Get provenance info for a document by PK.
        ProvenanceInfo GetProvenance(int docPk);

        //! Get document-local vars for a document by PK.
        AZStd::vector<DocVar> GetDocVars(int docPk);

        //! Get extended docvar info including category, group, and suggested entity.
        AZStd::vector<DocVarExtended> GetDocVarsExtended(int docPk);

        //! Update metadata: merge setJson keys, remove listed keys.
        //! setJson is a JSON object string, removeKeys is a list of keys to delete.
        bool UpdateMetadata(int docPk, const AZStd::string& setJson,
                            const AZStd::vector<AZStd::string>& removeKeys);

        //! Get bonds for a specific token in a document.
        //! If tokenId is empty, returns top starters with their total bond counts.
        AZStd::vector<BondEntry> GetBondsForToken(int docPk, const AZStd::string& tokenId);

        //! Get ALL starters for a document (no LIMIT). For bond search.
        AZStd::vector<BondEntry> GetAllStarters(int docPk);

        //! Get raw PGconn* for cross-DB queries. Not for general use.
        PGconn* GetConnection() const { return m_conn; }

    private:
        PGconn* m_conn = nullptr;
        int m_lastDocPk = 0;
    };

    // ---- Entity cross-reference (free functions, take explicit PGconn*) ----

    struct EntityInfo
    {
        AZStd::string entityId;
        AZStd::string name;
        AZStd::string category;
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> properties;
    };

    //! Find fiction entities whose name tokens appear in a document's starters.
    //! @param ficEntConn Connection to hcp_fic_entities
    //! @param pbmConn Connection to hcp_fic_pbm
    //! @param docPk Document PK in hcp_fic_pbm
    AZStd::vector<EntityInfo> GetFictionEntitiesForDocument(
        PGconn* ficEntConn, PGconn* pbmConn, int docPk);

    //! Find a non-fiction author entity by name substring.
    //! @param nfEntConn Connection to hcp_nf_entities
    //! @param authorName Author name to match (case-insensitive substring)
    EntityInfo GetNfAuthorEntity(PGconn* nfEntConn, const AZStd::string& authorName);

} // namespace HCPEngine
