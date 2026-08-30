# Pass 3 — Engine and code (`hcp-engine/` 226, `src/` 45, `tests/` 5)

**2026-08-30. Dispositions per REBASE_REVIEW_PLAN.md. Nothing executes until Pass 7.**
Module granularity for the C++ tree; file granularity for Python. Nothing is ported in this phase — the value here is the successor map and the carried engineering practice.

**Totals: ~265 AR · 9 CF · 5 DR.** The engine archives wholesale; the CF survivors are the arrayed-pair codec, the DB connectors and Kaikki loader (Pass 4 probe tooling), the Gutenberg fetcher, and the codec tests.

## Cluster D — `hcp-engine/` (module level)

| module (glob) | ~files | disp | what it was | successor concept | pull-worthy design knowledge |
|---|---|---|---|---|---|
| Core resolution pipeline (`Gem/Source/HCPByteIngest*`, `HCPCharRun.h`, `HCPTokenizer*`, `HCPResolutionChamber.h`, `HCPCacheMissResolver*`, `HCPVocabBed*`, `HCPVocabulary*`, `HCPPrimePhases.h`) | 15 | AR | Byte-floor → CharRun segmentation → chamber settling → canonical ids; host-resident after PhysX removal | Intake as forced definition at the byte-particle floor; accretion replaces matching | "THE SEAM" doctrine: deterministic byte→codepoint floor handling *every* encoding before any semantics — survives as the byte-intake face; triple-pipeline workspaces (load CPU / simulate GPU / drain CPU) = launch-tick pipelining precedent |
| `Gem/Source/Settle/` | 10 | AR | AZSL port of the particle settle, slice 1 | The single balancing op's GPU kernel (Taichi/C++ era) | **Oracle-first methodology**: portable CPU reference as deterministic oracle + GPU-equivalence harness asserting GPU==CPU on real hardware + transpile validation — the strongest transferable practice for taichi_core kernels |
| `Gem/Source/Pack/` | 7 | AR | Compact-ID packer: `{compact-id → chars}` LMDB store, CPU-side compact→canonical ledger | **Closest code ancestor of flat-pool + chain bookkeeper**: identity lives CPU-side; GPU gets only chars/positions | Dense window-local ids in slot order; per-length fixed-stride blobs (coalesced reads); minimality tests; see-it/mint-it + arrayed fixed-stride doctrine |
| `Gem/Source/Forces/` | 5 | AR | Section-1 Newtonian force generators (Cyclone port) + symplectic Euler | None — force zoo dissolved by single op | Port-from-live-source-not-memory discipline; symplectic Euler reference if explicit integration is ever needed |
| PBM/document storage (`HCPPbm*`, `HCPBond*`, `HCPDocumentQuery*`, `HCPDocVarQuery*`, `HCPGutenbergRunner*`, `HCPEntityAnnotator*`, `HCPPhysIngest*`, `HCPEulerReassembly_FUTURE.cpp`, `HCPParticlePipeline.h`) | 19 | AR | Pair-bond-map document ingest/reconstruction (>98% accuracy path), Gutenberg batch runner, entity annotation | Document intake becomes byte-particle accretion; PBM bonds → flows/connections | Lossless round-trip as proof-of-comprehension discipline; positions-as-INTEGER[] layout; Gutenberg runner's source-record bookkeeping feeds Pass 4 |
| DB backends (`HCPDatabaseBackend.h`, `HCPDbConnection*`, `HCPDbUtils.h`, `HCPPostgresBackend.cpp`, `HCPSqliteBackend.cpp`, `HCPStorage*`) | 10 | AR | Backend abstraction over Postgres/SQLite + storage glue | Read-only extraction clients (Pass 4 era), then retirement | none beyond conventional patterns |
| `HCPEnvelopeManager*` | 2 | AR | Declarative envelope system: envelope_definitions/queries tables → SQL against cold shards → LMDB sub-dbs, composed children, activation audit log | **View-chain precedent**: declarative, priority-ordered, composable materialization of working sets — the chain bookkeeper's ancestor | Envelope = declared view with audit trail; composition via childEnvelopeIds = chain nesting |
| Socket/JSON API (`HCPSocketServer*`, `HCPJsonInterpreter*`) | 4 | AR | Port-9720 JSON daemon API | Emission-side interface will be new | none |
| Workstation + editor tools (`Gem/Source/Workstation/`, `Gem/Source/Tools/`) | 12 | AR | Source workstation UI, O3DE editor widgets | Future viewer of the zoomable field (concept only) | none |
| Gem/module scaffolding (`HCPEngineModule*`, `HCPEngineSystemComponent*`, `Gem/Include/`, `gem.json`, `Gem/*.cmake`) | 15 | AR | O3DE Gem wiring | none — Taichi/C++ era has no Gem | none |
| O3DE project scaffolding (`CMakeLists*`, `CMakePresets`, `Platform/`, `Registry/`, `Resources/`, `Levels/`, `ShaderLib/`, `AssetBundling/`, `installer/`, `*.cfg`, `export.*`, `configure.sh`, `hcp-engine.service`, `Config/`) | ~90 | AR | Cross-platform O3DE project shell, icons, quality configs, installer | none | Installer package structure (db/engine/vocab/workstation split) is a sane packaging precedent |
| `hcp-engine/docs/` | 15 | AR (keep as provenance layer) | PhysX-era consultations + db-specialist notes; already self-marked "superseded — historical" | folds into unified archive un-flattened | Prior in-tree archive with its own README — same treatment as `docs/_archive/` |
| Meta (`AGENTS.md`, `TODO.md`, `ROADMAP.md`, `project.json`, `preview.png`, `.command_settings`, `autoexec.cfg`) | 7 | AR | Engine-local coordination + project identity | New top-level docs | **`ROADMAP.md` spells the acronym: NAPIER = "Not Another Proprietary Inference Engine, Really!"** — carry into new docs |
| Cruft (`project.json.bak0`; dead `Registry/physx*.setreg`) | 5 | DR | Backup file; orphaned PhysX configs | — | none |

