# Bootstrap guide: encoding-floor records into a Taichi working layout

**Status (2026-09-30):** A small, reviewable assembly example, not a built
cache-manager protocol or a final SNode schema. It uses the retained
[`hcp_core` encoding-floor snapshot](../../data/postgres/snapshots/2026-09-28-encoding-floor/MANIFEST.md)
on `main`. No database change or extra seed is required for this example.
The cache manager, analyst, and selective LoD assembly described below are
still planned; the C++ Taichi runtime and the simpler field arrays are built.

## Keep the three uses of “field” distinct

| Term here | Meaning |
|---|---|
| **Study focus** | The question shaping a warm view, such as hex encoding or language. Its chosen root and LoD are not permanent cold-store categories. |
| **Interaction field** | A model relationship among participating particle instances, with a centroid read by the interaction law. A parent value can connect occurrences in different ordered positions. |
| **Taichi data field** | Typed numerical values at indices in a Taichi layout, placed at an SNode leaf. It is storage, not an interaction field. |
| **Force** | The directed result of evaluating one particle against an interaction field's centroid. It is not the field's membership or its SNode layout. |

The [Taichi SNode terminology](../taichi/docs/lang/articles/internals/internal.md)
needs care here: the Python
frontend writes `ti.root.dense(...).place(x)`; our C++ path constructs a root
`SNode`, adds container nodes and `place` leaves, then calls
`Program::add_snode_tree`. The working example is
[`engine_smoke_test.cpp`](../tests/engine_smoke_test.cpp) and the calling notes
are in [ENGINE-NOTES.md](ENGINE-NOTES.md). A Taichi cell can have several
components with a fixed schema; a `pointer` component activates its own child
block, not a reference to an arbitrary pre-existing warm-cache object.

## Physics object assembly and the data bridge

The cold database is the reusable physics object library: atomic and composed
tokens, ordered constituent occurrences, and group memberships. It does not
store a pre-expanded tree for every possible universe or study. The intended
cache manager starts at the finest relevant tips, follows those explicit
links, and builds larger standard objects and selected rollups upward toward
the study root. The warm cache keeps those study-shaped prepared pieces;
the hot working structure instantiates the levels that can be manipulated
in the current simulation. Large SNode capacity is for these object rollups,
not a reason to inflate the cold record store.

This is literal physics object design. A simulation containing planets,
ships, cities, characters, and components can instantiate them from reusable
definitions and expose different constituent levels according to the study.
Each hot instance retains its `token_id` (the reusable token representing
the thing in the library) and has its own `particle_id` in the active model. Repeated
instances of one token can have different positions and connections.
`SNode::id` identifies Taichi's structural layout node, not that instance.

Taichi supplies the hierarchical storage format; the **C++ data bridge** is
the work still to build. It must translate cold definitions and links into
warm compositions, decide the selected LoD, assign and track hot instances,
stage their fields and relationships, and construct/register the Taichi
layout used by the physics kernels. The specific SNode layout and mapping
across recomposition remain open. Taichi's `pointer` SNode activates its own
fixed-schema child storage; it does not by itself reference a shared
object definition elsewhere in the library.

## What the current floor proves

The snapshot contains 16 single hex tokens (`0`–`F`, mass 1), the addressed
mass-zero `0x` token, 256 couplets (mass 2), and three **provisional** group
labels with assigned masses. The labels are not yet fully constructed tokens.
The `token_id` column is an array of Base62 pairs; the dotted `token_text` is
its rendering. `token_parent` stores ordered occurrences; `token_child` stores
a direct reverse follow once per distinct parent/composite pair. `member_of`
and `members` are the separate reciprocal group-membership axis.

Three existing couplets expose the important distinction:

| Couplet | Address suffix | Parent at ordinal 0 | Parent at ordinal 1 |
|---|---|---|---|
| `01` | `…11` | `0` | `1` |
| `10` | `…1G` | `1` | `0` |
| `11` | `…1H` | `1` | `1` |

