> **Recovered September 2026 record.** This document preserves the native-engine development/vetting context in which it was written. Absolute `/opt/project/...` paths, agent-routing instructions, and statements about branch/commit status are historical. Current paths/status are indexed in [README.md](README.md); current repository policy is in the root `AGENTS.md` and `CONTRIBUTING.md`.

# Vetting plan — workspace claims to verify against live code

Premise (Patrick, 2026-09-13): **none of the current workspace can be fully
trusted until vetted.** The docs in this folder are a point-in-time map, not
the territory. This plan lists the load-bearing claims, ordered so the ones the
harness architecture rests on are confirmed before anything is built on them.

Rules this plan honours:
- I (the orchestrator) examine only documents in `/opt/project/repo/engine`.
- Verification requires reading the engine source (`/opt/project/taichi`) and
  the workspace code (`src/`, `apps/`, `tests/`, `docs/api-reference/*.cpp`) —
  that is **agent work**, dispatched read-only. No code is modified.
- This is an **updated/forked TaiChi (64-bit addressing, larger SNode
  structures)**, so cited line numbers are *hints*; the fork may have moved
  them. Verify the **behaviour and the code**, not the line number.
- Each item carries a status: `[unvetted]`, `[vetted <date>]` (with what was
  checked), or `[refuted <date>]` (with the discrepancy).

## Priority 0 — the premise itself

- **P0.1** `[unvetted]` This is genuinely an updated TaiChi with **64-bit
  addressing** enabling **much larger SNode structures** than stock. Check:
  index/pointer widths in the SNode / runtime path (are offsets/addresses
  64-bit?), and whether capacity limits are widened accordingly. Everything
  depends on this being real, not stock 32-bit.

## Priority 1 — the rate-of-change seam (the whole harness design rests here)

- **P1.1** `[unvetted]` Capacities (`llvm_snode_capacity`,
  `llvm_snode_tree_capacity`) are **frozen at instantiation** — `CompileConfig`
  copied into `Program` at construction, `fit()` called, global restored.
  (Docs cite `program.cpp:75`, `compile_config.*`.) Confirm a live instance
  cannot change them.
- **P1.2** `[unvetted]` **Only a layout change forces recompilation.** A loop
  bound, a scalar, or an array can be a **launch argument**
  (`create_arg_load`, `create_ndarray_arg_load`), so one compiled kernel serves
  many launches. This is what lets a **force-activation flag** be a runtime
  argument with no recompile. Confirm.
- **P1.3** `[unvetted]` **Kernel name is the compilation key** — two kernels
  with the same name share one compilation; the second silently runs the
  first's code. Confirm the keying is by name.
- **P1.4** `[unvetted]` **SNode reconstruction = new layout = rebuild.** Adding
  / changing an SNode tree on a live runtime vs requiring a fresh instance:
  establish exactly what a "virtual node promoted into the model" costs
  mechanically (new tree on live runtime? or full instance restart?). This
  defines the harness's shut-down/restart responsibility.

## Priority 2 — abort/corruption constraints (get wrong → run dies)

- **P2.1** `[unvetted]` Outermost loop must carry a thread count
  (`runtime.cpu_threads()` / `cpu_max_num_threads`), or launch aborts in
  `threading.cpp` (`desired_num_threads > 0`).
- **P2.2** `[unvetted]` `TI_LIB_DIR` mandatory; engine aborts naming it if
  unset. Resolves `runtime_x64.bc`, `runtime_cuda.bc`, `slim_libdevice.10.bc`.
- **P2.3** `[vetted 2026-09-13]` `SNode::counter` is a **process-wide monotonic
  static** — identifiers rise past destroyed layouts, so capacity bounds
  identifiers issued over process life, not live objects. Confirmed via
  `vet-envelope` (`IMPLEMENTATION-STATUS.md:109`,
  `INSTALL-CONFIGURATION.md:55-61`). **Design consequence:** repeated
  virtual-node promotion consumes SNode capacity monotonically → the harness
  budgets a generous capacity up front, or uses its restart authority to
  reclaim the id space. (`reset_counter()` safe only with nothing live.)
- **P2.4** `[unvetted]` Runtime ABI version = 1; offline cache off by default
  (kernels recompiled in-process each run).

## Priority 3 — device envelope (numbers, re-measure not re-read)

- **P3.1** `[unvetted]` 1070 usable, 750 Ti deferred (first field alloc refused,
  `CUDA_ERROR_NOT_SUPPORTED` in `cuMemAllocAsync`). CUDA binds visible ordinal
  zero, no device-index setting.
- **P3.2** `[unvetted]` Storage = 4 bytes per i32 cell; 32 MiB reservation
  granularity; f64 ≈ 4×–27× f32 by boundedness. These are measured figures —
  re-run `build/engine_devices` on the host rather than trusting the doc; needs
  the actual hardware, so flag for a host run, not a source read.

## Priority 4 — the model surface (forces, ledger, virtual nodes)

