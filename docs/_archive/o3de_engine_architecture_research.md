# O3DE Architecture Research: Engine Runtime, Levels, and Asset Processing

**Research Date:** 2026-02-19
**Focus:** How the tokenizer/particle pipeline maps to O3DE's Asset Builder, Level, and Entity systems — and how conversations map to Levels with dynamically loaded document assets.

---

## Executive Summary

O3DE's existing infrastructure provides most of what the HCP engine has been building manually:

- **Asset Processor + Asset Builder** replaces the socket server ingest path, with built-in file watching, fingerprinting, caching, dependency tracking, and LoD
- **Levels (root spawnables)** map directly to conversation contexts — a single active workspace where document assets are loaded and manipulated
- **EntitySpawnTicket** provides dynamic document loading/unloading within a live conversation
- **PhysX scene per level** means token particles from multiple documents coexist in the same simulation space
- **Asset<T> with NoLoad/QueueLoad/PreLoad** gives us tiered document loading — summary first, full detail on demand

The tokenizer, vocabulary reader, and particle pipeline stay. They move from a socket handler into an Asset Builder's `ProcessJob()` and a Level's component entities.

---

## 1. Asset Builder — Document Processing Pipeline

### What an Asset Builder Is

An Asset Builder registers with the Asset Processor to handle specific file types. When a matching source file is detected (via file watching or manual scan), the builder is invoked in two phases:

1. **CreateJobs(request) → response** — Analyze the source file, declare what jobs to run
2. **ProcessJob(request) → response** — Execute the actual processing, emit product assets

### Key Data Structures

**AssetBuilderDesc** (registration):
```cpp
struct AssetBuilderDesc {
    AZStd::string m_name;                          // "HCPDocumentBuilder"
    AZStd::vector<AssetBuilderPattern> m_patterns;  // "*.txt", "*.gutenberg"
    AZ::Uuid m_busId;                               // Unique builder ID
    int m_version;                                  // Bump = force rebuild all
    CreateJobFunction m_createJobFunction;
    ProcessJobFunction m_processJobFunction;
    AZStd::string m_analysisFingerprint;            // Change = re-analyze
};
```

**ProcessJobRequest** (input to processing):
```cpp
struct ProcessJobRequest {
    AZStd::string m_fullPath;          // Absolute path to source file
    AZStd::string m_tempDirPath;       // Temp folder for outputs
    AZ::Uuid m_sourceFileUUID;         // Stable source identity
    JobDescriptor m_jobDescription;    // From CreateJobs
    PlatformInfo m_platformInfo;       // Target platform
};
```

**JobProduct** (output from processing):
```cpp
struct JobProduct {
    AZStd::string m_productFileName;                   // Output filename
    AZ::Data::AssetType m_productAssetType;            // Our custom type UUID
    AZ::u32 m_productSubID;                            // Stable product ID + LoD bits
    AZStd::vector<ProductDependency> m_dependencies;   // What this product needs
    ProductOutputFlags m_outputFlags;                  // ProductAsset / IntermediateAsset
};
```

### How Our Pipeline Maps

```
Source: data/gutenberg/texts/01952_The Yellow Wallpaper.txt
    ↓
HCPDocumentBuilder::CreateJobs()
    → JobDescriptor { jobKey="tokenize", platform="common", critical=false }
    ↓
HCPDocumentBuilder::ProcessJob()
    ├── Read source text (request.m_fullPath)
    ├── Tokenize (4-step space-to-space pipeline)
    ├── Position disassembly (2519 unique tokens, 18840 slots)
    ├── Base-50 encode positions
    ├── Derive PBM bonds (6774 unique bonds)
    └── Emit JobProducts:
        ├── SubID 0: token positions (word-level, ~51KB)
        ├── SubID 1: PBM bond data (6774 bonds)
        └── SubID with LoD bits: aggregated tiers (future)
    ↓
Products cached at: Cache/linux/data/gutenberg/texts/01952_The_Yellow_Wallpaper.*
```

### What We Get for Free

