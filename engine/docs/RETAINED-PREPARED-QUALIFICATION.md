# Retained prepared-source qualification

Contributor witness under #343, following [#373 review 5959051551](https://github.com/Human-Cognome-Project/human-cognome-project/pull/373#pullrequestreview-5959051551). Tested source: `8fa4dbb139936023e7b881dcc206608a0479c529`, based on corrected #373 `0e2add68`. **Result: verified retained-source membership refusal. A retained C3 publication/completed tick remains unqualified.**

## Exact retained input and observed blocker

The actual `viewport_letter_capture --study prepared-english-pair --backend cpu --ticks 1` executable was run against new isolated PostgreSQL 16.15 restores of the same exports used by `native-database`:

| Payload | Verified SHA-256 |
|---|---|
| 2026-10-13 `hcp_core.sql.gz` | `f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121` |
| 2026-10-13 `hcp_linguistics.sql.gz` | `9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296` |

Git LFS and Docker are unavailable locally. The public payloads were downloaded from the source-pinned GitHub media paths into `/tmp`, verified against both hashes, and restored into new empty databases in a new socket-only cluster. The system cluster and original retained stores were untouched. The restored cold databases received a SELECT-only role and read-only defaults. Every capture invocation received a fresh qualification warm database, with original schema and explicit migration 002. No retained deployment was migrated, reset, dropped or truncated. The owned fixture cluster was stopped normally after each run; its data remain available at `/tmp/hcp-retained-qualification.fLfZF3/pgdata`.

Read-only inspection and the real producer identify:

| Source record | Stored values |
|---|---|
| English A, `02.00.00.00.03` | Notation `A`, exact whole mass `2` |
| Ordered parent | `00.00.00.00.23`, ordinal `0`, stored parent-row mass NULL |
| Membership `02.00.00.00.00` | Notation `English`, whole mass NULL |
| Membership `02.00.00.00.01` | Notation `Alphabet (en)`, whole mass NULL |

The executable exits **1** with its fixed diagnostic:

```text
prepared English pair: membership comparison choices are unsupported
```

This is the actual production executable's supported-subset check after cold parent/membership resolution. The data were preserved, including both NULL group whole masses. The preset rejects the presence of memberships; these observations do not establish that NULL group mass is malformed or independently forbidden by the engine.

The old missing-connection blocker is resolved for this isolated qualification fixture. The current blocker is the preset's explicit comparison choice: it supplies SiblingDefinition English and ParentValue of its direct parent, and declines membership comparisons. Extending the preset requires an explicit reviewed topology/focus/resource decision preserving both memberships and the accepted unknown-mass distinction. Do not strip those references, invent whole mass/reciprocals, or infer the decision from a detached report. #324's mathematical distinction remains separate.

## Publication and lease boundary

The actual client configures the fresh warm 00/02 directory before its profile rejects. Independent warm readback verifies:

| State | Schema | Root bindings | Tool revisions | Bundle revisions | Leases |
|---|---:|---:|---:|---:|---:|
| Before actual invocation | 2 | 0 | 0 | 0 | 0 |
| After membership refusal | 2 | 2 | 0 | 0 | 0 |

No warm recipe publication, consumer acquisition, ActiveAssembly submission, C3 comparison installation or native tick was reached. There are consequently no acquired recipe/tool pins, full native body claims, comparison identities or native generation to qualify. `qualification.json` explicitly records `retained_tick_qualified: false`, null pin/generation/completed tick, empty native claims/comparisons and `lease_release: not_applicable_no_acquisition`. Zero remaining leases is not reported as a completed acquisition/release cycle.

The caller-supplied source definitions were never replaced by synthetic fixtures. The earlier six synthetic profile modes remain qualified only for their original fixture scope. A future positive retained witness must record the acquired source pins, both full native body claims, actual comparisons/generation, completed tick at least 1 and successful lease release.

## CI integration and local checks

`check_retained_prepared_capture.sh` is a bounded expected-refusal witness. It requires an empty v2 warm store before launching the actual command, inspects the exact retained A construction/membership shape through read-only connections, checks the producer's diagnostic/exit 1, verifies no capture JSON and checks the post-refusal warm counts. Probe exit zero means **verified unsupported memberships**, with no native tick qualification. Any different source shape, unexpected successful execution, changed diagnostic, publication or lease makes this witness fail and retains its observed record/logs.

