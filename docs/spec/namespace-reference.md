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
AB                          Text mode
├── AB.AA                   ASCII/Unicode Characters
│   └── AB.AA.AA.{cat}.{n}  Character tokens by category
└── AB.AB                   English Language Family (planned)
    └── AB.AB.{POS}.{n}.{n} Words (double-duty 3rd pair)
```

#### Word Addressing (AB.AB)

The 3rd pair uses double-duty encoding:
- 1st character: Major POS (A=morpheme, B=noun, C=verb, D=adj, etc.)
- 2nd character: Sub-POS (grammatical subdivision)

Example sub-categories:
- Nouns: countable, uncountable, plural-only, collective
- Verbs: transitive, intransitive, ambitransitive
- Morphemes: prefix, suffix, infix, interfix

Morphemes come first in the address space as foundational word parts.

## Key Concepts

### Atomization
The breakdown of a token into its component parts from the layer below.
- Unicode chars → byte sequences (per encoding table)
- Words → morphemes/letters
- Stored in token metadata with encoding table context

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
