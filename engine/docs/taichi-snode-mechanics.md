# Taichi SNode mechanics — reference for the warm-cache / harness translation

Status date: 2026-09-28. Written to ground the warm-cache SNode-tree design in
how Taichi's `SNode` structures actually work, using the software's own terms.
Every claim is cited to live source. Paths are under `engine/taichi/` unless
marked `[matched tree]`, which means `/opt/project/taichi` (the machine's build
tree). Canadian English.

This is engine-mechanics reference, not the model. The model record is
`HARNESS-NOTES.md` / `OPERATIONAL-PLAN.md`; the warm-cache data protocol is
`kernels/database/NOTES.md`.

## Taichi is a particle-based simulation system by design

Taichi is built for physical simulation, including particle methods. A
field over an `SNode` tree *is* the native way to represent a particle set;
particles are not a concept bolted onto a generic array runtime. The spatially
sparse `SNode` containers exist precisely so a grid/particle system materializes
only what is active (`docs/lang/articles/basic/sparse.md:43-55`). So indexing a
field as a set of particles is the engine used as intended.

## SNode anatomy

The `SNode` types are enumerated in `taichi/inc/snodes.inc.h`:
`root, dense, dynamic, pointer, bitmasked, hash, place, bit_struct, quant_array,
undefined`.

- **root** — the tree top; handed to `Program::add_snode_tree`
  (`engine/tests/engine_smoke_test.cpp:26-31`).
- **dense** — a contiguous array of cells, "`std::array<Cell, N>`", always
  present (`docs/design/llvm_sparse_runtime.md:30`). Not a sparse structure on
  its own (`docs/lang/articles/basic/sparse.md:55`).
- **pointer** — "`std::array<Cell*, N>`"; it "dynamically allocates memory only
  for activated cells and recycles it back into a memory pool once the cell is
  deactivated" (`docs/design/llvm_sparse_runtime.md:52`). Inactive cells
  dereference a shared `ambient_elements` block and read the default value
  (`:54`).
- **bitmasked** — dense storage plus "1-bit per pixel data to represent the
  pixel activity"; "like dense SNodes with auxiliary activity values"
  (`docs/lang/articles/basic/sparse.md:133,165`).
- **dynamic** — a variable-length list under a cell; LLVM CPU/CUDA only
  (`sparse.md:188`), one axis which must be the last (`:190`), and it must be
  directly placed with a field (`:192`).
- **hash** — a hashed sparse container; declared in the type list, **not
  exercised in our workspace** (untested here — flag).
- **place** — the leaf; it carries a data type `dt` (`taichi/ir/snode.h:101`;
  `is_place()` returns `type == place`, `snode.cpp:155`).

Which types are sparse (need runtime activation): `need_activation()` returns
true for `pointer || hash || bitmasked || dynamic` (`snode.cpp:268-271`).

**Assembly** (the built pattern, `engine/tests/engine_smoke_test.cpp:25-33`):
build a `root`, add a container with `root->dense(Axis(0), n)` (or
`root->pointer(...)`), add the leaf with `cell.insert_children(SNodeType::place)`
and set `leaf.dt`, then register the whole tree with
`runtime.program().add_snode_tree(std::move(root), /*compile_only=*/false)`
(`Program::add_snode_tree`, `taichi/program/program.cpp:238`).

## Identity mapping (in real Taichi terms)

Patrick's "particle id" and "SNode_id" map onto two distinct things:

- **The index into a container `SNode` is the particle id.** A container of `N`
  cells is addressed by its physical index; an instance is `(SNode, index)`.
  There is no separate scalar "particle id" primitive — the index is it
  (`snode.h:81` `physical_index_position[]`, `:97,307` `num_cells_per_container`).
- **`SNode::id`** is the id of the *structural node* (the schema element), not an
  instance: `id = counter++` from a process-global `std::atomic<int> counter`
  (`snode.cpp:12,220`).
- **`snode_tree_id_`** identifies the *tree*, separate from the node id
  (`snode.h:342,353`).

Each cell's slot is one of two things, and that is what says what the index
represents:

