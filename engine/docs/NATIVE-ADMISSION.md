# Native admission implementation work

Status: **private generated-row staging accepted in #205**; complete B3 remains
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
in scratch. Candidate success changes neither the installed hierarchy nor
generation, ticks, survivor physical/history state or existing observations.
Capacity, incomplete-hierarchy and late preclaim rejections remain typed and
permit retry. No candidate token or admission-ready guarantee is exposed: the
prospective contact union has not yet been validated.

## Remaining B3 work

Extend the prospective source/hierarchy path with contact validation and one
commit boundary, without bypassing any preflight checks. Add complete first admission under BootstrapCommands, explicit
prospective focal selection and D1 partial-focus/anchor placement. Validate the
candidate contact union and peak references before native claims. Publish source,
hierarchy/history, contacts, focus and activity together once, with no tick advance.
Exercise capacity/work rejection, healthy retry, survivor/history preservation,
late-failure unready disposition and ticks immediately before/after admission.

The enduring owner and operational Add/Remove/browser control remain later
continuation slices. Do not wire these controls to a fixture reset or expose
this private staging helper as a completed admission command.
