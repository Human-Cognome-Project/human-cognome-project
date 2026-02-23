# O3DE Architecture Research: Data Feeds and PostgreSQL Integration Patterns

**Research Date:** 2026-02-19
**Focus:** Understanding O3DE's asset processing, runtime data access, and optimal PostgreSQL integration patterns

## Executive Summary

O3DE follows a strict **source-to-product asset pipeline** model where the Asset Processor transforms source assets into runtime-optimized product assets. For PostgreSQL integration, there are **three primary architectural patterns**:

1. **Asset Processor Integration** (batch/offline) - Generate product assets from database at build time
2. **Runtime Service Layer** (dynamic/live) - Expose database as a service via EBus or AZ::Interface
3. **Custom FileIOBase Layer** (transparent) - Intercept file I/O to serve data from database

The best choice depends on whether your data is static (processed once), semi-dynamic (periodically updated), or fully dynamic (real-time queries).

---

## 1. O3DE Asset Processor Pipeline

### Architecture Overview

The Asset Processor is O3DE's build-time transformation system that converts source assets into runtime-optimized product assets.

**Source → Asset Processor → Product → Runtime**

- **Source Assets**: Portable files from third-party tools (FBX, PNG, custom formats)
- **Asset Builder**: C++ or Python module that transforms source → product
- **Product Assets**: Runtime-optimized files (.azmodel, .streamingimage, custom binary formats)
- **Asset Catalog**: Database tracking asset IDs, dependencies, and locations

### Key Characteristics

- **Build-time transformation**: Assets are processed during development, not at runtime
- **File-based**: Expects source files on disk (in scan directories)
- **Dependency tracking**: Builders emit product dependencies stored in Asset Database (SQLite)
- **Platform optimization**: Can generate different products per platform

### Asset Database (Development Only)

The Asset Processor maintains an **SQLite database** (`assetdb.sqlite`) that tracks:
- Scan directories and source files
- Asset Builder jobs and processing state
- File hashes and modification times
- Product dependencies

**Important**: This database is NOT used by the runtime. Editor/Launcher use the **Asset Catalog** (XML or bundled format).

### Custom Asset Types

You can register custom asset types via:

1. **Asset Builder**: Implement builder that processes your source format
2. **AssetHandler**: Register runtime handler for loading product assets

**Example Pattern**:
```
Custom Source (.mydata) → Custom Builder → Custom Product (.myproduct) → Custom AssetHandler loads at runtime
```

### Database Integration at Build Time

**Pattern**: Custom Asset Builder that queries PostgreSQL

- Source "asset" could be a metadata file (JSON, XML) pointing to database queries
- Asset Builder connects to PostgreSQL, fetches data, processes into binary product
- Product asset is generated as optimized runtime format
- Runtime loads static product asset normally

**Pros**:
- Clean separation of build-time vs runtime
- Uses standard O3DE asset pipeline
- Runtime performance is optimal (no database overhead)

**Cons**:
- Data is frozen at build time
- Changes require re-running Asset Processor
- Not suitable for dynamic/live data

