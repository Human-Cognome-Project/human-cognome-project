# Triple Scene Pipeline Plan

**Date**: 2026-03-04
**Author**: engine-specialist
**Status**: COMPLETED — implemented in commit c7b9cb6 (2026-03-04). `RunPhaseCascade` retained for rollback.
**Goal**: Eliminate GPU idle gaps between resolution phases (~50% of current wall time)

---

## 1. Current State

### Call chain

```
BedManager::Resolve()
  └── ResolveLengthCycle()          per word length (descending)
        ├── RunPhaseCascade()        Label pass (cap runs only)
        └── RunPhaseCascade()        common pass (all runs)
              └── ResolvePhase()     per RC_VOCAB_PER_PHASE (2000) vocab slice
                    ├── LoadWorkspaceBatch()   CPU: memcpy vocab + streams → GPU buffer
                    ├── ws->BeginSimulate()    GPU: dispatch — non-blocking
                    └── DrainWorkspace()       FetchSimResults() — BLOCKS + CPU collect
```

### What's already overlapped

Within a single `ResolvePhase` call, the loop already overlaps loading workspace B (CPU) with workspace A simulating (GPU):

```
Load ws_a → BeginSimulate(ws_a)   ← GPU starts
Load ws_b → BeginSimulate(ws_b)   ← CPU overlaps with ws_a GPU
DrainWorkspace(ws_a)              ← FetchSimResults + collect
DrainWorkspace(ws_b)              ← FetchSimResults + collect
```

Source: `HCPVocabBed.cpp:1183-1210`.

### Where the stall is

Between phase N and phase N+1 inside `RunPhaseCascade` (`HCPVocabBed.cpp:1299-1311`):

```
// Phase N:
ResolvePhase(...)        // GPU work + blocking drain
// <<< GPU IDLE STARTS HERE >>>
BuildVocabSliceFromEntries(wordLength, filteredVocab, start+N, RC_VOCAB_PER_PHASE)
// <<< GPU IDLE UNTIL LoadWorkspaceBatch in next ResolvePhase >>>
// Phase N+1:
ResolvePhase(...)
```

`BuildVocabSliceFromEntries` at 2000 entries × 8-char avg ≈ 16K particles: positions array fill, tier lookup build, entry copy. Pure CPU. Not huge, but it runs while the GPU sits idle, and it repeats for every phase in every length cycle.

The bigger stall: `DrainWorkspace` → `FetchSimResults()` blocks the calling thread until the scene's `fetchResults()` completes. During that CPU-side collection (CheckSettlement, CollectSplit, ResetDynamics), the next workspace's GPU work is already done (since BeginSimulate was kicked earlier), but the NEXT phase's GPU work hasn't started yet. The drain of phase N and the load of phase N+1 are both serial CPU operations with GPU idle.

### Timing context

From known benchmark data (A Study in Scarlet, 46,778 runs, GTX 1070):
- Total: 133s wall time
- Phase 1: 2.4s
- Phase 2: 115s (the BedManager resolve loop)
- Dracula: 145s Phase 2

Estimated GPU utilization: ~50% based on MEMORY.md. The other ~50% is CPU serial work with GPU idle.

---

## 2. What Exists (infrastructure already built)

Each `Workspace` already owns a dedicated `PxScene` (`m_scene`, `m_ownsScene = true`). This is the critical prerequisite — per-workspace scenes allow independent simulate/fetch without scene contention.

Async API on `Workspace`:
- `BeginSimulate(steps, dt)` — calls `m_scene->simulate()` once, returns immediately. GPU dispatched.
- `IsSimDone()` — non-blocking poll (checks scene simulate status)
- `FetchSimResults()` — blocks until scene `fetchResults()` completes

Source: `HCPVocabBed.h:133-141`.

The infrastructure for pipelining EXISTS. The orchestration loop is still serial.

---

## 3. Proposed Solution: Work-Queue State Machine

### Core idea

Replace the sequential phase loop with a work-queue dispatcher that keeps N workspaces rotating through three stages simultaneously:

```
Stage::Loading    — CPU: BuildVocabSlice + LoadWorkspaceBatch + BeginSimulate
Stage::Simulating — GPU: scene running; CPU free for next stage's prep
Stage::Draining   — CPU: FetchSimResults + CheckSettlement + CollectSplit + ResetDynamics
```

With 3 workspaces in the pool, at any point in time:
- ws_A: Draining (collecting results from last GPU run)
- ws_B: Simulating (GPU working)
- ws_C: Loading (CPU preparing next vocab slice)

This fills the GPU idle gap: ws_C loads while ws_B GPU is running, so by the time ws_B finishes and ws_C begins simulation, the GPU transitions immediately from one job to the next.

### Pipeline diagram

