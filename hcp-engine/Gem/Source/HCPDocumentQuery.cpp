#include "HCPDocumentQuery.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/conversions.h>
#include <libpq-fe.h>

namespace HCPEngine
{
    int HCPDocumentQuery::GetDocPk(const AZStd::string& docId)
    {
        PGconn* pg = m_conn.Get();
        if (!pg) return 0;

        const char* params[] = { docId.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT id FROM pbm_documents WHERE doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        int pk = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            pk = atoi(PQgetvalue(res, 0, 0));
        }
        PQclear(res);
        return pk;
    }

    int HCPDocumentQuery::GetDocPkByCatalogId(
        const AZStd::string& catalog,
        const AZStd::string& catalogId)
    {
        PGconn* pg = m_conn.Get();
        if (!pg) return 0;

        const char* params[] = { catalog.c_str(), catalogId.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT d.id FROM pbm_documents d "
            "JOIN document_provenance p ON p.doc_id = d.id "
            "WHERE p.source_catalog = $1 AND p.catalog_id = $2",
            2, nullptr, params, nullptr, nullptr, 0);
        int pk = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            pk = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return pk;
    }

    AZStd::vector<HCPDocumentQuery::DocumentInfo> HCPDocumentQuery::ListDocuments()
    {
        AZStd::vector<DocumentInfo> result;
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPDocumentQuery: Not connected");
            return result;
        }

