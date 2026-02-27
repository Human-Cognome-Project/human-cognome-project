
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>

#include <HCPEngine/HCPEngineBus.h>
#include "HCPVocabulary.h"
#include "HCPParticlePipeline.h"
#include "HCPStorage.h"
#include "HCPSocketServer.h"
#include "HCPBondCompiler.h"
#include "HCPCacheMissResolver.h"
#include "HCPResolutionChamber.h"  // TierAssembly
#include "HCPVocabBed.h"          // BedManager

namespace HCPEngine
{
    class HCPEngineSystemComponent
        : public AZ::Component
        , protected HCPEngineRequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(HCPEngineSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        HCPEngineSystemComponent();
        ~HCPEngineSystemComponent();

        // Singleton accessor — set during Activate, cleared during Deactivate
        static HCPEngineSystemComponent* Get() { return s_instance; }

        // Accessors for socket server and other subsystems
        const HCPVocabulary& GetVocabulary() const { return m_vocabulary; }
        HCPWriteKernel& GetWriteKernel() { return m_writeKernel; }
        CacheMissResolver& GetResolver() { return m_resolver; }
        HCPParticlePipeline& GetParticlePipeline() { return m_particlePipeline; }
        const HCPBondTable& GetCharWordBonds() const { return m_charWordBonds; }
        BedManager& GetBedManager() { return m_bedManager; }
        const TierAssembly& GetTierAssembly() const { return m_tierAssembly; }
        bool IsEngineReady() const { return m_vocabulary.IsLoaded() && m_particlePipeline.IsInitialized(); }

    protected:
        ////////////////////////////////////////////////////////////////////////
        // HCPEngineRequestBus interface implementation
        AZStd::string ProcessText(
            const AZStd::string& text,
            const AZStd::string& docName,
            const AZStd::string& centuryCode) override;
        AZStd::string ReassembleFromPBM(const AZStd::string& docId) override;
        bool IsReady() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Console commands — source workstation CLI
        // These are the O3DE-native interface to kernel ops.
        // Same operations are available via socket API for remote clients.
        void SourceIngest(const AZ::ConsoleCommandContainer& arguments);
        void SourceDecode(const AZ::ConsoleCommandContainer& arguments);
        void SourceList(const AZ::ConsoleCommandContainer& arguments);
        void SourceHealth(const AZ::ConsoleCommandContainer& arguments);
        void SourceStats(const AZ::ConsoleCommandContainer& arguments);
        void SourceVars(const AZ::ConsoleCommandContainer& arguments);
        void SourcePhysTokenize(const AZ::ConsoleCommandContainer& arguments);
        void SourcePhysWordTrial(const AZ::ConsoleCommandContainer& arguments);
        void SourcePhysWordResolve(const AZ::ConsoleCommandContainer& arguments);
        ////////////////////////////////////////////////////////////////////////

    private:
        static inline HCPEngineSystemComponent* s_instance = nullptr;

        HCPVocabulary m_vocabulary;
        HCPParticlePipeline m_particlePipeline;
        HCPWriteKernel m_writeKernel;
        HCPSocketServer m_socketServer;

        // PBM bond tables — force constants for physics detection
        HCPBondTable m_charWordBonds;
        HCPBondTable m_byteCharBonds;

        // Cache miss resolver — fills LMDB from Postgres on demand
        CacheMissResolver m_resolver;

        // Persistent vocab beds — Phase 2 (char→word) resolution
        TierAssembly m_tierAssembly;
        BedManager m_bedManager;

        // Console command registrations
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceIngest, AZ::ConsoleFunctorFlags::Null, "Encode a source file into the HCP pipeline");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceDecode, AZ::ConsoleFunctorFlags::Null, "Decode a stored document back to text");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceList, AZ::ConsoleFunctorFlags::Null, "List stored documents");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceHealth, AZ::ConsoleFunctorFlags::Null, "Show engine status and vocabulary counts");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceStats, AZ::ConsoleFunctorFlags::Null, "Show encoding stats for a stored document");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourceVars, AZ::ConsoleFunctorFlags::Null, "List unresolved vars in a document");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourcePhysTokenize, AZ::ConsoleFunctorFlags::Null, "Run physics-based byte->char superposition trial");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourcePhysWordTrial, AZ::ConsoleFunctorFlags::Null, "Run physics-based char->word superposition trial");
        AZ_CONSOLEFUNC(HCPEngineSystemComponent, SourcePhysWordResolve, AZ::ConsoleFunctorFlags::Null, "Run phase-gated char->word resolution chambers");
    };
}