**Sources:**
- [Asset Processing and the Asset Pipeline](https://docs.o3de.org/docs/user-guide/assets/pipeline/asset-processing/)
- [Asset Builders](https://docs.o3de.org/docs/user-guide/assets/pipeline/asset-builders/)
- [Custom Asset Example Gem](https://docs.o3de.org/docs/user-guide/gems/reference/assets/custom-asset-example/)
- [Asset Database](https://docs.o3de.org/docs/user-guide/assets/asset-processor/asset-database/)

---

## 2. O3DE Runtime Asset System

### AssetManager and AssetHandler

At runtime, the **AZ::Data::AssetManager** loads and manages assets via registered **AssetHandlers**.

**Key Classes**:
- `AZ::Data::AssetManager`: Central system managing all asset loading
- `AZ::Data::AssetHandler`: Per-asset-type handler (load/destroy/save)
- `AZ::Data::Asset<T>`: Smart pointer to asset data with reference counting
- `AssetCatalog`: Maps AssetId → file paths

### AssetHandler Interface

To create a custom asset type handler, derive from `AZ::Data::AssetHandler` and implement:

```cpp
class MyAssetHandler : public AZ::Data::AssetHandler
{
public:
    // Pure virtual methods to implement:

    AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override;
    // Instantiate new asset object

    LoadResult LoadAssetData(const Asset<AssetData>& asset,
                             AZStd::shared_ptr<AssetDataStream> stream,
                             const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
    // Load from stream, return LoadComplete/MoreDataRequired/Error

    void DestroyAsset(AssetPtr ptr) override;
    // Cleanup and deallocate

    void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override;
    // Declare what asset types this handler supports

    // Optional:
    bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override;
};
```

**Threading Model**:
- Asset handlers MUST be thread-safe
- May be called from multiple threads simultaneously
- OK to block calling thread during load
- NOT OK to queue work to another thread and block on same thread (deadlock risk)

### Asset Loading Flow

1. Component requests asset via `AssetId`
2. AssetManager checks if already loaded (cache)
3. If not, locates file via AssetCatalog
4. Opens file stream (`AZ::IO::FileIOStream`)
5. Calls registered AssetHandler's `LoadAssetData()`
6. Handler deserializes from stream into asset data
7. AssetManager signals completion via `AssetBus`

### Database Integration at Runtime

**Pattern**: Custom AssetHandler that queries PostgreSQL instead of file stream

- `CreateAsset()`: Allocate asset data structure
- `LoadAssetData()`: **Ignore stream parameter**, query PostgreSQL directly using AssetId as lookup key
- Deserialize database results into asset data
- Return `LoadComplete`

**Challenges**:
- AssetHandler expects to load from **stream**, not arbitrary sources
- Asset Catalog expects **file paths** for asset discovery
- AssetId is derived from source file UUID + SubId
- No built-in mechanism for "database-backed assets" in catalog

**Workaround**:
- Create dummy source files as asset catalog entries
- AssetHandler ignores stream contents, uses AssetId to query database
- Must ensure AssetIds are stable and map to database records

**Pros**:
- Can serve dynamic data at runtime
- Uses standard Asset<T> smart pointers and reference counting
- Integrates with existing asset dependencies

**Cons**:
- Hacky - abuses stream-based loading model
- AssetCatalog still expects files to exist
- Database latency affects asset load performance
- Threading complexity (must be thread-safe)

**Sources:**
- [AZ::Data::AssetHandler API](https://docs.o3de.org/docs/api/frameworks/azcore/class_a_z_1_1_data_1_1_asset_handler)
- [Asset Runtime System](https://www.docs.o3de.org/docs/user-guide/assets/runtime/)
- [AZ::Data::AssetManager API](https://docs.o3de.org/docs/api/frameworks/azcore/class_a_z_1_1_data_1_1_asset_manager)

---

## 3. O3DE EBus (Event Bus) System

### Architecture

**EBus** is O3DE's primary pub/sub messaging system for decoupled inter-component communication.

**Key Concepts**:
- **Global singletons**: Auto-created on first subscription, destroyed on last disconnect
- **Request Bus**: Send messages TO a component (invoke behavior)
- **Notification Bus**: Send messages FROM a component (broadcast events)
- **Address Policy**: Single global bus, or addressed by EntityId/other key

### Advantages Over Polling

- **Abstraction**: Minimize hard dependencies between systems
- **Event-driven**: Eliminate polling for scalable, high-performance code
- **Cleaner code**: Dispatch messages without concern for handlers

### Typical Component Pattern

Components provide two EBuses:

1. **Request Bus** (consumers call producer):
```cpp
class MyServiceRequests : public AZ::EBusTraits
{
public:
    virtual Data QueryData(const Params& params) = 0;
    virtual void ProcessData(const Data& data) = 0;
};
using MyServiceRequestBus = AZ::EBus<MyServiceRequests>;
```

2. **Notification Bus** (producer broadcasts to consumers):
```cpp
class MyServiceNotifications : public AZ::EBusTraits
{
public:
    virtual void OnDataUpdated(const Data& newData) = 0;
};
using MyServiceNotificationBus = AZ::EBus<MyServiceNotifications>;
```

### Database Service via EBus

**Pattern**: PostgreSQL backend as system component with EBus interface

```cpp
class DatabaseRequests : public AZ::EBusTraits
{
public:
    virtual TokenLookupResult QueryTokens(const AZStd::vector<TokenId>& ids) = 0;
    virtual void AsyncQueryTokens(const AZStd::vector<TokenId>& ids, QueryCallback callback) = 0;
};
using DatabaseRequestBus = AZ::EBus<DatabaseRequests>;
```

System component implements the bus:
```cpp
class DatabaseSystemComponent
    : public AZ::Component
    , public DatabaseRequestBus::Handler
{
    void Activate() override
    {
        DatabaseRequestBus::BusConnect();
        // Connect to PostgreSQL
    }

    TokenLookupResult QueryTokens(const AZStd::vector<TokenId>& ids) override
    {
        // Query PostgreSQL, return results
    }
};
```

Consumers use the service:
```cpp
TokenLookupResult result;
DatabaseRequestBus::BroadcastResult(result, &DatabaseRequests::QueryTokens, tokenIds);
```

**Pros**:
- Clean service abstraction
- Decoupled from database implementation
- Easy to mock for testing
- Can provide sync or async queries

**Cons**:
- EBus overhead for high-frequency calls
- No IDE autocomplete (string-based dispatch internally)
- Boilerplate for defining buses

**Sources:**
- [Event Bus (EBus) System](https://www.docs.o3de.org/docs/user-guide/programming/messaging/ebus/)
- [Components and EBuses](https://www.docs.o3de.org/docs/user-guide/programming/components/ebus-integration/)
- [Event Buses In Depth](https://docs.o3de.org/docs/user-guide/programming/messaging/ebus-design/)

---

## 4. AZ::Interface - Modern Service Provider Pattern

### Overview

**AZ::Interface** is a newer, lighter-weight alternative to EBus for singleton services.

**Key Differences from EBus**:
- Vastly improved performance (direct function calls vs EBus dispatch)
- IDE autocomplete support (standard C++ interface)
- Simpler registration (no bus connection boilerplate)
- Designed for 1:1 service provider pattern (not pub/sub)

### Core API

```cpp
// Define interface
class IMyService
{
public:
    virtual Data QueryData(const Params& params) = 0;
};

// Implement service and register
class MyServiceImpl : public IMyService, public AZ::Interface<IMyService>::Registrar
{
public:
    MyServiceImpl() { /* Registrar automatically registers in constructor */ }
    ~MyServiceImpl() { /* Registrar automatically unregisters in destructor */ }

    Data QueryData(const Params& params) override
    {
        // Implementation
    }
};

// Use service
if (auto* service = AZ::Interface<IMyService>::Get())
{
    Data result = service->QueryData(params);
}
```

### Manual Registration

Alternatively, manually register/unregister:
```cpp
AZ::Interface<IMyService>::Register(myServiceInstance);
// ... use service ...
AZ::Interface<IMyService>::Unregister(myServiceInstance);
```

### Database Service via AZ::Interface

**Pattern**: PostgreSQL backend as registered service interface

```cpp
class IDatabaseService
{
public:
    virtual TokenLookupResult SyncQueryTokens(const AZStd::vector<TokenId>& ids) = 0;
    virtual void AsyncQueryTokens(const AZStd::vector<TokenId>& ids, QueryCallback callback) = 0;
};

class PostgresService : public IDatabaseService, public AZ::Interface<IDatabaseService>::Registrar
{
    // Implementation with libpq or other PostgreSQL client
};

// Usage anywhere in codebase:
auto result = AZ::Interface<IDatabaseService>::Get()->SyncQueryTokens(tokenIds);
```

**When to use AZ::Interface vs EBus**:
- **AZ::Interface**: Single service provider, performance-critical, simple request/response
- **EBus**: Multiple listeners, broadcast notifications, entity-addressed messages

**Pros**:
- Best performance for service calls
- Clean C++ interface with autocomplete
- Minimal boilerplate
- Easy to test (swap implementations)

**Cons**:
- Only supports 1:1 provider/consumer pattern
- No built-in multi-threading safety (implementation dependent)
- Newer API (less documentation/examples than EBus)

**Sources:**
- [AZ::Interface Documentation](https://docs.o3de.org/docs/user-guide/programming/messaging/az-interface/)
- [AZ::Interface API Reference](https://docs.o3de.org/docs/api/frameworks/azcore/class_a_z_1_1_interface.html)

---

## 5. AZ::IO and FileIOBase

### Overview

O3DE's file I/O is abstracted through **FileIOBase**, allowing custom I/O backends to be layered.

**Key Classes**:
- `AZ::IO::FileIOBase`: Pure virtual base class defining file API
- `AZ::IO::LocalFileIO`: Standard filesystem implementation
- `AZ::IO::RemoteFileIO`: Network-transparent VFS for remote devices
- `AZ::IO::Streamer`: Asynchronous background streaming (uses FileIOBase internally)

### Layering Pattern

You can replace the FileIO instance with your own implementation:

```cpp
// Your custom implementation
class DatabaseFileIO : public AZ::IO::FileIOBase
{
public:
    Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override
    {
        // Map filePath to database query
        // Return handle to cached data
    }

    Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead) override
    {
        // Read from database-backed buffer
    }

    // ... implement all FileIOBase methods ...
};

// Install your layer
auto* previousIO = AZ::IO::FileIOBase::GetInstance();
auto* myIO = new DatabaseFileIO(previousIO);  // Chain to previous layer
AZ::IO::FileIOBase::SetInstance(myIO);
```

### RemoteFileIO and VFS

O3DE's **VFS (Virtual File System)** uses RemoteFileIO to read assets from remote devices (Android, iOS) via Asset Processor.

**Characteristics**:
- Network-transparent: file operations transmitted over network
- Enabled via `remote_filesystem` setting in `bootstrap.setreg`
- Allows live asset reloading on remote devices
- Asset Processor serves files over network

**Database Integration Pattern**:
- Implement custom FileIOBase that queries PostgreSQL
- Map "virtual file paths" to database queries
- AssetHandler and other O3DE systems call FileIO API transparently
- Database serves data masquerading as files

**Pros**:
- Completely transparent to rest of engine
- No changes needed to AssetHandlers or other systems
- Could serve assets directly from database

**Cons**:
- Very low-level - must implement entire file API
- Significant complexity and edge cases
- Database must serve data that looks like files
- Threading and caching challenges
- Debugging and error handling complexity

**Sources:**
- [Raw File Access in O3DE](https://docs.o3de.org/docs/user-guide/programming/file-io/)
- RemoteFileIO documentation (referenced in VFS feature)

---

## 6. Gems and System Components

### What is a Gem?

A **Gem** is O3DE's modular package system containing:
- Code (C++ libraries/executables)
- Assets (source and/or product assets)
- Manifest file (`gem.json`)
- CMake build file
- Optional icon

### System Components

**System Components** are components that control the engine itself, not individual entities.

**Lifecycle**:
- Registered on `AZ::Module` class of a Gem
- Activated when Gem loads
- Deactivated when Gem unloads

**Service Pattern**:
- System components expose services via EBus or AZ::Interface
- Can depend on other system component services
- Declare dependencies to ensure activation order

### Data Provider / Data Consumer Pattern

O3DE's **terrain system** demonstrates this pattern:

**Data Providers**:
- Generalized services that "serve up" data
- Don't do anything on their own (passive)
- Examples: Gradient components, surface data

**Data Consumers**:
- Query data from providers
- Take action based on data
- Examples: Terrain renderer, physics heightfield

**Routing Layer** (Terrain System):
- Single API entry point
- Unified view of all terrain data
- Routes requests from consumers to providers
- Routes change notifications from providers to consumers

**Benefits**:
- Design simplicity for each piece
- Decoupled systems
- Easy extensibility (add new providers/consumers without changing routing layer)

### PostgreSQL Gem Architecture

**Pattern**: Database Service Gem with System Component

```
DatabaseGem/
├── gem.json
├── Code/
│   ├── Source/
│   │   ├── DatabaseSystemComponent.h/cpp  (System component + EBus handler)
│   │   ├── DatabaseModule.cpp              (Registers system component)
│   │   ├── PostgresClient.h/cpp            (libpq wrapper)
│   │   └── DatabaseBus.h                   (EBus interface definitions)
│   └── CMakeLists.txt
└── Assets/
    └── (optional: SQL scripts, config files)
```

**System Component Responsibilities**:
- Connect to PostgreSQL on activation
- Implement DatabaseRequestBus or AZ::Interface<IDatabaseService>
- Manage connection pool, query caching
- Provide sync and async query methods
- Handle errors and reconnection

**Other Gems/Components Use the Service**:
```cpp
// Via EBus
DatabaseRequestBus::Broadcast(&DatabaseRequests::QueryTokens, tokenIds);

// Via AZ::Interface
AZ::Interface<IDatabaseService>::Get()->QueryTokens(tokenIds);
```

**Pros**:
- Clean separation of database functionality
- Reusable across projects
- Standard O3DE Gem packaging
- Service interface abstraction

**Cons**:
- Database service is runtime only (not integrated with Asset Processor)
- Requires runtime network access to PostgreSQL
- Must handle connection failures, latency

**Sources:**
- [System Components in O3DE](https://www.docs.o3de.org/docs/user-guide/programming/components/system-components/)
- [Terrain System Architecture](https://docs.o3de.org/docs/user-guide/visualization/environments/terrain/terrain-developer-guide/architecture/)
- [Gems in Open 3D Engine](https://www.docs.o3de.org/docs/user-guide/gems/)

---

## 7. Procedural and Dynamic Data Patterns

### Vegetation System (Procedural Runtime Generation)

O3DE's **Vegetation Gem** demonstrates runtime procedural generation:

**Architecture**:
- **Vegetation Areas**: Define what, where, how, if vegetation appears
- **Data Providers**: Gradients, surfaces, images (where to spawn)
- **Vegetation System**: Processes areas, examines points, places instances
- **Dynamic Runtime**: Vegetation spawned and managed dynamically

**Key Pattern**: Data-driven procedural generation
- Configuration specifies rules
- System queries data providers at runtime
- Instances created/destroyed dynamically based on data

### Gradient and Surface Data

**Gradient Components**: Provide 2D/3D scalar fields
- Can be procedural (noise, formulas)
- Can stream from images/textures
- Can query from terrain heightmaps
- Composable (combine multiple gradients)

**Surface Data Components**: Provide surface properties
- Tags, normals, bounds
- Can query terrain, meshes, vegetation
- Broadcast change notifications

### Streaming and Dynamic Updates

**Image Gradient Streaming**:
- Uses Atom `streamingimage` format
- Supports streaming mipmaps
- Runtime can load/unload mipmap levels

**Change Notifications**:
- Data providers broadcast when data changes
- Consumers receive notifications and update
- Example: Vegetation system rebuilds when gradient changes

### Database as Data Provider

**Pattern**: PostgreSQL backend providing gradient/procedural data

Could implement:
- `GradientRequestBus::Handler` - Query database for gradient values at coordinates
- `SurfaceDataProviderRequestBus::Handler` - Query database for surface properties
- System dynamically generates content based on database-provided parameters

**Example Use Cases**:
- Terrain biomes from database-defined rules
- Vegetation placement from database-controlled gradients
- Dynamic world state from live database queries

**Pros**:
- Leverages existing O3DE procedural systems
- Database provides configuration/rules, not raw assets
- Can update at runtime (live world changes)

**Cons**:
- Database queries in performance-critical paths
- Must cache aggressively
- Complex integration with spatial queries

**Sources:**
- [Vegetation Gem](https://docs.o3de.org/docs/user-guide/gems/reference/environment/vegetation/)
- [Terrain Architecture - Data Provider Pattern](https://docs.o3de.org/docs/user-guide/visualization/environments/terrain/terrain-developer-guide/architecture/)
- [Procedural Prefab](https://docs.o3de.org/docs/user-guide/assets/scene-pipeline/procedural_prefab/)

---

## 8. Recommended PostgreSQL Integration Patterns

Based on O3DE's architecture, here are the recommended patterns for different use cases:

### Pattern A: Static Data (Build-Time Processing)

**Use Case**: Token embeddings, vocabularies, static configuration

**Architecture**:
1. Custom Asset Builder (Python or C++)
2. Source "asset" is metadata file (e.g., `tokens.dbsource.json`) containing database query config
3. Asset Builder connects to PostgreSQL at build time
4. Fetches data, processes, generates binary product asset
5. Standard AssetHandler loads product at runtime

**Pros**:
- Best runtime performance (no database overhead)
- Uses standard O3DE pipeline
- Works with asset bundling for releases

**Cons**:
- Data frozen at build time
- Requires Asset Processor re-run for updates

**Best For**: Vocabularies, embeddings, static lookup tables

---

### Pattern B: Semi-Dynamic Data (Runtime Service Layer)

**Use Case**: Configuration, user data, periodic updates

**Architecture**:
1. Database Service Gem with System Component
2. Implements `AZ::Interface<IDatabaseService>` or `DatabaseRequestBus`
3. Connects to PostgreSQL at runtime
4. Other components query service via interface
5. Optional: Cache frequently-accessed data

**Pros**:
- Clean service abstraction
- Can update at runtime
- Easy to test/mock

**Cons**:
- Runtime database dependency
- Network latency
- Must handle connection failures

**Best For**: User preferences, world state, analytics, content that updates periodically

---

### Pattern C: Hybrid (Build-Time + Runtime Updates)

**Use Case**: Large static base + incremental updates

**Architecture**:
1. Asset Builder generates base product from database (bulk data)
2. Runtime service provides delta updates
3. AssetHandler loads base product
4. Component merges runtime updates from database service

**Example Flow**:
```
Build Time:
  PostgreSQL → Asset Builder → base_tokens.product (100K tokens)

Runtime:
  AssetHandler loads base_tokens.product
  DatabaseService queries for tokens added since build
  Merge base + deltas
```

**Pros**:
- Fast bulk loading (asset)
- Supports incremental updates
- Reduces runtime queries

**Cons**:
- Complex versioning/merging logic
- Must track "last build" timestamp

**Best For**: Large vocabularies with new tokens added over time

---

### Pattern D: Procedural Data Provider

**Use Case**: Dynamic world generation, runtime-calculated data

**Architecture**:
1. Implement Data Provider bus (e.g., `GradientRequestBus`)
2. System Component queries PostgreSQL on-demand
3. Aggressive caching of query results
4. O3DE's existing systems (terrain, vegetation) consume via standard interfaces

**Pros**:
- Integrates with existing O3DE procedural systems
- Database defines rules/parameters, not raw data
- Can support live updates

**Cons**:
- Performance-critical query paths
- Complex caching strategy needed
- Not suitable for bulk data

**Best For**: World generation rules, biome definitions, dynamic spawn parameters

---

### NOT Recommended: Custom FileIOBase Layer

**Why Not**:
- Very low-level and complex
- Must implement entire file API
- Edge cases and threading challenges
- Debugging complexity
- AssetHandlers and other systems still expect file-like semantics
- Better to use higher-level patterns (service layer or asset builder)

**Only Consider If**: You need absolutely transparent database access where every file read could potentially come from database, and you have very strong expertise in file system semantics.

---

## 9. Key Architectural Insights

### O3DE's Asset Philosophy

O3DE is designed around the principle:
> **"Process once at build time, load optimized data at runtime"**

This philosophy means:
- Asset Processor is the primary integration point for external data
- Runtime expects optimized, ready-to-use product assets
- Dynamic runtime data should use service layers (EBus/Interface), not asset system

### When to Use Asset System vs Service Layer

**Use Asset System (Asset Builder + AssetHandler) when**:
- Data is static or changes infrequently
- Data needs platform-specific optimization
- You want to leverage asset bundling and packaging
- Runtime performance is critical (no I/O overhead)

**Use Service Layer (EBus/Interface) when**:
- Data changes at runtime
- Data is user-specific or session-specific
- You need live queries or updates
- You want clean abstraction of data source

**Use Hybrid when**:
- Large base dataset + incremental updates
- Want fast bulk loading + runtime flexibility

### Threading Considerations

- **AssetHandlers**: Must be thread-safe, may be called from any thread
- **System Components**: Activated on main thread, but EBus/Interface methods may be called from any thread
- **FileIOBase**: May be called from any thread
- **Database Connections**: Use connection pooling, thread-local connections, or mutex protection

### Performance Considerations

- **Asset Loading**: Blocking file I/O acceptable (streaming happens on background threads)
- **Database Queries**: Minimize latency impact
  - Async queries with callbacks
  - Aggressive caching
  - Connection pooling
  - Batch queries
- **EBus Overhead**: Minimal, but AZ::Interface is faster for high-frequency calls

---

## 10. Concrete Recommendation for HCP Project

Given the HCP project's context (texture engine, PBM, token lookups):

### Recommended Architecture

**Pattern**: Hybrid Build-Time + Runtime Service

**Build Time (Asset Processor Integration)**:
1. **Custom Asset Builder** (`PBMAssetBuilder`)
   - Reads source file: `vocabulary.pbmsource` (metadata: database connection string, table name, filters)
   - Queries PostgreSQL for bulk token data (embeddings, frequencies, bond maps)
   - Processes into optimized binary format: `vocabulary.pbm` (product asset)
   - Emits asset dependencies

2. **Custom AssetHandler** (`PBMAssetHandler`)
   - Loads `vocabulary.pbm` product at runtime
   - Deserializes into memory structures (token tables, bond maps)
   - Registers with AssetManager

**Runtime (Service Layer)**:
3. **Database Service Gem** (`HCPDatabaseGem`)
   - System Component implements `AZ::Interface<IHCPDatabase>`
   - Provides methods:
     - `SyncQueryToken(TokenId)` - For individual token lookups
     - `AsyncBatchQueryTokens(vector<TokenId>, callback)` - For batch queries
     - `GetVocabularyMetadata()` - For stats, version info
   - Connection pooling, caching, error handling

**Integration**:
- **Texture Engine Component** (loads PBM asset, uses database service for runtime queries)
  - `Activate()`: Load `vocabulary.pbm` asset
  - `Process(text)`: Tokenize, query tokens from asset + database service for any new tokens
  - Merge base vocabulary (asset) + runtime deltas (database)

### Why This Works

- **Bulk data** (full vocabulary) loaded efficiently via asset system
- **Incremental updates** (new tokens) fetched via runtime service
- **Performance**: No database queries for common tokens (in asset), only for deltas
- **Flexibility**: Can update database without rebuilding assets (for new tokens)
- **O3DE-native**: Uses standard Gem, System Component, Asset Builder patterns

### Implementation Phases

**Phase 1**: Asset Builder Only
- Get basic pipeline working: PostgreSQL → Asset Builder → Product Asset → Runtime
- Proves build-time integration

**Phase 2**: Add AssetHandler
- Load product assets at runtime
- Validate data format and runtime performance

**Phase 3**: Add Runtime Service
- Implement Database Service Gem
- Add incremental token lookup capability

**Phase 4**: Hybrid Integration
- Merge asset-based and service-based data
- Optimize caching and query patterns

---

## 11. Additional Resources

### Official Documentation
- [O3DE Documentation](https://docs.o3de.org/)
- [Asset Pipeline Guide](https://docs.o3de.org/docs/user-guide/assets/)
- [Programming Guide - Components](https://docs.o3de.org/docs/user-guide/programming/components/)
- [Messaging Systems](https://docs.o3de.org/docs/user-guide/programming/messaging/)

### Source Code
- [O3DE GitHub Repository](https://github.com/o3de/o3de)
- [Asset Builder Examples](https://github.com/o3de/o3de/tree/development/Code/Tools/AssetProcessor)
- [Custom Asset Example](https://github.com/o3de/o3de/tree/development/Gems/CustomAssetExample) (if available)

### Community
- [O3DE Discord](https://discord.gg/o3de)
- [O3DE GitHub Discussions](https://github.com/o3de/o3de/discussions)
- [O3DE Forum](https://github.com/o3de/o3de/discussions)

---

## Summary

**O3DE provides three primary integration points for PostgreSQL:**

1. **Asset Processor** - Best for static/semi-static bulk data, processed at build time
2. **Service Layer (EBus/Interface)** - Best for dynamic runtime data, queries, and updates
3. **FileIOBase** - Possible but not recommended for most use cases

**For the HCP project**, a **hybrid approach** is recommended:
- Use Asset Builder for bulk vocabulary/embedding data (optimized offline processing)
- Use Runtime Service for incremental updates and dynamic queries
- This provides both performance and flexibility

The most **O3DE-native pattern** is to treat the Asset Processor as the primary integration point for external data sources, generating optimized product assets that runtime systems consume efficiently. For truly dynamic data that doesn't fit the "process once, load at runtime" model, implement a service layer using System Components with EBus or AZ::Interface.
