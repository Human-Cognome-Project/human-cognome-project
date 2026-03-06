# HCP Engine — TODO

Last updated: 2026-03-06

## Legend

- **[BLOCKED]** — waiting on another task or decision
- **[READY]** — can be picked up now
- **[IN PROGRESS]** — someone is working on it

---

## Active: Source Workstation Packaging

- [ ] **[IN PROGRESS]** Installer — multiple workstation configs, viewer first, then data manipulation + ingestion
- [ ] **[READY]** Workstation SIGTERM handling — UI ignores SIGTERM, needs SIGKILL. Fix shutdown.

## Pipeline

- [ ] **[READY]** Label propagation — if a word appears as a Label anywhere in text, restore firstCap on all suppressed instances of the same token_id.
- [ ] **[READY]** Known initialisms — U.S., U.K., M.D. etc. as curated list.
- [ ] **[READY]** Document metadata pipeline — HCPJsonInterpreter needs testing/rejig post-decomposition. Test with Gutenberg JSON files.
- [ ] **[READY]** Activity envelope engine wiring — HCPEnvelopeManager activation/eviction cycle.
- [ ] **[READY]** env_* variant loading — wire envelope-activated variants into resolution with VARIANT morph bits.

## Bugs

- [ ] **[BLOCKED]** Entity cross-ref OR matching — [GitHub #23](https://github.com/Human-Cognome-Project/human-cognome-project/issues/23). GetFictionEntitiesForDocument matches ANY name token instead of ALL. Blocked on entity data cleanup.
- [ ] **[READY]** Service file binary mismatch — [GitHub #25](https://github.com/Human-Cognome-Project/human-cognome-project/issues/25). hcp-engine.service references wrong binary name.

## Infrastructure

- [ ] **[READY]** Persist bond tables as files — [GitHub #5](https://github.com/Human-Cognome-Project/human-cognome-project/issues/5). Bond compiler still hits Postgres on startup (~3s).

---

## Workstation Features

- [ ] **[READY]** Var positions in panel — display position data for each docvar occurrence in the Vars tab.
- [ ] **[READY]** Proper candidate detection in tokenizer — flag out-of-place capitalization during tokenization.
- [ ] **[READY]** Alias grouping review workflow — confirm/reject/promote groups in the Vars tab panel.

---

## Format Builders (contributor tasks)

- [ ] **[READY]** PDF text extractor — [GitHub #14](https://github.com/Human-Cognome-Project/human-cognome-project/issues/14)
- [ ] **[READY]** EPUB text extractor — [GitHub #15](https://github.com/Human-Cognome-Project/human-cognome-project/issues/15)
- [ ] **[READY]** HTML text extractor — [GitHub #16](https://github.com/Human-Cognome-Project/human-cognome-project/issues/16)
- [ ] **[READY]** Markdown text extractor — [GitHub #17](https://github.com/Human-Cognome-Project/human-cognome-project/issues/17)
- [ ] **[READY]** Wikipedia dump processor — [GitHub #18](https://github.com/Human-Cognome-Project/human-cognome-project/issues/18)

---

## Future (not yet planned in detail)

- Custom physics engine (~65 core forces, linguist-defined)
- Conversation levels (documents as entities in level workspaces)
- Language shard system (new languages via vocabulary + force constants)
- Texture engine (linguistic force bonding — surface language rules)

---

## Completed (recent)

- [x] V-1/V-3 variant normalization — TryVariantNormalize in resolve loop (d59c2fa)
- [x] Scene pipeline refactor — RunPipelinedCascade, WS_PRIMARY_COUNT 3 (c7b9cb6)
- [x] Boundary fixes — /, [, ], •, trailing ., numeric var tagging (8ecade1)
- [x] Dead code cleanup — 3,132 lines removed (8ecade1)
- [x] WriteKernel decomposition — monolith split into 7 kernel modules (615bbfc)
- [x] Persistent vocab beds — 15 PBD systems replace 175 per-batch chambers (63750f5)
- [x] Morphological resolution — MorphBit field, RunTag routing, inflection stripping (650cea0)
- [x] Entity annotator — multi-word entity recognition from LMDB (8e9b3f6)
- [x] Manifest scanner — single-pass PBM from resolution manifest (fe573fe)
- [x] phys_ingest endpoint — full ingest pipeline via socket API (8e9b3f6)
- [x] Workstation overhaul — socket client, embedded DB kernels, systemd service (3b86915)
