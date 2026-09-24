# HCP execution handoff — 2026-09-11

Read this first when opening /opt/project/repo. This is a record of Patrick's
current specification and a proposed execution plan, not a replacement for his
input. Ask when ambiguity changes meaning. Do not normalize unfamiliar rules
into conventional algorithms. Later clarification supersedes earlier assumptions.

## Workspace and authority

- Execution project: /opt/project/repo, HEAD at review:
  014f7a6e97cc635302c8755fda7a0a84148207a8.
- Engine dependency: /opt/project/taichi; its completed capacity work is
  uncommitted. See ../IMPLEMENTATION-STATUS.md and ../INSTALL-CONFIGURATION.md.
- Sibling project was inspected read-only. An attempted source write was denied;
  the unit-mass correction is staged here, NOT applied. No database changes.
- There are pre-existing untracked field experiment reports. Preserve them.
- Read the target AGENTS.md and applicable subtree instructions. Old code,
  research, claim graphs, and documents are evidence to inspect, not automatic
  current specification. Some decisions evolved after code was written.
- User authorized this plan and earlier implementation work. Reopening a folder
  is not a request to reset the design or start the investigation over.
- Do not publish, push, or open external issues without authorization. The local
  instruction to start an issue does not authorize an external message.
- Do not run old loaders, migrations, or experiments against live databases
  merely because they exist. Historical databases remain read-only.

## Operating principles

1. Simplicity is king: if a piece cannot be explained, do not include it.
2. Declaration is near free; calculation has a cost; allocation is expensive.
3. Precise, explicitly linked data should eliminate repeated discovery work.
4. C++ for the controller and runner; Python may support experiments/tests but
   does not belong in the production hot path.
5. Preserve the mathematical specification; implementation conventions must
   not silently become additional mathematical rules.

## What is being built

An archive and digital cognitive engine over encoded knowledge, intended to
scale toward everything encodable of reality. Its computational model uses
similarity as attraction and one gravity-like inverse-square interaction rule.
This is NOT a request for a conventional celestial mechanics simulator.
No orbital integrators, alternate forces, diffusion grids, or multipole schemes
are to be introduced merely because they are familiar approaches.

Three primary active components:

- Active model physics: scoped force evaluations and hierarchical group-summary
  reductions on the engine substrate.
- Controller: the analyst/player terminal, providing access and actions.
- Runner: the intended intelligence, portal, and interpreter. It drives the
  controller, investigates requested topics or what it deems worthy, and examines
  commonalities, unusual pairwise connections, and imbalanced field behavior.
  It is NOT a synonym for a kernel launcher or job scheduler.

This identifies intended roles, not a finished runner algorithm. Do not bolt an
LLM or conventional agent framework onto the system as an assumed implementation.
Claims about eventual cognitive capability remain intended outcomes to validate.

## Canonical data and mass

- Store each distinct structure once; reuse by reference. Preserve occurrences,
  order, context, and provenance required for exact reconstruction.
- Use the largest reusable units that preserve full fidelity and reproducibility.
  Newly discovered commonalities can support refactoring existing compositions.
- Canonical addresses: up to five arrayed pairs of base-50 characters; dotted
  strings are display notation. Top couplet routes to topical cold shards.
  Encoding symbols are separate from address symbols: 16 hex digit values,
  two hex digits per byte. Do not conflate these alphabets.
- CURRENT MASS RULE: each hex digit has mass 1, each byte/couplet mass 2.
  Larger compositions aggregate constituent count, independent of digit value.
  This supersedes the old value+1 rule and old reported weight measurements.
- Preserve multiplicity within composition: reuse of one canonical component
  does not erase its repeated positions or their contribution to composition.
  Cross-group overlap accounting still needs an explicit worked example.

Each particle has three sections in Patrick's terminology:

1. Parent: its direct components in linear order, with cumulative component mass.
   This is composition, not conventional tree-parent nomenclature.
2. Sibling: pairwise groupings in which it participates, including contextual
   encoding-table/codepoint relationships at the foundation.