```
Time →
ws_A: [Load][Sim][Drain][Load][Sim][Drain]...
ws_B:      [Load][Sim][Drain][Load][Sim][Drain]...
ws_C:           [Load][Sim][Drain][Load][Sim][Drain]...
                      ^--- GPU never idles here ---^
```

### Work item queue

A work item represents one batch of runs against one vocab slice:

```cpp
struct PhaseWorkItem
{
    AZ::u32                          wordLength;
    AZ::u32                          phaseIndex;
    VocabPack                        pack;        // pre-built on CPU
    AZStd::vector<AZ::u32>           runIndices;  // which CharRuns to resolve
};
```

The queue is built upfront or lazily as phases are enumerated. For lazy: queue next item while current is simulating.

### Workspace state tracking

```cpp
enum class WorkspaceStage { Idle, Simulating, AwaitingDrain };

struct WorkspaceSlot
{
    Workspace*    ws;
    WorkspaceStage stage = WorkspaceStage::Idle;
    AZ::u32       phaseIndex = 0;
    const VocabPack* pack = nullptr;    // pointer into queue item
};
```

### New orchestrator loop (sketch)

Replaces `RunPhaseCascade`. The outer loop structure:

```
while (queue has work OR any slot is Simulating/AwaitingDrain):

    // Drain any finished simulations
    for each slot in Simulating:
        if slot.ws->IsSimDone():
            FetchSimResults, CheckSettlement, CollectSplit
            slot.stage = Idle

    // Load idle slots with next work items
    for each slot in Idle:
        if queue not empty:
            item = queue.pop_front()
            LoadWorkspaceBatch(slot.ws, item)
            slot.ws->BeginSimulate(RC_SETTLE_STEPS, RC_DT)
            slot.stage = Simulating

    // If nothing is simulating and queue is empty: done
    // Otherwise spin (with short sleep or yield to avoid busy-wait)
```

The check `IsSimDone()` is already non-blocking. The loop spins polling all Simulating slots, draining each as it completes, and refilling the slot immediately.

---

## 4. Implementation Changes

### 4a. Increase workspace pool size

`WS_PRIMARY_COUNT` (currently 2) needs to be 3 to enable full triple buffering. With only 2 workspaces, you get double-buffering (Load overlaps Sim, but Drain is still serial with the next Load). Going from 2 to 3 fills the remaining gap.

**Change**: `HCPVocabBed.h` line 35:
```cpp
static constexpr AZ::u32 WS_PRIMARY_COUNT = 3;  // was 2
```

Extended workspaces (`WS_EXTENDED_COUNT = 2`) handle lengths 11-20+, which are sparse. Triple-buffering them provides diminishing returns, but raising to 3 is cheap. Decision deferred.

### 4b. New method: `BedManager::RunPipelinedCascade`

Add new method alongside `RunPhaseCascade`. Does NOT remove `RunPhaseCascade` initially — keep old path for debugging/fallback, swap call site in `ResolveLengthCycle` when new path is proven.

New signature:
```cpp
void RunPipelinedCascade(
    AZ::u32 wordLength,
    const AZStd::vector<CharRun>& runs,
    const AZStd::vector<VocabPack::Entry>& filteredVocab,
    AZStd::vector<AZ::u32>& currentIndices,
    AZStd::vector<ResolutionResult>& results,
    AZ::u32& phaseIndex);
```

Same interface as `RunPhaseCascade` — drop-in replacement at the two call sites in `ResolveLengthCycle`.

### 4c. Pre-build VocabPacks before GPU starts

`BuildVocabSliceFromEntries` is pure CPU/memory. Build the next N packs (where N = workspace pool size) before starting the pipeline loop, then refill the queue one item ahead as each is consumed. This eliminates VocabPack build time from the critical path.

### 4d. `LoadWorkspaceBatch` extraction

Currently the batch-loading logic is inlined inside `ResolvePhase`. Extract it into a named helper that takes a workspace, a VocabPack, a run index slice, and an offset — returns overflow count. This lets the pipeline loop call it directly.

Existing helper `LoadWorkspaceBatch` is already a static local in the .cpp (see the call at `HCPVocabBed.cpp:1189`). Just needs to be promoted or callable from the new orchestrator.

### 4e. Result ordering

`ResolutionResult` carries `r.runIndex` (the original CharRun index). Results from pipelined phases will arrive out of phase order. The manifest assembler (`ScanManifestToPBM`) sorts by `runIndex` anyway, so ordering within Resolve() doesn't matter. Confirm: `ResolutionManifest.results` is NOT assumed sorted anywhere in `ScanManifestToPBM` or `HCPSocketServer.cpp` before the manifest is passed to the scanner.

**Action**: Verify this assumption before implementing. If `results` is assumed in-order somewhere, add a sort by `runIndex` at the end of `Resolve()`.

### 4f. Overflow handling

The current `ResolvePhase` inner while-loop handles runs that don't fit into the workspace capacity in one batch (overflow). The pipeline loop must preserve this: if a work item's run batch overflows a workspace, the overflow runs go back into the queue as a new work item with the same phase/pack.

