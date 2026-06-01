# The Kaikki / Wiktionary Pipeline

How the English vocabulary substrate is built from Wiktionary (via Kaikki/Wiktextract) and drained
into a clean, queryable form. This is the upstream prep that populates `hcp_english`.

Sources: claims 230 (source_english delta-dedup), 228 (characteristics LVM-layered),
88 (drain-from-source). Counts verified live (`source_english` = 1,454,988 entries).

---

## Drain from source, don't convert in place

The pipeline follows the **drain-from-source** discipline (claim 88): the raw source is kept intact
as the authoritative reference, and data is drained *into* a target — never converted in place. So
the lineage is:

```
source_wiktionary  ──drain──▶  source_english  ──curate/ingest──▶  hcp_english
(raw Wiktextract,             (clean, delta-dedup            (the live AB-namespace
 authoritative)                queryable substrate)            language shard, 1.494M)
```

---

## source_english: a delta-rule-deduplicated substrate

> *`source_english` is a delta-rule-deduplicated, queryable substrate drained from
> `source_wiktionary` (~1.45M entries, 100% drained, ~2.6 GB on disk).* — claim 230

The deduplication is structural: **any duplicated prose becomes its own pivot row referenced by many
entries.** For example:

- the gloss *"A surname."* is minted **once** and shared by **28,984** entries;
- one parent node *"A number of places in the United States:"* has **6,258** children — stored as a
  `gloss_nodes` hierarchy.

**Purpose:** provide a clean, compressed substrate for LLM-driven per-entry NSM review — triaging
real vs derived senses. The dedup is what makes that review tractable: you review the *minted* prose
once, not 28,984 times.

(Live count confirms: `source_english` = 1,454,988 entries.)

---

## Characteristics are stored LVM-layered

Word characteristics — register, temporal, geographic, derivation bits — are stored
**LVM-layered across three levels** (claim 228), OR'd together to yield a sense's full profile with
no cross-layer duplication:

| Layer | Scope | Examples |
|-------|-------|----------|
| **token-level** | applies to *all* senses | BORROWING, ABBREVIATION |
| **PoS-level** | applies to a grammatical role | (role-specific bits) |
| **gloss-level** | applies to an individual sense | (sense-specific bits) |

The full Kaikki English dump contains exactly **664 distinct tag values** — the finite input alphabet
mapped onto these characteristic bits via the kaikki-tag-mapping rules. (The mapping standards and
tag table are the `kaikki-curation-standards` and `kaikki-tag-mapping` source material — current
data-layer references retained as good source for the curation rules.)

---

## What lands in hcp_english

The curated output is the live shard documented in
[shards-and-schema.md](shards-and-schema.md): 1,494,216 entries across 11 tables, with token
addresses decomposed into `ns/p2/p3/p4/p5` and reference arrays as native ARRAY columns. Regular
inflections are *not* stored — they are derived at runtime (see
[tokenization-policies.md](tokenization-policies.md)).

---

## See also

- [tokenization-policies.md](tokenization-policies.md) — see-it-mint-it and inflection-at-runtime,
  the policies that decide what actually gets stored.
- [shards-and-schema.md](shards-and-schema.md) — the live schema this pipeline targets.
