# AZSL + AZSLc + Shader Resource Groups — Training Corpus (Module 01)

> **Audience:** an engineer about to port a GPU compute engine onto O3DE's portable AZSL shader path.
> **Scope:** research/reference only — no port code here. Read this, then study the cited files directly.
> **O3DE version studied:** `/opt/O3DE/25.10.2` (Linux profile build). 148 `.azsl` + 334 `.azsli` files on disk.
> **Canonical compute example used throughout:** AtomTressFX hair simulation (a real GPU physics solver — closest analog to a compute engine port).

---

## 0. The one-paragraph mental model

AZSL (**A**mazon **S**hading **L**anguage) is a **superset of HLSL**. You write `.azsl`
(entry points) and `.azsli` (includes), almost exactly like HLSL Shader Model 6.2/6.3. The
extra surface area over HLSL is small and is mostly about **how resources get bound**:
`ShaderResourceGroup` (SRG) blocks, SRG *semantics* (binding frequencies), `option`
variables (shader variants), and `rootconstant`. A `.shader` JSON file names the source and
the entry points. The **AZSLc** transpiler lowers AZSL → HLSL (SM6.3) **plus a pile of JSON
reflection** (resource layout, options, bindings). Then the platform compiler (DXC) takes
the HLSL → DXIL (D3D12) or SPIR-V (Vulkan), and SPIRV-Cross takes SPIR-V → MetalSL (Metal).
So: **your compute kernel body is essentially HLSL; the porting work is the SRG/binding layer
and the build wiring**, not the math.

---

## 1. AZSL syntax for a COMPUTE shader

### 1.1 The entry point — it IS HLSL compute

A compute entry point in AZSL is written identically to HLSL: `[numthreads(...)]` attribute,
`void` return, system-value semantics on parameters. From the canonical hair solver:

`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairSimulationCompute.azsl:47-54`

```hlsl
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void IntegrationAndGlobalShapeConstraints(
    uint  GIndex : SV_GroupIndex,
    uint3 GId    : SV_GroupID,
    uint3 DTid   : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, ...;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, ...);
    ...
}
```

Things to note for a port — all of these are plain HLSL, unchanged in AZSL:

| Construct | Meaning | File:line |
|---|---|---|
| `[numthreads(X,Y,Z)]` | Thread-group dimensions. Here `THREAD_GROUP_SIZE` is a `#define`. | `HairSimulationCompute.azsl:47` |
| `SV_DispatchThreadID` (`uint3 DTid`) | Global thread index across the whole dispatch. | `:51` |
| `SV_GroupID` (`uint3 GId`) | Which thread-group this is. | `:50` |
| `SV_GroupIndex` (`uint GIndex`) | Flattened thread index *within* the group. | `:49` |
| `SV_GroupThreadID` | (not used here) per-group XYZ thread index. | — |
| `GroupMemoryBarrierWithGroupSync()` | LDS barrier. | `HairSimulationCompute.azsl:69, 311, 355` |
| `groupshared` LDS | declared at file scope. | see §1.2 |

**Multiple compute entry points per file are allowed and normal.** `HairSimulationCompute.azsl`
defines five separate `[numthreads]` kernels in one file (`IntegrationAndGlobalShapeConstraints`,
`CalculateStrandLevelData`, `VelocityShockPropagation`, `LocalShapeConstraints`,
`LengthConstriantsWindAndCollision`, `UpdateFollowHairVertices`). Each is selected by a
separate `.shader` file naming the entry point (see §3.2). This is a key port pattern: one
`.azsl` translation unit, many dispatchable kernels.

### 1.2 Groupshared / LDS

Declared at global scope, exactly as HLSL. From the SSAO compute shader
`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/SsaoCompute.azsl:26`:

```hlsl
groupshared float LDS[LDS_SIZE];   // LDS_SIZE = 1024
```

The hair shader keeps its `sharedPos[]` / `sharedLength[]` / `sharedTangent[]` LDS arrays in
the `HairSimulationCommon.azsli` include and synchronizes with
`GroupMemoryBarrierWithGroupSync()` (`HairSimulationCompute.azsl:69`).

### 1.3 How a compute shader differs from "plain HLSL" in AZSL

