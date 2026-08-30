# Archive: the pre-rebase paradigm (sealed 2026-08-30)

This directory preserves the Human Cognome Project as it stood before the August 2026 rebase onto
the field-balancing physics basis. It is **design history**: kept whole, honestly, because the
project treats its own compressions the way it treats everyone else's — with the path through
declared. Nothing here is load-bearing for the current build.

**The disposition record** — what everything was, why it was archived, and what concept succeeded
it — lives in [`/review/`](../../review/): one note per pass (root docs, docs tree, code, data,
tooling, issues) plus [`decisions.md`](../../review/decisions.md). This README is the map and the
declared-loss statement, not the inventory.

## Map

| path | what it is |
|---|---|
| `root/` | The old identity documents (README, MANIFESTO, ROADMAP, CONTRIBUTING, AGENTS), a session transcript (`co.txt`), and the February 2026 invitation |
| `docs/` | The old documentation tree (00–07): NAPIER-as-db-functions keystone, NSM concept substrate, engine docs, status pages, decision records. Contains its own earlier archive layer (`docs/_archive/`, with supersession maps) — a prior compression's path-through record, preserved un-flattened |
| `hcp-engine/` | The O3DE/AZSL C++ engine: byte-floor → resolution chambers → canonical ids, PBM document storage (>98% reconstruction), envelope system, socket API. Its `docs/` is another self-declared historical layer |
| `src/`, `tests/` | The Python package (reference implementations, ingest pipeline, one-shot loaders) and its tests |
| `scripts/` | Data-construction passes, run in anger; their outcomes live in the shards |
| `tools/` | Standalone experiments: byte-floor reference, gloss-kernel fixpoint ladder, foma morphology |

Still live elsewhere (not archived): the physics packages (`/field`, `/ledger`), the data holdings
(`/db`, `/data`, `/sources` — read-only sources), the extraction toolkit (`/extraction` — carried
forward from `src/` and `scripts/wiktionary/`), and the legacy data maps (`/docs/legacy-data-maps/`).

## Declared losses

Per the review decisions ([review/decisions.md](../../review/decisions.md)):

1. **Private communications removed from the tree** (not archived): the outreach drafts and a
   personalized outreach document. The adjacent-worker register in `docs/06-status/validation.md`
   was **redacted at archive time** — records about specific people are private communications, not
   public records. (Tree removal only; git history was not rewritten.)
2. **Deleted as generated artifacts** (regenerable or detritus, not knowledge): two committed
   binaries (`gloss-kernel`, `english_morph.bin`), nine run logs/CSVs in `scripts/deprecated/`,
   two LibreOffice lock files, three orphaned PhysX configs, one `.bak` file.
3. **Known absences inherited from the era, recorded so nobody hunts for them**: original text
   fields in some stores were deliberately consumed by the destructive `self_tokenize` conversion
   (the script is preserved here as the record); `compile_vocab_lmdb.py` is referenced by two meta
   files but no longer existed anywhere at archive time; dump files predate the last migrations
   (live stores are the authority); migration filenames contain duplicate numbers (016, 038, 040,
   041) — applied schema state, not filename order, is authoritative.

## Why the paradigm was replaced — and what it got right

The old keystone ("cognition reduces to database functions") was superseded by the field-balancing
basis ([docs/physics-basis.md](../../docs/physics-basis.md)): one operation, one field, frequency
and amount permanently separated. But the old work converged on the new basis from below more than
once — `docs/04-engine/force-wiring-primes-to-newtonian.md` ("one mechanism, many axes"),
`docs/entry-points/cognitive-physics-the-open-problem.md` (poses the exact question the field basis
answers), `hcp-engine/Gem/Source/Pack/` and the AZSL corpus's host-pointer study (identity on CPU,
position on GPU — the current storage split). The essays in `docs/01-foundations/` are preserved
verbatim as dated emissions; they exist for whoever writes the story of how this all came to be.
