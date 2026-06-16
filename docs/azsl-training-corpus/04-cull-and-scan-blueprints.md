# 04 — Cull & Scan Blueprints (AZSL Compute)

> **Training corpus — RESEARCH ONLY. No port code here.** This document is for a DI to *study* before
> porting our GPU broad-phase culling and discrimination scans to O3DE AZSL compute. Every excerpt is
> cited to `file:line` (local O3DE 25.10.2 tree) or a GitHub/web URL. Each section ends with an explicit
> **"maps to our …"** note that names the transferable mechanic.

## TL;DR — what we are actually mining, and where it really lives

Two blueprints were requested. The *named* sources turned out to be partially dead ends, and the *real*
transferable AZSL lives elsewhere in the same O3DE tree. This is the single most important finding:

| Requested source | Verdict | Use instead |
|---|---|---|
| **A) MESHLETS** (GPU-driven culling) | **NOT local.** Web shows O3DE's `Gems/Meshlets` has **no cull shader at all** — it only builds an index buffer; culling is a `// future step` TODO. | O3DE-local **`LightCulling.azsl`** is a *complete, working* one-thread-per-candidate predicate + group-shared compaction. The canonical wave-compaction idiom comes from MS **`MeshletAS.hlsl`** (web). |
| **B) FastNoise / GradientSignal** (heavy parallel math → discrimination scans) | **CPU-only.** FastNoise is a CPU SIMD library (`FastNoise.h`); GradientSignal evaluates per-sample on CPU. **No `.azsl` in either Gem.** | O3DE-local **`LuminanceHistogramGenerator.azsl`** (predicate→bin→atomic), **`NewDepthOfFieldTileReduce.azsl`** (group-shared min/max reduce), **`DownsampleSinglePassLuminance.azsl`** (single-pass multi-level reduce), **`LightCullingTilePrepare.azsl`** (band-presence bitmask). |

So: **do not study FastNoise/GradientSignal for GPU code — they have none.** Study O3DE's Atom Feature
shaders. They are the production discrimination-scan and cull idioms, already written in AZSL, already
building on Linux in this exact install.

---

# BLUEPRINT A — BROAD-PHASE CULL (one-thread-per-candidate → predicate → compact survivors)