The **kernel body does not differ** — intrinsics, control flow, `asfloat`/`asint`, math, LDS,
barriers are all stock HLSL. What differs is **everything around resource access**:

1. You cannot declare a global `cbuffer` or a bare global resource at file scope and have it
   bind. Resources must live inside a `ShaderResourceGroup` (see §2). Global `static const`
   is fine; global mutable constants are a compile error.
2. Resource access is **scope-qualified**: `HairGenerationSrg::m_initialHairPositions`, not a
   bare global. The codebase hides this behind `#define` aliases (see §2.4).
3. Binding registers (`t0`, `u0`, `b0`, `space0`…) are **assigned by AZSLc**, not hand-written.
   You never write `: register(u0)`.

---

## 2. ShaderResourceGroups (SRG) — the part that matters most for a data-buffer port

This is where your engine's buffers enter the shader. Get this right and the kernel is easy.

### 2.1 Declaring an SRG

Syntax (from the O3DE AZSL language reference,
<https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azsl/>):

```cpp
ShaderResourceGroup <Name> : <Semantic>
{
    <resources, constants, structs, functions>
};
```

Real read-write compute SRG — the hair dynamic-data buffers
`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairComputeSrgs.azsli:88-103`:

```hlsl
ShaderResourceGroup HairDynamicDataSrg : SRG_PerObject   // space 1 - per instance / object
{
    RWBuffer<float4>                     m_hairVertexPositions;
    RWBuffer<float4>                     m_hairVertexPositionsPrev;
    RWBuffer<float4>                     m_hairVertexPositionsPrevPrev;
    RWBuffer<float4>                     m_hairVertexTangents;
    RWStructuredBuffer<StrandLevelData>  m_strandLevelData;

    // Per hair object offset to the start of each buffer within 'm_skinnedHairSharedBuffer'.
    uint m_positionBufferOffset;
    uint m_positionPrevBufferOffset;
    uint m_positionPrevPrevBufferOffset;
    uint m_tangentBufferOffset;
    uint m_strandLevelDataOffset;
};
```

Read-only inputs + an embedded constant-buffer struct
`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairSimulationComputeSrgs.azsli:83-94`:

```hlsl
ShaderResourceGroup HairGenerationSrg : SRG_PerDraw
{
    Buffer<float4>                      m_initialHairPositions;
    Buffer<float>                       m_hairRestLengthSRV;
    Buffer<float>                       m_hairStrandType;
    Buffer<float4>                      m_followHairRootOffset;
    StructuredBuffer<BoneSkinningData>  m_boneSkinningData;

    // Loose fundamental members get packed into ONE implicit constant buffer (a CBV):
    TressFXSimulationParams             m_tressfxSimParameters;
};
```

A shared scratch buffer at pass frequency
`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairComputeSrgs.azsli:62-65`:

```hlsl
ShaderResourceGroup PassSrg : SRG_PerPass
{
    RWStructuredBuffer<int>     m_skinnedHairSharedBuffer;   // the memory pool all dynamic
                                                             // buffers are sub-allocated from
};
```

**What goes in an SRG** (per the docs):
- **SRV** read-only views: `Buffer<T>`, `StructuredBuffer<T>`, `ByteAddressBuffer`, `Texture2D`, …
- **UAV** read-write views: `RWBuffer<T>`, `RWStructuredBuffer<T>`, `RWTexture2D`, `RWByteAddressBuffer`, …
- **CBV** — *loose* fundamental members (`float4`, `uint`, matrices, instantiated structs) are
  auto-gathered into a single implicit constant buffer for that SRG.
- **Sampler** states (dynamic or static — see §2.5).

### 2.2 SRG semantics = binding frequency (THE key concept)

