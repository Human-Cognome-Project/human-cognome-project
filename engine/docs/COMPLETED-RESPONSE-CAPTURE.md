# Completed native response capture

## Scope and source

Implementation source `5ef44d60fce59fd147f005a266f25109cb21abe9`, tree
`24a997b718d433e9d5a291ee6b7088f75312c17f`, starts at main `b528d494`.
This is a bounded observation prerequisite for [#343's fine-confounding and
next-level structural trace](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343#issuecomment-7137040893).
It is independent of the provisionally accepted, unmerged #430. The
[re-review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/430#issuecomment-7145505449)
resolves its preparation-budget finding; hosted native CPU CI is still separate.
Both contributions touch the live CLI help/request lines. Maintainer integration
should retain the independent response option alongside #430's whole-field mode.

`viewport::observe` accepts an independent optional `ResponseRequest`. It uses
`Harness::inspect_live_responses` in the same authorized, synchronous completed
execution as the body report, comparison inventory and optional intent diagnostic.
The owner checks, numerical kernels, admission and motion remain unchanged.
No source acquisition, numerical replay, host solve or new native storage is added.

## Reading the payload

`response_trace` is omitted unless requested. Success includes only the first
native `count` rows, in actual committed response order, plus the prepared native
capacity. Each row preserves the full body claim, child/parent packet slots,
local/inherited displacement kind, exact numerator/denominator as decimal strings,
delta XYZ, recipient XYZ, body mass, source mass, production eligibility and native
extraction validity. `packets` maps actual live packet slots to comparison IDs;
`execution.comparisons` supplies each full field claim and explicit level/precision.
Packet slots can have holes after refresh. Body record slots are not canonical
identities; resolve the full claim and this generation before interpreting them.

`production_eligible` retains the existing MultipleBodies/complement predicate.
It does not classify semantic structural versus confounding contributions. Local
and inherited are native displacement/path kinds. Nonfinite operands serialize as
null with the native extraction flag retained; finite extraction does not certify
positive mass, inverse-power arithmetic or a valid combined target. Exact coincidence
is retained. Contact is absent from this existing response surface.

The completed generation, tick and operative-frame anchor must match the same
snapshot. Native borrowed arrays are copied before optional intent inspection can
invalidate them. Stale capture metadata is rejected. The browser validates full
recipient claims, packet/comparison mappings, decimal ranges, row counts, validity
and work/readback accounting before accepting a new capture.

The existing `intent` candidate keeps its recorded diagnostic meaning. This change
does not label it a structural or confounded target. Those meanings require the
[composition/native binding trace](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324#issuecomment-7135833675)
and native arithmetic correspondence. Brake reference is already settled by
[the later direction](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324#issuecomment-7135937435);
this observation does not select another reference or change braking.

## Operator and accounting

With `HCP_WITH_VIEWPORT=ON`:

```sh
. engine/build/engine-env.sh
engine/build/viewport/viewport_live_study \
  --response-work-limit 18446744073709551615 \
  --commands tools/viewport/examples/live-study.commands > /tmp/responses.jsonl
```

This first operator remains the existing in-memory English-pair fixture, baseline
physical/source mass two. It creates, ticks, adds, ticks, removes and ticks one
prepared study. Use `--response-work-limit 0` for explicit observation refusal;
physical operations and the independent intent request continue normally.
The C++ owner observer can request responses without intent, or both separately.
Legacy flat sessions report unavailable with null prepared generation.

For native row count N and prepared response capacity R, the existing native
query charges `1 + N + 16*R`, limited by both the caller and CascadeWork, and
requests `112*R` readback bytes. A short limit returns `budget_refused` with no
rows, packet table, used work or readback. Old borrowed payload is never copied
on failure. No general host-serialization time/instruction bound follows: JSON,
identity maps and detached report allocation remain separately requested observer
work. Floating numeric-copy accounting adds eight doubles per copied live row;
native readback still includes capacity padding. The two copied flags become JSON
booleans. Full B, allocation policy and native resource admission are unchanged.

## Qualification

Contributor-local source qualification uses the clean public implementation pin
above, Ubuntu 24.04, LLVM/Clang 15.0.7, GCC 11.5.0 and actual native CPU. Taichi tree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648` and runtime bitcode SHA256
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`
are retained with exact submodule, configuration, executable and command pins.

- Quick local CI passes its three CTests, placement math/capacity controls,
  native build and actual-backend check.
- Nine Clang CTests pass: four one/two-worker bulk/direct live-operator profiles,
  four prepared-owner observer profiles and the snapshot suite. Four GCC
  prepared-owner profiles pass. Together these are **16 CTests**.
- Native assertions compare every copied response to the actual owner extraction:
  row order, full claims, packet/comparison association, exact fractions, coordinates,
  masses and both flags. They cover response-only and simultaneous intent/response,
  both binding overloads, pre/post tick and contact-driven generation refresh,
  exact/one-short/zero work, refusal without stale rows, legacy unavailable, unchanged
  physical state/readback accounting and stale metadata rejection. The simultaneous
  request also checks copied response survival after the subsequent intent query.
- Eighteen model tests and the locked bundle rebuild pass. Eight successful and
  eight zero-budget CLI captures validate against the schema and complete the same
  create/tick/add/tick/remove/tick sequence within each study. The refused trace has
  no rows while intent remains available and later physical commands succeed.
- Direct-file Chromium 141.0.7390.37 passes 128 existing assertions and 19 assertions
  importing all eight real native response captures, preserving the intent payload,
  refusing stale response metadata without replacing the current capture, and
  checking zero external requests/page errors. Browser software WebGL is separate
  from native CPU execution. Null-invalid/sparse-packet and >2^53 decimal reader
  cases are synthetic validation controls, not new native numerical qualification.

| Captured state | Tick / generation | Bodies | Responses / capacity | Native work | Readback bytes |
| --- | --- | ---: | ---: | ---: | ---: |
| Initial | 0 / 1 | 2 | 4 / 64 | 1,029 | 7,168 |
| Add entry 20 | 1 / 2 | 3 | 6 / 64 | 1,031 | 7,168 |
| Remove entry 20 | 2 / 3 | 2 | 4 / 64 | 1,029 | 7,168 |
| Following tick | 3 / 3 | 2 | 4 / 64 | 1,029 | 7,168 |

The [final native example](../../tools/viewport/examples/response-study.json)
retains two eligible local rows and two ineligible inherited rows with zero delta.
That equal-cohort witness establishes extraction, not transmitted cross-language
confounding. The candidate retains its diagnostic label and actual motion remains
in the body report.

Raw local evidence is `/tmp/hcp-response-5ef44d60-evidence.tar.gz`, SHA256
`bb2381017611b29c573daabc96e38f30c0782bc6862a83bfcc55dda7e65cd86a`.
Its manifest, nested manifests and a fresh archive extraction were personally
verified. It includes source bundle/patch, clean-source guards, full logs, configs,
compiler/runtime/executable hashes, both full CLI captures, capture/browser audits
and the initial browser sandbox refusal. The outside-sandbox offline rerun passes;
missing browser libraries were extracted under /tmp, with no system installation.
Initial Node/npm setup failures and native offline-cache lock warnings are retained;
no cache cleanup was performed. The local archive hash does not make those raw logs
remotely available; the committed example is portable evidence.

This is local CPU observation evidence for maintainer review. Hosted CI is
separate and authoritative. No GPU/concurrency, database acquisition, performance,
full B, allocation policy, general LoD or new target/brake behavior is qualified.

## Remaining capability

Continue with the English A/French A semantic parent rows through projection,
explicit roles, native bindings and published fields. Compare identical pre-tick
structures with imposed cross-language influence off/on, retaining structural
inputs and targets, and measure the actual transmitted contribution separately
from shared parentage. Include multibyte Unicode and ordered/repeated constituents;
preserve ordinals and count fine-to-coarse structural rollup once. Then establish
native target arithmetic and observe motion in the same study. This raw trace is
an observation prerequisite; it does not complete that worked example, broaden
#430's English single-byte owner profile, or establish linguistic settling.
