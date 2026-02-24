#include "HCPJsonInterpreter.h"
#include "HCPStorage.h"
#include "HCPVocabulary.h"
#include "HCPTokenizer.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <AzCore/Console/ILogger.h>

namespace HCPEngine
{
    // Serialize a rapidjson Value to a JSON string
    static AZStd::string ValueToJsonString(const rapidjson::Value& val)
    {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> w(sb);
        val.Accept(w);
        return AZStd::string(sb.GetString(), sb.GetSize());
    }

    // Extract the plain text URL from the "formats" object
    static AZStd::string ExtractTextUrl(const rapidjson::Value& formats)
    {
        if (!formats.IsObject())
            return {};

        // Prefer UTF-8 plain text
        for (auto it = formats.MemberBegin(); it != formats.MemberEnd(); ++it)
        {
            AZStd::string mime(it->name.GetString(), it->name.GetStringLength());
            if (mime.find("text/plain") != AZStd::string::npos &&
                mime.find("utf-8") != AZStd::string::npos &&
                it->value.IsString())
            {
                return AZStd::string(it->value.GetString(), it->value.GetStringLength());
            }
        }
        // Fallback: any text/plain
        for (auto it = formats.MemberBegin(); it != formats.MemberEnd(); ++it)
        {
            AZStd::string mime(it->name.GetString(), it->name.GetStringLength());
            if (mime.find("text/plain") != AZStd::string::npos && it->value.IsString())
            {
                return AZStd::string(it->value.GetString(), it->value.GetStringLength());
            }
        }
        return {};
    }

    // Known fields that map to specific DB targets
    // "title" — verify only (already set by caller)
    // "id" — provenance catalog_id
    // "authors", "subjects", "bookshelves", "languages", "copyright" — metadata JSONB
    // "formats" — provenance source_path (text URL)
    //
    // Known-discard fields (flagged for review discard testing):
    // "download_count" — irrelevant to content

    JsonInterpretResult ProcessJsonMetadata(
        const AZStd::string& jsonText,
        int docPk,
        const AZStd::string& catalog,
        HCPWriteKernel& writeKernel,
        const HCPVocabulary& vocab)
    {
        JsonInterpretResult result;

        rapidjson::Document doc;
        doc.Parse(jsonText.c_str(), jsonText.size());
        if (doc.HasParseError() || !doc.IsObject())
        {
            fprintf(stderr, "[HCPJsonInterp] Parse error or not an object\n");
            fflush(stderr);
            return result;
        }

        // Build metadata JSONB and collect unreviewed fields
        rapidjson::Document metaDoc(rapidjson::kObjectType);
        auto& alloc = metaDoc.GetAllocator();

        rapidjson::Value unreviewedObj(rapidjson::kObjectType);

        AZStd::string catalogId;
        AZStd::string sourceUrl;

        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            AZStd::string key(it->name.GetString(), it->name.GetStringLength());

            // ---- Known fields ----

            if (key == "title")
            {
                // Already set via docName in StorePBM — skip
                ++result.knownFields;
                continue;
            }

            if (key == "id")
            {
                // Catalog ID for provenance
                if (it->value.IsInt())
                    catalogId = AZStd::string::format("%d", it->value.GetInt());
                else if (it->value.IsString())
                    catalogId = AZStd::string(it->value.GetString(), it->value.GetStringLength());
                ++result.knownFields;
                continue;
            }

            if (key == "formats")
            {
                // Extract text URL for provenance, store full formats in metadata
                sourceUrl = ExtractTextUrl(it->value);
                rapidjson::Value fmtCopy(it->value, alloc);
                metaDoc.AddMember(
                    rapidjson::Value(key.c_str(), static_cast<rapidjson::SizeType>(key.size()), alloc),
                    fmtCopy, alloc);
                ++result.knownFields;
                continue;
            }

            if (key == "authors" || key == "subjects" || key == "bookshelves" ||
                key == "languages" || key == "copyright")
            {
                // Direct copy to metadata JSONB
                rapidjson::Value valCopy(it->value, alloc);
                metaDoc.AddMember(
                    rapidjson::Value(key.c_str(), static_cast<rapidjson::SizeType>(key.size()), alloc),
                    valCopy, alloc);
                ++result.knownFields;
                continue;
            }

            // ---- Known-discard fields (flagged for review) ----

            if (key == "download_count")
            {
                // Known irrelevant — flag for discard during review
                rapidjson::Value valCopy(it->value, alloc);
                rapidjson::Value discardKey("download_count", alloc);
                unreviewedObj.AddMember(discardKey, valCopy, alloc);
                ++result.unreviewedFields;
                fprintf(stderr, "[HCPJsonInterp] DISCARD candidate: '%s'\n", key.c_str());
                fflush(stderr);
                continue;
            }

            // ---- Unknown fields → unreviewed ----

            rapidjson::Value valCopy(it->value, alloc);
            unreviewedObj.AddMember(
                rapidjson::Value(key.c_str(), static_cast<rapidjson::SizeType>(key.size()), alloc),
                valCopy, alloc);
            ++result.unreviewedFields;
            fprintf(stderr, "[HCPJsonInterp] Unreviewed field: '%s'\n", key.c_str());
            fflush(stderr);
        }

        // Attach unreviewed block if non-empty
        if (unreviewedObj.MemberCount() > 0)
        {
            metaDoc.AddMember("unreviewed", unreviewedObj, alloc);
        }

        // ---- Write metadata JSONB ----
        if (metaDoc.MemberCount() > 0)
        {
            AZStd::string metaJson = ValueToJsonString(metaDoc);
            if (writeKernel.StoreDocumentMetadata(docPk, metaJson))
            {
                fprintf(stderr, "[HCPJsonInterp] Metadata stored: %d known, %d unreviewed\n",
                    result.knownFields, result.unreviewedFields);
                fflush(stderr);
            }
        }

        // ---- Write provenance ----
        if (!catalogId.empty())
        {
            AZStd::string sourceFormat = "txt";
            AZStd::string sourceType = "file";
            if (writeKernel.StoreProvenance(
                    docPk, sourceType, sourceUrl, sourceFormat, catalog, catalogId))
            {
                result.provenanceStored = true;
                fprintf(stderr, "[HCPJsonInterp] Provenance stored: catalog=%s id=%s\n",
                    catalog.c_str(), catalogId.c_str());
                fflush(stderr);
            }
        }

        return result;
    }

} // namespace HCPEngine