Each SRG is tagged with a *semantic* that declares **how often its data changes** and therefore
**which descriptor set / register space** it lands in. Semantics are declared with
`ShaderResourceGroupSemantic` and carry a `FrequencyId`.

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/SrgSemantics.azsli`:

```cpp
ShaderResourceGroupSemantic SRG_PerDraw    { FrequencyId = 0; ShaderVariantFallback = 128; }
ShaderResourceGroupSemantic SRG_PerObject  { FrequencyId = 1; }
ShaderResourceGroupSemantic SRG_PerMaterial{ FrequencyId = 2; }
ShaderResourceGroupSemantic SRG_PerSubPass { FrequencyId = 3; }
ShaderResourceGroupSemantic SRG_PerPass    { FrequencyId = 4; }
ShaderResourceGroupSemantic SRG_PerView    { FrequencyId = 5; }
ShaderResourceGroupSemantic SRG_PerScene   { FrequencyId = 6; }
ShaderResourceGroupSemantic SRG_Bindless   { FrequencyId = 7; }
```

Semantics:
- **`FrequencyId` defines the register-space / descriptor-set order.** Lower id = bound earlier.
  With `--use-spaces` each SRG gets its own `spaceN` ordered by `FrequencyId` (docs:
  <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azsl/srg-semantics/>). So
  `SRG_PerDraw` → space0, `SRG_PerObject` → space1, etc. The hair `HairDynamicDataSrg` comment
  literally says `// space 1` because it is `SRG_PerObject`.
- **`ShaderVariantFallback = 128`** marks an SRG as the owner of the bit array where shader
  *option* keys are encoded (see §3.4). Exactly one SRG per shader may own it. `SRG_PerDraw`
  owns it here (`ShaderVariantKeyBitCount`).
- **Frequency is a contract with the renderer**: the C++ side binds a `PerDraw` SRG once per
  draw, a `PerPass` SRG once per pass, etc. Choosing the wrong frequency means rebinding too
  often (slow) or sharing data you meant to be per-instance (wrong results).

### 2.3 `.srgi` files and `partial` SRGs

`.srgi` ("SRG include") files combine **partial** SRG fragments contributed by different
sources into one unified SRG. Your engine already ships the two canonical ones:

`/opt/project/repo/hcp-engine/ShaderLib/scenesrg.srgi:18-23`:
```cpp
#include <Atom/Features/SrgSemantics.azsli>

partial ShaderResourceGroup SceneSrg : SRG_PerScene
{
    /* Intentionally Empty. Add fields here based on the project's needs */
};
```
`/opt/project/repo/hcp-engine/ShaderLib/viewsrg.srgi:20-23` is the same shape for
`ViewSrg : SRG_PerView`. These are **per-project extension points**: O3DE's
`scenesrg_all.srgi` / `viewsrg_all.srgi` (under
`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/ShaderLib/`) `#include` your project's
`scenesrg.srgi`, merging your fields into the engine-wide Scene/View SRG.

`partial` rules (docs, AZSL ref):
1. Every block of a given SRG must be `partial`.
2. At least one block must state the **semantic** (`: SRG_PerScene`).
3. Multiple semantic-bearing blocks must all name the **same** semantic.
4. *"When AZSLc finds the first `partial ShaderResourceGroup` block, it will use it as the
   unified point of emission for all the data of a given SRG."*

A shader opts into Scene/View SRGs by including the `_all` headers, e.g. SSAO compute
`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/SsaoCompute.azsl:11-12`:
```cpp
#include <scenesrg_all.srgi>
#include <viewsrg_all.srgi>
```

### 2.4 The `#define` aliasing pattern (porting AMD/HLSL code with minimal edits)

Because AZSL requires scope-qualified access (`SrgName::member`) but the original TressFX HLSL
used bare globals like `g_HairVertexPositions`, the port wraps each SRG member in a `#define`.
`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairComputeSrgs.azsli:108-113`:
```cpp
#define g_HairVertexPositions      HairDynamicDataSrg::m_hairVertexPositions
#define g_HairVertexPositionsPrev  HairDynamicDataSrg::m_hairVertexPositionsPrev
#define g_StrandLevelData          HairDynamicDataSrg::m_strandLevelData
```
and even reaches *into* the CBV struct
(`HairSimulationComputeSrgs.azsli:106-123`):
```cpp
#define g_NumOfStrandsPerThreadGroup  HairGenerationSrg::m_tressfxSimParameters.m_Counts.x
#define g_TimeStep                    HairGenerationSrg::m_tressfxSimParameters.m_GravTimeTip.y
```
**This is the recommended porting technique:** declare your buffers in SRGs once, then
`#define` the old bare names so the existing kernel body compiles unchanged. The hair kernel in
`HairSimulationCompute.azsl` reads `g_InitialHairPositions[...]`, `g_HairVertexPositions[...]`
etc. and never mentions an SRG name — the defines bridge it.

