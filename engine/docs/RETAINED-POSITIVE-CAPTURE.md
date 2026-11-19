# Retained English pair: acquired source to native tick

Status: **provisionally accepted bounded CPU slice; authoritative hosted CI pending** under #343. [Maintainer review 6990227900](https://github.com/Human-Cognome-Project/human-cognome-project/pull/376#issuecomment-6990227900) accepts `9fbf11ac` provisionally and requests the wrapper-attribution correction made below. [Maintainer decision 6989241451](https://github.com/Human-Cognome-Project/human-cognome-project/pull/375#issuecomment-6989241451) provisionally accepts the [comparison/focus/resource decision](RETAINED-MEMBERSHIP-DECISION.md) and authorizes this separate preset. Tested source is `e86b588e16dfa1f5429d10f5955f786f40032b07`, tree `4b81ee8eb447b87c848a4049bc0f9240c6518d59`, stacked on reviewed #375 head `eea1314b` after main `22ea6d55` integrated #374. All merges remain maintainer operations.

## Explicit producer preset

Select `viewport_letter_capture OUTPUT --study prepared-retained-english-pair --backend cpu --ticks 1 --cpu-threads 1 --intent-work-limit 1000000`. Supply the three existing connection settings: retained core/Linguistics connections must be read-only; warm must be a **new, dedicated qualification database**, initialized with `schema.sql` and explicitly migrated with `002-structural-recipes.sql`. The producer does not migrate automatically. For direct `viewport_letter_capture` use, a fresh dedicated store is a caller precondition: the producer checks schema v2 and calls `Administrator::configure`. The [check_retained_positive_capture.sh wrapper](../scripts/check_retained_positive_capture.sh) enforces freshness before starting the producer.

The preset validates the complete pinned A → English/Alphabet diamond and its Germanic/Indo-European closure. It preserves definition revisions, epochs, parent rows and all four NULL naming masses. Two aggregate occurrences (entries 2 and 10) use physical mass 1.5 each, exact source mass 2, centred radius 0.125, one pointer-free conceptual root with units 1, and ParentValue of the pinned byte as sole focus. The six caller-selected comparisons are SiblingDefinition → ParentValue → Alphabet → English → Germanic → Indo-European, IDs 100–105, labels 0/1 and 52 retained fraction bits. Fields/Nodes/Levels are 6 and Links 5; other documented finite bounds remain. Recomputed scratch is 272358340 bytes; metadata reservations remain 16 MiB bytes and 16 MiB cumulative allocation work.

Publication/acquisition still runs through the existing shared warm manager; `submit_prepared`, native owner/tick and the existing observer remain the execution route. The acceptance checker only reads exported evidence. It cannot supply admission inputs, install hierarchy or mutate a database. Default legacy capture and `prepared-english-pair` retain their behavior; the latter's unsupported-membership refusal remains executable beside the positive case.

## Actual retained qualification

The source-pinned run uses PostgreSQL 16.15, the existing isolated restores of both verified 2026-10-13 payloads, SELECT-only cold connections with read-only defaults, and a newly created warm v2 store. No original service/store is accessed. All created databases are retained. Only this fixture cluster is started/stopped.

The producer acquires recipe selection `3f6b4d54-f12c-4af6-8589-86bf8e335fe7`, revision `581a98c5-f252-44de-8023-53d5e21de8c5`. Both capture files identify the same eight exact shared tool pins; the checker matches them to `tool_revision` and every `bundle_definition` pin, and matches the recipe to its persisted head. All tool pins have epoch `e70fbb83-88c4-4403-8204-308057e7d9e8` in this run; full revisions/masses are retained in `qualification.json` and `warm-after.json`. These minted warm revisions pin the acquired publication; they are not cold PostgreSQL MVCC versions. All four group naming masses and A's stored parent-row mass remain NULL. The parent is the actual retained byte `00.00.00.00.23`, rather than the synthetic fixture's byte address.

Study `d3b2f78f-309b-472e-894d-0379aa794415` has these actual full native claims at publication and after tick 1:

| Occurrence entry | Context | Incarnation | Address | Record/slot |
|---:|---:|---:|---:|---:|
| 2 | 1 | 1 | 40 | 0 |
| 10 | 1 | 2 | 41 | 1 |

The six comparison identities retain their full field claims across that tick:

| Comparison ID | Field context | Incarnation | Address/record | Label | Calculation/storage/fraction bits |
|---:|---:|---:|---:|---:|---|
| 100 | 2 | 1 | 0 | 0 | 64/64/52 |
| 101 | 2 | 2 | 1 | 1 | 64/64/52 |
| 102 | 2 | 3 | 2 | 1 | 64/64/52 |
| 103 | 2 | 4 | 3 | 1 | 64/64/52 |
| 104 | 2 | 5 | 4 | 1 | 64/64/52 |
| 105 | 2 | 6 | 5 | 1 | 64/64/52 |

