# HCP Engine — Agent & Contributor Guide

This file provides instructions for both AI coding agents and human contributors working on the HCP Engine Gem. A root-level version will eventually reference this file and others for project-wide guidance.

## Project Structure

```
hcp-engine/
├── Gem/
│   ├── Source/              # All C++ source (engine + builders)
│   ├── CMakeLists.txt       # Build targets (runtime + builder)
│   ├── hcpengine_files.cmake        # Runtime source list
│   └── hcpengine_builder_files.cmake # Builder source list (planned)
├── Assets/                  # Project assets
├── Levels/                  # Level definitions
├── Registry/                # O3DE settings (.setreg files)
├── build/linux/             # Build output (ninja)
├── Cache/                   # Asset Processor output
├── ROADMAP.md               # What we're building and why
├── AGENTS.md                # This file
└── TODO.md                  # Current task list
```

## Roles

The project has specialist roles. Each owns specific areas:

| Role | Owns | Does NOT touch |
|------|------|----------------|
| **Engine** | Tokenizer, particle pipeline, Asset Builders, runtime components | DB schema, LMDB format, linguistics |
| **DB** | PostgreSQL schema, migrations, LMDB export, vocabulary data | Engine C++ code, physics |
| **PBM** | Pair Bond Map encoding/reconstruction, OpenMM integration | DB schema, tokenizer internals |
| **Linguistics** | Force definitions, sub-categorization patterns, conceptual mesh | Engine code, DB schema |
| **Infrastructure** | CI/CD, repo structure, deployment, contributor tooling | Domain-specific code |

If you're uncertain which role owns something, ask before changing it.

## Branch Naming

```
<type>/<short-description>

Types:
  feat/     — New functionality
  fix/      — Bug fix
  refactor/ — Code restructuring (no behaviour change)
  docs/     — Documentation only
  build/    — Build system, CMake, dependencies
  test/     — Tests only

Examples:
  feat/document-builder
  fix/marker-table-pk-collision
  docs/roadmap-phase-2
  build/builder-cmake-target
```

Keep branch names short and lowercase with hyphens. No ticket numbers in branch names (we're not using an issue tracker yet).

## Commit Messages

```
<imperative summary> (max 72 chars)

<optional body — what and why, not how>

Co-Authored-By: <agent name> <noreply@anthropic.com>
```

- Use imperative mood: "Add document builder", not "Added" or "Adds"
- First line is the full message for small changes
- Body for anything non-obvious
- AI agents MUST include Co-Authored-By tag
- Use Patrick's email as author (git config is already set)

## Development Process

### Before Writing Code

1. Read ROADMAP.md to understand where your work fits
2. Read TODO.md to find current priorities
3. Read the relevant source files before modifying them
4. If your change touches module boundaries (e.g., tokenizer interface, vocabulary API, position format), discuss first

### Building

```bash
cd /opt/project/hcp-engine/build/linux
ninja
```

After CMakeLists.txt changes:
```bash
cmake -B /opt/project/hcp-engine/build/linux \
  -S /opt/project/hcp-engine \
  -G Ninja \
  -DLY_3RDPARTY_PATH=~/.o3de/3rdParty/packages
ninja
```

### Testing

- Build must succeed with `ninja` (zero warnings preferred, zero errors required)
- Tokenizer changes: verify output against known-good results (Yellow Wallpaper: 9,122 tokens, 18,840 slots)
- New modules: include a self-test path callable from the system component or a standalone harness

### Code Style

- Follow existing patterns in `Gem/Source/`
- `AZ_` macros for O3DE integration (AZ_COMPONENT, AZ_RTTI, etc.)
- `HCP` prefix for all our classes
- No Python for engine work — C++ only
- No over-engineering: minimum complexity for current requirements

## Adding a New Format Builder

This is the primary contribution path. The `.txt` builder is the reference implementation.

### What's Format-Specific (you write this)

- Text extraction / reading step (how to get UTF-8 text from your source format)

### What's Universal (you reuse this)

- Tokenizer pipeline (4-step space-to-space)
- Vocabulary access (Postgres query interface)
- Position encoding (base-50)
- Product format (position map + metadata)
- Builder registration pattern (CreateJobs / ProcessJob)

### Steps

1. Create `HCP<Format>Builder.h/.cpp` following the pattern in `HCPDocumentBuilder.h/.cpp`
2. Register your file patterns in `CreateJobs` (e.g., `*.pdf`, `*.epub`)
3. Implement text extraction in `ProcessJob` — get UTF-8 text from your format
4. Pass extracted text to the shared tokenizer pipeline
5. Add your files to `hcpengine_builder_files.cmake`
6. Build, test with a sample file, verify position map output

### Dependencies You'll Need

| Format | Library | Notes |
|--------|---------|-------|
| PDF | poppler or pdftotext | Text extraction, handle encoding |
| EPUB | libzip + XML parser | Unzip, parse XHTML content |
| HTML | libxml2 or similar | Strip markup, preserve structure |
| Wikipedia | custom parser | MediaWiki markup → text |

## Key Design Rules

These are non-negotiable. Violating them wastes everyone's time.

1. **Engine IS the tokenizer** — all processing in C++/PhysX. Do not write Python that simulates engine work.
2. **Disassembly AND reassembly are physics operations** — do not write sequential algorithms and call them physics.
3. **PostgreSQL is source of truth** — do not invent DB formats or modify LMDB structure. DB specialist owns that.
4. **PBM is derived, not stored** — position maps are the product. Do not store pre-computed PBM bond tables.
5. **Bonds are directional** — "the->cat" and "cat->the" are different bonds. Document order is preserved in bond direction.
6. **Nothing is ever stripped except space (0x20)** — newlines, tabs, CR, all punctuation, all structural markers are preserved as tokens. This is exact reproduction.
7. **Punctuation lives in hcp_core (AA namespace)** — not in language shards.
8. **~65 core forces expected** — categorization TBD, linguist-driven. Do not hardcode force counts.

## Module Boundaries

These modules are designed for reuse across all tools (builder, inspector, tester, runtime):

| Module | Interface | Implementations |
|--------|-----------|-----------------|
| **Vocabulary** | LookupChunk, CheckContinuation, LookupChar | HCPVocabulary (LMDB, runtime), HCPDocumentBuilderVocab (Postgres, build-time) |
| **Tokenizer** | ProcessText → token stream | HCPTokenizer (shared between runtime and builder) |
| **Position Map** | Read/Write position maps | Shared format, one reader, one writer |
| **PBM Derivation** | Positions in → bond counts out | Single function, on-the-fly |
| **Connection Config** | Postgres connection string, pool settings | Shared across all tools |

## For AI Agents Specifically

- Read this file and ROADMAP.md on every new session
- Check TODO.md for current priorities
- Do not guess at design decisions — ask if unclear
- Do not create files unless necessary; prefer editing existing ones
- Do not add comments, docstrings, or type annotations to code you didn't change
- Log large outputs to files, never dump to stdout
- When modifying shared interfaces (vocabulary, tokenizer), check both runtime and builder consumers
