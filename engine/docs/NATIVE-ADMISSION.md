# Native admission implementation work

Status: **private generated-row staging accepted in #205 and source/hierarchy
preflight accepted in #207**; complete B3 remains
in progress, following accepted B2 (#203) and local CI tooling (#204). The [accepted admission plan](../../docs/native-admission-contact-transaction-plan.md)
governs B3. No operational admission API is implemented by the initial increment.

## Native generated-row staging

`detail::Placement::stage` is private to the engine transaction boundary and
test access. It uses the same source preflight, occupancy capture and certified
placement solver as the public B2 query, but does not publish a query token or
replace its diagnostic output. Native initialization copies certified coordinates
directly from candidate arrays into the prepared `new_rows_array`, remapping
canonical placement rank to original command index. Mass and radius come from
validated intrinsic inputs. Velocity, force and determiner entries start at zero;
unused command rows are cleared on each successful stage.

Positions never pass through a host readback/re-upload initialization path.
The host carries structural command indices and intrinsic inputs only. The
additional command-index column, kernel bindings and bounded initialization work
are included in prepared storage/work accounting. The kernel is compiled and
warmed during preparation; attempts do not allocate or compile it. Emitted-code
ownership checks include the new kernel and reject omitted scratch output.

Staging has no authority to claim records, install hierarchy, change activity,
advance generation/ticks or publish a model. It retains the current-focus D2
candidate semantics and ordinary Batch limit. A failed stage must never be
consumed as a transaction: scratch has no independent validity/identity token.
Existing public update/refresh guards remain unchanged, and fixture starts stay
private to their existing test adapter.

## Prospective source and hierarchy preflight

The private `Harness::stage_current_admission` coordinator immediately follows
successful native placement with the existing source/hierarchy preflight. It
requires an installed hierarchy, retains the ordinary Batch limit and stages
the same commands against the same generation. `GeneratedPreflight` consumes
the native start array without uploading the fixture host rows over it. It
validates the prospective source, hierarchy ownership/coverage and numerical
repair, then returns before the first native record claim.

Only this private producer-to-consumer call uses that mode. Public update and
refresh still use fixture validation and cannot consume generated starts left
in scratch. Preflight success changes neither the installed hierarchy nor
generation, ticks, survivor physical/history state or existing observations.
Capacity, incomplete-hierarchy and late preclaim rejections remain typed and
permit retry. No candidate token or admission-ready guarantee is exposed.

## Private prospective contacts

The coordinator now follows successful source/hierarchy numerical validation
with `detail::AdmissionContacts`. Native copying uses the checked provisional
row states: survivors retain their actual resident values, new rows read the
generated start array, and outgoing/unused rows are excluded. Compaction includes
sleepers. This is bounded geometry preparation over physical capacity, not an
awake-work selector or potential-address scan. Host uploads contain only checked
structural contact endpoints; numerical positions are never replayed on the host.

The shared geometry builders retain the existing contact predicate, explicit
source ordering/orientation, duplicate source mappings, canonical derived-only
order and both adjacency directions. The complete union is discovered before
capacity rejection, including source-empty overlap. Tangency remains admissible
and produces no derived contact under the existing predicate. Prospective
response and leaf ownership remain validated by the preceding hierarchy stage.

The private contact arrays and bindings are allocated, compiled and warmed during
preparation only for placement with live responses. Their storage and measured
native metadata join the existing placement reservation. GeometryWork reserves
`192*max(P,1) + 128*max(C,1) + 32` conservative preparation/remap/publication
visits, then `n*(n-1)/2 + source_count^2 + 2*union_count^2` comparisons. Both
provisional and resolved finalization are reserved before claims. Finalization runs only
after its work reservation succeeds. Reference demand adds twice the installed
union plus twice the prospective union to the existing simultaneous source/tree
refresh peak. Outgoing claims do not fund new claims.

Contact/reference/work exhaustion returns before claims, preserving published
geometry and placement observations, activity, history, generation and ticks.
`UpdateResult` reports attempted contact pairs/work; those counts confer no
publication authority. Scratch is attempt-local, with no reusable admission
token. The ordinary commit resolves provisional row names as described below.
The compiler audit covers scratch ownership and guarded publication of the
seven contact kernels. CPU evidence does not qualify any GPU backend.

## Private ordinary combined commit

The coordinator's private commit mode requires an installed hierarchy and the
contact-safe tick owner. It keeps the ordinary Batch limit and genuine-new-body
D2/D3 semantics; it does not implement initial bootstrap or representation
exchange. `GeneratedCommit` follows the same native placement and complete
source/hierarchy/contact preflight as `GeneratedPreflight`, then uses the
existing native claim, numerical/hierarchy repair and retirement boundary.

Returned claims must have the expected map context, address, live incarnation
and unused in-range record destination. The complete surviving/new destination
map must be one-to-one. Invalid native results after claims make the execution
unready. The shared CascadeWork reservation includes bounded verification work.

The native remapper translates contact endpoints through that verified map,
preserving explicit orientation and restoring the original source-to-temporary
mapping. Finalization reorders derived pairs by actual canonical endpoint keys
and rebuilds both adjacency directions. It cannot treat provisional row order
as final allocator order. An additional C-sized source map and the fixed native
bindings are reserved at preparation.

Native CHI kernel cache keys use the kernel name. Helpers with fixed capacities
or embedded SNode identities therefore use a prepared-owner suffix; the actual
endpoint ordering variant has its own suffix as well. Sharing a generic name
across owners or different ordering IR is not valid kernel reuse.

After retirement, guarded native publication copies the resolved union,
flags, source map, adjacency and counters into the tick owner's installed view.
Changed union rows wake their old/new surviving endpoints through the existing
native particle activity set; released bodies cannot be reactivated. Existing
source/hierarchy publication owns focus, field history and structural wakes.
The exclusive boundary finishes with one generation and hierarchy stamp update,
without advancing completed ticks. A following contact-safe tick consumes that
installed union. Earlier query tokens become stale with the generation change.

No healthy rejection remains after native claims. Injected failures after claims
or contact publication leave the execution unready, with no rollback claim.
Fixture update/refresh retain their original route. This remains private engine
integration, not an operational owner or public Add API.

## Remaining B3 work

Add complete first admission under BootstrapCommands, explicit
prospective focal selection and D1 partial-focus/anchor placement. Validate the
first-admission contact union and peak references before native claims. Publish source,
hierarchy/history, contacts, focus and activity together once, with no tick advance.
Exercise capacity/work rejection, healthy retry, survivor/history preservation,
late-failure unready disposition and ticks immediately before/after admission.

The enduring owner and operational Add/Remove/browser control remain later
continuation slices. Do not wire these controls to a fixture reset or expose
this private staging helper as a completed admission command.