---

## 5. Risks and Open Questions

### PhysX scene thread safety

`PxScene::simulate()` is not thread-safe to call concurrently on different scenes from the same `PxPhysics` object unless PhysX was initialized with `PxSceneFlag::eENABLE_GPU_DYNAMICS` and proper task dispatcher. The current implementation uses a single-threaded task dispatcher.

**Question**: Can `scene_A->simulate()` be called while `scene_B->fetchResults()` is in flight, given both scenes share the same CUDA context? PhysX documentation says scenes are independent, but CUDA context sharing adds a constraint: only one thread may own the CUDA context at a time.

**Likely answer**: Since all GPU work goes through a single CUDA context (`PxCudaContextManager`), `simulate()` dispatches GPU work asynchronously but `fetchResults()` may internally acquire the CUDA context. Need to verify whether PhysX serializes CUDA context access internally or expects the caller to do it.

**Mitigation if problematic**: Use a single polling thread that cycles through `IsSimDone()` calls and serializes `FetchSimResults()` calls while the main thread handles loading. This preserves the pipelining benefit (VocabPack build + LoadWorkspaceBatch overlapping GPU) even if FetchSimResults must be serialized.

### Memory budget

Adding a 3rd primary workspace adds 131,072 × (positions float4 + phases u32 + velocity float4) particles to GPU memory:
- Per workspace: ~131K particles × 36 bytes ≈ 4.7MB VRAM
- 3rd workspace adds ~4.7MB

GTX 1070 has 8GB VRAM. Current 4 workspaces (2 primary + 2 extended) use ~18.8MB. Adding one more is negligible.

### Result accumulation and phase index assignment

`DrainWorkspace` sets `r.tierResolved = phaseIndex`. With pipelined phases, `phaseIndex` must be captured at the time the work item is created (before dispatch), not at drain time. Current code passes `phaseIndex` as a parameter to `DrainWorkspace`. The pipeline loop must bind phase index to the work slot at load time.

### Spin loop latency

The poll loop calling `IsSimDone()` on all simulating workspaces burns CPU cycles. For 115s runs this is acceptable (the CPU was idle anyway). For potential future multi-text batch processing, a condition variable wakeup would be preferable. Not a blocker for this implementation.

### Regression testing

No automated test suite currently. Before and after timing on the same texts (A Study in Scarlet, Dracula) is the primary correctness check. Resolution rate must not regress from known baselines (97.5% and 98.5% respectively). Resolution counts are deterministic for fixed vocab/text.

---

## 6. Out of Scope (this plan)

- Async std::thread for VocabPack pre-build (pure CPU overlap is sufficient gain; threading adds complexity)
- Extended workspace triple-buffering (lengths 11-20+ are sparse, not a bottleneck)
- Multi-text batch processing (scheduling multiple Resolve() calls through the same pipeline)
- LMDB prefetch during simulation (worth doing later; LMDB reads are mmap'd so page faults, not syscalls; current bottleneck is GPU/CPU sync, not LMDB)

---

## 7. Expected Outcome

Current Phase 2 (115-145s) estimated breakdown:
- GPU simulation time: ~50-60s (actual particle settling)
- CPU serial overhead (VocabPack builds, DrainWorkspace, between-phase gaps): ~55-85s

Pipelining eliminates CPU serial overhead from the critical path. Target: Phase 2 time approaches GPU-only bound, estimated 55-70s for the same texts — roughly 2× speedup on Phase 2.

---

## 8. Implementation Order

1. **Verify result ordering assumption** (read ScanManifestToPBM callers, confirm runIndex sort or add it)
2. **Extract `LoadWorkspaceBatch` as a callable helper** (refactor from inline static)
3. **Add `WorkspaceStage` enum and slot state tracking** to BedManager (header only)
4. **Implement `RunPipelinedCascade`** alongside old `RunPhaseCascade`
5. **Raise `WS_PRIMARY_COUNT` to 3** (one-line change, allocates 3rd workspace at startup)
6. **Swap call sites** in `ResolveLengthCycle` to use `RunPipelinedCascade`
7. **Benchmark** on A Study in Scarlet and Dracula, compare resolution rates and wall times
8. **Remove `RunPhaseCascade`** once new path is verified

---

## 9. Files to Modify

| File | Change |
|------|--------|
| `HCPVocabBed.h` | Add `WorkspaceStage` enum; `WS_PRIMARY_COUNT = 3`; declare `RunPipelinedCascade` |
| `HCPVocabBed.cpp` | Extract `LoadWorkspaceBatch`; implement `RunPipelinedCascade`; swap call sites in `ResolveLengthCycle` |

No other files require changes. `HCPEngineSystemComponent`, `HCPSocketServer`, and `ScanManifestToPBM` are unaffected — the `Resolve()` interface is unchanged.