### 2.5 Samplers (dynamic vs static) — AZSL extension

Per the AZSL reference, the `Sampler` keyword auto-resolves to `SamplerState` or
`SamplerComparisonState` (the latter if a `ComparisonFunc` is present):
```cpp
Sampler m_dynamicSampler;                 // runtime-set, no body
Sampler m_staticSampler { AddressU = Wrap; MagFilter = Linear; };   // baked at compile time
```

### 2.6 How data gets bound from C++ (the runtime side)

The shader-side SRG declaration is **reflected by AZSLc into JSON** (resource layout, register
assignments, the CBV member offsets, options). At runtime the Atom RHI/RPI consumes that
reflection: the engine creates a `ShaderResourceGroup` instance from the shader asset, then sets
resources/constants **by name** matching the AZSL member name. The conceptual flow:

1. `.azsl`/`.azsli` SRG declaration → AZSLc → SRG-layout JSON (`--srg`) + reflection.
2. The shader asset (`.azshader`) carries that layout.
3. C++ creates an SRG instance and calls the equivalent of
   `SetBufferView("m_hairVertexPositions", view)`, `SetConstant("m_tressfxSimParameters", data)`,
   `SetImageView(...)`, `Compile()` — names map straight to the AZSL members.
4. The SRG is bound at its declared **frequency** (per-draw / per-pass / …) and AZSLc-assigned
   register space, so dispatch just references the bound descriptor sets.

The C++ reflection/compiler glue lives in
`/opt/O3DE/25.10.2/Gems/Atom/Asset/Shader/Code/Source/Editor/AzslCompiler.h` (parses the
AZSLc JSON outputs into engine structures) — read it when wiring the runtime binding.

> **Port takeaway:** your engine's GPU buffers become SRG members; the C++ side sets them by the
> exact member name; the *frequency semantic* you pick decides the descriptor set and how often
> you rebind. Match each buffer's update cadence to a `FrequencyId` (per-dispatch scratch →
> `SRG_PerPass`, per-instance data → `SRG_PerObject`, immutable inputs → `SRG_PerDraw`).

---

## 3. AZSLc — what it is, what it emits, how the build invokes it

### 3.1 The transpile chain

Source: O3DE Shader Build Pipeline
(<https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/shader-build-pipeline/>) and the
local `AzslcHeader.azsli` files.

```
.azsl ──(mcpp preprocess #1)──► append AzslcHeader ──(mcpp preprocess #2)──►
   AZSLc  ──►  HLSL (Shader Model 6.3)  +  reflection JSON
                       │
                       ├─ D3D12 : DXC ───────────────► DXIL
                       ├─ Vulkan: DXC (-spirv) ──────► SPIR-V
                       └─ Metal : DXC → SPIR-V → SPIRV-Cross ─► MetalSL (.metal)
```

Two preprocessor passes use **mcpp**. The platform-specific `AzslcHeader.azsli` is appended
before AZSLc; the Linux/Vulkan one
(`/opt/O3DE/25.10.2/bin/Linux/profile/Default/Builders/ShaderHeaders/Platform/Linux/Vulkan/AzslcHeader.azsli:9-15`)
documents this:
```cpp
/* The shader build pipeline has 2 preprocess stages. The first one happens
 * after appending this header and pre-processing with mcpp ahead of azslc. */
static const float4 s_AzslDebugColor = float4(165.0/255.0, 30.0/255.0, 36.0/255.0, 1);
#include <Atom/RPI/Platform/Linux/AzslcPlatformHeader.azsli>
```
Per-platform AzslcHeaders exist for Windows DX12, Windows/Linux/Android **Vulkan**, Mac/iOS
**Metal**, and **Null** — i.e. the same `.azsl` retargets across all of them.

### 3.2 The `.shader` file (entry points + render state)

A `.shader` is JSON that names the AZSL source and its entry points. Minimal compute example
`/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairGlobalShapeConstraintsCompute.shader`:
```json
{
    "Source" : "HairSimulationCompute.azsl",
    "ProgramSettings": {
        "EntryPoints": [
            { "name": "IntegrationAndGlobalShapeConstraints", "type": "Compute" }
        ]
    }
}
```
Note: one `.azsl` with several kernels → several `.shader` files, each selecting one entry
point by name. (`DepthUpsample.shader:9` does the same with `MainCS`.)

