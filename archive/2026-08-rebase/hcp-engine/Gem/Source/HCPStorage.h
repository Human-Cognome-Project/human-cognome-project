#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

// Forward declare libpq types
struct pg_conn;
typedef pg_conn PGconn;

namespace HCPEngine
{
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
