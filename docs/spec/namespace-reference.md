# Namespace Reference Guide

## Purpose

This document provides a human-readable overview of the namespace addressing scheme. The canonical source of truth for all namespace allocations is the `namespace_allocations` table in the core database (`hcp_core`).

```sql
SELECT pattern, name, description, alloc_type, parent
FROM namespace_allocations ORDER BY pattern;
```

## Addressing Recap

Token IDs are base-50 letter pairs (see [token-addressing.md](token-addressing.md)). Each pair represents one LoD level. Reading left to right: broad category → sub-category → specific token.

```
XX.XX.XX.XX.XX
│   │   │   │   └── Atomic element
│   │   │   └────── Token within sub-category
│   │   └────────── Sub-category
│   └────────────── Category within mode
└────────────────── Mode (LoD scope marker)
```

## Ingestion Rules

### New token creation
When ingesting a source, new tokens are created one LoD level below the source's own level. If the source defines things that require a new category, that category is created one level up as necessary.

### TBD tokens
When a source contains references that cannot be resolved to existing tokens (because the resolving source hasn't been ingested yet):
- Assign a placeholder ID: `TBD{count}` where count is sequential within the ingestion run
- Record the TBD in the PBM metadata
- When the resolving source is ingested later, sweep and replace TBDs with real addresses

A high TBD count on ingestion signals a missing prerequisite source.

### No shortcuts
Every distinct form that appears in source data gets its own token. Source-specific abbreviations, notation variants, and shortcodes are all independent tokens in the abbreviation class (AA.AD). Canonical mappings between them are recorded as bonds, not as ingestion-time substitutions.

**Rule: If it appears in a source, it has a token. No silent translations.**

## Current Namespace Structure

### Universal Mode (AA)

```
AA                          Universal mode
├── AA.AA                   Encoding Tables & Definitions
│   ├── AA.AA.AA.AA.{n}     Byte Codes (0-255)
│   └── AA.AA.AA.AB.{n}     NSM Primitives (~65)
├── AA.AB                   Text Encodings
│   └── AA.AB.AA.{table}.{byte}  Encoding table entries (UTF-8, Latin-1, etc.)
├── AA.AC                   Structural Tokens
└── AA.AD                   Abbreviation Classes
```

### Text Mode (AB)

```
AB                              Text mode
├── AB.AA                       Unicode Characters
│   └── AB.AA.AA.{cat}.{n}      Character tokens by category (2,032 tokens)
└── AB.AB                       English Language Family (1.16M tokens)
    ├── AB.AB.A{sub}.{n}.{n}    Layer A: Affixes (3,696 tokens)
    ├── AB.AB.B{sub}.{n}.{n}    Layer B: Fragments (reserved)
    ├── AB.AB.C{sub}.{n}.{n}    Layer C: Words (1,146,520 tokens)
    ├── AB.AB.D{sub}.{n}.{n}    Layer D: Derivatives (3,979 tokens)
    └── AB.AB.E{sub}.{n}.{n}    Layer E: Multi-word (9,084 tokens)
```

#### Word Addressing (AB.AB)

The 3rd pair uses double-duty encoding:
- 1st character: Layer (by derivation, bottom to top)
- 2nd character: Sub-category within layer

##### Layers (by derivation chain)

| Layer | Prefix | Description | Count |
|-------|--------|-------------|-------|
| A | AA-AF | Affixes: prefix, suffix, infix, interfix, circumfix | 3,696 |
| B | B* | Fragments (reserved for incomplete words) | - |
| C | CA-CR | Words: noun, verb, adj, adv, prep, conj, etc. | 1,146,520 |
| D | DA-DE | Derivatives: abbreviation, initialism, acronym, contraction, clipping | 3,979 |
| E | EA-EC | Multi-word: phrase, prep_phrase, proverb | 9,084 |

Reading bottom-up: affixes → fragments → words → derivatives → multi-word.
Each layer atomizes to the layer below (or to characters if at word level).

##### Layer A: Affixes

| Sub | Prefix | POS | Count |
|-----|--------|-----|-------|
| AA | prefix | 2,281 |
| AB | suffix | 1,319 |
| AC | infix | 51 |
| AD | interfix | 39 |
| AE | circumfix | 5 |
| AF | affix (generic) | 1 |

##### Layer C: Words

| Sub | Prefix | POS | Count |
|-----|--------|-----|-------|
| CA | noun | 787,662 |
| CB | verb | 180,945 |
| CC | adj | 148,596 |
| CD | adv | 23,729 |
| CE | prep | 509 |
| CF | conj | 209 |
| CG | det | 173 |
| CH | pron | 608 |
| CI | intj | 3,024 |
| CJ | num | 456 |
| CK | symbol | 409 |
| CL | particle | 36 |
| CM | punct | 50 |
| CN | article | 5 |
| CR | character | 109 |

##### Layer D: Derivatives

| Sub | Prefix | Type | Count |
|-----|--------|------|-------|
| DA | abbreviation | 3,465 |
| DB | initialism | 21 |
| DD | contraction | 493 |

##### Layer E: Multi-word

| Sub | Prefix | Type | Count |
|-----|--------|------|-------|
| EA | phrase | 4,690 |
| EB | prep_phrase | 2,864 |
| EC | proverb | 1,530 |

### ~~Name Components Mode (yA)~~ — RETIRED

> **Decision 002 (2026-02-12):** The yA namespace and hcp_names database are retired. A "name" is a Proper Noun construct (bond pattern), not a token property. Name-bearing words move to hcp_english as regular tokens with capitalized form variants. Words that are only names get PoS = `label`. See [Decision 002](../decisions/002-names-shard-elimination.md).

The hcp_names database (~150,528 tokens) is preserved pending migration of useful data into hcp_english.

### Entity Modes (v*, w*, x*) — Reserved

```
vA.*                            People entities (future)
wA.*                            Place entities (future)
xA.*                            Thing entities (future)
```

Specific named entities. Each atomizes to word tokens in the appropriate language shard (e.g. AB.AB for English words). A Proper Noun is a bond pattern assembled from word tokens — the entity namespace records the construct, while the component words live in their language shard.

> **Planned change:** These namespaces will eventually shift to sequential allocation rather than scattered single-letter prefixes. Current allocation is provisional.

### Source PBM Mode (zA) — Reserved

```
zA.*                            Stored PBMs, documents, expressions
```

#### Atomization Rules

- **Affixes (A)** → Character Token IDs
- **Words (C)** → Character Token IDs
- **Derivatives (D)** → Character Token IDs + link to root form
- **Phrases (E)** → Word Token IDs if all components exist; else Character Token IDs

Derivatives don't have independent definitions; they reference their expanded form.

## Key Concepts

### Atomization
The breakdown of a token into its component parts from the layer below.
- Phrases → word tokens (if all components exist)
- Words → character tokens
- Characters → byte tokens (per encoding table)
- Stored in token metadata as an array of Token IDs

### Composition (future)
Building up from tokens to semantic concepts. Reserved for later work.

### Grammar as NSM Bridge
Grammar profiles (word order, agreement patterns from WALS) act as the transformation layer between universal NSM primitives and language-specific expression. The grammar mesh shapes interpretation space.

## Architecture Notes

### Database Strategy
- **PostgreSQL**: Workbench for analysis, writes, cross-shard assembly
- **LMDB**: Read-only inference layer (optimized snapshots from Postgres)
- **Shards**: ~2GB target size for hot-loading multiple shards simultaneously

### Shard Types
- Language shards (English, etc.) with accompanying PBMs
- Operational shards (variables, logging) sized to workload

### Token ID Efficiency
- IDs encode shard routing path directly (no lookup needed)
- Within-shard storage can drop common prefix (implied context)
- Full IDs for cross-shard communication
