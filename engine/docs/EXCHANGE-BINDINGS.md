# Private committed-source bindings for exchange

This coordinator prerequisite connects the [logical declaration](EXCHANGE-CONTRACT.md)
to a generation-bound view of committed prepared source metadata. It does not
call replacement staging, read physical mass/motion, claim records or publish.

`assembly::detail::ExchangeBindings` has a private preparation constructor and
private validation method. Only the future private coordinator and the test
adapter can access them. Preparation consumes a validated projection, its exchange
contract, and trusted occurrence/field mappings from committed admission results.
These mappings are **not accepted from an exchange request**. Their initial
identity association is an owner responsibility: structurally identical physical
bodies cannot reveal which logical copy they represent. The sealed view prevents
later requests from choosing another copy's handles; it does not infer identity
from numerical equality or claim to authenticate an arbitrary caller's mapping.

The active occurrence inventory must be complete for this bounded execution:
every projected active occurrence maps once to a distinct current full handle,
and its exact source mass matches. Inactive alternatives, foreign studies,
missing copies and duplicate mappings reject. Field names and physical field
handles cannot alias. Both maps validate context, address, record and incarnation
against the committed source headers. The contract's outgoing alternative must
be exposed in that inventory. Native kind/layout/scale installation is not
inferred from a record number; the initial trusted owner must establish it from
the projection's node bindings. This view validates source identity/accounting,
not the complete physical installation.

For each outgoing occurrence, preparation requires all declared sibling,
ordered direct-parent and inherited membership contributions in the committed
participation ledger, with exact integer counts. Repeated contributions occupy
separate source rows. No additional incident participation is allowed. It retains
the actual source-row identity/ordinal, both full endpoints, count and share;
definition-parent ordinals and native ledger row ordinals remain distinct.
It does not create or deduplicate a new hierarchy.

## Revalidation and lifetime

Validation checks owner readiness, the expected and committed generation, current
full body/field handles, exact source masses and the captured outgoing source
rows. An additional row newly incident on an outgoing record rejects even when
the total edge count is unchanged. Missing source rows, altered counts/shares,
changed endpoints/order and stale physical incarnations reject. Another source
generation invalidates the entire view, including changes to unrelated objects;
updating bindings coherently at publication remains coordinator work.

The check performs no allocation, kernel launch, synchronization, upload or
numerical readback, and writes no source, activity, history or result state.
Ordinary rejection leaves execution usable. An already unready owner is refused.
The diagnostic `UpdateResult` is not a reusable authorization token: only an
immediate coordinator operation under exclusive ownership may consume success.
The prepared harness must outlive the view.

The declaration has shared const ownership. The standalone preparation adapter
moves its supplied declaration into owned immutable storage; the persistent
exchange owner shares an adoption-retained declaration instead. Neither path
borrows a temporary declaration or changes its descriptors at publication. Live
binding validation remains generation-bound even though immutable declarations
can be selected for later operations. Binding maps, masks and membership
enumeration still allocate during preparation.

The conservative validation reservation is
`16 * (bound_occurrences + bound_fields + captured_edges + live_edges + 1)`
structural units, capped by the prepared `PlacementWork` limit before scanning.
Preparation rejects overflow. Host vectors for identity maps, source rows and
record/edge masks are allocated during preparation, not during validation. They
are coordinator metadata outside the harness's existing scratch manifest;
integration into an enduring owner's total resource accounting remains pending.
The backing-reuse tests qualify validation, not the complete allocation budget
of an operational owner.

## Limits and following work

The following [atomic coordinator](ATOMIC-EXCHANGE.md) consumes this sealed view,
generates declared incoming participation and connects native accounting to
prospective validation and publication. It retains actual-claim occurrence
mappings on success; the old source view stays invalidated, not silently rebound.
That private single-transaction integration does not provide an enduring owner.

This view alone checks committed **exact source metadata**, not resident physical mass,
velocity or native mass-ratio array contents. Native mass/momentum reconciliation
and actual replacement centre/offset reporting are supplied by
[native accounting](EXCHANGE-ACCOUNTING.md). Incoming
commands, prospective source/hierarchy/contact state, old-plus-new claims and
atomic unticked replacement/retirement publication belong to the coordinator,
not this source-view class.
The existing numerical staging and all numerical kernels are unchanged.
An enduring owner must establish and update these bindings from its own commits;
this class is not an operational initialization or rebind API.

`exchange_bindings` runs separately or within the combined placement suite. It
uses an installed single-root hierarchy with two aggregate copies and a byte
alternative, nonidentity record/field locations, sleepers and retained history.
It checks both declaration directions, exact inherited memberships, all full
handle components, stale generation/records, changed source metadata, additional
incident edges, work exhaustion, unchanged retry, 32 allocation-free validations,
stable prepared backing and refusal after an injected execution failure.
See [local evidence](../tests/admission/README.md) for source pins and profiles.