Actual backend is `native_cpu`, mode `installed_hierarchy`, profile `trial_f64`. Native generation advances 1 → 2 with the completed tick 0 → 1, as the existing tick can advance generation on coverage change. Capture session/study/logical generation remain coherent and full claims remain unchanged. The ParentValue focal cohort has physical mass 3. Native demand records six fields, twelve participation rows, twelve force requests, two evaluations and ten zero responses. This equal-cohort qualification does not resolve #324 or establish independent category forces.

Warm activity records **one publication, one acquisition and one release**, and SQL readback finds zero remaining leases. This is an observed release after acquisition, distinct from #374's no-acquisition refusal. Nine corrupted real readback controls refuse missing/wrong recipe/tool pins, a zero incarnation, wrong comparison identity, absent generation, uncompleted tick, a retained lease, a missing acquisition and a filled unknown naming mass.

The old profile's negative case passes on a separate fresh warm store, with no publication/acquisition/native tick. The populated-store control invokes `check_retained_positive_capture.sh`, which refuses reuse of the positive store at its freshness check before producer execution; complete warm fingerprints remain equal. This control qualifies the wrapper's refusal. Direct producer use requires the caller to provide a fresh dedicated store. Complete cold before/after fingerprints match for both positive and control runs:

- Core: `6c476266a07787f0155a9b164c7359d78fabb15863b3f829173806869849beb5`.
- Linguistics: `891c12823850ae7029b335ca0bba99e8716cce0ce5e505a9e009ae327d2e999b`.

Compressed export SHA256 values also match the verified pair: core `f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121`; Linguistics `9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296`. Captured export labels remain configured provenance; the independent hash/fingerprint checks establish this source identity.

## Validation and handoff

All checks below pass at clean source `e86b588e` with before/after source checks:

- `run_local_ci.sh quick`: matching fork and engine build, actual CPU backend, three CTests, 632 independent interval-math samples and placement capacity.
- Clang 15: **7/7** suites in 271.28 s, including all four new worker/path modes, existing profile, decision fixture and options.
- GCC 11: **4/4** suites in 157.04 s, including new 1-worker/bulk and 2-worker/direct modes plus existing profile/options; native readback checker also builds with warnings as errors.
- Actual retained producer/checker and nine corrupt-readback controls; preserved old refusal; populated-warm refusal by `check_retained_positive_capture.sh` before producer execution and unchanged warm/cold fingerprints.
- Bash syntax/ShellCheck, both transport modes and existing hosted orchestration/provenance contracts.

The synthetic tests preserve their `retained_tick=unqualified` labels. The separate actual-source run supplies the retained qualification above. The fixture extraction is a separate mechanical commit (`d38de829`); production/acceptance source is the following `e86b588e`. Native engine, fork, warm manager, cold adapters and database/schema implementations are unchanged.

The existing hosted database job now invokes the positive mode on its own new warm fixture after legacy and the preserved refusal. It stages the C++ readback checker with the matching native loader/libraries, keeps container Bash/psql on their own runtime, and retains captures, warm SQL manifest, qualification JSON and failure logs even on failure. Local shell/transport checks qualify that orchestration boundary; actual Docker delivery and the full native matrix remain authoritative GitHub checks. No manual duplicate run is requested.

Verified local bundle: `/tmp/hcp-positive-e86b588e-evidence.tar.gz`, SHA256 `71ef9bb1189c21855dcee8b16db2fc9c2c8014376cf4e7d7aaad4b337c13cc32`. Its manifest passes. It retains source/tree/cleanliness, exact commands/configurations, test/build logs, actual captures, warm manifest, qualification, binary/runtime hashes, complete before/after fingerprints, controls and separate development failures. This machine-local hash does not publish the raw bundle. GitHub CI is submitted with this contribution and remains pending; Docker delivery/full matrix results are not inferred from local success. The final documentation commit changes no tested executable, script or workflow.

Pre-commit development evidence is separate: the producer already ticked, but the first checker failed on a PostgreSQL `SET` command tag and then an incorrect fixed-generation assumption. Quiet SQL output and a non-regressing observed generation fix those checker defects. Original readbacks and failed logs are preserved, and the committed-source qualification reruns the complete path.

Next: maintainer confirmation of this documentation correction and reconciliation with hosted results, then maintainer integration of the dependency and this contribution. Preserve the open #376 child when integrating #375 with automatic head-branch deletion enabled. GPU/CUDA execution, mixed cohorts/exposures, general category-force semantics (#324), general prepared revisions/refresh and wider accounting remain separate.
