# HCP execution plan after centroid and compound-response clarification

Prepared by Codex (AI agent), 2026-09-11. Preparation artifact; implementation must follow the settled decisions below. Direct conversation clarifications supersede the
initial handoff where they differ.

## Specification to preserve

Every connection is a centroid grouping. The database preserves the exact map of
predicate relationships, ordered composition, multiplicity, context and provenance.
Compression must permit exact recovery; no optimization may discard predicate
interaction or recovery. Canonical addresses and encoding symbols are different
alphabets; device handles are neither of them. Each hex digit contributes mass 1.

The effective second element in a field comparison is a group's total source mass
and central pull, not an all-to-all enumeration of its members. Force magnitude is
m1*m2/d^2. This replaces the earlier interpretation that groups merely cache
member-pair discovery. Each tick performs the calculation required by this active
representation. Grouping structure is retained; conditional numerical effects
are calculated in real time.

Use prepared group summaries for the tick's parallel force work. Derive the next
centroids from the calculated endpoints of the groupings processed in that tick.
End-of-tick reduction avoids making current force work wait for a beginning-of-tick
reduction. The lag is a scheduling tradeoff, not a required physical propagation
law. Nominal error and subsequent recovery are expectations to test, not proofs.
Identical predicates are expected to give stable centroid calculations; specify
the complete unchanged inputs in the stability fixture.

A component interacts according to its affected mass. Its force is expressed
through its position and the full containing mass, including matter inert for
that field. Do not pre-discount force by a component mass fraction or independently
move the component while ignoring its containing artifact. Parent means ordered
direct composition; Child supports inverse traversal; Sibling records groupings.

The runner assembles a compressed working slice with representatives for excluded
sections. Their influence participates in active calculations and can motivate
finer analysis. Lossless structural recovery does not imply exact equality of
centroid pull and expanded member pulls. Patrick explicitly accepts centroid pull
as the abstraction. Greater simultaneous SNode scope enables deeper LoD and fluid
expansion. Related multilingual elements may gather without losing their distinct
identities; viewed dimensions and LoD reveal specificity. Three-dimensional
presentation must not fix the mathematical space to three dimensions.

## Implementation evidence reviewed

- field/field_engine.py: Pool uses value+1 mass; Oracle.tick deposits channels,
  performs Jacobi relaxation, reads gradients and applies capped movement.
  make_kernel allocates persistent fields; Kernel.tick runs that diffusion path.
  This is not the desired centroid-group calculation. Keep it as historical
  evidence; recommend retaining the staged mass patch as a regression reference.
- No current C++ source/build files were returned by the non-archive repository
  scan. A controller integration must be established.
- /opt/project/taichi/modernization/IMPLEMENTATION-STATUS.md and
  INSTALL-CONFIGURATION.md report completed CPU/GTX1070 capacity work and native
  C API loading. These results were read, not rerun for this planning task.
- /opt/project/taichi/benchmarks/snode_patterns.py uses synthetic group sources,
  softened distance and an equal-mass centroid. Reuse its capacity/lifetime
  evidence, not its numerical policy as the production specification.
- Initial handoff and saved ledger traces live in
  /opt/project/taichi/modernization/hcp-handoff/. Local step-one issue:
  review/issue-ledger-example-2026-09-11.md. Its later clarifications and this plan
  supersede earlier unanswered questions about whether connections are groupings.

## Execution sequence and acceptance

1. Complete the small ledger fixture and compare it with bounded read-only live
   records. Use byte A1 under the two identified single-byte tables; retain exact
   addresses only when observed. Include two occurrences, component mass, context,
   and reciprocal traversal. Add a compound with an active component and inert
   mass, plus an overlapping grouping example for numerical clarification.
   Acceptance: exact reconstruction, order and multiplicity; unresolved addresses
   or identity decisions explicitly flagged, never synthesized as facts. Resolve
   those gaps before declaring the fixture accepted. No migrations or old loaders.

2. Write the worked tick specification before numerical kernels. Show initial
   state, prepared group source mass/centroid, affected component force, expression
   through the containing artifact, endpoints, and next group summaries. Include
   an unequal-mass group and overlapping membership. Ask only the questions that
   determine those calculations (below). Acceptance: Patrick can trace every
   quantity without an unstated mathematical convention.

3. Define minimal C++ records and controller actions from the accepted fixture.
   Separate canonical records from compact local handles; preserve ordered
   component occurrences, contextual group memberships, reverse traversal and
   recoverable expansion references. Runtime buffers hold state and current/next
   summaries with their scope/state generation. Proposed actions: load a slice,
   inspect a record/group, advance ticks, expand a grouping, and request a readout.
   Keep a caller boundary for the future runner. Acceptance: every field/action
   has an explained purpose and embeds no unresolved numerical policy.

