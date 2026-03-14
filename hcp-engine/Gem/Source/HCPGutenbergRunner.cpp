#include "HCPGutenbergRunner.h"
#include "HCPEngineSystemComponent.h"
#include "HCPPhysIngest.h"

#include <rapidjson/document.h>

#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace HCPEngine
{
    // Scan textsDir for the first entry whose name starts with prefix (e.g. "00035_").
    // Returns the full path, or empty string if none found.
    static AZStd::string FindFileByPrefix(const AZStd::string& textsDir, const char* prefix)
    {
        DIR* dir = opendir(textsDir.c_str());
        if (!dir) return "";

        AZStd::string found;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0)
            {
                found = textsDir + "/" + entry->d_name;
                break;
            }
        }
        closedir(dir);
        return found;
    }

    GutenbergRunResult RunGutenbergTexts(
        int workingDocId,
        const AZStd::string& textsDir,
        const AZStd::string& centuryCode,
        bool fictionFirst,
        HCPEngineSystemComponent* engine,
        PGconn* varConn)
    {
        GutenbergRunResult result;

        if (!varConn || PQstatus(varConn) != CONNECTION_OK)
        {
            result.errors.push_back("No hcp_var connection");
            return result;
        }

        // Fetch raw_content from hcp_var.working_docs
        AZStd::string idStr = AZStd::to_string(workingDocId);
        const char* params[1] = { idStr.c_str() };
        PGresult* pgr = PQexecParams(varConn,
            "SELECT raw_content FROM working_docs WHERE id = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(pgr) != PGRES_TUPLES_OK || PQntuples(pgr) == 0)
        {
            PQclear(pgr);
            result.errors.push_back(
                AZStd::string("working_doc id=") + idStr + " not found");
            return result;
        }

        const char* rawJson = PQgetvalue(pgr, 0, 0);
        rapidjson::Document jdoc;
        jdoc.Parse(rawJson);
        PQclear(pgr);

        if (jdoc.HasParseError() || !jdoc.IsArray())
        {
            result.errors.push_back("working_doc raw_content is not a JSON array");
            return result;
        }

        for (const auto& entry : jdoc.GetArray())
        {
            if (!entry.IsObject()) continue;
            if (!entry.HasMember("id") || !entry["id"].IsInt()) continue;

            int catalogId = entry["id"].GetInt();

            // Zero-pad catalog ID to 5 digits for filename prefix match
            char prefix[16];
            snprintf(prefix, sizeof(prefix), "%05d_", catalogId);

            AZStd::string filePath = FindFileByPrefix(textsDir, prefix);
            if (filePath.empty())
            {
                ++result.skipped;
                fprintf(stderr, "[GutenbergRunner] No .txt for catalog_id=%d (prefix %s)\n",
                    catalogId, prefix);
                fflush(stderr);
                continue;
            }

            // Read text file
            std::ifstream ifs(filePath.c_str());
            if (!ifs.is_open())
            {
                ++result.failed;
                result.errors.push_back(AZStd::string("Cannot open: ") + filePath);
                continue;
            }
            std::string stdText((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            ifs.close();
            AZStd::string text(stdText.c_str(), stdText.size());

            // Doc name = filename stem (no extension)
            size_t lastSlash = filePath.rfind('/');
            size_t lastDot   = filePath.rfind('.');
            if (lastSlash == AZStd::string::npos) lastSlash = 0; else ++lastSlash;
            if (lastDot == AZStd::string::npos || lastDot <= lastSlash) lastDot = filePath.size();
            AZStd::string docName = filePath.substr(lastSlash, lastDot - lastSlash);

            AZStd::string catalogIdStr = AZStd::to_string(catalogId);

            ++result.processed;
            fprintf(stderr, "[GutenbergRunner] [%d/%u] catalog_id=%d: %s (%zu bytes)\n",
                result.processed, jdoc.GetArray().Size(),
                catalogId, docName.c_str(), text.size());
            fflush(stderr);

            PhysIngestResult pir = PhysIngestText(
                text, docName, centuryCode,
                "gutenberg", catalogIdStr,
                fictionFirst, false, engine);

            if (pir.ok)
            {
                ++result.succeeded;
                fprintf(stderr, "[GutenbergRunner]   → %s (%llu bonds, %llu pairs)\n",
                    pir.docId.c_str(), (unsigned long long)pir.bondTypes,
                    (unsigned long long)pir.totalPairs);
            }
            else
            {
                ++result.failed;
                AZStd::string errMsg =
                    AZStd::string("catalog_id=") + catalogIdStr + ": " + pir.errorMessage;
                result.errors.push_back(errMsg);
                fprintf(stderr, "[GutenbergRunner]   FAILED: %s\n", pir.errorMessage.c_str());
            }
            fflush(stderr);
        }

        fprintf(stderr, "[GutenbergRunner] Done: %d processed, %d succeeded, %d failed, %d skipped\n",
            result.processed, result.succeeded, result.failed, result.skipped);
        fflush(stderr);

        return result;
    }

} // namespace HCPEngine