- **P4.1** `[unvetted]` What the **forces** actually are, and whether the
  `field/*.json` control configs (`ctl-off-excl`, `off-pull`, etc., outside my
  scope) are the force-activation surface. Commit text names "identity pull,
  exclusion, condensation." Patrick does not know the set — this is discovery.
- **P4.2** `[unvetted]` What "the database / ledger of known relationships" is
  in this workspace — a live DB, the SNode structure, or the `field/` +
  `ledger/` artifacts. Establishes where known vs uncalibrated dimensions live.

Note: P4 is **discovery**, not confirmation — findings feed
`HARNESS-NOTES.md`, and anything structural gets confirmed with Patrick before
it is treated as settled (no single locus of truth).

---

## Vetting results — 2026-09-13 (fork `vet-seam-2`, Opus 4.8, read-only, live `/opt/project/taichi`)

- **P0.1 — REFUTED as stated.** Logical indices / element counts are still
  **32-bit**: `runtime.cpp:280` `PhysicalCoordinates { i32 val[...] }`;
  `ListManager` (`runtime.cpp:419-454`) `num_elements` is `i32`,
  `reserve_new_element()` returns `i32` via `atomic_add_i32`. Only byte sizes
  and host directory pointers are 64-bit (ordinary host-pointer width, not a
  widening of the logical/index space). The real "larger structures"
  mechanism is **configurable startup capacities + a paged `ListManager`
  directory** (`runtime.cpp:421`), NOT 64-bit indexing. The fork's own
  `modernization/IMPLEMENTATION-STATUS.md` states this: :37 "Do not imply
  completed 64-bit indexing migration", :112 "logical indices are not globally
  widened." **RECONCILED (Patrick, 2026-09-13):** the intent behind "64-bit
  addressing" was to **lift the default 1024 SNode capacity ceiling and use
  modern cards fully**; configurable capacities meet that, so 64-bit indexing
  is NOT required for the MVP. Target run hardware: **RTX 3060** on another
  machine once stable; the 1070 is local dev. Open confirmation dispatched
  (fork `vet-envelope`): does the fork genuinely lift the 1024 ceiling and
  support 3060-scale use.
- **P1.1 — CONFIRMED.** `program.cpp:74-75` copies `default_compile_config`
  into the instance's own config at construction; capacities are plain `int`
  (`compile_config.h:64-65`, consistent with no widening). A live instance is
  not reached by editing the global afterward.
- **P1.2 — CONFIRMED.** `ir_builder.h:148/153/249/251` — `create_arg_load`,
  `create_ndarray_arg_load`, `create_global_ptr`, `create_external_ptr`.
  Scalars/arrays/bounds are runtime args; only a layout (SNode-tree) change
  needs recompilation. ⇒ a **force-activation flag can be a scalar launch arg,
  no recompile.**
- **P1.3 — CONFIRMED** (IRBuilder / non-AST kernels).
  `kernel_compilation_manager.cpp:186-187` — for `!ir_is_ast()` the cache key
  IS `get_name()`; same name ⇒ shared compilation ⇒ second silently reuses the
  first's code. (AST/Python kernels hash the AST instead, :189.)
- **P1.4 — CONFIRMED, with nuance.** `program.cpp:238-247` `add_snode_tree`
  materializes a **new tree onto the LIVE program** — no fresh instance needed
  to add a tree. Tree **ids are recycled**, not strictly monotonic
  (`destroy_snode_tree` frees the id `:235`; `allocate_snode_tree_id` pops the
  free list `:559-565`). A full **instance restart is required only to change
  the startup capacities** (P1.1), not to add/replace a tree. ⇒ "SNode
  reconstruction" for virtual-node promotion can be a **live `add_snode_tree`**,
  and the harness's shut-down/restart is reserved for **capacity** changes.
  Caveat: the separate per-node `SNode` id budget is described as
  monotonic-including-destroyed (`IMPLEMENTATION-STATUS.md:109`) but was NOT
  source-verified — remains P2.3, still `[unvetted]`.

Next, per the fork's pointer: `/opt/project/taichi/modernization/` holds
`IMPLEMENTATION-STATUS.md`, `PROJECT-PLAN.md`,
`independent-code-review-2026-09-09.md` — the authoritative record of what the
fork actually changed. Worth a read-only investigation pass to pin the real
capability envelope before design proceeds.

---

## Envelope results — 2026-09-13 (fork `vet-envelope`, Opus 4.8, read-only)

- **Q1 — 1024 ceiling liftable via config: CONFIRMED, honoured end-to-end.**
  Hard-baked count constants are GONE (no `taichi_max_num_snodes` /
  `kMaxNumSnodeTreesLlvm` in `taichi/`). Capacity sourced from config at every
  gate: `llvm_runtime_executor.cpp:39-42` stores + validates `>0`; tables sized
  at `:714`, `:748`; bound checks use the configured value at
  `struct_llvm.cpp:254-262` and at load via **global** ids `:401-413` (closes
  an earlier within-one-tree OOB gap). Settable via env /
  `ti.init` / native `CompileConfig` (`export_lang.cpp:196-198`). End-to-end
  probe: capacity 2048 loads a 1,025-field AOT and runs; 1024 rejects; 0
  rejects (`IMPLEMENTATION-STATUS.md:125-138`). Practical max effectively
  memory-unbounded (`int` capacity; 24 B/SNode + 16 B/tree slot).