The **same parent value `1`** can connect `01`, `10`, and `11` in one
interaction field across both positions. Each occurrence still carries its
own ordinal for ordered effects: `11` has two distinct occurrences even
though its `token_child` return link from `1` appears only once. The warm
assembly follows that link and reads the couplet's ordered `token_parent`
rows to recover the occurrences. **Do not key field identity by
`(parent value, ordinal)`**: that would sever cross-position correlations.
The precise positional motion rule remains separate work; this example does
not install a new force formula or row/column labels.

## Assembly walk, with built and planned boundaries

1. **Read cold records (built):** choose an addressed starting point and follow
   the stored parent/child and membership/member links needed for the study.
   Keep `token_id`, every parent occurrence and ordinal, and direct group
   memberships. Do not treat the three provisional labels as fully formed
   naming literals. `hcp_core` is the always-relevant core database, not the
   whole future cold archive.
2. **Prepare a study-shaped warm view (planned):** collect relevant pieces and
   indicate which can be exposed as nested detail and which have an aggregate
   at the selected LoD. The warm view may attach the same token definition at
   several loci without minting a new cold identity. For the current snapshot,
   the `Hex Code Patterns` / `Hex Couplets` memberships give a small provisional
   study boundary; their incomplete label construction remains explicit.
3. **Choose hot instances (planned):** assign a model `particle_id` to each
   exposed instance and retain its `token_id` reference. Several instances
   may refer to one token; that automatic same-token sibling interaction is
   intended, not yet generated by the native field harness. An SNode index
   can be used for a particle slot, but stable identity across recomposed
   trees needs an explicit mapping; `SNode::id` identifies a schema node.
4. **Construct a Taichi layout (C++ mechanism built; HCP composition planned):**
   create a root SNode, add a container for the chosen hot slots, attach typed
   `place` leaves for numerical values, and register the tree with the Program.
   This minimal *mechanics illustration* holds one mass value per slot:

   ```cpp
   using namespace taichi::lang;
   auto root = std::make_unique<SNode>(0, SNodeType::root);
   SNode &slots = root->dense(Axis(0), capacity);
   SNode &mass = slots.insert_children(SNodeType::place);
   mass.dt = PrimitiveType::f32;
   runtime.program().add_snode_tree(std::move(root), false);
   ```

   Further leaves could hold positions and other typed values. A sparse
   `pointer` branch can represent optional active blocks, but
   [`Pointer_activate`](../taichi/taichi/runtime/llvm/runtime_module/node_pointer.h)
   allocates its child block. A fixed particle pool claimed by value assignment
   is a distinct model mechanism; the precise combined layout has not been
   selected. The Python `ti.root` spelling describes the upstream frontend,
   not the required HCP runtime language.
5. **Stage interactions and run (simple arrays built; full translation
   planned):** the present [`field::Harness`](../src/field/field.h) already
   accepts particle, group and edge arrays, uploads them, seeds inclusive
   centroids, ticks, and downloads results. It does **not** yet load this
   database, build the warm SNode view, store `token_id` per particle, create
   sibling groups, or apply the intended ordered-parent reorientation. A
   first assembly check can verify the `01`/`10`/`11` occurrences and the
   choice of hot slots without claiming a validated physical outcome.

## What must be decided before a numerical end-to-end bootstrap

- The initial study focus, view shape and LoD choices, including what may
  expand and how a model instance survives tree recomposition.
- How `token_id` references and `particle_id` slots are represented in the
  hot C++ layout, including repeated instances and the mass-zero `0x` free
  state. The addressed `0x` row is distinct from an inactive Taichi pointer.
- Initial positions and the exact use of ordered parent occurrences in
  motion. The dump gives `token.mass`, but its couplet `token_parent.mass`
  values are SQL `NULL`; a numerical staging rule must not silently turn
  `NULL` into zero or assume that the per-occurrence mass is populated.

These are **translation choices, not requests for more seed data**. The
existing floor suffices to check address follows, direct group membership,
cross-position parent commonality, repetition, and a small SNode/slot staging
path. Language/PoS is a later study-shaped projection and needs its own
records to validate that content, not to establish these mechanics.