The existing `run_warm_active_acceptance.sh` gains the explicit `prepared-membership-refusal` mode. Its default legacy mode is preserved. The new mode reuses existing executable/loader/library/runtime packaging, creates a separate new warm DB, applies migration 002 explicitly, invokes the same probe and retains its reports even on failure. It verifies both payload hashes and fingerprints the complete cold data before/after. The hosted `native-database` route calls this mode after its existing integration, reusing that run's exact isolated cold restores. Artifacts retain the actual command, source hashes, English record, producer stderr/exit, warm readback and qualification JSON. Passing the generic database integration or this refusal witness does not establish a retained completed tick.

At clean source `8fa4dbb1`, local CI for the changed scope passes:

- Bash syntax and ShellCheck for both changed scripts; existing hosted CPU orchestration contracts pass (shards, failure propagation, inventory and bundle provenance guards).
- The canonical probe invokes the real production executable through a copied loader/library/runtime package against the verified retained restores and a **new** v2 warm store. Exact memberships, source refusal and unpublished/unacquired state pass.
- A second helper invocation against that now-populated qualification store exits 1 at `freshness`, before native execution. A full warm data fingerprint is identical before/after this guard, preserving its configured policy.
- Complete read-only cold data fingerprints match before/after both invocations: core `6c476266a07787f0155a9b164c7359d78fabb15863b3f829173806869849beb5`, linguistics `891c12823850ae7029b335ca0bba99e8716cce0ce5e505a9e009ae327d2e999b`. These are normalized PostgreSQL data-only dump hashes, distinct from the compressed source-payload hashes above.

Engine, fork, warm/common implementation and viewport behavior are byte-identical to corrected #373. No numerical suite was rerun merely to onboard. The CLI-only correction in `0e2add68` rebuilds the capture/options programs and passes the existing options CTest at a clean source. Its three-file diff from `d8eb44f2` contains the usage warning and two Markdown files. Both profiles now require a **fresh, dedicated qualification warm store per invocation**, because a new actor/epoch/policy updates persistent 00/02 binding policy. Ownership/schema alone are insufficient. The explicit migration boundary is preserved.

Docker packaging/execution remains untested locally; GitHub CI is authoritative for that route. No full regression, no-unroll, GPU execution, positive retained C3 tick, acquired-pin lifetime or lease-release qualification is claimed. All merges remain with the maintainer. #371 is integrated on main as `bf47b2ae`; #372/#373 retain separate review/hosted gates. This slice remains stacked on corrected #373.

Verified machine-local bundle: `/tmp/hcp-retained-qualification-8fa4dbb1-evidence.tar.gz`, SHA-256 `96759ebcd8242bb26587f2e66a2c993a40c26d65d37fffb74fc51c1c60d9adff`. Its manifest passes verification. It contains the initial restore/attempt and pinned probe commands/logs, exact source/record evidence, before/after fingerprints, expected populated-store refusal, source cleanliness, script patch and binary/runtime hashes. Snapshot payloads, PG data and copied libraries/cache remain retained separately, avoiding another large artifact copy. Publishing this hash does not publish the raw machine-local evidence. Normal data-only dump circular-FK warnings are retained; no suggested destructive restore operation was performed.

Reproduce in the existing owned qualification container after restoring/verifying the two payloads through `native-database`:

```sh
bash engine/scripts/run_warm_active_acceptance.sh engine/build \
  YOUR_OWNED_CONTAINER YOUR_CORE_DB YOUR_LINGUISTICS_DB prepared-membership-refusal
```

For an explicit local fresh v2 qualification store, supply the three existing connection settings externally, source `engine/build/engine-env.sh`, then run `bash engine/scripts/check_retained_prepared_capture.sh /absolute/new/evidence engine/build/viewport/viewport_letter_capture`. The helper only inspects and runs the producer; it does not create or migrate a database. Each positive producer attempt requires a separate fresh warm store. Review the membership comparison decision before extending the profile; rerun the actual retained source-to-tick witness after that decision is implemented.