| Feature | Manual implementation | Asset Processor |
|---|---|---|
| File watching | Socket server + ingest script | Automatic — FileWatcher monitors scan dirs |
| Change detection | None | Fingerprint (content hash + builder version) |
| Skip unchanged | None | Automatic — same fingerprint = no rebuild |
| Parallel processing | Sequential socket calls | Job scheduler with priority queues |
| Dependency tracking | None | Source deps, job deps, product deps |
| Error handling | Socket error responses | Job status tracking, retry, diagnostics |
| LoD | Not built | SubID LoD bits (15 tiers) |
| Hot-reload | Not built | OnAssetReloaded event, automatic |

### SubID and LoD Encoding

SubIDs use bit packing for LoD:
```
Bits 0-15:  Product identifier (up to 65535 products per source)
Bits 16-19: LoD level (0-15, where 0 = base/highest detail)
```

Helper functions:
```cpp
AZ::u32 ConstructSubID(AZ::u32 subIndex, AZ::u32 lodLevel, AZ::u32 fromSubIndex = 0);
AZ::u32 GetSubID_ID(AZ::u32 packedSubId);
AZ::u32 GetSubID_LOD(AZ::u32 packedSubId);
```

For document products:
```
SubID = ConstructSubID(0, 0)  → Full token positions (base detail)
SubID = ConstructSubID(1, 0)  → PBM bond data (base detail)
SubID = ConstructSubID(0, 1)  → Sentence-level aggregation (LoD 1)
SubID = ConstructSubID(0, 2)  → Clause-level aggregation (LoD 2)
```

**SubID stability rules** (critical):
- Same source → same SubID across builds
- Source file moves → SubID stays the same
- Must be stable across platforms
- Multiple products must have mutually exclusive SubIDs

---

## 2. Levels as Conversation Workspaces

### What a Level Is

A Level in O3DE is a **root spawnable** — a compiled prefab that represents the active world state. Key properties:

- **One root spawnable at a time** — loading a new level unloads the previous one
- **Entity hierarchy** — the level contains entities with components
- **PhysX scene** — belongs to the level, all physics simulation happens here
- **Dynamic spawning** — additional entities can be spawned/despawned at runtime via `EntitySpawnTicket`

### Level Data Flow

```
Source (editor):       DefaultLevel.prefab (JSON entity hierarchy)
                           ↓ [Prefab Conversion Pipeline]
Product (runtime):     DefaultLevel.spawnable (compiled binary)
                           ↓ [SpawnableLevelSystem]
Active (in-memory):    Root spawnable with live entities + PhysX scene
```

### Prefab Structure (on disk)

```json
{
    "Instances": {
        "nested_prefab_1": {
            "Source": "path/to/source.prefab",
            "Patches": { /* overrides */ }
        }
    },
    "Entities": {
        "Entity1": {
            "Components": { /* ... */ },
            "Transform": { /* ... */ }
        }
    }
}
```

### Spawnable Structure (runtime)

```cpp
class Spawnable : public AZ::Data::AssetData {
    EntityList m_entities;              // Flat list of entity templates
    EntityAliasList m_entityAliases;    // Cross-prefab entity linking
    SpawnableMetaData m_metaData;       // Read-only key-value metadata
};
```

### Prefab Conversion Pipeline

Pluggable processor stack converts prefabs to spawnables:

```cpp
class PrefabConversionPipeline {
    PrefabProcessorStack m_processors;
    // Default processors:
    //   EditorInfoRemover — strips editor-only data
    //   AssetPlatformComponentRemover — removes platform-specific components
    //   ComponentRequirementsValidator — validates component dependencies
    //   Custom processors can be added (e.g., HCPAssetPreprocessor)
};
```

### How Conversations Map to Levels

| Conversation concept | O3DE Level mechanism |
|---|---|
| Active conversation | Root spawnable (one at a time) |
| Conversation workspace | PhysX scene (particles live here) |
| Vocabulary (shared) | PreLoad asset on vocabulary entity |
| Document in context | Entity spawned via EntitySpawnTicket |
| Document detail level | Asset LoD tier (NoLoad → QueueLoad on demand) |
| Remove document | DespawnEntity() or DespawnAllEntities() |
| Pause document | DeactivateEntity() — state preserved |
| Resume document | ActivateEntity() — picks up where left off |
| Switch conversation | AssignRootSpawnable(newLevel) — old auto-unloads |

