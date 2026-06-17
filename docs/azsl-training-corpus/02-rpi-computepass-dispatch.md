# 02 — AZ::RPI::ComputePass and the O3DE Pass System

**Training corpus for the O3DE compute-engine port.** This is the replacement for raw
CUDA kernel launches. Where CUDA gives you `kernel<<<grid, block>>>(args)`, O3DE gives you a
**ComputePass** that is scheduled into a **frame graph** by the **Pass System**, and dispatched
through an **RHI DispatchItem** onto whichever backend (DX12 / Vulkan / Metal) is active.

Read this to *learn the model*, then read the cited files directly.

---

## 0. Source-of-truth note (READ FIRST)

The local SDK at `/opt/O3DE/25.10.2` is a **headers + assets install**. The RPI/RHI `.cpp`
implementation files are **NOT present** (verified: `find .../Atom/RPI/Code/Source -name '*.cpp'`
returns 0 results). So:

- **Behaviour / class declarations / JSON field names** → cited from local headers and `.pass`
  assets under `/opt/O3DE/25.10.2` (authoritative for *this exact version*).
- **Method bodies** (what `BuildInternal`/`LoadShader`/`CompileResources` actually *do*) → cited
  from the O3DE `development` branch on GitHub:
  `https://raw.githubusercontent.com/o3de/o3de/development/Gems/Atom/RPI/Code/Source/RPI.Public/Pass/ComputePass.cpp`
  Treat method-body details as "true in spirit, verify the exact line against your synced source."

**Version drift to know about:** the local `25.10.2` header keeps a method named
`MatchDimensionsToOutput()` (`ComputePass.h:71`) and per-pass thread fields
`m_totalNumberOfThreadsX/Y/Z`; the `development` branch folded the fullscreen-sizing logic into
`CompileResources()` and dropped that method name. The **JSON keys are stable**: `ThreadCountX/Y/Z`,
`FullscreenDispatch` (confirmed present in local `.pass` files below).

---

## 1. The Pass System model

### 1.1 Three concepts: Template, Request, Pass

| Concept | What it is | File |
|---|---|---|
| **PassTemplate** | Data blueprint: slots, attachments, child requests, pass-data. The reusable "class". | `Include/Atom/RPI.Reflect/Pass/PassTemplate.h` |
| **PassRequest** | An instantiation of a template by name, with input connections + overrides + ordering. The "new". | `Include/Atom/RPI.Reflect/Pass/PassRequest.h` |
| **Pass** (C++ instance) | The live object built from a `PassDescriptor`; does the GPU work. | `Include/Atom/RPI.Public/Pass/Pass.h` |

**PassTemplate** (`PassTemplate.h:27-97`) carries:
- `m_passClass` (`Name`) — which **registered C++ pass class** to instantiate (e.g. `"ComputePass"`,
  `"SkinnedMeshComputePass"`). This string is the bridge from JSON to your C++ subclass.
- `m_slots` — the input/output attachment slots (`PassSlotList`).
- `m_connections` / `m_fallbackConnections` — wiring (usually outputs → owned attachments).
- `m_imageAttachments` / `m_bufferAttachments` — descriptors for attachments the pass **owns**.
- `m_passRequests` (`PassRequestList`) — **requests to create child passes** → this is how the tree nests.
- `m_passData` (`AZStd::shared_ptr<PassData>`) — typed init blob; for compute it's `ComputePassData`.

**PassRequest** (`PassRequest.h:30-75`) carries:
- `m_passName` (instance name) + `m_templateName` (which template to build from).
- `m_connections` — **input** connections that point at *other passes'* outputs.
- `m_imageAttachmentOverrides` / `m_bufferAttachmentOverrides` — override template attachments.
- `m_executeAfterPasses` / `m_executeBeforePasses` — explicit ordering when there's no shared
  attachment to imply it.
- `m_passEnabled` — initial enabled/disabled state.

