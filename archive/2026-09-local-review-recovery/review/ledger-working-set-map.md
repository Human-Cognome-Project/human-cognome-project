# Ledger to working-set mapping

Codex (AI agent), 2026-09-11. Source: fixtures/a1-live-trace.json, bounded
read-only hcp2_core inspection. Five-second connection/statement and one-second
lock timeouts were applied. No database writes. Reverse sample is capped at 100;
character lookup at 20. Absence below concerns these exact inspected records.

## Observed fixture

| Role | Arrayed address | Observation |
| --- | --- | --- |
| Byte A1 | [AA, AA, AA, AA, DL] | Stored name UTF8 CONTINUATION 33; not universal meaning |
| ISO-8859-1 resolution | [AA, AD, AB, AA, AA] | Table identity exists |
| ISO-8859-2 resolution | [AA, AD, AB, AB, AA] | Table identity exists |
| ¡ U+00A1 | [AA, AB, AA, AL, AB] | ISO-8859-1 ordered component 0 is byte A1 |
| Ą U+0104 | [AA, AB, AA, AA, rQ] | UTF-8 and codepoint_hex components only in inspected atomizations |

Local codec round trips establish A1 -> ¡ under ISO-8859-1 and A1 -> Ą under
ISO-8859-2. The latter relationship is not present among the inspected Ą
atomization rows or its metadata. It must not be silently manufactured as a
stored predicate. No atomization rows have byte A1 as parent. Thus the exact
single-hex component addresses and their reciprocal traversal are not established.
The fixture is evidence-backed but not a complete accepted model fixture yet.

ISO-8859-1 stored weight is 13 (legacy). Current byte mass is 2. Retain stored
weight in raw evidence; never load it as current source mass. Codepoint_hex
rows reference the byte namespace and include zero padding; preserve them as
observations without treating them as approved hex-particle composition.

## Concrete CPU representation proposal

Use compact local handles for the observed records, with an immutable mapping
back to their arrayed canonical addresses. The observed composition edge is:

    literal [AA, AB, AA, AL, AB]
      resolution [AA, AD, AB, AA, AA], ordinal 0
      component [AA, AA, AA, AA, DL]

Its inverse traversal entry contains the same literal, resolution and ordinal;
CPU assembly can derive this from the loaded row without inventing a new
relationship. Do not merge it with another resolution's edge to the same byte.
A two-occurrence A1 A1 test has local occurrence slots 0 and 1 referencing the
same byte handle, mass 4; it does not require minting a database identity.

Keep observed compositions distinct from force-group memberships until the
stored relationship definition is traced. Every model connection is a centroid
grouping, but a loader must not guess which legacy row implements that contract.
Keep missing links flagged and recoverable raw metadata alongside the fixture.

## Storage -> working database -> SNode

| Layer | Carries | Work |
| --- | --- | --- |
| Storage | Exact address/relationship records, context, ordered composition, provenance | Authoritative traversal and recovery |
| CPU assembled working database | Selected records, local handles, occurrence ranges, inverse ranges, defined group memberships, scope/LoD map, aggregate recovery references | Follow explicit links; prepare GPU data and retain excluded sections through aggregates |
| SNode layouts | Compact particles/active points, memberships and defined LoD levels, position/velocity, mass, current/next summaries | Evaluate every applicable relationship vector and reduce calculated endpoints |

An aggregate's recovery reference points to its retained CPU relationship
structure. Multiple storage levels may map into one active aggregate; no storage
prefix truncation alone proves force-group membership. Group levels come from
the actual declared relationships. The incomplete A1 fixture cannot yet provide
an honest numerical group aggregate or complete force test.

The bounded archive scan found prefix reconstruction in
archive/2026-08-rebase/src/hcp/engine/storage.py: fixed prefixes plus stored suffix
columns. This is historical evidence, not an implementation of the current
relative address convention. extraction/token_id.py provides base-50 conversion
and dotted boundary helpers, not a relative codec. Do not replace observed arrayed
addresses with dotted strings or infer a new persisted delta format.

## SNode mechanism and next implementation boundary

The inspected Taichi snode.py supplies activation, activity queries, deactivation
and index rescaling between layouts. These are substrate operations. The HCP
assembly must supply the intended relationships and LoD mapping for kernels to
follow; the synthetic capacity benchmark does not prove that semantic mapping.
Keep runtime ownership persistent. Initial assembly can be synchronous; the later
pre-cache thread belongs behind that same CPU assembly boundary.

Use ordinary gravity mechanics with retained velocity and all applicable vectors.
Do not add sophisticated integrators or force modifications. Patrick identifies
a small tick-discretization correction; its exact form is not recorded. Keep it
separate until the numerical tick implementation reaches it. No mathematics
question is needed to complete this structural mapping.

Next: trace the encoding declarations/forwarding for the missing foundation links,
then implement the smallest native fixture assembler with exact reconstruction
checks. Do not repair the live database or synthesize missing canonical records.