---

## 3. Dynamic Entity Spawning at Runtime

### EntitySpawnTicket

The core handle for spawning/despawning entities within a live level:

```cpp
// Create ticket for a spawnable asset
EntitySpawnTicket ticket(documentSpawnable);

// Spawn all entities from the template
SpawnableEntitiesInterface::Get()->SpawnAllEntities(ticket, {
    .m_priority = SpawnablePriority_Default,
    .m_preInsertionCallback = [](auto id, auto& view) { /* modify before activate */ },
    .m_completionCallback = [](auto& ticket, auto ids) { /* track spawned entities */ }
});

// Later: despawn specific entities
SpawnableEntitiesInterface::Get()->DespawnEntity(entityId, ticket);

// Or despawn all from this ticket
SpawnableEntitiesInterface::Get()->DespawnAllEntities(ticket);
```

**Key properties:**
- Thread-safe — spawn/despawn from any thread
- Reusable — same ticket for multiple spawn/despawn cycles
- Tracks ID mapping — prototype entity IDs → spawned instance IDs
- Priority-based command queue (High priority < 64, Regular >= 64)

### SpawnableEntitiesManager

Two-tier command queue:
- **High priority queue** — level loading, critical entities
- **Regular priority queue** — document entities, dynamic content

14 command types including:
- SpawnAllEntities / SpawnEntities (by index)
- DespawnAllEntities / DespawnEntity
- ReloadSpawnable (hot-reload)
- UpdateEntityAliasTypes (runtime alias changes)
- LoadBarrier (wait for dependencies)
- ListEntities / ClaimEntities

### EntityAlias Types

```cpp
enum class EntityAliasType : uint8_t {
    Original,   // Spawn the entity as-is
    Disable,    // Skip this entity (don't spawn)
    Replace,    // Spawn alias entity instead
    Additional, // Spawn original + alias (alias gets new EntityId)
    Merge       // Spawn original, add alias's components
};
```

**Runtime updatable** — `UpdateEntityAliasTypes()` can change alias behavior after spawning.

### Entity Lifecycle

```
Constructed → Init() → Activate() ←→ Deactivate() → Destroyed
                         ↑                    ↑
                     Entity active         Entity paused
                     Components running    State preserved
```

Entities can be deactivated and reactivated without despawning. Component state persists across the cycle. This is important for "pausing" a document's participation in the conversation without unloading it.

---

## 4. Runtime Asset Loading

### Load Behaviors

| Behavior | When loaded | Parent ready? | Use case |
|---|---|---|---|
| PreLoad | Immediately during deserialization | Parent waits until all PreLoad deps ready | Core vocabulary, must-have data |
| QueueLoad | Immediately during deserialization | Parent ready immediately (dep loads async) | Most document assets |
| NoLoad | Not loaded until explicit QueueLoad() call | N/A | On-demand LoD tiers |

### Loading Flow

```
Component references Asset<HCPDocumentAsset> with NoLoad
    ↓ (conversation needs this document)
asset.QueueLoad()
    ↓
AssetManager queries catalog for file location
    ↓
Streamer async reads from Cache/
    ↓
HCPDocumentHandler::LoadAssetData() deserializes
    ↓
OnAssetReady event fires on main thread
    ↓
Component spawns particles into PhysX scene
```

### Asset Containers (Parallel Dependency Loading)

```cpp
class AssetContainer {
    // Load root asset + all dependencies in parallel
    GetDependencies()            → map of all loaded assets
    GetUnloadedDependencies()    → set of NoLoad assets not yet loaded
    GetNumWaitingDependencies()  → progress tracking
};
```

When loading a document that depends on vocabulary data, the container loads both in parallel, respecting PreLoad ordering.

### Hot-Reload

