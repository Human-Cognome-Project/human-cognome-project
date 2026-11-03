# Native admission implementation work

Status: **in progress**, following accepted B2 (#203) and local CI tooling
(#204). The [accepted admission plan](../../docs/native-admission-contact-transaction-plan.md)
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

## Remaining B3 work

Connect generated rows to one prospective source/hierarchy/contact validation
and commit boundary, bypassing fixture initialization without bypassing any
preflight checks. Add complete first admission under BootstrapCommands, explicit
prospective focal selection and D1 partial-focus/anchor placement. Validate the
candidate contact union and peak references before native claims. Publish source,
hierarchy/history, contacts, focus and activity together once, with no tick advance.
Exercise capacity/work rejection, healthy retry, survivor/history preservation,
late-failure unready disposition and ticks immediately before/after admission.

The enduring owner and operational Add/Remove/browser control remain later
continuation slices. Do not wire these controls to a fixture reset or expose
this private staging helper as a completed admission command.