> Rule of thumb (from `PassTemplate.h:74-77`, `PassRequest.h:54-58`):
> **input** connections live on the **PassRequest** (hook to another pass), **output** connections
> live on the **PassTemplate** (connect own Output slot to an owned attachment via `"Pass":"This"`).

### 1.2 The `.pass` file = a `PassAsset` wrapping a `PassTemplate`

A `.pass` JSON file deserializes to a `PassAsset` whose `ClassData.PassTemplate` is the template.
Real local example — `Gems/Atom/Feature/Common/Assets/Passes/BRDFTexture.pass` (a plain `ComputePass`):

```jsonc
{
  "Type": "JsonSerialization", "Version": 1,
  "ClassName": "PassAsset",
  "ClassData": { "PassTemplate": {
    "Name": "BRDFTextureTemplate",
    "PassClass": "ComputePass",                       // generic compute pass class
    "Slots": [
      { "Name": "Output", "SlotType": "InputOutput",
        "ScopeAttachmentUsage": "Shader",
        "LoadStoreAction": { "LoadAction": "Clear" } }
    ],
    "ImageAttachments": [
      { "Name": "BRDFTexture",
        "Lifetime": "Imported",                       // persists across frames (see §4)
        "AssetRef": { "FilePath": "Textures/BRDFTexture.attimage" } }
    ],
    "Connections": [
      { "LocalSlot": "Output",
        "AttachmentRef": { "Pass": "This", "Attachment": "BRDFTexture" } }
    ],
    "PassData": {
      "$type": "ComputePassData",
      "ThreadCountX": "256", "ThreadCountY": "256", "ThreadCountZ": "1",
      "ShaderAsset": { "FilePath": "Shaders/BRDFTexture/BRDFTextureCS.shader" }
    }
  }}
}
```
(Local file: `BRDFTexture.pass:1-48`.)

### 1.3 Slot fields (the SRG-binding contract)

From `PassAttachmentReflect.h` + RHI `AttachmentEnums.h`, observed across local `.pass` files:

- **`SlotType`**: `Input` (read / SRV), `Output` (write / UAV), `InputOutput` (read-write / UAV).
- **`ScopeAttachmentUsage`**: for compute, slots use **`Shader`**; an indirect-args buffer uses
  **`Indirect`**. (Full enum: `RenderTarget, DepthStencil, Shader, Copy, Resolve, Predication,
  Indirect, SubpassInput, InputAssembly, ShadingRate`.)
- **`ShaderInputName`**: the SRG member the slot binds to (e.g. `"m_skinnedMeshOutputStream"`).
  Default `"AutoBind"`; `"NoBind"` to skip SRG binding. The frame graph uses this to bind the
  attachment's resource view into the pass SRG automatically.
- **`LoadStoreAction`**: `LoadAction`/`StoreAction` (`Load`/`Clear`/`DontCare` / `Store`/`DontCare`)
  and an optional `ClearValue`.

Example with explicit SRG names — `Gems/Atom/Feature/Common/Assets/Passes/Skinning.pass`:
```jsonc
"Slots": [
  { "Name": "SkinnedMeshOutputStream", "ShaderInputName": "m_skinnedMeshOutputStream",
    "SlotType": "InputOutput", "ScopeAttachmentUsage": "Shader" } ],
"PassData": { "$type": "ComputePassData",
  "ShaderAsset": { "FilePath": "Shaders/SkinnedMesh/LinearSkinningCS.shader" } }
```

### 1.4 How a pass joins the pipeline (frame graph)

- Passes form a **tree** rooted at the pipeline's root pass. A **`ParentPass`** is a composite that
  holds children; a **`RenderPass`** is a leaf that does GPU work (`RenderPass.h:36-41`).
- Pipelines are themselves authored as `.pass` trees (e.g. `MainPipeline.pass`,
  `AtomTressFX_MainPipeline.pass`) selected via a `MainRenderPipeline.azasset`.
