# Document Processing Workstation — User Stories

**Date:** 2026-02-19
**Status:** Initial brainstorm, needs Patrick's input

## Design Principle

Build modular tools on the O3DE platform. Each tool addresses a specific actor's needs. All tools interact with Postgres (source of truth) and the Asset Processor pipeline. MVP builds the reference implementation (.txt); AGENT.md and TODO.md guide contributors for additional formats and tools.

## Actors

| Actor | Description | Technical level |
|---|---|---|
| Document processor | Feeds texts into the system. Patrick's hubby, volunteers. | Non-technical — needs GUI or simple CLI |
| Linguist | Defines forces, sub-cat patterns, conceptual mesh | Domain expert, not necessarily a coder |
| Engine developer | Tokenizer, physics pipeline, runtime code | C++ developer |
| DB specialist | Schema, vocabulary, aggregation, data integrity | SQL + systems |
| Format contributor | Adds builders for new input types (PDF, EPUB, etc.) | Developer following reference pattern |
| Language contributor | Adds new language shards (vocabulary, force constants) | Linguist + some technical |

## User Stories — Document Processing

- As a document processor, I drop .txt files in a folder and they get tokenized and stored automatically
- As a document processor, I see processing status, progress, and errors in a GUI dashboard (Asset Processor)
- As a document processor, I batch-process 300K+ texts headlessly overnight or in CI/CD
- As a document processor, I can see which documents failed and why (missing vocab, encoding issues, etc.)
- As a document processor, I can re-process documents after vocabulary updates without manual intervention
- As a document processor, I don't need to install the full engine — just the SDK + builder tool

## User Stories — Vocabulary & Language

- As a linguist/DB specialist, I can see which tokens the tokenizer couldn't resolve (var_request emissions)
- As a DB specialist, I can add missing words to vocabulary and trigger reprocessing of affected documents
- As a linguist, I can browse vocabulary coverage statistics (% resolved, common misses)
- As a linguist, I can test tokenization of sample text interactively and see the token stream
- As a language contributor, I can add a new language shard and verify tokenization against test texts

## User Stories — Inspection & Analysis

- As an engine developer, I can view a document's position map (token stream with positions)
- As an engine developer, I can derive and inspect PBM bonds for any document on the fly
- As a linguist, I can compare tokenization across documents (same word tokenized differently?)
- As a DB specialist, I can see cross-document bond aggregation statistics
- As Patrick, I can do visual flow work — inspect document structure, bond patterns, particle layouts in the editor

## User Stories — Development

- As a format contributor, I can build and test a new format builder without the full engine
- As an engine developer, I can test tokenizer changes against known-good outputs (regression testing)
- As a format contributor, I follow AGENT.md and TODO.md to add a new input format
- As an engine developer, I can run the builder independently from the runtime engine

## Tools That Fall Out of These Stories

(TBD — to be derived from the stories above)

Candidates:
- **Asset Processor** (existing) — document processing GUI + batch mode
- **Vocabulary inspector** — browse vocab, see coverage, find gaps
- **Tokenization tester** — interactive "paste text, see tokens" tool
- **Document inspector** — view position maps, derive PBM, inspect structure
- **Builder test harness** — run a builder against a single file, verify output
- **Batch processor** — headless CLI for bulk processing (AssetProcessorBatch + config)

## First Analysis Tool: Boilerplate / Plagiarism / Comparison

After document reconstruction works, build a comparison method using physics:

- Two documents laid side-by-side as parallel particle streams
- **Same tokens attract** (one-directional magnetic force across the gap)
- **Different tokens repel** (push away)
- **Spaces are sticky but not rigid** — soft constraints allow sliding placement so phrases can align even with extra/missing words
- **Convergence clusters = shared passages** — where particles clump across the gap
- Simple first version: unidirectional magnetic forces only
- This is the first real PhysX particle pipeline use case beyond self-test

User stories:
- As a document processor, I can compare two documents and see shared passages highlighted
- As a document processor, I can detect boilerplate (Gutenberg headers, repeated prefaces)
- As a linguist, I can see how the same content is expressed differently across documents

## Open Questions

- What other actors are there?
- What other stories can we predict?
- Which tools are separate binaries vs panels in the Asset Processor GUI?
- How do tools authenticate to Postgres? Shared connection config?
- What's the minimum viable inspection tool for the MVP?
