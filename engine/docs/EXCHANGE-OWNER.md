# Persistent private exchange owner

`assembly::detail::ExchangeOwner` retains one logical study across successive
aggregate/parts exchanges on a borrowed prepared `field::Harness`. The harness
must outlive the owner, and all owner and harness calls must be exclusive. This
is a private exchange-lifecycle increment of continuation slice C, not the full
operational owner or a public Add/control API.

## Identity and live projection

Adoption takes a structural-v2 `LetterProjection` and complete trusted occurrence
and field mappings from actual admission claims. It validates current handles,
exact masses and every selected occurrence's participation against the prepared
execution. It cannot infer logical identity from numeric values or certify a
caller-supplied layout association. The private adapter retains that trust boundary.

The study UUID, occurrence keys, definitions and pinned recipe remain stable.
`LetterProjection::recipe()` still returns the immutable initial recipe; its
selection exposure is not current live state. `exposure(entry)` returns current
exposure. Active occurrence flags, parent contributions and the structural binding's
selection exposure follow that live view. Inherited memberships derived from the
projection consequently follow the current alternative and exact source masses.
Exchange declarations use the current exposure while their alternatives still
come from the immutable recipe. No new projection UUID is minted between exchanges.

## Preparation and publication

Preparation constructs a prospective projection and a fresh generation-bound
[coordinator](ATOMIC-EXCHANGE.md), using the owner's current complete maps.
Incoming layout/geometry bindings and the prospective hierarchy are still supplied
by trusted preparation. Preparation and hierarchy sealing do not publish any live
projection or mapping. A new preparation discards previous pending intent; a
preparation exception preserves committed identity, mappings and last report.

Execution reserves 64 PlacementWork units for owner publication before delegating
the remaining work to the existing coordinator. The native transaction still
validates old-plus-new claims, conservation, placement, source, contacts and
hierarchy. After its success, no-throw vector and pointer swaps publish occurrence
and field mappings with the prospective projection, followed by fixed-size report
and generation assignments. All publication is inside the same exclusive boundary.
There is no intermediate healthy owner visible to another caller.

Ordinary rejection leaves the committed owner and execution unchanged and permits
retry of the same prepared operation. Success consumes that plan; the owner can
prepare another operation against the newly committed projection and actual native
handles. Last reports remain descriptions of a completed exchange, never numerical
input and never a claim of current state after ticks. An unexpected failure after
native mutation, including injected failure point 8 before owner publication,
leaves the harness unready and the previous owner snapshot intact. This is not
rollback or recovery of a partially published execution.

Ticks with unchanged source generation remain compatible. An unrelated operation
that advances the source generation makes this owner stale; it does not silently
rebind. Integrating admission, removal/reuse, focus/rebase and restart through the
same owner is subsequent work.

## Resource boundary and evidence

Preparation allocates/copies host projection, command, mapping and hierarchy
metadata and releases the preceding plan. The old projection and mappings remain
retained after successful publication until the next preparation or owner teardown.
The complete recipe is currently copied with the prospective projection. These
surfaces are outside the native scratch manifest: allocation-free reusable
preparation and complete peak/lifetime accounting are **not qualified** by this
increment. No new native numerical kernel or backing array is introduced.

`exchange_owner`, also included in the combined placement matrix, alternates 32
exchanges of one two-byte letter on one execution, with a tick after each. It checks
stable identity/immutable recipe, current structural exposure and contributions,
actual handles, retained focus, report/mapping publication, consumed-plan rejection,
unchanged stale/work/ordinary rejection and retry, and late unready behavior.
Native backing observation and C++ `new` guards cover successful execution, ordinary
rejection and ticks; host preparation and exception construction are excluded.
Independent-copy preservation remains separately exercised by `exchange_commit`;
the new repeated-owner fixture has one logical occurrence.

[Local evidence](../tests/admission/README.md) records source and configurations.
This CPU scope does not qualify GPU behavior, all operation rows, full owner
resources, browser control or the integrated local study.