- **Connections** drive execution order. A `PassConnection` = `{ LocalSlot, AttachmentRef{Pass, Attachment} }`.
  The `Pass` field uses **special keywords** (local `Pass.h:445-448`):
  `PassNameThis{"This"}`, `PassNameParent{"Parent"}`, `PipelineKeyword{"Pipeline"}`,
  `PipelineGlobalKeyword{"PipelineGlobal"}`. (Note: **no `PreviousPass`** keyword — you name the
  source pass explicitly.) `This` = own attachment, `Parent` = parent's, `Pipeline` = pipeline
  settings, `PipelineGlobal` = a binding exposed pipeline-wide by global name.
- When pass B's input connects to pass A's output attachment, the frame graph **infers the A→B
  dependency** and serializes/barriers them. Most ordering is implicit from attachments; use
  `m_executeAfterPasses`/`m_executeBeforePasses` only when two passes share no attachment.

Real wiring example — `AtomTressFX_PassRequest.azasset` hooks the hair parent pass's input slots to
upstream passes by name (`DepthPrePass`, `OpaquePass`, `Shadows`, `LightCullingPass`):
```jsonc
{ "ClassName": "PassRequest", "ClassData": {
  "Name": "HairParentPass", "TemplateName": "HairParentShortCutPassTemplate", "Enabled": true,
  "Connections": [
    { "LocalSlot": "DepthLinear", "AttachmentRef": { "Pass": "DepthPrePass", "Attachment": "DepthLinear" } },
    { "LocalSlot": "TileLightData", "AttachmentRef": { "Pass": "LightCullingPass", "Attachment": "TileLightData" } } ] } }
```
This is **the** pattern for "insert my compute work into the live pipeline": author a PassRequest in
the pipeline tree, connect its inputs to existing pass outputs.

---

## 2. ComputePass specifically

