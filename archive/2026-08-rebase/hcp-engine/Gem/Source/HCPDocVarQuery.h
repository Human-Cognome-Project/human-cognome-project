#pragma once

#include "HCPDbConnection.h"
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace HCPEngine
{
    //! DocVar query kernel.
    //! Reads document-local variable data from pbm_docvars and docvar_groups.
    class HCPDocVarQuery
    {
    public:
        explicit HCPDocVarQuery(HCPDbConnection& conn) : m_conn(conn) {}

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

        //! Get document-local vars for a document by PK.
        AZStd::vector<DocVar> GetDocVars(int docPk);

        //! Get extended docvar info including category, group, and suggested entity.
        AZStd::vector<DocVarExtended> GetDocVarsExtended(int docPk);

    private:
        HCPDbConnection& m_conn;
    };

} // namespace HCPEngine