3. Child: inverse of Parent relationships, for fast CPU database traversal.
   This section is traversal support rather than an active force section.

Direct links at each level permit recursive expansion to byte detail. Do not
infer that every ancestor must be physically copied into every record.
A literal is factual input/cell content whose order must be preserved. 'Spin'
means alignment associated with ordered composition, not an instruction to add
rigid-body angular dynamics. Component effects can pull the containing particle
or align it, according to the model; exact update equations are not yet captured.

## Working set and algorithmic rollup

- Cold shard swarm: topical databases, selected by top couplet.
- Instance-local cache: common working sets and high-level tangential aggregates.
- Assembled working database: compact selected slices feeding the active model.
- GPU holds a small active portion, not the universal archive.
- At the end of each tick, compute group summaries for use in the next tick.
  Aggregate source strength supplies m2; centroid supplies position for group
  pull. Net received force does NOT substitute for source mass.
- Scope/viewport expands relevant groups and represents others by aggregate
  particles. Stored references retain exact reconstruction. Centroid pull is the
  model's chosen abstraction, not a claim of universal exact gravitational
  equivalence to all members. Do not keep proposing extra physical mechanics.
- Pairwise tax ledger: store discovered contextual relationships so they need
  not be rediscovered. Force values may change as state changes.
- Group membership, stale-summary rules, tick ordering, overlap treatment, and
  scope policy require explicit examples before implementation choices fix them.

## Taichi substrate already completed

Configurable startup LLVM SNode/tree metadata capacities; paged ListManager
pointer directories; skipped unused scalar-place lists; fixed initialization of
interleaved tree IDs; runtime ABI cache/AOT protection. CPU and GTX1070 verified,
including 6,657 SNodes and native C API environment capacity consumption.

Multiple independent SNode trees can coexist. Trees contain nested nodes; they
are NOT trees physically nested in other trees. Kernels can use fields across
trees. Preserve existing trees and add new ones within startup capacities.
Compiled layouts cannot freely grow branches; affected layouts may need replacement
and device-side migration. New fields may require kernel specializations.

Keep the owning runtime alive to preserve GPU allocations. Runtime finalization
releases them. Capacity remains fixed at startup; IDs remain monotonic across
layout destruction; complete sparse reclamation is not solved. Do not promise
unlimited live growth or memory survival across full process teardown.
GTX750Ti qualification is deferred. No blanket 64-bit conversion is requested;
audit wide-address offset arithmetic where necessary.

## Verified repository/database findings

Working iteration confirmed by user: hcp2_core. Access settings are available in
project files; keep credentials out of this handoff and committed code.
Read-only schema inspection used default_transaction_read_only=on, five-second
statement/connect timeouts, and a one-second lock timeout.

- tokens(address text[] primary key, metadata jsonb, descriptive/provenance fields).
  Planner estimate roughly 6.27 million rows, NOT an exact count.
- atomizations(parent text[], resolution text[], ord, child text[], weight, provenance).
  Primary key (parent, resolution, ord). No child-leading index found.
- engine.declarations_v0 exists, along with forwarding and registry tables.
- JSON can contain further structure; column inspection alone does not establish
  absence of sibling relationships.

Actual ranges in the inspected database:

- AA.AA.AA.AA.*: 256 byte records; no atomization rows with this parent prefix.
  Single-hex decomposition was not established there.
- AA.AA.AA.AB.*: 321 older semantic primitive/molecule/operation records.
- AA.AA.AA.AC.*: no token records.
- AA.AB.AA.*.*: character/encoding catalogue used by current loader.
- AA.AD.AB.*.*: encoding identities, including UTF-8 and codepoint_hex.

Worked record: AA.AB.AA.AA.tV, U+01AC, LATIN CAPITAL LETTER T WITH HOOK.
Under UTF-8 resolution AA.AD.AB.Ad.AA, its ordered children represent C6 AC.
Under codepoint_hex AA.AD.AB.Ag.AA, its children represent 0,1,A,C.
Stored weights follow older rules; don't interpret them as current unit mass.
Reverse relationships can be queried from atomizations but no reverse-leading
index was found. This is evidence of part of the ledger, not verification that
all intended Parent/Sibling/Child links or interpretation distinctions exist.