## Cluster E — `src/` and `tests/`

| path | disp | what it is | successor concept | pull-worthy |
|---|---|---|---|---|
| `src/hcp/AGENTS.md` | AR | Python-tools agent guide | new ops docs | Documents "NVIDIA Warp kernels in .py" convention — historical Python-GPU flirtation relevant to the Taichi/C++ decision record |
| `src/hcp/ROADMAP.md` | AR | Python-tooling roadmap | — | "Python is NOT the runtime" doctrine statement as design history |
| `src/hcp/TODO.md` | AR | Stale task list | issues sweep (Pass 6 overlap) | none |
| `src/hcp/core/token_id.py` | **CF** | **The base-50 arrayed-pair codec**: encode/decode dotted ↔ integer tuples, 1–5 pairs, `token_depth` (LoD depth), namespace anchors | direct tooling for the Pass 4 blob round-trip probe; executable statement of the precept | The whole module — it IS the universal parse rule in working code, with the alphabet (52 letters minus O/o) |
| `src/hcp/core/byte_codes.py` | AR | 256-byte classification + bond-class taxonomy | byte-particle layer needs no bond taxonomy | Byte category table trivially rederivable; bond classes = design history |
| `src/hcp/db/postgres.py` | CF | hcp_core connector + schema-as-code | Pass 4 read-only probe reuse | **`token_id TEXT PRIMARY KEY` = the precept violation in the flagship table** |
| `src/hcp/db/english.py` | CF | hcp_english connector | Pass 4 probe reuse | atomization JSONB shape (composition data to pull) |
| `src/hcp/db/names.py` | CF | hcp_names connector | Pass 4 probe reuse | Cross-linguistic shard rationale |
| `src/hcp/db/kaikki.py` | CF | Kaikki JSONL → Postgres loader + source-side schema | extraction map for the landing-lattice intake | JSONB-preserving load pattern (raw kept alongside indexed = path-through-friendly) |
| `src/hcp/db/pbm.py` | AR | PBM build/store/read (FPB/FBR) | flows/chains replace PBMs | none |
| `src/hcp/cache/resolver.py` | AR | Postgres→LMDB backfill, var mint-or-return | chain-bookkeeper lineage | LMDB sub-database contract + zero-copy mmap discipline; var mint-or-return = undefined-slot minting precedent |
| `src/hcp/engine/` (8 files) | AR | Python reference implementation of the retired C++ engine | field intake replaces tokenize-disassemble | "The separation event IS the whitespace" — emergent-boundary idea; validate's roundtrip methodology |
| `src/hcp/ingest/` core pipeline (8 files) | AR | The working PBM document-ingestion pipeline | intake-as-forced-definition replaces it | Verifier's three-level round-trip discipline; scanner's edge-case inventory = real intake noise catalogue |
| `src/hcp/ingest/` one-shot loaders (12 files) | AR | Executed seed/load scripts; outcomes live in the shards | DB contents are the source now | words.py documents the AB.AB layer addressing actually in the data (Pass 4 needs it); nsm_molecules' definition-graph walk = transitive-chain precedent |
| `src/hcp/ingest/gutenberg_fetch.py` | **CF** | Gutendex API fetcher with rich metadata | reusable front-end intake tooling for aggregator sources | Metadata-with-text fetch pattern fits provenance-path requirements |
| `src/hcp/reconstruction/spacing.py` | AR | SQLite-rule-driven spacing reconstruction | — | Ties to `db/spacing_rules.sql` (uncommitted working-tree edit — Pass 4 item) |
| `src/hcp/__init__.py` + module `__init__.py` (5) | follows parent | Package scaffolding | — | none |
| `tests/test_token_id.py` | **CF** | Codec tests (alphabet, round-trip, depth) | validation harness for the Pass 4 probe | Ready-made round-trip assertions |
| `tests/test_byte_codes.py` | AR | Tests retired byte taxonomy | — | none |
| `tests/test_spacing.py` | AR | Tests retired spacing rules | — | none |
| `tests/conftest.py` | CF | Trivial pytest scaffolding | — | none |