## A.1 The working O3DE reference: `LightCulling.azsl`

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/LightCulling/LightCulling.azsl`

This is a *tiled* light-cull: one thread-group per screen tile, and within a group, **threads stride over a
huge candidate buffer**, each evaluating an inclusion predicate, and **survivors are compacted into
group-shared memory via an atomic counter**, then flushed to a global list. This is exactly our broad-phase:
strided one-thread-per-candidate, uniform predicate, atomic-counter compaction.

### A.1.a Strided one-thread-per-candidate loop

Every cull function uses the same grid-stride loop — `groupIndex` is the thread's start, stride is the
group size, so N threads sweep an arbitrarily large candidate array (`LightCulling.azsl:305-310`):

```hlsl
void CullSimplePointLights(uint groupIndex, TileLightData tileLightData, float3 aabbCenter, float3 aabbHalfExtents)
{
    for (uint lightIndex = groupIndex ; lightIndex < PassSrg::m_simplePointLightCount ; lightIndex += TILE_DIM_X * TILE_DIM_Y)
    {
        SimplePointLight light = PassSrg::m_simplePointLights[lightIndex];
        CullPointLight(lightIndex, light.m_position, light.m_invAttenuationRadiusSquared, tileLightData, aabbCenter, aabbHalfExtents);
    }
}
```

### A.1.b The inclusion predicate (cheap reject → mark survivor)

`LightCulling.azsl:285-301` — a coarse test (`TestSphereVsAabbInvSqrt`), and only on pass does it call the
compaction primitive. Note the *staged* predicate: cheap sphere-vs-AABB first, finer tests gated behind it
(`CullSimpleSpotLights`, `:333-341`, does hemisphere then cone, `continue`-ing on each reject):

```hlsl
void CullPointLight(uint lightIndex, float3 lightPosition, float invLightRadius, TileLightData tileLightData, float3 aabbCenter, float3 aabbHalfExtents)
{
    lightPosition = WorldToView_Point(lightPosition);
    bool potentiallyIntersects = TestSphereVsAabbInvSqrt(lightPosition, invLightRadius, aabbCenter, aabbHalfExtents);
    if (potentiallyIntersects)
    {
        uint inside = 0;
        float2 minmax = ComputePointLightMinMaxZ(rsqrt(invLightRadius), lightPosition);
        if (IsObjectInsideTile(tileLightData, minmax, inside))
        {
            MarkLightAsVisibleInSharedMemory(lightIndex, inside);   // <-- compaction entry point
        }
    }
}
```

### A.1.c **THE KEY TRANSFERABLE BIT — atomic-counter compaction**

`LightCulling.azsl:193-201`. A survivor reserves its slot with one `InterlockedAdd` on a group-shared
counter; the returned pre-increment value *is* its compacted write index. This is stream compaction
without a prefix-sum pass:

```hlsl
void MarkLightAsVisibleInSharedMemory(uint lightIndex, uint inside)
{
    uint sharedLightIndex;
    InterlockedAdd(shared_lightCount, 1, sharedLightIndex);   // returns OLD value = my dense slot

    sharedLightIndex = min(sharedLightIndex, NVLC_MAX_POSSIBLE_LIGHTS_PER_BIN - 1);  // clamp = bounded overflow

    shared_lightIndices[sharedLightIndex] = PackLightIndexWithBinMask(lightIndex, inside);
}
```

Group-shared declarations (`LightCulling.azsl:57-58`):
```hlsl
groupshared uint shared_lightCount;
groupshared uint shared_lightIndices[TILE_DIM_X * TILE_DIM_Y];
```

### A.1.d Flush compacted survivors to global memory (the second half of compaction)

`LightCulling.azsl:203-211`. After the group barrier, each thread copies one survivor from group-shared to
the global list at a per-group offset — group-shared compaction first (cheap atomics), global write once
(coalesced):

```hlsl
void CopySharedLightsToMainMemory(uint lightCount, uint groupIndex, uint3 groupID)
{
    if( groupIndex < shared_lightCount )
    {
        uint offset = min(lightCount + groupIndex, NVLC_MAX_POSSIBLE_LIGHTS_PER_BIN - 1);
        uint index = GetLightListIndex(groupID, PassSrg::m_constantData.m_gridWidth, offset);
        PassSrg::m_lightList[index] = shared_lightIndices[groupIndex];
    }
}
```

### A.1.e Barrier discipline (clear → cull → barrier → flush → reset)

`LightCulling.azsl:489-494` and the `MainCS` driver `:586-643`. The pattern per candidate-type is:
`ClearSharedLightCount` → cull (atomics into group-shared) → `GroupMemoryBarrierWithGroupSync` →
flush → double-barrier reset before the next type. Study the double barrier — it prevents a fast thread
from zeroing the counter while a slow thread is still reading it:

```hlsl
void ClearSharedLightCountWithDoubleBarrier(uint groupIndex)
{
    GroupMemoryBarrierWithGroupSync();
    ClearSharedLightCount(groupIndex);   // only thread 0 writes 0
    GroupMemoryBarrierWithGroupSync();
}
```

> **Maps to our broad-phase cull:**
> - **Candidate buffer** = our huge candidate set (`m_simplePointLights[]` → our candidates).
> - **Grid-stride loop** (`A.1.a`) = our one-logical-thread-per-candidate sweep over a buffer larger than the dispatch.
> - **Staged predicate** (`A.1.b`) = our exclusion predicate, cheap-reject-first. `continue` on early reject is free parallelism.
> - **`InterlockedAdd(counter,1,slot)` group-shared compaction** (`A.1.c`) = **our survivor compaction.** This is the transferable core. No prefix-sum needed when survivors-per-group fit in LDS; the atomic's *return value* is the dense index.
> - **`min(slot, MAX-1)` clamp** = our bounded-overflow policy. The shader never overruns LDS; it drops on saturation. Decide our policy: drop vs. spill to a second pass.
> - **Two-level compaction** (LDS atomic → single coalesced global write, `A.1.d`) = our pattern to keep global-atomic contention near zero.

## A.2 Per-tile predicate *input* construction — band-presence bitmask (`LightCullingTilePrepare.azsl`)

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/LightCulling/LightCullingTilePrepare.azsl`

Before culling, this pass reduces the depth buffer per tile to a **min/max range + a 32-bit band-presence
bitmask** — "which z-subsections of this tile actually contain geometry" (`:29-42`):

```
// The minimum/maximum depth values in the tile form a range ... the corresponding bit in a 32-bit bitmask
//  32-bit mask  |X|X|X|X|X|X|X|X|X|X|X| | | | | | | |X| |X| |X|X|X| |X
```