## Draft implementation findings

field/field_engine.py and *_v0.py are Python/NumPy/Taichi experiments. The main
prototype uses dense 3D grids per component slot and nibble value, 40 Jacobi
iterations per default tick, full-grid copy passes, cell residency/exclusion,
and movement caps. It does not implement the currently described scoped
force-and-centroid graph. Its CPU/GPU equivalence tests validate that diffusion
procedure, not the desired model. Existing GPU fields do remain allocated;
do not falsely claim the prototype reallocates all GPU storage every tick.
No current C++ controller was found outside archived work in the bounded scan.

The staged unit-mass patch corrects count mass in the legacy prototype and
changes equality tests from weights to ordered values; otherwise equal-length
strings would incorrectly merge. Five tests passed using the built Taichi
CPU/CUDA module plus NumPy; git apply --check passed. Patch not applied.
This preserves the prototype's existing grain/merge behavior; it does NOT
approve those semantics for the new model. Existing checkpoints and stored
weights were not migrated, and old experiment results do not validate the patch.

## Proposed execution sequence and acceptance criteria

### 1. Confirm one complete ledger example

Trace a byte interpreted in at least two single-byte tables, including different
codepoint meanings, its two unit-mass components, and reciprocal traversal.
Compare intended records with actual records. Ask Patrick to resolve contextual
identity, composition multiplicity, and any missing links. Produce a small
fixture with exact expected addresses/relations, not a broad database rewrite.
Accept when reconstruction, order, context, and mass match his example.

### 2. Define the minimal C++ data and action contracts

Use that fixture to specify canonical records, compact local handles, ordered
component ranges, group membership, reverse traversal, and aggregate buffers.
Keep canonical addresses distinct from GPU-local indices. Controller actions
should cover loading/scoping, advancing analysis, inspecting results, and adding
analysis. Preserve a caller boundary for the future runner.
Accept when every field and action has an explained purpose and no unconfirmed
mathematical policy is embedded in representation.

### 3. Build one vertical execution slice

Read-only load -> compact working set -> persistent GPU fields -> scoped force
calculation -> group centroid/count reduction -> next tick -> requested readout.
C++ controller/execution plumbing; the cognitive runner is not yet implemented.
First validate a small CPU reference from the agreed equations, then GPU behavior
with stated numerical tolerances. Reuse prepared storage across ticks. Measure
transfers, allocation, dispatch, force work, and reduction separately.
Accept when intended outputs match and no per-tick database access or full-state
readback is required for ordinary operation.

### 4. Verify incremental expansion

Retain running runtime and old trees, add one independent analysis unit, compile
new specializations where required, switch at a tick boundary. Check retained
state and overlap behavior against a fresh reference. Record headroom and peak
allocation requirements. No live-capacity growth redesign without evidence.

### 5. Introduce runner behavior through the controller

Only after specifying how the runner interprets field results, chooses a next
scope, identifies a candidate commonality, and requests a recorded grouping.
Implement one requested analysis path before autonomous priorities. These are
missing behavioral specifications, not permission to substitute a standard
agent architecture.

## Clarifications required as their dependent work begins

- Exact n-dimensional distance/direction formula and coincident-position rule.
- How component attraction becomes containing-particle movement versus alignment.
- Whether zero-padding in codepoint_hex is actual composition or display formatting.
- Whether alternate encodings share an identity versus have distinct literal
  identities connected by contextual sibling relationships.
- How overlapping sibling memberships affect counts and force accumulation.
- Which scope changes require recomputing summaries and in what tick order.
- Runner's first concrete task and expected interpretable output.

Ask these against examples, not as one large up-front questionnaire.

## Resume instruction

Read this file and target instructions, confirm write access and working-tree
state, then begin step 1 with the existing live-schema evidence. Do not restart
the Taichi review, port the diffusion solver wholesale, apply migrations, or
invent runner cognition. Discuss whether to apply the staged legacy mass patch
or retain it as a regression reference while building the new C++ slice.
