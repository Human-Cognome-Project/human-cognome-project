#pragma once

#include "HCPDbConnection.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    //! Document query and metadata kernel.
    //! Handles listing, detail, provenance, and metadata operations on pbm_documents.
    class HCPDocumentQuery
    {
    public:
        explicit HCPDocumentQuery(HCPDbConnection& conn) : m_conn(conn) {}

        // ---- Data structs ----

        struct DocumentInfo
        {
            AZStd::string docId;
            AZStd::string name;
            int starters = 0;
            int bonds = 0;
        };

        struct DocumentDetail
        {
            int pk = 0;
            AZStd::string docId;
            AZStd::string name;
            int totalSlots = 0;
            int uniqueTokens = 0;
            int starters = 0;
            int bonds = 0;
            AZStd::string metadataJson;
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

        // ---- Queries ----

        //! Resolve a doc_id string to its integer PK. Returns 0 on failure.
        int GetDocPk(const AZStd::string& docId);

        //! Resolve a (catalog, catalog_id) pair to an existing document PK via
        //! document_provenance. Returns 0 if no match found.
        int GetDocPkByCatalogId(const AZStd::string& catalog, const AZStd::string& catalogId);

        //! List all stored documents with basic stats.
        AZStd::vector<DocumentInfo> ListDocuments();

        //! Get full document detail including metadata JSONB.
        DocumentDetail GetDocumentDetail(const AZStd::string& docId);

        //! Get provenance info for a document by PK.
        ProvenanceInfo GetProvenance(int docPk);

        // ---- Writes ----

        //! Merge a key-value pair into pbm_documents.metadata JSONB.
        bool StoreMetadata(int docPk, const AZStd::string& key, const AZStd::string& value);

        //! Replace the full metadata JSONB blob for a document.
        bool StoreDocumentMetadata(int docPk, const AZStd::string& metadataJson);

        //! Update metadata: merge setJson keys, remove listed keys.
        bool UpdateMetadata(int docPk, const AZStd::string& setJson,
                            const AZStd::vector<AZStd::string>& removeKeys);

        //! Insert/update a row in document_provenance.
        bool StoreProvenance(
            int docPk,
            const AZStd::string& sourceType,
            const AZStd::string& sourcePath,
            const AZStd::string& sourceFormat,
            const AZStd::string& catalog,
            const AZStd::string& catalogId);

    private:
        HCPDbConnection& m_conn;
    };

} // namespace HCPEngine
