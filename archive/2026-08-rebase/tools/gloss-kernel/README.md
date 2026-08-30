# gloss-kernel

Dedicated C++ process that converts every resolvable gloss in `hcp_english` into a concept
formula, fixpoint-laddering outward from the core: senses whose content words are all known
concepts get minted; minted words become known; repeat until nothing new resolves.

Doctrine: claims 531–541 (senses are conceptual payloads; gloss = address written in the skin;
ordered+sectioned collapse keys; one-directional links — concepts never know their words).

Standalone for now; `GlossKernel` is shaped for lift into the HCPEngine O3DE Gem
(kernel-takes-connection pattern, cf. `hcp-engine/Gem/Source/HCPDbConnection.h`).

## Build & run

```
make
PGPASSWORD=… ./gloss-kernel                  # full corpus
./gloss-kernel --limit 20000                 # smoke test
./gloss-kernel --max-residue 1               # relaxed mint threshold
./gloss-kernel --include-dated               # archaic/obsolete (deferred; future
                                             # cross-linguistic-linking example)
```

Connection via PGHOST/PGPORT/PGUSER/PGPASSWORD (defaults: Haven hcp_english, dev role).

## What it does

1. Loads the lemma map from `entries` (lowercase single common words only — the interim
   ingestion rule; capitalized = label ring, multiword = deferred, both preflight-excluded),
   plus `cx_coremap` / `cx_scaffold` / `cx_lemma_fix`.
2. Streams eligible senses (form-of/alt-of always excluded; obsolete/archaic/dated excluded
   unless `--include-dated`).
3. Parses each gloss: paren-depth filtering, `[;]` section boundaries, greedy multiword
   pattern folding to structural markers (`the act of`→`#ACT_OF`, `one who`→`#AGENT`,
   `of or relating to`→`#REL_TO`, …), classification (core concept / scaffold-drop / content).
4. Key = md5 of the ordered, sectioned structure — verified byte-identical to Postgres
   `md5()`, so kernel keys and SQL-pilot keys are comparable.
5. Fixpoint mint loop, then `TRUNCATE`+`COPY` results to:
   - `kx_concept(ckey, structure, first_sense, pass)` — no word back-references (claim 540)
   - `kx_word_concept(word, sense_id, ckey, pass)` — word-side links, the authoritative direction
   - `kx_status(sense_id, word, status, residue, pass)` — complete/incomplete/empty + final
     residue (the next frontier, pre-weighted)
