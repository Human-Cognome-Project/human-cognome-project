# Local issue: confirm the first complete contextual ledger fixture

Opened by: Codex (AI agent), 2026-09-11. Status: awaiting contextual identity specification.
Execution step: 1 of the authorized handoff. No external issue opened.

## Authority and preserved state

Current specification: /opt/project/taichi/modernization/hcp-handoff/START-HERE.md.
Repository HEAD matches its reviewed revision, 014f7a6e97cc635302c8755fda7a0a84148207a8.
Pre-existing untracked field reports are preserved. Legacy unit-mass patch remains
staged in the handoff directory, unapplied; recommendation is to retain it as a
regression reference during the new C++ slice. No database or engine edits made.

## Concrete example for Patrick

Byte A1 has ordered hex components A, 1, each mass 1, total mass 2.
Two consecutive occurrences A1 A1 retain both positions and total composition
mass 4 even if both refer to one canonical byte structure.

Local Python codec round trips verified:

- ISO-8859-1: A1 -> U+00A1, INVERTED EXCLAMATION MARK (¡) -> A1.
- ISO-8859-2: A1 -> U+0104, LATIN CAPITAL LETTER A WITH OGONEK (Ą) -> A1.

These are codec observations, not proof of database interpretation links.
The saved hcp-example-trace.json identifies the corresponding table records as
[AA, AD, AB, AA, AA] and [AA, AD, AB, AB, AA]. The saved hcp-prefix-trace.json
contains byte record [AA, AA, AA, AA, DL], named UTF8 CONTINUATION 33.
That stored name must not become the byte's universal interpretation.

## Acceptance gaps

Confirm whether the two table interpretations are distinct literal identities
linked to the same canonical byte through contextual Sibling relationships,
or whether context is attached differently. Then trace their exact live
addresses and reciprocal traversal; do not synthesize canonical addresses.

The handoff reports no byte-parent atomization rows in the inspected prefix.
Separate canonical hex component addresses and reciprocal links have not been
established. Older architecture's prohibition on nibble references cannot
silently decide the new representation. Ask how the two components are to be
referenced before making an executable fixture requiring those addresses.

Multiplicity within A1 A1 is specified. Cross-group overlap is not: if two
Sibling groups both contain A1, group/source counting and force accumulation
must await a worked rule. No force, movement, alignment, summary invalidation,
or runner behavior is implemented by this issue.

## Next action

Resolve the contextual identity example with Patrick, inspect only its bounded
read-only database records, and produce an exact fixture with missing evidence
explicitly flagged. Step 1 is not accepted yet; dependent C++ contracts remain
pending.

## Subsequent direct clarification from Patrick

These decisions supplement the handoff and supersede conflicting earlier wording.
Recorded by Codex from the conversation; intended behavior is distinguished from
mathematical results still to be demonstrated.

- Orbital mechanics is the intended mathematical expression of force relationships
  between comparable elements. Periodicity around a point that is not fully stable
  describes an orbit. This does not supply an integrator or additional force law.
- The analytical target is complete accounting of all particle forces and an exact
  traversable relationship map, stabilizing as an n-dimensional lattice. Patrick
  regards complete accounting and stasis as the same completed state. Convergence
  to that state remains to be demonstrated with the agreed equations.
- Unicode is the encompassing centroid in the example; tables are siblings under
  Unicode and gather their components. Component connections establish relative
  arrangement. Unmapped cross-table relationships leave interchangeable identities
  incompletely constrained and are intended to appear as oscillation.
- Every connection is a centroid grouping. The database maps the centroid
  relationships each particle participates in: its active field effects. It does
  not store conditional centroid values as relationship facts.
- Compute conditional centroid values at the end of each tick for the next tick.
  This is the current rule, not a claim about previous engine implementations.
- The runner assembles a compressed slice of the full data table, including
  algorithmically compressed representatives of excluded sections. Correctly
  structured active representatives participate in engine force balancing.
- A nonzero delta here means an out-of-scope element's influence on the active
  force calculation, not a separately specified comparison of consecutive states.
  Such influence can expose a pairwise relationship for analysis. No numerical
  threshold for sufficient influence has been specified.
- Patrick explicitly confirmed: lossless reconstruction of the stored relationship
  structure is distinct from centroid force approximation. Centroid pull remains
  the chosen abstraction; underlying relationships remain available for expansion
  when their influence warrants analysis. No exact all-member force equivalence
  is claimed for a collapsed representative.
- The SNode capacity expansion is intended to admit more complex relationships
  simultaneously and greater relative LoD depth in one load, making pursuit of
  nonzero relationships more fluid. Existing substrate limits still apply:
  independent trees, startup capacities, persistent owning runtime, and possible
  layout replacement/specialization. This clarification does not establish
  arbitrary live branch growth or physically nested independent trees.

The question of whether a connection declares a centroid grouping is resolved:
all connections do. Do not ask it again. The exact canonical identity/address
fixture, overlap accounting, distance/direction and coincident-position rules,
and movement/alignment equations remain unspecified by these clarifications.
Do not implement those dependent policies by importing conventional defaults.

## Follow-up authority

See [updated execution plan](execution-plan-2026-09-11.md), especially
“Settled follow-up decisions — do not reopen.” It supersedes this issue's earlier
requests for overlap policy and abstract scope-transition policy. Shared membership
retains all relationship vectors; Parent active points determine compound
alignment; velocity persists; mass stays fixed for fixed participating content.
The next structural artifact includes the assembled working database boundary,
arrayed relative-address traversal and compact SNode LoD mapping.
