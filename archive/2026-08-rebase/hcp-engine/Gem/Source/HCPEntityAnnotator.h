#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include "HCPResolutionChamber.h"  // ResolutionManifest, ResolutionResult

struct MDB_env;  // LMDB environment (defined in lmdb.h)

namespace HCPEngine
{
    // Compile-time constants matching scripts/compile_entity_lmdb.py
    static constexpr AZ::u32 ENTITY_MAX_NAME_TOKENS = 4;
    static constexpr AZ::u32 ENTITY_TOKEN_ID_WIDTH = 14;
    static constexpr AZ::u32 ENTITY_ENTRY_SIZE = 88;

    // One entity name sequence (e.g., "Sherlock Holmes" = 2 token_ids)
    struct EntitySequence
    {
        AZStd::string entityId;
        AZ::u8 nameGroup = 0;
        AZ::u8 nameType = 0;
        AZStd::vector<AZStd::string> tokenIds;  // 1-4 tokens (ordered)
    };

    //! Loads entity name sequences from LMDB and annotates resolved manifests
    //! with multi-word entity recognition.
    //!
    //! Fiction and non-fiction entities are kept in separate starter maps.
    //! Annotation walks the sorted manifest in document order, checking
    //! capitalized tokens against the starter index, greedy longest-match.
    class EntityAnnotator
    {
    public:
        //! Load entity sequences from LMDB sub-databases.
        //! Opens 'entities_fic' + 'entities_nf' from the shared vocab.lmdb env.
        bool Initialize(MDB_env* lmdbEnv);

        //! Annotate a resolved manifest with entity recognition.
        //! Walks document order, checks Label tokens (firstCap/allCaps) against
        //! entity starter index, attempts longest-match sequence matching.
        //! @param fictionFirst If true, check fiction entities before non-fiction.
        void AnnotateManifest(ResolutionManifest& manifest, bool fictionFirst) const;

        bool IsInitialized() const { return m_initialized; }
        size_t FictionSequenceCount() const { return m_ficTotal; }
        size_t NonfictionSequenceCount() const { return m_nfTotal; }
        size_t FictionStarterCount() const { return m_ficStarters.size(); }
        size_t NonfictionStarterCount() const { return m_nfStarters.size(); }

    private:
        //! Load one LMDB sub-database into a starter map.
        bool LoadSubDb(MDB_env* env, const char* dbName, const char* metaDbName,
                       AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>>& out,
                       size_t& totalOut);

        //! Try to match entities from a starter map at position i in results.
        //! Returns number of tokens consumed (0 = no match).
        AZ::u32 TryMatch(
            const AZStd::vector<ResolutionResult>& results,
            size_t i,
            const AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>>& starters) const;

        bool m_initialized = false;
        size_t m_ficTotal = 0;
        size_t m_nfTotal = 0;

        // Keyed by starter token_id → sequences sorted by length DESC (longest first)
        AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>> m_ficStarters;
        AZStd::unordered_map<AZStd::string, AZStd::vector<EntitySequence>> m_nfStarters;
    };

} // namespace HCPEngine