It builds the range with group-shared atomic min/max (`:56-57`):
```hlsl
DEPTH_InterlockedMin(shared_depthNear, asuint(data.x), orig);
DEPTH_InterlockedMax(shared_depthFar,  asuint(data.y), orig);
```
and turns a value-range into a contiguous run of set bits (`CreateFilledBitmask`, `:128`).

> **Maps to our discrimination scan (band-presence):** this is *literally* the band-presence predicate
> in miniature — reduce a domain to a min/max range, then set a bit per occupied band of a fixed-resolution
> spectrum. Our "which frequency/space bands are present" reduces to exactly `InterlockedMin/Max` for the
> range plus a `CreateFilledBitmask`-style range→bitmask. Steal this whole construction.

## A.3 Canonical wave-intrinsic compaction (web — the modern upgrade to A.1.c)

O3DE's `LightCulling.azsl` predates wave intrinsics and uses a group-shared atomic per survivor. The modern
idiom does **one atomic per *wave*** instead of per thread. Two authoritative references:

**MS D3D12 MeshletCull, `MeshletAS.hlsl`** — frustum (sphere-vs-6-planes) + normal-cone backface predicate,
then `WavePrefixCountBits` packs survivors into a payload, `WaveActiveCountBits` is the survivor count:
<https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletAS.hlsl>
```hlsl
bool visible = (dtid < MeshInfo.MeshletCount) && IsVisible(MeshletCullData[dtid], ...);
if (visible)
{
    uint index = WavePrefixCountBits(visible);   // my dense slot = #visible lanes before me
    s_Payload.MeshletIndices[index] = dtid;
}
uint visibleCount = WaveActiveCountBits(visible);
DispatchMesh(visibleCount, 1, 1, s_Payload);     // launch ONLY survivors (indirect-style)
```

