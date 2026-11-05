# Private atomic representation exchange

`assembly::detail::ExchangeCoordinator` connects the trusted generation-bound
[source view](EXCHANGE-BINDINGS.md), [native accounting](EXCHANGE-ACCOUNTING.md),
and existing private generated admission transaction. It replaces the declared
alternative and retires its outgoing bodies in one unticked generation. This is
a bounded private transaction, not an operational owner or browser command API.

## Preparation and authority

Preparation consumes an owned `ExchangeBindings` view and one trusted incoming
definition/layout/intrinsic binding per declared incoming occurrence. Each binding
must match the exact occurrence, pinned definition and projected node. The preparing
owner remains responsible for native kind/scale and complete geometry binding;
arbitrary application requests cannot manufacture these associations. The class's
constructor, hierarchy sealing and execution are private; the executable tests use
a dedicated friend adapter. No public bind/rebind or exchange API is added.

The coordinator generates commands itself. It does not accept a caller-authored
participation diff. Incoming exact mass comes from the immutable declaration.
Each incoming body receives its sibling-definition contribution, every ordered
direct-parent contribution and each distinct inherited membership. The binding
view already checked the outgoing occurrence's exact inherited membership set.
Counts and native share metadata are derived from those exact source values.
Repeated ordered parents remain separate ledger entries. Definitions precede
other commands in declaration order, fixing the pending-handle indices.

The plan retires exactly the sealed outgoing handles, removes their captured
source rows and removes source contacts incident on them. Unrelated source rows,
contacts and occurrence mappings are retained. All incoming fields must already
have trusted committed mappings. This increment neither creates nor retires field
families, changes focus, nor invents a hierarchy. The owner supplies a prospective
hierarchy using existing fields and the generated command indices; preparation
copies and seals it within the configured node/link/reference limits. Native
preflight subsequently validates its complete prospective source coverage.

In particular, compressing the last participant out of a retained field does not
make an empty hierarchy node valid. Such a plan rejects. Field-family lifecycle
and the associated focus/hierarchy policies remain follow-on owner integration.
The witnesses retain independent aggregate and byte copies so both tested
directions preserve a valid prepared field inventory.

## Exclusive transaction

Execution takes only expected generation and work/cell limits. Under exclusive
ownership it revalidates the sealed source view, reserves coordinator work, and
calls the existing private admission boundary in exchange mode. That boundary:

1. Reduces outgoing native state, reconciles source/physical mass and generates
   certified nearest-free replacement starts with full-f64 transferred motion.
2. Checks the native momentum bound and computes actual centre/offset diagnostics.
3. Validates the prospective source ledger, hierarchy and complete contact union.
   Old-plus-new particle/reference claims remain reserved; retirement cannot fund
   replacement allocation, even when the final population fits.
4. Claims actual native records, remaps source/hierarchy/contact destinations,
   publishes numerical state and topology, retires old records and advances the
   generation exactly once. It does not execute a tick.

After that transaction succeeds, the coordinator reads the eight-double outgoing
and 32-double balance reports, then fills its preallocated incoming mapping entries
from the actual resolved native claims. A vector swap publishes the complete new
occurrence map, followed by its generation and descriptive reports. Surviving
logical identities and full handles remain unchanged. No fallible preparation or
allocation follows these fixed-size assignments. All steps remain inside the same
exclusive operation; no caller can observe a healthy intermediate generation.

Reports describe the completed exchange. They never supply numerical input and
must not be interpreted as current state after subsequent execution. Their layout
and floating bounds are documented in EXCHANGE-ACCOUNTING.md. This adds 320 bytes
of diagnostic readback per successful exchange to that transaction's accounting.

Ordinary stale/invalid/capacity/numerical rejection occurs before claims, preserves
live state/history/mappings and allows retry of the sealed plan. Once claims begin,
an unexpected failure leaves the harness unready. The same rule applies if report
readback fails or a failure is injected immediately before mapping publication:
old mappings are not advertised as a healthy new generation. There is no rollback
or recovery claim for partially published execution.

Success consumes the prepared transaction; replay rejects as stale. The former
generation-bound source view is not silently rebound. The committed occurrence
map is retained for the future owner's next preparation. Sustained preparation,
projection exposure updates, mapping lifecycle across unrelated operations and
recovery/restart belong to the enduring owner.

## Resources and qualification

Host preparation owns bounded command/intrinsic arrays, an alternate occurrence
map, copied hierarchy arrays and fixed diagnostic reports. Runtime checks allocate
no new vectors. Coordinator reservation is the source-view reservation plus
`16*(next_occurrences+1)` structural units, deducted from PlacementWork before
placement. Existing source/hierarchy/contact work and peak checks still apply.
These host preparation objects remain outside the harness scratch manifest;
complete enduring-owner resource accounting is not qualified here. Existing native
kernels and their prepared backing are reused unchanged.

`exchange_commit`, also included in the combined placement suite, exercises a
two-byte letter in both directions, independent copies, nonidentity records/fields,
full/reduced position precision, source revalidation, invalid incoming identity,
invalid prospective hierarchy, work and old-plus-new claim exhaustion, prospective
contact exhaustion, unchanged rejection/retry, transferred motion, actual mapping
resolution, survivor numerical preservation, no tick advance, following ticks,
stable native backing and no observed recurring C++ allocation. Injected failures
after claims, numerical writes, contact publication and before mapping publication
must all leave execution unready. Earlier empty-field compression fixtures were
correctly rejected and their failed logs are separate from passing evidence.

See [source-pinned local evidence](../tests/admission/README.md). CPU execution does
not qualify GPU behavior. Full operational controls, field-family lifecycle and
the integrated local study remain separate work.