- **Q2 — paged ListManager: CONFIRMED implemented (not aspirational).**
  `runtime.cpp:419-476` — 2 KiB root directory + lazy 4 KiB pages; Pascal-safe
  publication via integer-atomic RMW + fence. Native size 1,048,616 B →
  2,088 B.
- **Q3 — no hard ceiling binds before card memory (dense i32/f32).** The
  per-container `num_elements` i32 (~2.1B) is the **sparse iteration-list**
  path, bypassed by fully-dense trees. Logical coords i32 → ~2.1B cells/axis ≈
  8.6 GB @ 4 B, so the index ceiling and a 12 GB card's memory bind at ~the
  same scale. Millions × hundreds of slots is far below. Other i32 counters
  bind only on the sparse allocator path, which the dense layout avoids.
- **Q4 — bottom line: SUFFICE-WITH-CONFIGURATION for the MVP intent.**
  Validated on LLVM CPU + GTX 1070 CUDA (target class). Non-blocking caveats:
  (1) **SNode ids monotonic across destroyed layouts** within a Program
  lifetime → budget capacity or restart (`IMPLEMENTATION-STATUS.md:109`,
  `INSTALL-CONFIGURATION.md:55-61`); (2) sparse metadata not fully reclaimed on
  tree destroy; (3) logical indices 32-bit, not widened; (4) only LLVM-CPU +
  CUDA qualified (no AMDGPU/Vulkan/Metal/DX12).
- **Q5 — build-vs-source staleness: RESOLVED — NOT stale.** The linked build
  (2026-09-10 17:54) post-dates every engine source file (newest 17:46);
  `find taichi -newer …libtaichi_core_static.a` is empty. The modernized
  capacity/ABI features are compiled into the linked component archives
  (`libllvm_codegen.a`, `libllvm_runtime.a` carry the capacity-check + ABI
  strings; absent from `libtaichi_core_static.a` only because it isn't
  self-contained). Source is a **dirty, uncommitted** working tree
  (modernization edits to `struct_llvm.cpp`, `constants.h`, `compile_config.*`,
  `llvm_runtime_executor.*`, `runtime.cpp`, etc.) but those mtimes precede the
  build and are already built — a clean rebuild reproduces the same features.
  No doc calls for a newer engine binary (the docs' "rebuild" refers to the
  *model's* runtime SNode-tree recomposition, not the engine). The one
  post-build patch (`hcp-handoff/unit-mass.patch`, 2026-09-11) targets the
  **Python model/field layer**, not the engine C++.

**⇒ P0.1 fully resolved.** The current setup meets the MVP intent AND the
linked build faithfully reflects the modernized source.
**Remediation path status:** the fallback ("theoretically updated source →
more-correct engine buildable") is **not supported by un-built newer source** —
the current source is already what is built. A genuinely "more correct" engine
would need *new* source changes, not a rebuild of existing files.
**Hygiene flag (outside my scope — refs owned by another account, not mine to
act on):** the engine modernization lives in an **uncommitted** working tree in
`/opt/project/taichi`; it should be committed so it cannot be lost.

---

## Doc-accuracy audit — 2026-09-13 (fork `vet-docs`, Opus 4.8, read-only)

All five workspace docs verified against live source + the workspace
CMake/wrapper. **Verdict: substantively accurate — every behavioural claim, the
config surface, defaults, calling convention, and abort/corruption constraints
check out; no misleading claims.** The three `docs/api-reference/*.cpp` copies
are `diff`-identical to their cited engine origins; the "stale" mark on
`run_snode.cpp` is a runtime claim (needs a host run) but its conservative
caveat is safe.

Corrections applied by the parent (line-number drift only, none misleading):
- `CONFIGURATION.md`: `lang_util.cpp:30`→`:35`; `llvm_runtime_executor.cpp:41`→
  `:43` (positive-capacity error string); `:712`→`:713`
  (`runtime_get_memory_requirements` call).
- `DEVICES.md`: `cuda_context.cpp:50`→`:39`
  (`CU_DEVICE_ATTRIBUTE_MEMORY_POOLS_SUPPORTED`).

Structural DEVICES claims vetted ACCURATE (**P3.1 structural = vetted**): binds
visible ordinal zero (`cuda_context.cpp:22`), no device-index setting,
`TI_VISIBLE_DEVICE` Vulkan-only (`:111`). **P3.2 and all DEVICES numeric
measurements remain `[unvetted]` — UNVERIFIABLE-NEEDS-HOST-RUN**; re-run
`build/engine_devices` on real hardware to confirm rather than trusting the
figures. Not chasing now (target is a 3060 on another machine; 1070 numbers are
dev reference).