**Compaction into a *global* survivor buffer** (Interplay of Light, "Stream compaction using wave
intrinsics") — per-wave prefix for the local slot, one `InterlockedAdd` per wave to reserve a contiguous
global range, broadcast the base:
<https://interplayoflight.wordpress.com/2022/12/25/stream-compaction-using-wave-intrinsics/>
```hlsl
uint local_index = WavePrefixCountBits(needs_ray);   // local slot
uint wave_count  = WaveActiveCountBits(needs_ray);    // survivors in this wave
uint base;
if (WaveIsFirstLane())
    InterlockedAdd(g_counter[0], wave_count, base);   // ONE atomic per wave
base = WaveReadLaneFirst(base);                       // broadcast
if (needs_ray)
    g_out[base + local_index] = payload;              // compacted global write
```

> **Maps to our broad-phase cull:** `A.1.c` (one atomic per survivor) is the *floor*; this wave form is the
> *ceiling*. For our high-survivor-rate scans, switch to per-wave `WavePrefixCountBits` + one
> `InterlockedAdd(count)` per wave to slash atomic contention. Same output, far less serialization.
> Verify wave-intrinsic support in our AZSL target (SM6.0+ / our RHI backend) before relying on it.

## A.4 O3DE Meshlets gem — explicit non-result

`Gems/Meshlets/Assets/Shaders/MeshletsCompute.azsl` (web, o3de/o3de development branch) has **no cull**.
Its own header says culling is unwritten:
```
// A future step should be to run a culling compute before this and generate
// a visibility list of the meshlets. ... hence saving most of the rasterization of culled meshlets.
```
CPU side is a plain direct dispatch, one group per meshlet, no compaction
(`Gems/Meshlets/Code/Source/Meshlets/MeshletsDispatchItem.cpp`):
```cpp
RHI::DispatchDirect dispatchArgs(meshletsAmount, 1, 1, MESHLETS_THREAD_GROUP_SIZE, 1, 1);
```
<https://github.com/o3de/o3de/tree/development/Gems/Meshlets> — **do not mine this for compaction; it has
none.** It is here only to close the loop on the original "MESHLETS" ask.

---

# BLUEPRINT B — DISCRIMINATION SCANS (uniform predicate over a huge domain → reduce / histogram)

Our discrimination scans are *predicate + reduce*: run one op over every element of a big buffer and reduce
(null histogram, range / space-frequency, band-presence). O3DE has all three reduction idioms in AZSL.

## B.1 Histogram idiom — `LuminanceHistogramGenerator.azsl` (predicate → bin → two-level atomic add)

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/LuminanceHistogramGenerator.azsl`

This is the textbook GPU histogram: group-shared bins, each thread classifies one element into a bin and
atomic-adds locally, then the bins are merged to the global histogram with one atomic per bin.

Declarations + clear (`:23-43`):
```hlsl
RWStructuredBuffer<uint> m_outputTexture;          // global histogram (note: SB not RWBuffer — Metal atomics)
groupshared uint shared_histogramBins[NUM_HISTOGRAM_BINS];

void ClearSharedMemory(uint groupIndex)
{
    if (groupIndex < NUM_HISTOGRAM_BINS) shared_histogramBins[groupIndex] = 0;
    GroupMemoryBarrierWithGroupSync();
}
```

Classify (the *predicate*) + local atomic, then merge to global (`:58-87`):
```hlsl
[numthreads(NUM_THREADS_PER_DIM, NUM_THREADS_PER_DIM, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID, uint groupIndex)
{
    ClearSharedMemory(groupIndex);

    const uint2 textureIndex = dispatchThreadID.xy;
    if (textureIndex.x < dim.x && textureIndex.y < dim.y)         // bounds = ragged-domain guard
    {
        const float ev  = NitsToEv100Luminance(CalculateLuminance(color.rgb, ...));
        const uint  bin = GetHistogramBinFromEv(ev);             // <-- the discrimination predicate
        uint orig; InterlockedAdd(shared_histogramBins[bin], 1, orig);   // local accumulate
    }
    GroupMemoryBarrierWithGroupSync();

    if (groupIndex < NUM_HISTOGRAM_BINS)
    {
        uint orig; InterlockedAdd(PassSrg::m_outputTexture[groupIndex], shared_histogramBins[groupIndex], orig);  // merge LDS -> global
    }
}
```
Note the binning function clamps out-of-range into the end bins (`GetHistogramBinFromEv`, `:51-56`,
`clamp(bin, 0, NUM_HISTOGRAM_BINS-1)`).

> **Maps to our discrimination scan (null histogram / band-presence):**
> - **`GetHistogramBinFromEv`** = our discrimination predicate. Swap "EV of a pixel" for "our per-element
>   classifier" (null vs non-null bucket; which space/frequency band; which range bucket).
> - **Two-level atomic add** (LDS bins → one global atomic per bin) = the contention-minimizing histogram
>   reduction. This *is* our predicate+reduce pass. One pass, no multi-dispatch needed if bins fit in LDS.
> - **The `< dim` bounds check** = our ragged-buffer / partial-tile guard. Do not skip it.
> - **`RWStructuredBuffer<uint>` not `RWBuffer`** (`:20-23` comment) = portability gotcha: typed buffers
>   become textures on Metal and break atomics. Use structured buffers for any atomic target in our AZSL.

## B.2 Tiled min/max reduction — `NewDepthOfFieldTileReduce.azsl` (range scan)

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/NewDepthOfFieldTileReduce.azsl`

A uniform op (read a value) reduced to a per-tile **min/max range** using group-shared atomic min/max. Two
transferable tricks: (1) the **float→uint monotonic remap** so integer `InterlockedMin/Max` works on floats,
and (2) **hierarchical reduce** — per-column atomics, then reduce the columns.

Float-as-uint atomic min/max (`:63-66`):
```hlsl
// For atomic min/max to work with uints, floating point values should be positive
// Map from [-1, 1] range to [0, 2] and cast as uint
InterlockedMin( LDS_MIN_COC[group_thread_id.x], asuint(cocMin + 1) );
InterlockedMax( LDS_MAX_COC[group_thread_id.x], asuint(cocMax + 1) );
```
Reduce-the-columns + single writer (`:76-91`):
```hlsl
GroupMemoryBarrierWithGroupSync();
InterlockedMin( LDS_MIN_COC[0], LDS_MIN_COC[group_thread_id.x] );   // collapse columns into [0]
InterlockedMax( LDS_MAX_COC[0], LDS_MAX_COC[group_thread_id.x] );
GroupMemoryBarrierWithGroupSync();
if(group_thread_id.x == 0)
{
    cocMin = asfloat(LDS_MIN_COC[0]) - 1;                           // undo the +1 remap
    cocMax = asfloat(LDS_MAX_COC[0]) - 1;
    PassSrg::m_minMaxCoC[group_id.xy] = float2(cocMin, cocMax);     // one write per tile
}
```
It also uses `GatherAlpha` to fetch 2×2 at once (`:59`) — 4 elements per memory op.

> **Maps to our discrimination scan (range / space-frequency):** our "what is the value range present in
> this region" is exactly this. The **float→uint monotonic remap** is mandatory for our float predicates
> if we use integer atomic min/max — and the remap must be order-preserving (this one only works because
> values are made positive first; for signed data use the standard sign-flip trick). Hierarchical
> "reduce columns then reduce [0]" is our two-stage reduction template.

## B.3 Single-pass multi-level reduction — `DownsampleSinglePassLuminance.azsl` (FidelityFX SPD)

`/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/DownsampleSinglePassLuminance.azsl`

The heavyweight idiom: AMD FidelityFX **Single Pass Downsampler** reduces a full image to *all* mip levels
in **one dispatch** using a `globallycoherent` buffer + a global atomic counter so the last finishing
group does the final reduction. This is the reference if our reduction tree is too deep for one group.

The custom reduce operator (here min/avg/max with a validity weight — note it carries *three* reductions at
once, `:137-147`):
```hlsl
AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3)
{
    const float weightSum = v0.w + v1.w + v2.w + v3.w;
    const float minValue  = min(MinWithFlag(v0, v1), MinWithFlag(v2, v3));
    const float avgValue  = weightSum > 0. ? (v0.y*v0.w + v1.y*v1.w + v2.y*v2.w + v3.y*v3.w)/weightSum : 0.;
    const float maxValue  = max(MaxWithFlag(v0, v1), MaxWithFlag(v2, v3));
    return AF4(minValue, avgValue, maxValue, weightSum * 0.25);
}
```
The single-pass cross-group sync via global atomic counter (`:86-89`):
```hlsl
void SpdIncreaseAtomicCounter(AU1) { InterlockedAdd(PassSrg::m_globalAtomic[0].m_counter, 1, s_spdCounter); }
```
Group-shared intermediates `s_spdIntermediate{R,G,B,A}[16][16]` (`:42-45`), `globallycoherent` working mip
(`:24-25`), driver just calls `SpdDownsample(...)` (`:151-161`). Upstream reference cited in-file
(`:9`): <https://github.com/GPUOpen-Effects/FidelityFX-SPD>

> **Maps to our discrimination scan (multi-pass reduce, fused):** when our reduction is deeper than one
> thread-group (huge buffers), SPD is the model: a custom `SpdReduce4` carrying **min+avg+max
> simultaneously** is exactly our "one pass, multiple statistics" need. The `globallycoherent` +
> last-group-finishes pattern fuses what would be a multi-dispatch reduction tree into one dispatch.
> Note `SpdReduce4` proves you can fuse several discrimination statistics into one reduce op — design our
> reduce to carry the whole feature vector, not one scalar.

## B.4 Why FastNoise / GradientSignal are NOT the scan reference

- **FastNoise** (`/opt/O3DE/25.10.2/Gems/FastNoise/`) is a thin O3DE wrapper around the **CPU** FastNoise
  C++ SIMD library (`Code/External/FastNoise/FastNoise.h`). The per-cell evaluation is
  `FN_DECIMAL GetNoise(x,y,z) const` (`FastNoise.h:182,205`) and the inner gradient/value coords
  (`GradCoord3D`, `ValCoord3DFast`, `:305-309`) — all `inline` host C++, **no `.azsl`, no compute**. The
  Gem exposes it per-sample via `GetValue` / `GetValues(span positions, span outValues)`
  (`FastNoiseGradientComponent.h:95-96`) — a CPU `EBus` query, front-end feed only.
- **GradientSignal** (`/opt/O3DE/25.10.2/Gems/GradientSignal/`) likewise evaluates gradients on the **CPU**
  (`PerlinImprovedNoise.h`, `RandomGradientComponent.h`, `ImageGradientComponent.h`). **No `.azsl`, no
  compute shader** in the Gem.

> **Takeaway:** the "uniform op over a huge domain" these Gems represent is real, but they run it on the
> CPU one sample at a time. The *GPU* expression of that same idea — run one predicate over a giant buffer
> and reduce — is the Atom Feature shaders in B.1–B.3. Port from those, not from FastNoise/GradientSignal.
> (Consistent with our own rule: Python/CPU is front-end feed only; engine math is GPU/C++ kernels.)

---

# Cross-cutting AZSL compute idioms (study list for the DI)

1. **Grid-stride candidate loop** — `for (i = groupIndex; i < N; i += GROUP_SIZE)` sweeps a buffer larger
   than the dispatch. (`LightCulling.azsl:305`)
2. **Group-shared atomic compaction** — `InterlockedAdd(counter,1,slot)`; the *return value* is the dense
   index. Clamp with `min(slot, MAX-1)` for bounded overflow. (`LightCulling.azsl:196-198`)
3. **Wave-intrinsic compaction** (modern) — `WavePrefixCountBits` for the local slot + one
   `InterlockedAdd` per wave for the global base. (MS `MeshletAS.hlsl`; Interplay of Light)
4. **Group-shared histogram** — LDS bins, classify→`InterlockedAdd` local, merge→one global atomic per bin.
   (`LuminanceHistogramGenerator.azsl:78,86`)
5. **Atomic min/max on floats** — `asuint`/`asfloat` with an order-preserving remap (`+1` to force
   positive). (`NewDepthOfFieldTileReduce.azsl:63-66`)
6. **Hierarchical reduce** — per-column LDS atomics → reduce columns into `[0]` → single writer.
   (`NewDepthOfFieldTileReduce.azsl:76-91`)
7. **Single-pass fused reduce** (SPD) — `globallycoherent` + global atomic counter, last group finishes;
   custom reduce op carrying multiple statistics. (`DownsampleSinglePassLuminance.azsl:86-89,137`)
8. **Band-presence bitmask** — value-range → run of set bits via `InterlockedMin/Max` + filled-bitmask.
   (`LightCullingTilePrepare.azsl:128-152`)
9. **Barrier discipline** — `GroupMemoryBarrierWithGroupSync` around every LDS phase; **double barrier**
   around a counter reset between passes. (`LightCulling.azsl:489-494`)
10. **Portability gotchas** — atomic targets must be `RWStructuredBuffer` not `RWBuffer` (Metal);
    `[[verbatim("globallycoherent")]]` for cross-group coherent buffers. (`LuminanceHistogramGenerator.azsl:20`; `DownsampleSinglePassLuminance.azsl:24`)

---

# Source index

### Local (O3DE 25.10.2 — read directly, cite file:line)
- `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/LightCulling/LightCulling.azsl` — **cull + compaction (A.1)**
- `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/LightCulling/LightCullingTilePrepare.azsl` — **band-presence bitmask (A.2)**
- `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/LuminanceHistogramGenerator.azsl` — **histogram (B.1)**
- `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/NewDepthOfFieldTileReduce.azsl` — **min/max range reduce (B.2)**
- `/opt/O3DE/25.10.2/Gems/Atom/Feature/Common/Assets/Shaders/PostProcessing/DownsampleSinglePassLuminance.azsl` — **single-pass fused reduce / SPD (B.3)**
- `/opt/O3DE/25.10.2/Gems/FastNoise/Code/External/FastNoise/FastNoise.h` — CPU-only (B.4, non-result)
- `/opt/O3DE/25.10.2/Gems/FastNoise/Code/Source/FastNoiseGradientComponent.h` — CPU EBus feed (B.4)
- `/opt/O3DE/25.10.2/Gems/GradientSignal/Code/Include/GradientSignal/...` — CPU-only, no AZSL (B.4)

### Web
- MS D3D12 MeshletCull (`MeshletAS.hlsl`) — frustum+cone predicate, wave-prefix compaction: <https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/MeshletCull/MeshletAS.hlsl>
- Interplay of Light — stream compaction with wave intrinsics: <https://interplayoflight.wordpress.com/2022/12/25/stream-compaction-using-wave-intrinsics/>
- O3DE Meshlets gem (no cull shader — non-result): <https://github.com/o3de/o3de/tree/development/Gems/Meshlets>
- AMD FidelityFX SPD (upstream of B.3): <https://github.com/GPUOpen-Effects/FidelityFX-SPD>
- Alex Tardif — histogram luminance (HLSL): <https://alextardif.com/HistogramLuminance.html>
- graphicsdemoskeleton — tree parallel reduction (HLSL): <https://github.com/speps/graphicsdemoskeleton/blob/master/04_DirectCompute%20Parallel%20Reduction%20Case%20Study/02_ParallelReduction/ParallelReduction.hlsl>