- a **`place`** leaf → a **direct-value slot** (the particle's own value);
- a **`pointer`** cell → a **reference to a nested tree** (its child block),
  which is **null when the cell is inactive** — the `0x` / unused state
  (`llvm_sparse_runtime.md:52-54`).

A container `SNode` holds its children in `std::vector<std::unique_ptr<SNode>>
ch` (`snode.h:76`) — that is the schema-level "reference to nested SNode ids".

## Load-bearing verdict: shared subtree is a schema/DB property, not physical storage

**There is no Taichi mechanism to make one physical child subtree be shared by
multiple parent cells.** Each active cell owns its own child storage. The reason
is structural: `insert_children` builds **one** child `SNode` whose
`num_elements_from_root` is the **product** of the shapes down the path
(`snode.cpp:14-23`, the `num_elements_from_root *=` loop; also `:83`). So
`root.dense(16).dense(16)` is a handful of `SNode` *objects* but describes **256
storage cells**, and `dense` cells are always present
(`llvm_sparse_runtime.md:30`).

"Stored once, attached by read, not a dupe" is therefore real at two levels and
absent at a third:

1. **SNode schema / id level — genuinely once.** A child subtree is one
   structural definition reused as the child-shape for every parent cell. You
   spend a handful of `SNode::id`s, not one per leaf — the "33 pieces, not 256"
   structural economy holds here.
2. **Warm-cache / DB level — genuinely once.** The single source of truth for a
   reused element's *definition and membership* lives in the warm cache / DB
   layer. The engine stores per-cell *values*, not a shared "definition" object.
3. **Physical storage — per cell, not shared.** When a study materializes the
   tree, each active cell gets its own child storage.

Physical leanness therefore comes from **sparsity, not from a shared reference**:
use `pointer` (or `bitmasked` where per-cell activity matters) so dormant /
below-LoD cells cost ~nothing until touched (`sparse.md:52-54,133`), backed by
the paged `ListManager` (below). `dense` is for an always-materialized floor.

**Corollary — full materialization is expected, not a cost to avoid.**
Materializing every leaf is the correct result of pulling in the whole construct
at base LoD. It is *assignment into the fixed particle pool* (declaration), not
allocation, bounded only by the configured budget N. Sparsity is only about
**partial** pulls, where the finer LoD is simply not brought in.

## Fixed particle budget, and claim = assignment not allocation

The particle budget is fixed per system at initial configuration (the pool sized
to the card, allocated once). Using a particle is **assignment of values**
(claim / declaration) into a slot that already exists; an unused slot is `0x`
(inert); releasing is zeroing it back to `0x`. This is the model's pool
mechanic (`OPERATIONAL-PLAN.md` §3.11); on the engine side it is exactly why a
`pointer` cell's active/inactive state is the claim/release, and why a `dense`
floor is "already there."

## Node ids monotonic; tree ids recycle; restart reclaims

- **Node ids are monotonic within a `Program` instance's lifetime** (destroyed
  layouts still count toward it). `SNode::counter` is a static `std::atomic<int>`
  (`snode.cpp:12`, incremented `id = counter++` at `:220`) but is **reset to 0 in
  the `Program` constructor** (`program.cpp:144`; only one instance at a time,
  `:141`), so the count is per-`Program`-instance, not permanently process-global.
  The wrapper's `issued_snode_ids()` returns exactly `SNode::counter`
  (`engine/src/engine/runtime.cpp:60-61`). The matched tree records the same
  limit: "monotonic SNode IDs still count destroyed layouts toward the
  instance lifetime budget" (`[matched tree] modernization/IMPLEMENTATION-STATUS.md:109`).
- **Tree ids recycle.** `destroy_snode_tree` pushes the id onto
  `free_snode_tree_ids_` (`program.cpp:214-235`) and `allocate_snode_tree_id`
  pops from it (`program.cpp:559-565`).
- **The budget is asserted at compile.** `snode->id >= llvm_snode_capacity` and
  the tree id `>= llvm_snode_tree_capacity` are hard errors in codegen
  (`taichi/codegen/llvm/struct_llvm.cpp:254-260`). Defaults are
  `llvm_snode_capacity{1024}` and `llvm_snode_tree_capacity{512}`
  (`compile_config.h:64-65`), configurable via `TI_LLVM_SNODE_CAPACITY` /
  `TI_LLVM_SNODE_TREE_CAPACITY` (`compile_config.cpp:84-88`).

**Consequence:** churning `add_snode_tree` recomposition spends the monotonic
node-id budget. It is reclaimed by **restart**, which the process separation
exists to make clean: the analyst restarts the harness, or restarts the engine
through the harness; a fresh engine instance resets the counter (the old instance
is torn down first — only one at a time, `program.cpp:141`). So the budget is
bounded by a controlled restart, not something to pre-budget infinitely — budget
sensibly for a working session.

## Sparsity / paging (dormant-below-LoD cost)

- `pointer` / `dynamic` allocate per activated cell and recycle to a pool on
  deactivate; inactive cells share `ambient_elements`
  (`llvm_sparse_runtime.md:52-54`). `bitmasked` gives per-cell activity at 1 bit
  (`sparse.md:133,165`).
- The paged `ListManager` (the fork's lazy-metadata work): "Data are organized in
  chunks, where each chunk is allocated on demand"
  (`taichi/runtime/llvm/runtime_module/runtime.cpp:412`); a 2 KiB root directory
  plus lazily allocated 4 KiB pointer pages, `chunks_per_page = 512`
  (`runtime.cpp:419-426`). Compiled `ListManager` metadata dropped from
  1,048,616 bytes to 2,088 bytes (`[matched tree]
  IMPLEMENTATION-STATUS.md:64-65,81`). This is the "a collapsed structure costs
  ~nothing until touched" mechanism behind demand-driven expansion.

## Live recomposition

`add_snode_tree` on a live `Program` materializes a new tree alongside the
existing ones; the smoke test builds two trees on one runtime and confirms they
coexist and stay independent
(`engine/tests/engine_smoke_test.cpp:90-99,134-158`). One compiled kernel serves
many launches with differing **launch arguments** (the smoke test launches one
writer twice with different `scale`, `:116-132`); kernels are keyed by name.

## Honest flags

- **Recompile trigger not re-traced this pass.** "Only a layout / SNode-tree
  change forces recompilation; a scalar / loop-bound / array is a launch
  argument; kernel name is the compilation key" is cross-checked against the
  vetted `HARNESS-NOTES.md` "rate-of-change seam (P1.1–P1.4 CONFIRMED)" and the
  smoke test's observed behaviour, **not** freshly traced to `taichi/codegen` /
  the offline-cache source here. Treat as vetted-elsewhere, not re-verified in
  this document.
- **`hash` SNode** is declared (`snodes.inc.h`) but not exercised in our
  workspace — behaviour untested here.
- **Upstream deprecation banners** for `dynamic` (removed v1.3.0) and
  `pointer` / `bitmasked` (slated v1.4.0) apply to the **Metal / non-LLVM**
  backends; the LLVM CPU/CUDA backends "offer the full functionality"
  (`sparse.md:43-45`), and our path is LLVM.