A graphics `.shader` also carries fixed-function render state — e.g.
`/opt/O3DE/25.10.2/Gems/AtomLyIntegration/AtomBridge/Assets/Shaders/SimpleTextured.shader`:
```json
{
    "Source" : "SimpleTextured.azsl",
    "DepthStencilState" : { "Depth" : { "Enable" : false, "CompareFunc" : "Always" } },
    "RasterState" : { "DepthClipEnable" : false, "CullMode" : "None" },
    "GlobalTargetBlendState" : { "Enable": true, "BlendSource": "One",
                                 "BlendDest": "AlphaSourceInverse", "BlendOp": "Add" },
    "DrawList" : "2dpass",
    "ProgramSettings": { "EntryPoints": [
        { "name": "MainVS", "type": "Vertex" },
        { "name": "MainPS", "type": "Fragment" } ] }
}
```
For a pure compute port you only need `Source` + `ProgramSettings.EntryPoints` (`type:"Compute"`);
the render-state blocks are irrelevant.

### 3.3 AZSLc command-line surface (from the actual binary)

`azslc --help` on the shipped Linux binary
(`/opt/O3DE/25.10.2/bin/Linux/profile/Default/Builders/AZSLc/azslc`) — the flags you will care
about for a port:

| Flag | Effect | Why it matters for a port |
|---|---|---|
| `-o FILE` | output file (else stdout) | basic invocation |
| `--namespace dx\|vk\|mt …` | activate API attribute namespace + API features | retargeting per platform |
| `--use-spaces` | each SRG → own register space, ordered by FrequencyId | how SRGs map to `spaceN` |
| `--unique-idx` | `b0,t0,u0,s0` → `b0,t1,u2,s3` (no per-type register reuse) | **required for Vulkan/Metal** |
| `--srg` | emit the **SRG layout** instead of shader code | this is the reflection your C++ binds against |
| `--options` | emit the list of available **shader options** | drives variant generation |
| `--ia` | emit VS input-assembler layouts **and CS `numthreads`** | confirms your compute group size survived |
| `--om` | emit Output-Merger layout | raster only |
| `--bindingdep` | which entry points access which resources | dead-resource analysis / `--strip-unused-srgs` |
| `--full` | code + IA + OM + SRG + options + bindingdep | one-shot dump for inspection |
| `--strip-unused-srgs` | strip SRGs no entry point uses | **already enabled in your engine** (see §3.6) |
| `--root-const N` / `--pad-root-const` | size & pad the root-constant CB | if you use `rootconstant` |
| `--Zpr` / `--Zpc` | matrix packing row/column major | must match `DefaultMatrixOrder` (§3.6) |
| `--pack-dx12` / `--pack-vulkan` / `--pack-opengl` | strict per-API cbuffer packing | struct layout parity across APIs |
| `--no-alignment-validation` | skip DXIL↔SPIR-V alignment check | by default a mismatch **fails the build** — see §4 |
| `--no-ms` | rewrite `Texture2DMS*` → non-MS | MS/non-MS pipeline sharing |
| `--sc-options` | encode shader options as **specialization constants** | alt to variant keys (Vulkan) |
| `--max-spaces N` / `--min-descriptors …` | respect descriptor/space limits | console/mobile descriptor budgets |

AZSLc's two output categories (docs): **(1)** transpiled HLSL, **(2)** reflection JSON —
"the shader constants layout, resource binding information, shader variant options, and more."

### 3.4 Shader options / variants

`option` variables are compile-time switches declared at global scope. Real example
`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/ScreenSpace/DeferredFog.azsl:86-89`:
```cpp
option bool o_useNoiseTexture = true;
option bool o_enableFogLayer  = true;
option enum class FogMode { LinearMode, ExponentialMode, ExponentialSquaredMode }
       o_fogMode = FogMode::LinearMode;
```
Rules (AZSL ref):
- Allowed types: `bool`, `int` (with `[[range(min,max)]]`), `enum`. **No float, no struct.**
- All options for a shader are packed into a **single bit array** — the *shader variant key* —
  which is owned by the SRG marked `ShaderVariantFallback` (§2.2; here `SRG_PerDraw`,
  `ShaderVariantFallback = 128` = `RPI::ShaderVariantKeyBitCount`).