4. Establish native Taichi ownership and build/loading path. First prove C++ can
   create a persistent runtime, load/execute a minimal prepared kernel, and retain
   state. Evaluate native C API/AOT for the fixed fixture; verify how new layouts
   and specializations can join the same runtime before choosing the expansion
   path. Build-time Python tooling is permitted; production ticks are C++ driven.
   Acceptance: native smoke test and documented ownership, ABI and capacity use.
   Do not infer dynamic expansion capability from the existing loader probe alone.

5. Implement the agreed tick first as a small deterministic C++ CPU reference,
   then as persistent GPU work: prepared summaries -> parallel affected-mass pulls
   -> compound response/endpoints -> group reductions -> publish next summaries.
   Use two summary buffers as an engineering proposal to prevent mixed generations.
   A one-time initial-summary preparation is required; confirm its inputs in step 2.
   Hierarchical endpoint dependencies must be explicit rather than silently mixing
   current and prior generations. Acceptance: CPU/GPU equivalence within declared
   tolerances; normal ticks need no database access or full-state readback.

6. Validate semantics and approximation separately. Test unit mass, order-sensitive
   identity, repeated components, context and exact expansion round trips. Test
   unequal-mass endpoints, active/inert compound response, overlap policy and stable
   inputs against agreed outputs. Compare lagged summaries with a diagnostic fresh
   summary evaluation on small and large movement cases. Measure force differences
   across collapsed/expanded views without asserting exact equivalence. Check that
   predicates remain recoverable and represented according to the agreed scope.
   Report oscillation, drift and convergence; do not add damping, softening,
   thresholds or movement caps to make a failing case appear stable.

7. Exercise incremental LoD expansion. Start with a collapsed external grouping,
   expose its influence, explicitly request expansion, retain the owning runtime
   and existing trees, and introduce/reuse the required layouts. Publish coherent
   scope and summaries at the agreed boundary. Compare retained state with a fresh
   reference at the same scope. Measure transfers, allocation, compilation,
   dispatch, force work and reduction separately. Record peak memory and monotonic
   SNode-ID headroom. No promise of unlimited growth or complete sparse reclamation.

8. Implement the first requested runner analysis only after its interpretation and
   scope-selection behavior are specified. Candidate demonstration: investigate
   cross-language relationships around English cat, inspect an influencing group,
   expand it, and report traversable predicate evidence. This is a proposed task,
   not permission to invent autonomous cognition or add an LLM framework.

## Settled follow-up decisions — do not reopen

Patrick clarified the following after the initial plan:

- Each particle carries its previous velocity. Motion combines all vectors from
  all force relationships to which it is subject; gravity pulls toward zero
  separation. Do not replace this with a memoryless displacement rule.
- Parent ordering preserves active points. Forces affect the whole mass, while
  their expression relative to those points creates cross-field alignment.
- Mass does not change with endpoints or ticks. With the same participating
  mass points, mass remains fixed; their relative positions determine the center
  when calculated. Include all points of force and mass in the representation.
- Every applicable force relationship contributes its vector. Shared group
  membership does not authorize deduplicating away an interaction. This resolves
  the earlier overlap-policy question; repeated composition positions also remain.
- Define LoD granularity steps, level contents and complete component relationships
  in the assembled SNode structure. Relative stepping is intended to follow that
  structure. Do not solicit an additional abstract scope-transition policy before
  finding a concrete implementation gap.
- Preserve the dual database architecture: full storage database and assembled
  working database. The present small dataset permits direct storage use. CPU
  assembly selects detail and aggregates defined out-of-scope sections while
  retaining their influence and exact recovery. Several database levels may map
  to a compact aggregate within an SNode; database depth is not SNode depth.
- Arrayed address couplets define traversal; relative addresses record only the
  delta from the current address. Follow explicit relationships rather than
  repeatedly discovering them. Do not invent a delta wire format without checking
  the existing data/code. Encoding values remain separate from address symbols.
- Later, a secondary pre-cache thread will maintain the working database for
  runner/controller needs and handle cache misses. Begin with synchronous assembly
  at moderate load, retaining a boundary for this future thread.

Concrete numerical implementation details still require evidence before coding:
which existing velocity/time-step integration method to use, the mathematical
coordinate/active-point representation, and behavior at exact zero separation.
These are not reasons to reopen settled model choices or stop structural work.
Raise a focused question only when an actual implementation needs the answer.
Earlier steps mentioning overlap policy and scope policy now mean implementing
and testing the above decisions, not obtaining those decisions again.

## Immediate next deliverable

The exact ledger fixture and a storage-to-working-set-to-SNode mapping.
Read-only record inspection and representation drafting can proceed now. Follow
with a compound/group tick using carried velocity and all relationship vectors;
ask only about concrete numerical details not established by the current decisions. No production code,
live data, engine patch, or external publication is changed by this plan.