## Consolidated flags

1. **NAPIER's expansion recovered**: "Not Another Proprietary Inference Engine, Really!" (`hcp-engine/ROADMAP.md`) — matches the survives-decision; state it in the new docs.
2. **`core/token_id.py` + `test_token_id.py` are the highest-value code carry**: the arrayed-pair precept as a tested executable codec. Lift verbatim into Pass 4 probe tooling.
3. **The precept violation is documented at source level**: `db/postgres.py` declares `token_id TEXT PRIMARY KEY` while the codec proving the array form was always available sits in the same package. Evidence for "convention ignored, not absent."
4. **Oracle-first GPU validation** (Settle/, Forces/) is mandated practice for every taichi_core/C++ kernel: CPU reference oracle → GPU mirror → hardware equivalence harness → transpile validation.
5. **Architecture lineage to cite**: Pack/ (identity CPU-side, GPU gets fixed-stride position data) and the envelope system (declarative, composable, audited working sets → view chains). Envelope table schema gets a look in Pass 4 before archiving.
6. **`ingest/` was real pipeline logic in Python** — the front-end-only rule was honoured in labels while ingestion did production data work. Recorded as design history feeding the pending Python/Taichi-vs-C++ decision; not blame.
7. **Stale configs**: `src/hcp/db/` connectors hardcode `localhost:5432 / hcp_dev`, contradicting the HAVEN ops doc (192.168.68.60:5435). The Pass 4 probe must not trust in-code connection info.
8. Engine TODO items mirror GitHub issues #48/#30/#28 — Pass 6 closes them together. `scripts/` may contain dead twins of `ingest/` — Pass 5 checks.
