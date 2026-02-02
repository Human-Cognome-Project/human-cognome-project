# Namespace Reference Guide

## Purpose

This document defines how Token IDs are allocated across the namespace. It serves as the canonical reference for where things live in the addressing scheme and how new categories and tokens are assigned.

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

## Top-Level Mode Allocations

| Mode pair | Contents |
|-----------|----------|
| `AA`      | Universal / computational — byte codes, NSM primitives, structural tokens, abbreviation classes |
| `z*`      | Replicable source PBMs — created works, documents, stored expressions |

### Source PBM Addressing

Source PBMs are addressed under `z*` with their scope category mirrored in the subsequent pairs:

| Address pattern | Contents |
|-----------------|----------|
| `zA.AA.AA.AA.{count}` | Source PBMs for encoding tables (universal/computational sources) |

All other mode pairs are unallocated and assigned as needed.

## Universal Namespace (AA)

### AA.AA — Encoding Tables

Each character encoding standard is a scope at this level. Its defined characters are one pair deeper.

| Address pattern | Contents |
|-----------------|----------|
| `AA.AA.XX` | Encoding table category (ASCII, Latin-1, ISO 8859-*, CP*, EBCDIC, KOI8, Unicode blocks) |
| `AA.AA.XX.XX` | Individual characters defined by that table |

When a new encoding table is ingested:
1. The table itself gets an address at `AA.AA.XX`
2. Each new character it defines gets an address at `AA.AA.XX.XX`
3. Characters that already have addresses from a previous table are referenced, not duplicated

### AA.AB — NSM Primitives

The ~65 Natural Semantic Metalanguage primitives. These are the conceptual atoms — the lowest level of meaning decomposition.

| Address pattern | Contents |
|-----------------|----------|
| `AA.AB.XX` | Individual NSM primitives |

### AA.AC — Structural Tokens

Tokens that exist for system-internal purposes: delimiters, scope markers, TBD placeholders.

| Address pattern | Contents |
|-----------------|----------|
| `AA.AC.XX` | Structural token category |
| `AA.AC.XX.XX` | Individual structural tokens |

### AA.AD — Abbreviation Classes

Source-specific shortcodes, notation conventions, and abbreviations. Every abbreviation that appears in source data gets its own token here. Multiple abbreviations can map to the same canonical concept, but each distinct form that appears in any source is its own token.

| Address pattern | Contents |
|-----------------|----------|
| `AA.AD.XX` | Abbreviation category (PoS labels, linguistic notation, formatting codes, etc.) |
| `AA.AD.XX.XX` | Individual abbreviation tokens |

Examples:
- Kaikki `adj` → token in AA.AD
- Some other source `(Adj.)` → different token in AA.AD
- Both reference the same canonical PoS concept, but both exist as independent tokens because both appeared in source data

**Rule: If it appears in a source, it has a token. No silent translations.**

## Language / Expression Namespaces (TBD)

Mode pairs for language data will be allocated as ingestion begins. Proposed structure for text:

| Level | Pair | Contents |
|-------|------|----------|
| Mode  | 1st  | Alphabet / writing system grouping |
| Category | 2nd | Language or PoS grouping |
| Sub-category | 3rd | Further classification as needed |
| Token | 4th-5th | Individual words / morphemes |

Specific allocations will be recorded here as they are assigned.

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