When Asset Processor reprocesses a source file:
1. New product written to Cache/
2. `AssetManager::ReloadAsset()` triggered
3. `OnAssetPreReload()` notification
4. Handler loads fresh data, replaces in-place (same asset pointer)
5. `OnAssetReloaded()` notification
6. Component sees same `Asset<T>` reference, but with new data

This means: edit a source text → Asset Processor re-tokenizes → level gets updated document automatically.

### Asset Events

```cpp
class AssetEvents {
    OnAssetReady(asset)           // Fully loaded, safe to use
    OnAssetReloaded(asset)        // Hot-reloaded with new data
    OnAssetUnloaded(id, type)     // Released from memory
    OnAssetError(asset)           // Load failed
    OnAssetCanceled(id)           // Load aborted (refs released)
    OnAssetContainerReady(asset)  // All deps in container ready
};
```

Components connect to AssetBus by AssetId to receive these events.

---

## 5. Conversation Runtime Model

### The Full Picture

```
Conversation starts
    → Load conversation level (root spawnable)
    → PhysX scene created (PBD particle space)
    → Vocabulary entity activates
        → Vocabulary asset PreLoaded (must be ready before level "active")

User references Document A
    → Create EntitySpawnTicket for Document A
    → Document A asset: QueueLoad() on base tier (LoD 0 = positions)
    → OnAssetReady fires
    → HCPDocumentComponent::Activate()
        → Token particles spawned into PhysX scene
        → PBM derivation runs across all active particles

User references Document B
    → Create EntitySpawnTicket for Document B
    → Both documents' particles coexist in same PhysX scene
    → Cross-document PBM bonds derived

Need more detail on Document A
    → Document A LoD 1 asset: QueueLoad()
    → Higher-detail positions loaded
    → Particles updated in scene

Document B no longer relevant
    → DespawnEntity() — particles removed from scene
    → Or DeactivateEntity() — paused, state kept, reactivable later

Conversation ends
    → AssignRootSpawnable(newLevel) or release root
    → All entities automatically despawned
    → PhysX scene destroyed
    → Assets released (ref count → 0 → OnAssetUnloaded)
```

### Multiple Conversations

O3DE enforces one root spawnable at a time. Options:

1. **Switch conversations** — `AssignRootSpawnable(newConversation)` unloads old, loads new
2. **Sub-conversations** — use non-root EntitySpawnTickets for secondary document groups within the same level
3. **Conversation state persistence** — serialize conversation state before switching, restore on return

### Component Architecture for a Conversation Level

```
Conversation Level (root spawnable)
├── VocabularyEntity
│   └── HCPVocabularyComponent (PreLoad vocabulary asset)
├── ParticlePipelineEntity
│   └── HCPParticlePipelineComponent (owns PxScene, CUDA context)
├── ConversationManagerEntity
│   └── HCPConversationComponent (manages document spawn tickets)
└── [Dynamically spawned per document:]
    └── DocumentEntity (spawned via EntitySpawnTicket)
        └── HCPDocumentComponent (holds Asset<HCPDocumentAsset>, manages particles)
```

---

## 6. What We Keep vs What Changes

### Keep (code moves, logic stays)

| Component | Current location | New location |
|---|---|---|
| 4-step tokenizer pipeline | HCPTokenizer.cpp (socket handler) | HCPDocumentBuilder::ProcessJob() |
| Vocabulary LMDB reader | HCPVocabulary.cpp | Vocabulary asset or service |
| PBD particle pipeline | HCPParticlePipeline.cpp | HCPParticlePipelineComponent on level entity |
| Position encoding (base-50) | HCPStorage.cpp | HCPDocumentBuilder output format |
| PBM derivation | HCPParticlePipeline.cpp | Runs in level's PhysX scene |
| Control tokens | HCPVocabulary.h constants | Same, referenced by builder + components |

### Drop or Reshape

| Component | Reason |
|---|---|
| HCPSocketServer | Asset Processor replaces ingest path |
| HCPWriteKernel (Postgres doc writes) | Products go to Cache/, catalog handles registry |
| Manual LMDB cache management | Could become asset type or service layer |
| ingest_texts.py script | Drop files in scan dir instead |
| HCPEngineSystemComponent (monolith) | Split into per-entity components |