- A "root variant" compiles with all options at their fallback/default; specific
  `.shadervariant` combinations are baked on demand. `--options` lists them; the shader-asset
  builder produces `.azshadervariant` files.

For a compute port you may not need options at all — but if your engine has compile-time feature
toggles, this is the mechanism (vs `#define` permutations).

### 3.5 Root constants

`rootconstant` (AZSL ref) is a globally-scoped, fast-path constant backed by the D3D12 root
signature:
```cpp
rootconstant float4x4 s_objectMatrix;
rootconstant uint     s_materialIndex;
```
Budget is tiny (root signature ≤ 64 DWords / 256 bytes). Size/pad it with `--root-const` /
`--pad-root-const`. Use for a handful of per-dispatch scalars you want cheaper than a CBV.

### 3.6 How **your** engine already invokes AZSLc

`/opt/project/repo/hcp-engine/Config/shader_global_build_options.json`:
```json
{
    "Type": "JsonSerialization", "Version": 1, "ClassName": "GlobalBuildOptions",
    "ClassData": {
        "ShaderCompilerArguments" : {
            "DefaultMatrixOrder" : "Row",
            "AzslcAdditionalFreeArguments" : "--strip-unused-srgs"
        }
    }
}
```
Two project-wide facts to honour in the port:
- **Matrices default to ROW major** (`DefaultMatrixOrder: "Row"` → `--Zpr`). If your C++ uploads
  column-major matrices, transpose or annotate, or the math silently flips. (The hair CBV uses
  explicit `row_major float4x4` at `HairSimulationComputeSrgs.azsli:68` to be safe.)
- **`--strip-unused-srgs` is on**: an SRG that no entry point references gets removed. If your
  C++ tries to bind a buffer the kernel never reads, the SRG may not exist in the layout.

---

## 4. Gotchas / constraints vs plain HLSL that will bite a port

1. **No bare global resources or `cbuffer`s.** Every resource/constant must live in an SRG
   (or `rootconstant`, or `option`). Global mutable constants are a *compile error*; only
   `static const` globals are allowed. (AZSL ref.) → wrap your buffers in SRGs.

2. **Scope-qualified access.** `SrgName::member`. Port via the `#define alias` pattern (§2.4)
   to keep the kernel body unchanged.

3. **You never write registers.** No `: register(t0, space1)`. AZSLc assigns them from
   `FrequencyId`. Picking the wrong semantic silently changes the descriptor set the C++ must
   bind to.

4. **mcpp can't do arithmetic macros.** *"MCPP doesn't support C Macros with arithmetic
   expressions."* The hair code even left a macro commented out for this reason
   (`HairComputeSrgs.azsli:53-55`: the `BYTE_OFFSET` macro is noted as unusable). Move any
   compute-in-the-preprocessor into real shader code or a build step.

5. **DXIL vs SPIR-V alignment is validated and fails the build by default.** AZSLc's
   `--no-alignment-validation` exists precisely because cbuffer/struct member alignment can
   differ between the D3D12 and Vulkan packing rules. Use `[[pad_to(N)]]` (N a multiple of 4)
   on struct members and explicit padding fields (the hair CBV has `m_pad1/m_pad2` at
   `HairSimulationComputeSrgs.azsli:62-63`) to keep `ConstantBuffer` and `StructuredBuffer`
   layouts identical across APIs. Do **not** just disable the check.

6. **Matrix order is project-global (Row here).** Mismatched upload order = wrong transforms.
   Annotate `row_major`/`column_major` explicitly where it matters.

7. **`--strip-unused-srgs` removes unreferenced SRGs.** A buffer declared but unused by the
   selected entry point won't appear in the layout your C++ binds against.

8. **`--unique-idx` is mandatory on Vulkan/Metal** (they don't separate registers by resource
   type). If you ever invoke AZSLc directly for those targets, this must be set or bindings
   collide. The O3DE build does this for you; a hand-rolled invocation must not forget it.

9. **Structs/classes are restricted.** Structs are global, data-only, no constructors, no
   default initializers; classes are global, no ctor/dtor (inheritance only since azslc 1.8.9).
   Declare struct/SRG members *before* the methods that use them — "late declaration access …
   may not be tracked properly." (AZSL ref.)

