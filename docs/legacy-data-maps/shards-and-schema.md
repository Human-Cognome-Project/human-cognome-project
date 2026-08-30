# Shards and Schema

The data substrate: which databases exist, the live English shard's scale, and the schema
decomposition pattern that keeps token addresses traversable.

Sources: claims 203 (scale + shard layout), 207 (decomposition pattern), 208 (names as
constructs), 209 (AA/AB namespaces), 210 (classification policies). Counts and schema verified
live against NAS HAVEN on 2026-05-30 and reconfirmed during this rewrite.

---

## Live shard layout

All databases live on **NAS HAVEN, `192.168.68.60:5435`** (claim 203). Verified live:

**10 data shards:**

| Shard | Role |
|-------|------|
| `hcp_core` | universal computational concepts (AA namespace); cold-resident, always loaded |
| `hcp_english` | English text forms (AB namespace) — the language shard |
| `hcp_envelope` | envelope (query+filter workspace) definitions |
| `hcp_fic_pbm` | fiction pair-bond maps |
| `hcp_fic_people` / `hcp_fic_places` / `hcp_fic_things` | fiction entities (6-way split, fiction side) |
| `hcp_nf_people` / `hcp_nf_places` / `hcp_nf_things` | non-fiction entities (6-way split) |

Plus **`source_english`** and **`source_wiktionary`** (upstream prep — see
[kaikki-pipeline.md](kaikki-pipeline.md)) and **`hcp_orchestrator`** (the claim-graph memory layer
that sources these docs).

Entity DBs are the **6-way split** (fiction/non-fiction × people/places/things). Entity tokens are
**language-independent** — language shards link *to* shared entity tokens, not the reverse, so the
entity DBs are reusable across future language shards (claim 210).

---

## hcp_english scale

> *`hcp_english` holds ~**1,494,216 entries** (verified live).* — claim 203

This **supersedes** the earlier 569,471-token tree-model curation (the 2026-03-17 figure). **Do not
cite 569K as current** — any doc that does is stale. The 1.494M came from the full Kaikki Wiktionary
re-ingestion completed 2026-04-07.

The 11 live tables in `hcp_english`: `entries`, `senses`, `sense_categories`, `sense_examples`,
`relations`, `forms`, `phrase_components`, `english_characters`, `inflection_rules`, `sounds`,
`translations`. The old `tokens` / `token_pos` / `token_variants` tables are **dropped.**

---

## The decomposition pattern (decisions 001 + 005, reconciled)

The live schema pattern (claim 207, verified against `hcp_english.entries`):

**(A) A token's own address is decomposed** into `ns / p2 / p3 / p4 / p5` TEXT columns plus a
generated dotted `token_id` TEXT key, with a compound B-tree index on `(ns,p2,p3,p4,p5)` for
prefix-compressed traversal. This is **decision 001, still current**, used uniformly across
`hcp_english` and all 6 entity shards.

**(B) Token-reference arrays are native ARRAY columns** directly on the row — `spelling`,
`morphology`, `tokenized_etymology`, `form_of_tags`, `alt_of_tags`. Decision 005's original approach
(junction tables for every array ref, "zero TEXT[]", ~18.4M junction rows) was
**abandoned/superseded**; those junction tables are dropped. (See
[../06-status/decisions/](../06-status/decisions/) for the annotated decision records.)

**(C) The remaining debt:** those array *elements* are stored as full dotted `token_id` strings —
lost B-tree compression, ~5× bloat. This is exactly the substrate-efficiency debt (claim 31). The
proper form is `text[]` with diff-only encoding.

> **Why this debt is an *intelligence* problem, not just storage:** dot-strings in the connection
> columns cripple the index-walking that *traversal* depends on — and intelligence is data-quality ×
> traversal (claim 255). See
> [../02-architecture/intelligence.md](../02-architecture/intelligence.md) and
> [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## Namespaces: AA (concept) vs AB (text)

A shared surface label does **not** mean a shared entity (claim 209, decision 002):

- **AA-namespace** tokens (`hcp_core`) are the universal computational concepts.
- **AB-namespace** tokens (`hcp_english`) are how English expresses them.

The NSM prime **WANT** (AA) and the English word **want** (AB.AB) carry the same spelling but are
distinct entities. *"Words in the core database are labels for understanding; they are not the thing
itself."* This is the namespace-level realization of the
[linguistic/conceptual separation](../02-architecture/linguistic-vs-conceptual.md).

---

## Names are constructs, not a token property

> *A "name" is not a token property — it is a **Proper Noun construct**, i.e. a bond pattern.* — claim 208

"Alice" is the word "alice" with a capitalized spelling; it becomes a *name* only when it enters a
Proper-Noun bond pattern that assembles a Person/Place/Thing entity from word tokens. Consequences:

1. Capitalized forms are stored as **spelling variants** of the base word, not separate tokens.
2. Words that are *only* names (e.g. "Bartholomew") are entries with `PoS = label`, whose meaning
   comes entirely from the construct they participate in.
3. The old `hcp_names` shard (~150K "name component" tokens) was **eliminated** as pointless
   duplication — it is gone from the live DB.

This underlies the chamber's first-stage Label resolution (claim 175, see
[../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md)): shards hold surface
Labels; entity DBs hold meaning.

---

## Classification policies

Durable data-pipeline policies (claim 210):

1. **Everything gets a token** — no exclusion at mint time; categories filter via broadphase rather
   than gatekeeping what exists.
2. **Entity tokens are language-independent** — reusable across future language shards.
3. **Categories are query optimization** — the token graph itself encodes knowledge;
   `sense_categories` are broadphase tags (29,549 distinct), not the meaning.
4. **Mythology is nonfiction** — religious/mythological figures classify as nonfiction entities, all
   traditions treated equally, no validity judgment encoded.

---

## See also

- [kaikki-pipeline.md](kaikki-pipeline.md) — how `source_english` was built and drained.
- [tokenization-policies.md](tokenization-policies.md) — see-it-mint-it, inflection-at-runtime.
- [var-and-continuation.md](var-and-continuation.md) — the var DB and continuation index.