### DB Specialist's Role Shifts

- **Vocabulary management** — could be a separate Asset Builder producing vocabulary assets, or a runtime service via AZ::Interface
- **Cross-document aggregation** — PBM queries across documents still need a query layer (Postgres or new mechanism)
- **LMDB** — one option among several for the vocabulary cache; the Asset Processor's own caching may replace some of its role
- **Forward DB** — could be an asset product (built offline), or a runtime service

---

## 7. Key Source Files

### Asset Builder SDK
- `/opt/O3DE/25.10.2/Code/Tools/AssetProcessor/AssetBuilderSDK/AssetBuilderSDK/AssetBuilderSDK.h` — Main SDK header
- `/opt/O3DE/25.10.2/Code/Tools/AssetProcessor/AssetBuilderSDK/AssetBuilderSDK/AssetBuilderBusses.h` — EBus definitions

### Asset Runtime
- `/opt/O3DE/25.10.2/Code/Framework/AzCore/AzCore/Asset/AssetCommon.h` — Asset<T>, AssetId, AssetBus events
- `/opt/O3DE/25.10.2/Code/Framework/AzCore/AzCore/Asset/AssetManager.h` — AssetManager, AssetHandler
- `/opt/O3DE/25.10.2/Code/Framework/AzCore/AzCore/Asset/AssetContainer.h` — Parallel dependency loading
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Asset/AssetCatalog.h` — Runtime catalog

### Spawnable / Level System
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Spawnable/Spawnable.h` — Core spawnable, EntityAlias
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Spawnable/SpawnableEntitiesInterface.h` — Spawn API, EntitySpawnTicket
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Spawnable/SpawnableEntitiesManager.h` — Command queue
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Spawnable/RootSpawnableInterface.h` — Level management
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Spawnable/SpawnableMetaData.h` — Read-only metadata

### Prefab System
- `/opt/O3DE/25.10.2/Code/Framework/AzToolsFramework/AzToolsFramework/Prefab/Instance/Instance.h` — Prefab instance
- `/opt/O3DE/25.10.2/Code/Framework/AzToolsFramework/AzToolsFramework/Prefab/Template/Template.h` — Prefab template
- `/opt/O3DE/25.10.2/Code/Framework/AzToolsFramework/AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h` — Conversion pipeline

### Entity / Component
- `/opt/O3DE/25.10.2/Code/Framework/AzCore/AzCore/Component/Entity.h` — Entity class
- `/opt/O3DE/25.10.2/Code/Framework/AzCore/AzCore/Component/Component.h` — Component base class
- `/opt/O3DE/25.10.2/Code/Framework/AzFramework/AzFramework/Entity/EntityContextBus.h` — Entity lifecycle API

### Asset Processor
- `/opt/O3DE/25.10.2/Code/Tools/AssetProcessor/native/AssetManager/assetProcessorManager.h` — Orchestrator
- `/opt/O3DE/25.10.2/Code/Tools/AssetProcessor/native/AssetDatabase/AssetDatabase.h` — SQLite tracking DB

---

## 8. Distribution Model — Standalone Asset Processor Package

### O3DE Distribution Tiers

Our install at `/opt/O3DE/25.10.2/` is already an **SDK build** (`"sdk_engine": true` in `engine.json`). Pre-built binaries, no source compilation required.

O3DE has three distribution models:

| Model | Contents | Use case |
|---|---|---|
| **Full Source** (`o3de`) | Complete engine source, must compile everything | Engine developers |
| **SDK Install** (`o3de-sdk`) | Pre-built binaries + headers, projects build only their code | Project developers, contributors |
| **Exported Package** | Launcher + processed assets, no tools | End users / deployment |

### What the SDK Includes