        PGresult* res = PQexec(pg,
            "SELECT d.doc_id, d.name, "
            "  (SELECT COUNT(*) FROM pbm_starters s WHERE s.doc_id = d.id) AS starters, "
            "  (SELECT COALESCE(SUM(wb.count),0) + COALESCE(SUM(cb.count),0) + "
            "          COALESCE(SUM(mb.count),0) + COALESCE(SUM(vb.count),0) "
            "   FROM pbm_starters s2 "
            "   LEFT JOIN pbm_word_bonds wb ON wb.starter_id = s2.id "
            "   LEFT JOIN pbm_char_bonds cb ON cb.starter_id = s2.id "
            "   LEFT JOIN pbm_marker_bonds mb ON mb.starter_id = s2.id "
            "   LEFT JOIN pbm_var_bonds vb ON vb.starter_id = s2.id "
            "   WHERE s2.doc_id = d.id) AS total_bonds "
            "FROM pbm_documents d ORDER BY d.doc_id");
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            for (int i = 0; i < PQntuples(res); ++i)
            {
                DocumentInfo info;
                info.docId = PQgetvalue(res, i, 0);
                info.name = PQgetvalue(res, i, 1);
                info.starters = atoi(PQgetvalue(res, i, 2));
                info.bonds = atoi(PQgetvalue(res, i, 3));
                result.push_back(AZStd::move(info));
            }
        }
        PQclear(res);
        return result;
    }

    HCPDocumentQuery::DocumentDetail HCPDocumentQuery::GetDocumentDetail(const AZStd::string& docId)
    {
        DocumentDetail detail;
        PGconn* pg = m_conn.Get();
        if (!pg) return detail;

        const char* params[] = { docId.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT d.id, d.doc_id, d.name, "
            "  COALESCE(d.total_slots, 0), COALESCE(d.unique_tokens, 0), "
            "  COALESCE(d.metadata::text, '{}'), "
            "  (SELECT COUNT(*) FROM pbm_starters s WHERE s.doc_id = d.id), "
            "  (SELECT COALESCE(SUM(sub.cnt), 0) FROM ("
            "    SELECT SUM(wb.count) AS cnt FROM pbm_starters s2 "
            "      JOIN pbm_word_bonds wb ON wb.starter_id = s2.id WHERE s2.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(cb.count) FROM pbm_starters s3 "
            "      JOIN pbm_char_bonds cb ON cb.starter_id = s3.id WHERE s3.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(mb.count) FROM pbm_starters s4 "
            "      JOIN pbm_marker_bonds mb ON mb.starter_id = s4.id WHERE s4.doc_id = d.id "
            "    UNION ALL "
            "    SELECT SUM(vb.count) FROM pbm_starters s5 "
            "      JOIN pbm_var_bonds vb ON vb.starter_id = s5.id WHERE s5.doc_id = d.id "
            "  ) sub) "
            "FROM pbm_documents d WHERE d.doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            detail.pk = atoi(PQgetvalue(res, 0, 0));
            detail.docId = PQgetvalue(res, 0, 1);
            detail.name = PQgetvalue(res, 0, 2);
            detail.totalSlots = atoi(PQgetvalue(res, 0, 3));
            detail.uniqueTokens = atoi(PQgetvalue(res, 0, 4));
            detail.metadataJson = PQgetvalue(res, 0, 5);
            detail.starters = atoi(PQgetvalue(res, 0, 6));
            detail.bonds = atoi(PQgetvalue(res, 0, 7));
        }
        PQclear(res);
        return detail;
    }

    HCPDocumentQuery::ProvenanceInfo HCPDocumentQuery::GetProvenance(int docPk)
    {
        ProvenanceInfo prov;
        PGconn* pg = m_conn.Get();
        if (!pg) return prov;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };
        PGresult* res = PQexecParams(pg,
            "SELECT source_type, source_path, source_format, source_catalog, catalog_id "
            "FROM document_provenance WHERE doc_id = $1",
            1, nullptr, params, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            prov.sourceType = PQgetvalue(res, 0, 0);
            prov.sourcePath = PQgetvalue(res, 0, 1);
            prov.sourceFormat = PQgetvalue(res, 0, 2);
            prov.catalog = PQgetvalue(res, 0, 3);
            prov.catalogId = PQgetvalue(res, 0, 4);
            prov.found = true;
        }
        PQclear(res);
        return prov;
    }

    bool HCPDocumentQuery::StoreMetadata(
        int docPk,
        const AZStd::string& key,
        const AZStd::string& value)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPDocumentQuery: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str(), key.c_str(), value.c_str() };
        PGresult* res = PQexecParams(pg,
            "UPDATE pbm_documents SET metadata = metadata || jsonb_build_object($2, $3::jsonb) "
            "WHERE id = $1::integer",
            3, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPDocumentQuery] StoreMetadata failed: %s\n", PQerrorMessage(pg));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    bool HCPDocumentQuery::StoreDocumentMetadata(
        int docPk,
        const AZStd::string& metadataJson)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPDocumentQuery: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str(), metadataJson.c_str() };
        PGresult* res = PQexecParams(pg,
            "UPDATE pbm_documents SET metadata = metadata || $2::jsonb "
            "WHERE id = $1::integer",
            2, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPDocumentQuery] StoreDocumentMetadata failed: %s\n", PQerrorMessage(pg));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    bool HCPDocumentQuery::UpdateMetadata(
        int docPk,
        const AZStd::string& setJson,
        const AZStd::vector<AZStd::string>& removeKeys)
    {
        PGconn* pg = m_conn.Get();
        if (!pg) return false;

        AZStd::string pkStr = AZStd::to_string(docPk);
        bool ok = true;

        // Merge new keys
        if (!setJson.empty() && setJson != "{}")
        {
            const char* params[] = { pkStr.c_str(), setJson.c_str() };
            PGresult* res = PQexecParams(pg,
                "UPDATE pbm_documents SET metadata = COALESCE(metadata, '{}'::jsonb) || $2::jsonb "
                "WHERE id = $1::integer",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[HCPDocumentQuery] UpdateMetadata merge failed: %s\n",
                    PQerrorMessage(pg));
                fflush(stderr);
                ok = false;
            }
            PQclear(res);
        }

        // Remove keys one at a time
        for (const auto& key : removeKeys)
        {
            const char* params[] = { pkStr.c_str(), key.c_str() };
            PGresult* res = PQexecParams(pg,
                "UPDATE pbm_documents SET metadata = metadata - $2 "
                "WHERE id = $1::integer",
                2, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[HCPDocumentQuery] UpdateMetadata remove '%s' failed: %s\n",
                    key.c_str(), PQerrorMessage(pg));
                fflush(stderr);
                ok = false;
            }
            PQclear(res);
        }

        return ok;
    }

    bool HCPDocumentQuery::StoreProvenance(
        int docPk,
        const AZStd::string& sourceType,
        const AZStd::string& sourcePath,
        const AZStd::string& sourceFormat,
        const AZStd::string& catalog,
        const AZStd::string& catalogId)
    {
        PGconn* pg = m_conn.Get();
        if (!pg)
        {
            AZLOG_ERROR("HCPDocumentQuery: Not connected");
            return false;
        }

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = {
            pkStr.c_str(), sourceType.c_str(), sourcePath.c_str(),
            sourceFormat.c_str(), catalog.c_str(), catalogId.c_str()
        };
        PGresult* res = PQexecParams(pg,
            "INSERT INTO document_provenance "
            "(doc_id, source_type, source_path, source_format, source_catalog, catalog_id) "
            "VALUES ($1::integer, $2, $3, $4, $5, $6) "
            "ON CONFLICT (doc_id) DO UPDATE SET "
            "source_type = EXCLUDED.source_type, "
            "source_path = EXCLUDED.source_path, "
            "source_format = EXCLUDED.source_format, "
            "source_catalog = EXCLUDED.source_catalog, "
            "catalog_id = EXCLUDED.catalog_id",
            6, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPDocumentQuery] StoreProvenance failed: %s\n", PQerrorMessage(pg));
            fflush(stderr);
        }
        PQclear(res);
        return ok;
    }

    int HCPDocumentQuery::DeleteDocument(int docPk)
    {
        PGconn* pg = m_conn.Get();
        if (!pg) return 0;

        AZStd::string pkStr = AZStd::to_string(docPk);
        const char* params[] = { pkStr.c_str() };

        // Delete in dependency order (no ON DELETE CASCADE on most tables).
        // Bond tables reference pbm_starters, which references pbm_documents.
        static const char* childDeletes[] = {
            "DELETE FROM pbm_word_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = $1::integer)",
            "DELETE FROM pbm_char_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = $1::integer)",
            "DELETE FROM pbm_marker_bonds WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = $1::integer)",
            "DELETE FROM pbm_var_bonds    WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = $1::integer)",
            "DELETE FROM pbm_starters     WHERE doc_id = $1::integer",
            "DELETE FROM pbm_docvars      WHERE doc_id = $1::integer",
            "DELETE FROM docvar_staging   WHERE doc_id = $1::integer",
            "DELETE FROM document_provenance    WHERE doc_id = $1::integer",
            "DELETE FROM document_cross_refs    WHERE source_doc_id = $1::integer",
            nullptr
        };
        for (int i = 0; childDeletes[i]; ++i)
        {
            PGresult* res = PQexecParams(pg, childDeletes[i], 1, nullptr, params, nullptr, nullptr, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "[HCPDocumentQuery] DeleteDocument child delete failed: %s\n",
                    PQerrorMessage(pg));
                fflush(stderr);
                // Reset connection error state so subsequent statements can run
                PQclear(res);
                PQexec(pg, "ROLLBACK");
                continue;
            }
            PQclear(res);
        }

        // Delete the document itself — ON DELETE CASCADE handles pbm_position_caps + pbm_morpheme_positions
        PGresult* res = PQexecParams(pg,
            "DELETE FROM pbm_documents WHERE id = $1::integer",
            1, nullptr, params, nullptr, nullptr, 0);
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        if (!ok)
        {
            fprintf(stderr, "[HCPDocumentQuery] DeleteDocument failed: %s\n", PQerrorMessage(pg));
            fflush(stderr);
        }
        int deleted = ok ? atoi(PQcmdTuples(res)) : 0;
        PQclear(res);
        return deleted;
    }

} // namespace HCPEngine
