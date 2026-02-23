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
│   ├── AA.AC.AA             Conceptual Mesh (NSM primitives + structural tokens, 2,719 tokens)
│   └── AA.AC.AB             Force Infrastructure (25 tokens)
│       ├── AA.AC.AB.AA      Force types (7)
│       ├── AA.AC.AB.AB      Relationship types (7)
│       ├── AA.AC.AB.AC      LoD levels (8)
│       └── AA.AC.AB.AD      Structural principles (3)
├── AA.AD                   Abbreviation Classes
├── AA.AE                   PBM/Document Structural Tokens (93 tokens)
│   ├── AA.AE.AA             Block-level markers (33)
│   ├── AA.AE.AB             Inline formatting markers (22)
│   ├── AA.AE.AC             Annotation markers (14)
│   ├── AA.AE.AD             Alignment/layout markers (13)
│   ├── AA.AE.AE             Non-text content markers (10)
│   └── AA.AE.AF             Stream boundary anchors (2) — engine particle types
└── AA.AF                   Entity Classification Tokens
    ├── AA.AF.AA             Person sub-types (4)
    ├── AA.AF.AB             Place sub-types (6)
    ├── AA.AF.AC             Thing sub-types (7)
    └── AA.AF.AD             Entity relationship types
```

### Text Mode (AB)

```
AB                              Text mode
├── AB.AA                       Unicode Characters
│   └── AB.AA.AA.{cat}.{n}      Character tokens by category (2,032 tokens)
└── AB.AB                       English Language Family (~1.4M tokens)
    ├── AB.AB.A{sub}.{n}.{n}    Layer A: Affixes (3,696 tokens)
    ├── AB.AB.B{sub}.{n}.{n}    Layer B: Fragments (reserved)
    ├── AB.AB.C{sub}.{n}.{n}    Layer C: Words (~1.29M tokens)
    ├── AB.AB.D{sub}.{n}.{n}    Layer D: Derivatives (3,979 tokens)
    ├── AB.AB.E{sub}.{n}.{n}    Layer E: Multi-word (9,084 tokens)
    └── AB.AB.FA.{sub}.{n}      Sub-cat Patterns (30 tokens)
        ├── AB.AB.FA.AA          Verb patterns (17)
        ├── AB.AB.FA.AB          Noun patterns (5)
        ├── AB.AB.FA.AC          Adjective patterns (5)
        └── AB.AB.FA.AD          Preposition patterns (3)
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
| C | CA-CR | Words: noun, verb, adj, adv, prep, conj, etc. | ~1,290,000 |
| D | DA-DE | Derivatives: abbreviation, initialism, acronym, contraction, clipping | 3,979 |
| E | EA-EC | Multi-word: phrase, prep_phrase, proverb | 9,084 |
| F | FA | Sub-categorization patterns | 30 |

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
| CA | noun | 930,925 |
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

Note: CA (noun) count includes ~143K label tokens from the names merge (Decision 002/Migration 004).

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

##### Layer F: Sub-categorization Patterns

| Sub | Prefix | Type | Count |
|-----|--------|------|-------|
| FA.AA | verb patterns | 17 |
| FA.AB | noun patterns | 5 |
| FA.AC | adjective patterns | 5 |
| FA.AD | preposition patterns | 3 |

### Non-Fiction Namespaces

```
z*                              Non-fiction PBMs
├── zA                          Source PBMs (universal/text-mode)
│   ├── zA.AA                   Byte-level computational content
│   └── zA.AB                   Text-mode content
│       ├── zA.AB.A*             Tables (CSV, TSV, etc.)
│       ├── zA.AB.B*             Dictionaries, lexicons
│       ├── zA.AB.C*             Books (non-fiction)
│       ├── zA.AB.D*             Articles, papers, essays
│       └── zA.AB.E*             Correspondence, letters

y*                              Non-fiction People Entities
├── yA                          Real people (English-primary shard)
│   ├── yA.AA.*                 Individuals
│   ├── yA.BA.*                 Collectives
│   ├── yA.CA.*                 Deities/divine figures
│   └── yA.DA.*                 Named creatures

x*                              Non-fiction Place Entities
├── xA                          Real places (English-primary shard)
│   ├── xA.AA.*                 Settlements
│   ├── xA.BA.*                 Geographic features
│   ├── xA.CA.*                 Buildings/structures
│   ├── xA.DA.*                 Regions/territories
│   ├── xA.EA.*                 Worlds/planes
│   └── xA.FA.*                 Celestial bodies

w*                              Non-fiction Thing Entities
├── wA                          Real things (English-primary shard)
│   ├── wA.AA.*                 Objects/artifacts
│   ├── wA.BA.*                 Organizations
│   ├── wA.CA.*                 Species/races
│   ├── wA.DA.*                 Concepts/systems
│   ├── wA.EA.*                 Events
│   ├── wA.FA.*                 Languages
│   └── wA.GA.*                 Materials/substances
```

### Fiction Namespaces

```
v*                              Fiction PBMs
├── vA                          Fiction text-mode PBMs
│   └── vA.AB                   Text-mode fiction content
│       ├── vA.AB.C*             Books (fiction)
│       ├── vA.AB.D*             Stories, scripts, screenplays
│       └── vA.AB.E*             Poetry, lyrics

u*                              Fiction People Entities
├── uA                          Fiction characters (English-primary shard)
│   ├── uA.AA.*                 Individuals
│   ├── uA.BA.*                 Collectives
│   ├── uA.CA.*                 Deities/divine figures
│   └── uA.DA.*                 Named creatures

t*                              Fiction Place Entities
├── tA                          Fiction places (English-primary shard)
│   ├── tA.AA.*                 Settlements
│   ├── tA.BA.*                 Geographic features
│   ├── tA.CA.*                 Buildings/structures
│   ├── tA.DA.*                 Regions/territories
│   ├── tA.EA.*                 Worlds/planes
│   └── tA.FA.*                 Celestial bodies

s*                              Fiction Thing Entities
├── sA                          Fiction things (English-primary shard)
│   ├── sA.AA.*                 Objects/artifacts
│   ├── sA.BA.*                 Organizations
│   ├── sA.CA.*                 Species/races
│   ├── sA.DA.*                 Concepts/systems
│   ├── sA.EA.*                 Events
│   ├── sA.FA.*                 Languages
│   └── sA.GA.*                 Materials/substances
```

### Database Hosting

| Database | Namespaces | Contents |
|----------|------------|----------|
| hcp_core | AA | Universal tokens, encoding, structural, force infrastructure |
| hcp_english | AB | English language family (~1.4M tokens) |
| hcp_en_pbm | zA | Non-fiction PBMs — DROPPED: prior implementation was incorrect (flat word list, not bond maps). Will be recreated with correct aggregated bond pair schema. |
| hcp_fic_entities | u*, t*, s* | All fiction entities (bootstrap — splits later) |
| hcp_nf_entities | y*, x*, w* | All non-fiction entities (bootstrap — splits later) |
| (future) | vA | Fiction PBMs |

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
- Entity shards (fiction/non-fiction, split by entity type when needed)
- Operational shards (variables, logging) sized to workload

### Token ID Efficiency
- IDs encode shard routing path directly (no lookup needed)
- Within-shard storage can drop common prefix (implied context)
- Full IDs for cross-shard communication
