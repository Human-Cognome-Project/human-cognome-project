
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/ILogger.h>

#include "HCPEngineSystemComponent.h"
#include "HCPTokenizer.h"
#include "HCPStorage.h"

#include <HCPEngine/HCPEngineTypeIds.h>

// PhysX access — we link against Gem::PhysX5.Static which exposes internal headers
#include <System/PhysXSystem.h>

namespace HCPEngine
{
    AZ_COMPONENT_IMPL(HCPEngineSystemComponent, "HCPEngineSystemComponent",
        HCPEngineSystemComponentTypeId);

    void HCPEngineSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HCPEngineSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void HCPEngineSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("HCPEngineService"));
    }

    void HCPEngineSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("HCPEngineService"));
    }

    void HCPEngineSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // Require PhysX to be initialized before we activate
        required.push_back(AZ_CRC_CE("PhysXService"));
    }

    void HCPEngineSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    HCPEngineSystemComponent::HCPEngineSystemComponent()
    {
        if (HCPEngineInterface::Get() == nullptr)
        {
            HCPEngineInterface::Register(this);
        }
    }

    HCPEngineSystemComponent::~HCPEngineSystemComponent()
    {
        if (HCPEngineInterface::Get() == this)
        {
            HCPEngineInterface::Unregister(this);
        }
    }

    void HCPEngineSystemComponent::Init()
    {
    }

    void HCPEngineSystemComponent::Activate()
    {
        HCPEngineRequestBus::Handler::BusConnect();

        AZLOG_INFO("HCPEngine: Activating — loading vocabulary and initializing PBD particle system");

        // Load vocabulary hash table from PostgreSQL
        if (!m_vocabulary.Load())
        {
            AZLOG_ERROR("HCPEngine: Failed to load vocabulary — engine will not function");
            return;
        }

        // Get PhysX physics and foundation from O3DE's PhysX system
        PhysX::PhysXSystem* physxSystem = PhysX::GetPhysXSystem();
        if (!physxSystem)
        {
            AZLOG_ERROR("HCPEngine: PhysX system not available");
            return;
        }

        physx::PxPhysics* pxPhysics = physxSystem->GetPxPhysics();
        if (!pxPhysics)
        {
            AZLOG_ERROR("HCPEngine: PxPhysics not available");
            return;
        }

        // Get foundation from physics tolerances (there's only one per process)
        physx::PxFoundation& foundation = pxPhysics->getFoundation();

        // Initialize PBD particle pipeline with CUDA + GPU scene
        if (!m_particlePipeline.Initialize(pxPhysics, &foundation))
        {
            AZLOG_ERROR("HCPEngine: Failed to initialize PBD particle pipeline");
            return;
        }

        AZLOG_INFO("HCPEngine: Ready — vocab: %zu words, %zu labels, %zu chars; PBD particle system active",
            m_vocabulary.WordCount(), m_vocabulary.LabelCount(), m_vocabulary.CharCount());
    }

    void HCPEngineSystemComponent::Deactivate()
    {
        AZLOG_INFO("HCPEngine: Deactivating — shutting down PBD pipeline");
        m_particlePipeline.Shutdown();
        HCPEngineRequestBus::Handler::BusDisconnect();
    }

    bool HCPEngineSystemComponent::IsReady() const
    {
        return m_vocabulary.IsLoaded() && m_particlePipeline.IsInitialized();
    }

    AZStd::string HCPEngineSystemComponent::ProcessText(
        const AZStd::string& text,
        const AZStd::string& docName,
        const AZStd::string& centuryCode)
    {
        if (!IsReady())
        {
            AZLOG_ERROR("HCPEngine: Not ready — call Activate first");
            return {};
        }

        AZLOG_INFO("HCPEngine: Processing '%s' (%zu chars)", docName.c_str(), text.size());

        // Step 1: Tokenize — hash table lookup, all inside the engine
        AZStd::vector<AZStd::string> tokenIds = Tokenize(text, m_vocabulary);
        if (tokenIds.size() < 3) // At least start + one token + end
        {
            AZLOG_ERROR("HCPEngine: Tokenization produced too few tokens");
            return {};
        }

        // Step 2: Disassemble via PBD particle physics
        PBMData pbmData = m_particlePipeline.Disassemble(tokenIds);
        if (pbmData.bonds.empty())
        {
            AZLOG_ERROR("HCPEngine: Disassembly produced no bonds");
            return {};
        }

        // Step 3: Store to hcp_fic_pbm prefix tree schema
        AZStd::string docId = StorePBM(docName, centuryCode, pbmData);
        if (docId.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to store PBM");
            return {};
        }

        AZLOG_INFO("HCPEngine: Stored PBM %s — %zu tokens, %zu unique bonds, %zu total pairs",
            docId.c_str(), tokenIds.size(), pbmData.bonds.size(), pbmData.totalPairs);

        return docId;
    }

    AZStd::string HCPEngineSystemComponent::ReassembleFromPBM(const AZStd::string& docId)
    {
        if (!IsReady())
        {
            AZLOG_ERROR("HCPEngine: Not ready");
            return {};
        }

        AZLOG_INFO("HCPEngine: Reassembling from PBM %s", docId.c_str());

        // Load PBM from database
        PBMData pbmData = LoadPBM(docId);
        if (pbmData.bonds.empty())
        {
            AZLOG_ERROR("HCPEngine: Failed to load PBM %s", docId.c_str());
            return {};
        }

        // Reassemble via PBD particle physics
        AZStd::vector<AZStd::string> sequence = m_particlePipeline.Reassemble(pbmData, m_vocabulary);

        // Convert token sequence back to text
        AZStd::string text;
        text.reserve(sequence.size() * 5);

        for (size_t i = 0; i < sequence.size(); ++i)
        {
            const AZStd::string& tid = sequence[i];

            // Skip anchors
            if (tid == STREAM_START || tid == STREAM_END)
            {
                continue;
            }

            // Word token (AB.AB.*)
            if (tid.starts_with("AB.AB."))
            {
                AZStd::string word = m_vocabulary.TokenToWord(tid);
                if (!word.empty())
                {
                    // Insert space before words (unless at start or after newline)
                    if (!text.empty() && text.back() != '\n' && text.back() != ' ')
                    {
                        text += ' ';
                    }
                    text += word;
                    continue;
                }
            }

            // Character/punctuation token (AA.*)
            char c = m_vocabulary.TokenToChar(tid);
            if (c != '\0')
            {
                text += c;
                continue;
            }

            // Label token — check for newline
            AZStd::string label = m_vocabulary.LookupLabel("newline");
            if (!label.empty() && tid == label)
            {
                text += '\n';
                continue;
            }
        }

        AZLOG_INFO("HCPEngine: Reassembled %zu tokens into %zu chars", sequence.size(), text.size());
        return text;
    }
}