10. **CBV auto-gather is implicit.** Loose fundamental members of an SRG silently become one
    constant buffer. The order/packing of those members is what your C++ `SetConstant` must
    match — keep the struct/member layout stable and padded.

11. **One `.azsl`, many kernels, many `.shader` files.** Each compute entry point needs its own
    `.shader` (or supervariant) selecting it by name. Don't expect one `.shader` to expose all
    kernels in a file.

12. **Shader options are not floats and are key-encoded.** If you port `#define` permutations to
    `option`s, remember only bool/int/enum are allowed and they consume the variant-key bits in
    the `ShaderVariantFallback` SRG.

---

## 5. Quick reference — files to study on disk

| Topic | File |
|---|---|
| Compute entry points (5 kernels, 1 file) | `/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairSimulationCompute.azsl` |
| RW/SRV compute SRGs, frequency choices, LDS helpers | `/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairComputeSrgs.azsli` |
| CBV struct in SRG, `#define` aliasing, `row_major` | `/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairSimulationComputeSrgs.azsli` |
| All SRG semantics / FrequencyIds | `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/SrgSemantics.azsli` |
| `groupshared` LDS compute pattern | `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/SsaoCompute.azsl` |
| `option` (bool/enum) declarations | `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/ScreenSpace/DeferredFog.azsl:86` |
| Minimal compute `.shader` | `/opt/O3DE/25.10.2/Gems/AtomTressFX/Assets/Shaders/HairGlobalShapeConstraintsCompute.shader` |
| `.shader` with render state | `/opt/O3DE/25.10.2/Gems/AtomLyIntegration/AtomBridge/Assets/Shaders/SimpleTextured.shader` |
| Project's Scene/View SRG extension points | `/opt/project/repo/hcp-engine/ShaderLib/scenesrg.srgi`, `viewsrg.srgi` |
| Project's global AZSLc args | `/opt/project/repo/hcp-engine/Config/shader_global_build_options.json` |
| Two-stage preprocess + platform header | `/opt/O3DE/.../ShaderHeaders/Platform/Linux/Vulkan/AzslcHeader.azsli` |
| C++ side that parses AZSLc reflection | `/opt/O3DE/25.10.2/Gems/Atom/Asset/Shader/Code/Source/Editor/AzslCompiler.h` |
| The AZSLc binary itself (`--help`) | `/opt/O3DE/25.10.2/bin/Linux/profile/Default/Builders/AZSLc/azslc` |

---

## 6. Sources

**Local (cited inline with file:line).** O3DE `25.10.2` tree under `/opt/O3DE/25.10.2` and the
project engine under `/opt/project/repo/hcp-engine`.

**Web (O3DE official docs):**
- AZSL language reference — <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azsl/>
- AZSLc compiler — <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azslc/>
- SRG semantics — <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azsl/srg-semantics/>
- Shader build pipeline — <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/shader-build-pipeline/>
- Shader variants & fallback key — <https://docs.o3de.org/docs/atom-guide/dev-guide/shaders/azsl/shader-variants-fallback-key/>
- AZSLc source / wiki — <https://github.com/o3de/o3de-azslc>, <https://github.com/o3de/o3de-azslc/wiki/Features>

---

## 7. Gaps / unresolved

- **Exact C++ binding API names** (the real method signatures on `RHI::ShaderResourceGroup` /
  `RPI::ShaderResourceGroup` — `SetBufferView`, `SetConstant`, `Compile`, etc.) were inferred
  from the reflection model, not quoted from a doc page. Verify against
  `AzslCompiler.h` and the RPI `ShaderResourceGroup` headers before writing binding code.
- **The exact JSON schema AZSLc emits** (`--srg`, `--options`, `--bindingdep` output shape) was
  described from docs but not captured field-by-field. Run `azslc --full` on a real `.azsl` to
  capture ground truth — recommended as Module 02.
- **Supervariant mechanics** (how a `.shader` declares multiple supervariants, and how
  `--no-ms`/`--no-subpass-input` tie in) are only sketched; the docs page on supervariants
  should be pulled if the port needs MS/non-MS or subpass variants.
- **Bindless (`SRG_Bindless`, FrequencyId 7)** is listed but not explored — relevant only if the
  port needs descriptor-indexing-style access to large resource arrays.