```
/opt/O3DE/25.10.2/
├── bin/Linux/profile/Default/
│   ├── AssetProcessor          (8.0M, GUI — Qt-based dashboard)
│   ├── AssetProcessorBatch     (7.5M, CLI — headless processing)
│   ├── AssetBundler            (bundling tool)
│   ├── AssetBuilder            (builder host process)
│   └── Builders/               (built-in builders load from here)
├── Code/Framework/             (SDK headers for gem development)
├── Gems/                       (80+ built-in gems)
├── cmake/                      (build system, needed by projects)
├── scripts/o3de/               (CLI tools + export scripts)
├── lib/                        (pre-compiled engine libraries)
└── Registry/                   (default settings)
```

### Contributor Workflow

A contributor needs three things:

1. **O3DE SDK** — pre-built installer, downloadable package (~8GB)
2. **HCPEngine project** — our repo (Gem code + project config)
3. **Build our Gem** — `cmake + ninja`, compiles only HCPEngine code against SDK headers (~2 min)

```
Contributor installs O3DE SDK (one-time)
    → All tools pre-built (AssetProcessor, AssetBuilder, etc.)

Contributor clones HCPEngine repo
    → cmake configure + ninja build (just our Gem)

Contributor launches AssetProcessor (GUI)
    → Our HCPDocumentBuilder registered automatically
    → Drop .txt files into scan folder
    → GUI dashboard: job status, errors, dependencies, progress
    → Products appear in Cache/

Contributor does NOT need:
    → Full engine source
    → PhysX / CUDA / GPU runtime
    → The conversation-level engine
    → Postgres or LMDB (builder handles vocabulary internally)
```

### Lighter Distribution: Tools-Only Package

O3DE's export system supports packaging just the tools + custom builders (~50-100MB):

```
hcp-document-processor/
├── bin/
│   ├── AssetProcessorBatch     (headless processing)
│   ├── AssetProcessor          (GUI dashboard, optional)
│   └── Builders/
│       └── libHCPEngine.so     (our builder)
├── lib/                        (minimal engine libs)
├── Registry/                   (config)
└── README.md
```

The export scripts detect `sdk_engine: true` and skip source compilation. `export_source_built_project.py` handles packaging.

### Scale Implications

The Asset Processor is designed for game studios processing thousands of assets. For document processing at scale:

- **Parallel job processing** — multiple documents processed simultaneously
- **Incremental rebuilds** — fingerprint-based, only reprocesses changed files
- **Headless batch mode** — `AssetProcessorBatch` for CI/CD and bulk processing
- **File watching** — GUI mode watches scan dirs, auto-processes new files
- **Docker/container** — SDK + builder in a container for cloud processing

For Gutenberg (300,000+ English texts) or Wikipedia:
```
AssetProcessorBatch --project-path /path/to/hcp-engine --platforms common
    → Scans all .txt files in data/ scan folders
    → Parallel tokenization across CPU cores
    → Products cached, only new/changed files processed on re-run
    → Progress tracking via job database
```

---

## 9. Open Questions

1. **Vocabulary as asset vs service** — Should the vocabulary/forward DB be a build-time asset (processed from Postgres into binary product) or a runtime service (AZ::Interface querying Postgres/LMDB on demand)? Hybrid (Pattern C from DB research) is likely the answer.

2. **Document granularity** — Is each source text file one asset producing multiple LoD products? Or is each LoD tier a separate source? Single source with multiple SubID products is cleaner.

3. **Cross-document PBM** — PBM derivation across documents in a conversation happens in the PhysX scene. But aggregated PBM data across ALL documents (the global vocabulary bond map) needs a different mechanism — likely the DB specialist's domain.

4. **Conversation persistence** — When switching conversations (levels), how is the conversation state serialized and restored? The spawnable system doesn't persist runtime state. Need a custom serialization layer.

5. **LMDB role** — If vocabulary becomes an asset product and the forward DB becomes part of the builder, does LMDB still have a role? Possibly as a fast runtime cache for the vocabulary service, but maybe not as the primary store.

6. **Asset Processor availability** — In production (non-editor), Asset Processor doesn't run. New documents would need to be processed some other way. This may not matter if all documents are pre-processed.
