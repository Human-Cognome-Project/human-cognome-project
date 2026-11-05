# Prepared logical declaration for letter exchange

This is the structural first increment of the private D7 coordinator. It builds
an immutable declaration from the existing validated v2 `LetterProjection`;
it does not bind native handles, call placement or publish an exchange.
The [continuation plan](../../docs/core-local-study-continuation-plan.md#next-agent-task--2026-11-05)
still owns the remaining coordinator and operation acceptance.

`declare_letter_exchange` requires the exact study/aggregate occurrence key and
the other declared exposure. It rejects foreign studies, byte-path owners,
unknown occurrences, same-alternative requests, unknown exposure values and
legacy v1 recipes. There is no implicit conversion of recipe starts or definition
lookup outside the supplied projection. The current exposure here is the
projection's declaration, not an observation of a running native owner.

The contract retains the recipe revision and separate canonical outgoing and
incoming descriptors: occurrence key, pinned definition revision, exact integer
mass, study-node binding and ordered direct parent rows, including stored source
mass provenance. The existing projection validates letter source form through
character/byte/nibble construction. Preparation records every ordered nibble
contribution with character and byte ordinals, and checks the aggregate/parts
integer mass totals with overflow rejection. Repeated bytes and repeated nibble
definitions remain distinct occurrences/contributions; they are not deduplicated.
Direct aggregate parent fields and byte parent fields are deliberately distinct.
The leaf declaration is common construction provenance, not a claim that those
different live field ledgers are interchangeable.

`matches` checks complete outgoing/incoming descriptor bijections while allowing
adapter row permutations. It rejects duplicate/missing/foreign copies, changed
definition revisions, masses, scale bindings or parent rows. Matching allocates
nothing and mutates neither declaration nor projection. The caller must reserve
at least `match_work()` structural comparison units; preparation computes the
conservative sum `n * (8 + 4 * parent_count)` across each side's descriptors,
rejecting overflow. This is a bounded metadata comparison count, not a timing or
device-work estimate. Construction and vectors belong to preparation, not an
ordinary native transaction's allocation-free path.

## Remaining authority boundary

A successful descriptor comparison is **not live exchange authority**. A caller
can supply structurally correct descriptors without owning the corresponding
physical records. The next coordinator increment must bind these keys to the
owner's committed occurrence-to-full-handle ledger and expected source generation,
verify current exposure and exact native source/mass contributions, and reject
stale/foreign/mismatched bindings before calling private replacement staging.
No address, native record number or engine dependency is introduced into the
engine-independent projection library by this increment.

Native momentum checks, actual replacement centre/initialization-offset reporting,
prospective hierarchy/contact validation, old-plus-new capacity and one atomic
unticked replacement/retirement publication remain unbuilt. The existing
[native scratch staging](EXCHANGE-PLACEMENT.md) is unchanged. The enduring
operational owner and browser controls remain later integration work.

## Tests

The existing `structural_recipe_test` now exercises both directions, multiple
copies, repeated and multibyte constructions, all rejection categories above,
comparison work exhaustion and unchanged projection state. The existing hosted
and standalone projection builds discover it without adding a new test index.
Local commands, source pins and limitations are recorded in
[admission evidence](../tests/admission/README.md). These are host structural
checks, not Taichi numerical execution or GPU qualification.