`ComputePass : public RenderPass, private ShaderReloadNotificationBus::Handler`
(`ComputePass.h:27-30`). `RenderPass : public Pass, public RHI::ScopeProducer`
(`RenderPass.h:38-41`). **Every pass is an `RHI::ScopeProducer`** — it produces one *scope* in the
per-frame frame graph. That is the hinge for everything (and for §3's standalone question).

### 2.1 `ComputePassData` (the JSON → C++ init blob)

`Include/Atom/RPI.Reflect/Pass/ComputePassData.h:19-44` (local):
```cpp
struct ComputePassData : public RenderPassData {
    AssetReference m_shaderReference;                 // JSON "ShaderAsset"
    uint32_t m_totalNumberOfThreadsX/Y/Z = 0;         // JSON "ThreadCountX/Y/Z"
    bool     m_fullscreenDispatch = false;            // JSON "FullscreenDispatch"
    AZ::Name m_fullscreenSizeSourceSlotName;          // JSON "FullscreenSizeSourceSlotName"
    bool     m_indirectDispatch = false;              // JSON "IndirectDispatch"
    AZ::Name m_indirectDispatchBufferSlotName;        // JSON "IndirectDispatchBufferSlotName"
    bool     m_useAsyncCompute = false;               // JSON "UseAsyncCompute" → async compute queue
};
```
`fullscreenDispatch` and `indirectDispatch` are **mutually exclusive** (asserted in the ctor).

### 2.2 Key state on the pass (`ComputePass.h:73-93`)

```cpp
Data::Instance<Shader>                m_shader;                  // the compute shader
Data::Instance<RPI::ShaderResourceGroup> m_drawSrg;             // variant fallback-key SRG
RHI::DispatchItem                     m_dispatchItem;            // THE dispatch (one per pass)
bool                                  m_fullscreenDispatch;      // size from an attachment each frame
bool                                  m_indirectDispatch;        // args read from a GPU buffer
RHI::Ptr<RHI::IndirectBufferSignature> m_indirectDispatchBufferSignature;
RHI::IndirectBufferView               m_indirectDispatchBufferView;
```
(The pass-level SRG `m_shaderResourceGroup` lives on `RenderPass`, `RenderPass.h:125`.)

### 2.3 Lifecycle methods (what runs, when)

The base `Pass` drives a fixed lifecycle; `ComputePass` overrides these (`ComputePass.h:62-71`,
bodies from `development` `ComputePass.cpp`):

1. **Constructor** — reads `ComputePassData`; seeds `m_dispatchItem` with `RHI::DispatchDirect`
   from the JSON `ThreadCountX/Y/Z`; calls `LoadShader()`; sets default attachment stage to
   `ComputeShader`.
2. **`LoadShader()`** (`ComputePass.h:100`) —
   - If `m_useAsyncCompute`, sets `m_hardwareQueueClass = HardwareQueueClass::Compute`
     (runs on the **async compute queue**; default is `Graphics`, `RenderPass.h:128`).
   - Loads the shader from `m_shaderReference` → `Shader::FindOrCreate`.
   - **Creates the pass SRG** from the shader's `Pass` SRG layout (`ShaderResourceGroup::Create`),
     binds JSON data-mappings into it.
   - **Reads `[numthreads(x,y,z)]` from the compiled shader** to fill `threadsPerGroup` (only for
     Direct dispatch). → **JSON supplies *total* threads; the shader supplies *group size*.**
   - Builds the dispatch pipeline state (`PipelineStateDescriptorForDispatch`) and stores it on the
     dispatch item.
   - Subscribes to `ShaderReloadNotificationBus` for hot-reload; on reload calls
     `OnShaderReloadedInternal()` → your `m_shaderReloadedCallback` (a good place to re-call
     `SetTargetThreadCounts`).
3. **`BuildInternal()`** (`ComputePass.h:63`) — after `RenderPass::BuildInternal()`, resolves the
   indirect-buffer binding + builds an `IndirectBufferSignature` (one `Dispatch` command), or
   resolves the fullscreen size-source binding (defaults to first Output, else first InputOutput).
4. **`CompileResources(FrameGraphCompileContext)`** (`ComputePass.h:66`) — **per frame, compile
   phase**: binds + `Compile()`s the pass SRG and draw SRG. If fullscreen, reads the size-source
   image's descriptor and calls `SetTargetThreadCounts(width, height, max(depth, arraySize))`.
   If indirect, builds `m_indirectDispatchBufferView` and sets
   `m_dispatchItem.SetArguments(RHI::DispatchIndirect(1, view, 0))`.
5. **`BuildCommandListInternal(FrameGraphExecuteContext)`** (`ComputePass.h:67`) — **per frame,
   execute phase**: the actual submit.
   ```cpp
   RHI::CommandList* commandList = context.GetCommandList();
   SetSrgsForDispatch(context);                  // binds scene/view/pass SRGs (RenderPass.cpp)
   commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
   ```
   The generic `ComputePass` submits **exactly one** dispatch item. (Compare §3.2's multi-dispatch.)

### 2.4 `SetTargetThreadCounts` and the group-count math

`SetTargetThreadCounts(x,y,z)` (`ComputePass.h:42`) writes the three **totals** onto the dispatch
item's *direct* args. The **ceil-division into thread groups** is done in the RHI struct, NOT in the
pass — local `DeviceDispatchItem.h:40-53`:
```cpp
uint32_t GetNumberOfGroupsX() const {
    return DivideAndRoundUp(m_totalNumberOfThreadsX, aznumeric_cast<uint32_t>(m_threadsPerGroupX));
}   // = (total + perGroup - 1) / perGroup ; same for Y, Z
```
So the contract is exactly:
```
m_totalNumberOfThreads*   ← JSON ThreadCount* (or SetTargetThreadCounts, or fullscreen image size)
m_threadsPerGroup*        ← shader [numthreads(...)]
dispatch groups           = ceil(total / perGroup)   ← computed by RHI at submit
```
This is the **direct mental-model translation of CUDA**: `total` ≈ your problem size, `perGroup` ≈
`blockDim`, `groups` ≈ `gridDim`. You set the problem size; O3DE computes the grid.

`DispatchDirect` itself (local `DeviceDispatchItem.h:21-64`) holds the 6 numbers; the comments make
the relation explicit: `m_totalNumberOfThreadsX = numberOfGroupsX * m_threadsPerGroupX`.

### 2.5 Direct vs Indirect dispatch

`DeviceDispatchArguments` (local `DeviceDispatchItem.h:78-102`) is a **tagged union**:
```cpp
enum class DispatchType : uint8_t { Direct = 0, Indirect };
struct DeviceDispatchArguments {
    DispatchType m_type;
    union { DispatchDirect m_direct; DeviceDispatchIndirect m_indirect; };
};
```
- **Direct**: thread/group counts are passed straight into the backend `Dispatch()` call. This is
  what you get from JSON `ThreadCount*` or `FullscreenDispatch`.
- **Indirect**: the dispatch dimensions are **read from a GPU buffer** at submit time
  (`DeviceDispatchIndirect = DeviceIndirectArguments`). Set `IndirectDispatch: true` +
  `IndirectDispatchBufferSlotName` pointing at a slot whose `ScopeAttachmentUsage` is `Indirect`.
  The pass builds an `IndirectBufferSignature` for a single `Dispatch` command. Use this when an
  **earlier compute pass computes the workload size** (e.g. compaction / culling) and a later pass
  must dispatch over the result without a CPU round-trip.

### 2.6 Choosing the dispatch sizing mode (decision table)

| You know the size... | Use | JSON |
|---|---|---|
| at author time, fixed | direct, static | `ThreadCountX/Y/Z` |
| equal to an output image's WxH | fullscreen | `FullscreenDispatch: true` (+ optional `FullscreenSizeSourceSlotName`) |
| at runtime on CPU | direct, dynamic | call `SetTargetThreadCounts()` (e.g. in the reload/prepare callback) |
| only on GPU (from a prior pass) | indirect | `IndirectDispatch: true` + `IndirectDispatchBufferSlotName` |

For **our data-compute workload** (size known on CPU at submit time), the path is:
plain `ComputePass` + override/owner calls `SetTargetThreadCounts(elementCount, 1, 1)` once the
buffer length is known.

---

## 3. Authoring a custom ComputePass + standalone dispatch

### 3.1 Steps to author a custom ComputePass from scratch

1. **Write the compute shader** (`.azsl` → `.shader`). Declare the Pass SRG
   (`ShaderResourceGroup ... : SRG_PerPass`) with your buffers, and `[numthreads(N,1,1)]` on the
   entry. (Covered in corpus doc 01/03 — AZSL.)
2. **Author the `.pass` `PassAsset`** with `"PassClass": "ComputePass"` (or your subclass name),
   the slots (one per SRG buffer, with `ShaderInputName` = the SRG member), and
   `PassData.$type = "ComputePassData"` pointing at your `.shader`. Pick the sizing mode (§2.6).
3. **Register the template** so the Pass System knows it:
   - Data path: list the `.pass` in a Pass Registry `.azasset` (an `AssetAliasesSourceData`); the
     Asset Processor registers it. TressFX does this in
     `AtomTressFX_PassTemplates.azasset`.
   - Code path: `PassSystemInterface::Get()->AddPassTemplate(name, template)`. Register a custom
     **pass class** with `AddPassCreator(...)` (or the `AZ_RPI_PASS` macro on your subclass, used by
     `ComputePass`, `ComputePass.h:31`).
4. **Insert into a pipeline** via a `PassRequest` in the pipeline tree, connecting your input slots
   to upstream pass outputs (see `AtomTressFX_PassRequest.azasset` above).
5. **(Optional) subclass `ComputePass`** when you need custom per-frame logic. Override
   `OnShaderReloadedInternal()` to call `SetTargetThreadCounts()`, or override
   `BuildCommandListInternal()` to submit multiple dispatch items (§3.2). The hair gem's
   `HairSkinningComputePass` and the engine's `SkinnedMeshComputePass`/`MorphTargetComputePass` are
   the canonical subclasses (see `Skinning.pass`, `MorphTarget.pass`,
   `HairGlobalShapeConstraintsCompute.pass`).

### 3.2 The "compute, not draw" pattern: multi-dispatch ComputePass

The generic `ComputePass` does **one** dispatch. For many dispatches (one per work item) the
canonical pattern is a subclass that overrides `BuildCommandListInternal` to loop `Submit` over a
per-frame list, using the scope's submit-range so the scheduler multithreads submission. The
engine's `SkinnedMeshComputePass` and `MorphTargetComputePass` (templates `SkinningPassTemplate`,
`MorphTargetPassTemplate` — local `Skinning.pass`, `MorphTarget.pass`) do exactly this: a feature
processor accumulates one dispatch item per instance each frame, the pass finds itself in the
pipeline by template name and submits them all. The `Meshlets` gem's `MultiDispatchComputePass`
(`development` branch) is the cleanest public reference:
```cpp
void MultiDispatchComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) {
    RHI::CommandList* commandList = context.GetCommandList();
    SetSrgsForDispatch(context);
    // iterate context.GetSubmitRange() [startIndex, endIndex)
    for (...; ++it)
        commandList->Submit((*it)->GetDeviceDispatchItem(context.GetDeviceIndex()), index);
    m_dispatchItems.clear();           // re-populated next frame
}
```
This is **the** model to copy for a batched data-compute engine: build N `RHI::DispatchItem`s on the
CPU each tick, hand them to the pass, let the frame graph schedule the submission.

### 3.3 OPEN QUESTION — standalone / non-render-pipeline compute

**Short answer: there is no API to fire a GPU dispatch *outside* the frame graph.** Every dispatch
is a `ScopeProducer` scope on the per-frame frame graph (`RenderPass` IS-A `ScopeProducer`,
`RenderPass.h:40`). **But "render pipeline" does NOT mean "draws to a window".** A pipeline/scope
need not touch a swapchain, a view, or any render target. Four mechanisms exist (the first three
verified locally via headers/assets; pipeline factories below cited from `development`):

**(a) A `ComputePass` whose only outputs are buffers / off-screen images — EXISTS, common.**
It lives in a pipeline but never writes the swapchain. Local proof: `Skinning.pass`,
`MorphTarget.pass`, `LightCulling.pass`, `LuminanceHistogramGenerator.pass`, and all the
TressFX `*Compute.pass` files are compute passes producing buffers/textures, not screen pixels.
Terrain clipmap generation (`TerrainMacroClipmapGenerationPass`, `TerrainDetailClipmapGenerationPass`
— local `.pass` files exist) is pure data-gen compute gated by an `IsEnabled()` override.

**(b) A custom multi-dispatch `ComputePass` (§3.2).** The sanctioned "lots of compute, no draw"
mechanism. Cleanest public API.

**(c) Raw RHI `ScopeProducer` (the substrate).** At the RHI layer
(`Atom/RHI/Code/Include/Atom/RHI/ScopeProducer.h` + `FrameScheduler::ImportScopeProducer`) a scope
can declare zero color attachments, only buffers, and run on `HardwareQueueClass::Compute`. Every
RPI pass registers itself this way. Rarely used directly outside RHI internals/tests, but it is the
floor everything sits on.

**(d) A windowless / off-screen `RenderPipeline` — the real "compute-only pipeline".**
`RenderPipelineDescriptor` has **no window field** and an `m_executeOnce` flag. Of the
`RenderPipeline` factory functions, only `CreateRenderPipelineForWindow(...)` creates a
`SwapChainPass`; `CreateRenderPipeline(desc)`, `CreateRenderPipelineFromAsset(...)`, and
`CreateRenderPipelineForImage(desc, imageAsset)` build pipelines with **no swapchain** (the last
renders into an imported `AttachmentImage`). One-shot execution via the pipeline `RenderMode`
(`RenderEveryTick` / `RenderOnce` / `NoRender`) + `AddToRenderTickOnce()`. → Fire a compute graph
once and remove it.
> ⚠️ These four factories are cited from the `development` branch; **verify the exact factory
> signatures against `RenderPipeline.h` in your synced 25.10.2 source before relying on them.**

**What does NOT exist:** a dispatch outside the frame graph. The sanctioned recipe for "standalone
GPU compute" is **(d) a windowless `RenderPipeline` containing (b) custom multi-dispatch
ComputePasses**, ticked once or per-frame, attached to a `Scene` that has no window. This is the
target shape for a data-compute engine that should not also be rendering.

---

## 4. Buffer attachments lifecycle (persistent GPU buffers across frames)

### 4.1 Transient vs Imported

`RHI::AttachmentLifetimeType` (`AttachmentEnums.h`): **`Imported`** (user-owned, persists across
frames) vs **`Transient`** (owned by the transient pool, valid for one frame only). In `.pass` JSON
the field is `"Lifetime": "Imported" | "Transient"`; **default is Transient.**

- **Transient** attachments are recreated by the frame graph every frame from a `SizeSource` /
  `FormatSource` / `ImageDescriptor`. Use for scratch that doesn't outlive the frame.
- **Imported** attachments wrap a real, persistent `RPI::AttachmentImage` / `RPI::Buffer` that you
  own — **this is how a compute pass keeps GPU state across frames.** The resource is imported into
  the frame graph once per frame (`ImportImage`/`ImportBuffer`) but the memory persists.

Local proof of an imported buffer-like resource — `BRDFTexture.pass:19-27`:
```jsonc
"ImageAttachments": [
  { "Name": "BRDFTexture", "Lifetime": "Imported",
    "AssetRef": { "FilePath": "Textures/BRDFTexture.attimage" } } ]
```

### 4.2 Importing a buffer at runtime from code

`Pass` exposes (local `Pass.h:226-228`):
```cpp
void AttachBufferToSlot(AZStd::string_view slot, Data::Instance<Buffer> buffer);
void AttachBufferToSlot(const Name& slot, Data::Instance<Buffer> buffer);
void AttachImageToSlot(const Name& slot, Data::Instance<AttachmentImage> image);
```
These build an `Imported`-lifetime attachment descriptor and bind your persistent `RPI::Buffer` /
`AttachmentImage` to the named slot. At frame-graph build the pass imports it
(`ImportBuffer`/`ImportImage`) so the scope can read/write it; the resource itself survives because
it comes from a persistent pool, not the transient pool. This is the **CPU→persistent-GPU-buffer
hand-off** for a data engine: create an `RPI::Buffer` once, `AttachBufferToSlot` it, and every frame
the compute pass reads/writes it in place.

### 4.3 Cross-frame read-last/write-this (ping-pong / double-buffer)

Canonical example: **TAA** — local `Taa.pass:46-85` declares **two `Imported`** images:
```jsonc
"ImageAttachments": [
  { "Name": "Accumulation1", "Lifetime": "Imported",
    "ImageDescriptor": { "Format": "R16G16B16A16_FLOAT", "BindFlags": "ShaderReadWrite" } },
  { "Name": "Accumulation2", "Lifetime": "Imported", ...same... } ]
```
The `TaaPass` C++ subclass swaps, each frame, which image is bound as **history input**
(`LastFrameAccumulation` slot) vs **write target** (`OutputColor` slot), toggling an index and
rebinding the slots in `FrameBegin`. That's the general recipe for "read last frame's buffer, write
this frame's":
1. Declare N (usually 2) `Imported` attachments in the `.pass`.
2. Hold them in your pass subclass.
3. Each `FrameBegin`: bind buffer[i] as the read/history slot, buffer[i^1] as the write slot, then
   flip `i`.

Note `Taa.pass` also shows `FallbackConnections` (line 86-91) — when the pass is disabled, its
output slot falls back to an input, so downstream passes still get a valid attachment.

### 4.4 Attachment connection mechanics

`PassAttachmentBinding` chains passes: an input binding's `m_connectedBinding` points at another
pass's output binding so they **share the same `PassAttachment`** (and thus the same GPU resource).
This is how output→input wiring (§1.4) shares memory rather than copying. Helper API on `Pass`:
`FindAttachmentBinding(slotName)` (`Pass.h:178`), `GetInputBinding`/`GetOutputBinding`/
`GetInputOutputBinding` (`Pass.h:214-220`).

---

## 5. Quick reference — local file map (25.10.2)

| Topic | File:line |
|---|---|
| ComputePass class | `Gems/Atom/RPI/Code/Include/Atom/RPI.Public/Pass/ComputePass.h:27-108` |
| ComputePassData (JSON blob) | `.../Include/Atom/RPI.Reflect/Pass/ComputePassData.h:19-44` |
| RenderPass (ScopeProducer base) | `.../Include/Atom/RPI.Public/Pass/RenderPass.h:36-180` |
| Dispatch group math (ceil) | `Gems/Atom/RHI/Code/Include/Atom/RHI/DeviceDispatchItem.h:40-102` |
| DispatchItem | `Gems/Atom/RHI/Code/Include/Atom/RHI/DispatchItem.h` |
| PassTemplate | `.../Include/Atom/RPI.Reflect/Pass/PassTemplate.h:27-97` |
| PassRequest | `.../Include/Atom/RPI.Reflect/Pass/PassRequest.h:30-78` |
| Pass keywords / attach API | `.../Include/Atom/RPI.Public/Pass/Pass.h:226-228, 445-448` |
| Plain ComputePass example | `Gems/Atom/Feature/Common/Assets/Passes/BRDFTexture.pass` |
| Subclassed compute (skinning) | `.../Assets/Passes/Skinning.pass`, `MorphTarget.pass` |
| Buffer SRG-bind example | `.../Assets/Passes/LightCulling.pass`, `LuminanceHistogramGenerator.pass` |
| Cross-frame imported ping-pong | `.../Assets/Passes/Taa.pass` |
| Pipeline insertion (PassRequest) | `Gems/AtomTressFX/Assets/Passes/AtomTressFX_PassRequest.azasset` |
| Template registry | `Gems/AtomTressFX/Assets/Passes/AtomTressFX_PassTemplates.azasset` |
| TressFX compute passes | `Gems/AtomTressFX/Assets/Passes/Hair*Compute.pass` |

### Web sources (verify method bodies here; SDK lacks .cpp)
- ComputePass.cpp — `https://raw.githubusercontent.com/o3de/o3de/development/Gems/Atom/RPI/Code/Source/RPI.Public/Pass/ComputePass.cpp`
- Pass System docs — `https://docs.o3de.org/docs/atom-guide/dev-guide/passes/`
  (live sub-pages: `pass-system/`, `pass-template-file-spec/`, `authoring-passes/`)
- Multi-dispatch reference — `Gems/Meshlets/Code/Source/Meshlets/MultiDispatchComputePass.cpp` (development)
- Off-screen pipeline factories — `Gems/Atom/RPI/Code/Include/Atom/RPI.Public/RenderPipeline.h` (development)

---

## 6. Port-relevant takeaways (CUDA → O3DE mental model)

- **Kernel launch → ComputePass + DispatchItem.** `kernel<<<grid,block>>>` becomes: author a
  `ComputePass`, set total threads, let RHI ceil-divide by the shader's `[numthreads]` into groups,
  and `commandList->Submit(dispatchItem)` inside `BuildCommandListInternal`.
- **You don't pick the backend.** Same pass runs on DX12/Vulkan/Metal via the RHI.
- **No free-floating dispatch.** Everything is a frame-graph scope. For "headless" compute, use a
  **windowless RenderPipeline** of compute passes (§3.3d) — not an out-of-band launch.
- **Persistent GPU state = `Imported` attachments** + `AttachBufferToSlot` (§4). Cross-frame
  read/write = double-buffered imported attachments, rebound each frame (TAA pattern).
- **Batched work = multi-dispatch subclass** (skinning/meshlets pattern, §3.2), not one pass per item.
