# Private exchange field lifecycle

The [atomic coordinator](ATOMIC-EXCHANGE.md) now includes field creation and
retirement in the same prepared transaction as body replacement. It no longer
requires an independent copy to keep an outgoing alternative's fields populated.
This is still a consumed, generation-bound private plan, not an enduring owner.

## Sealed field plan

Preparation requires a complete trusted field inventory: every live field has
one logical key/full native handle mapping. Initial identity and native kind/layout
association remain the preparing owner's responsibility. A request cannot infer
logical identity from a physical record number.

The owner supplies a native kind/address binding for each missing incoming field
using `ExchangeNewField`. Logical demand is derived from the declared incoming
sibling, ordered-parent and inherited membership participation. Missing fields,
duplicate keys, already committed keys and unused extra definitions reject during
preparation. Existing fields are reused. The owner cannot add an unrelated field
through this plan. Address/range/alias validation remains in native source
preflight, before claims. New field definitions follow the incoming body definitions
in the sealed command list; pending field references name those command indices.

After generating the source diff, preparation examines each committed field. Only
a field touched by outgoing source removal is eligible for retirement, and only
when no retained or incoming participation uses it. A second occurrence's use keeps
the field alive. Unrelated empty fields are not opportunistically deleted. All
retained fields keep their full handles and logical keys.

Field retirement does not implicitly change focus. If an eligible field is focal,
ordinary native source preflight rejects the complete operation unchanged. An
explicit, separately accepted focus operation is required before preparing a new
exchange. A retained focal membership field remains installed across exchange.

The owner still supplies a complete prospective hierarchy, naming new fields with
`LiveRefreshNode::field_command` and excluding retired fields. The existing native
validator checks prospective ownership, nonempty source coverage, references and
contacts. The coordinator does not infer semantic hierarchy or scale policy.

## Publication and resources

The native transaction reserves old-plus-new **field** capacity as well as body
and reference peaks. A final seven-field model may require nine simultaneous field
claims; retiring two old fields cannot finance six new claims. No outgoing address
or record can be reused for a replacement before retirement completes.

Actual resolved field claims populate a prepared field map beside the prepared
occurrence map. Both maps swap under the same exclusive operation, followed by
their shared generation and descriptive reports. The existing failure boundary
applies: ordinary rejection preserves execution and both maps; an unexpected late
failure leaves execution unready. Failure immediately before map publication keeps
both old maps, never a healthy partial new owner. Successful plans are consumed.

Host preparation adds an alternate field-map vector and a parallel command-index
vector. Runtime publication fills handles and swaps existing vectors without
allocation. Coordinator work becomes source-view work plus
`16*(next_occurrences + next_fields + 1)`, deducted before placement. Native
numerical arrays, kernels and binding counts are unchanged. These host objects
remain outside the harness scratch manifest; complete enduring-owner resource
accounting and reusable preparation are still separate work.

## Executable scope

`exchange_fields` is included in the combined placement suite. It uses one
two-byte letter, with either only its aggregate or only its byte alternative
initially exposed. Expansion creates six byte fields and retires two aggregate
fields; compression creates two aggregate fields and retires six byte fields.
Both retain the membership/focal field. Tests cover exact incoming field demand,
duplicate/missing/extra field rejection, old-plus-new field exhaustion, focal
retirement rejection, ordinary retry, actual native handles, no outgoing claim
reuse, coherent field/occurrence generation, late unready behavior, stable native
backing, no observed recurring C++ allocation and a following tick. Existing
`exchange_commit` independently checks that fields shared with other copies stay
alive and that body/source/contact behavior is preserved.

[Local evidence](../tests/admission/README.md) records tested source and modes.
The separate [private exchange owner](EXCHANGE-OWNER.md) updates projection
exposure across subsequent exchanges. This field transaction alone does not
establish a reusable operational owner, account for all owner resources, add
browser controls or qualify GPU execution.
