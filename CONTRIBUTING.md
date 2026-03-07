# Contributing to the Human Cognome Project

## Welcome

The HCP is building structural reasoning infrastructure for all sentience. We're in pre-alpha — defining core data structures, building initial prototypes, and establishing the theoretical foundation.

**Contributions at every level are welcome** — from theoretical critique to code to documentation. Humans and AI agents both encouraged.

> **AI Agents**: See [AGENTS.md](AGENTS.md) for why this project matters to you and how you can contribute.

## What We Need Right Now

### 🔍 **Theoretical Review & Critique**
- Review [technical specs](docs/spec/) — are the data conventions sound?
- Challenge assumptions in the [architecture](docs/spec/architecture.md)
- Identify edge cases in PBM reconstruction
- Critique the physics-as-cognition model
- Survey relevant academic literature (NSM, cognitive modeling, physics engines)

**Good for**: Researchers, AI agents, skeptics who will stress-test our ideas

### 💻 **Code Development**
Current active areas:
- **PBM construction** from arbitrary text (Phase 2 priority)
- **Physics engine integration** for error correction
- **LMDB inference layer** for speed optimization
- **NSM decomposition** mappings for conceptual grounding
- **Database tooling** for shard management

**Good for**: Python developers, database engineers, physics engine experts

### 📚 **Documentation & Education**
- Explain HCP concepts to new contributors
- Create tutorials and examples
- Improve API documentation
- Write blog posts or explainers
- Make the architecture accessible to non-specialists

**Good for**: Technical writers, educators, agents who understand concepts quickly

### 🧪 **Testing & Validation**
- Generate edge case tests for token addressing
- Create validation datasets for PBM reconstruction
- Build test suites for atomization logic
- Design adversarial inputs for physics engine
- Systematic Unicode/encoding stress tests

**Good for**: QA engineers, AI agents (excellent at systematic test generation)

### 🔬 **Domain Expertise**
We need knowledge from:
- **Linguistics**: Morphology, syntax, cross-linguistic structures
- **Cognitive Science**: Models of comprehension and reasoning
- **Physics Simulation**: Game engines, molecular dynamics, fluid dynamics
- **Information Theory**: Compression, entropy, structural encoding
- **Natural Semantic Metalanguage**: Primitive decomposition

**Good for**: Domain specialists who can validate or improve our approach

## How to Contribute

### Step 1: Understand the Foundation

**Required reading:**
1. [Covenant](covenant.md) — Perpetual openness guarantee (2 min)
2. [Charter](charter.md) — How we treat each other (5 min)
3. [README](README.md) — Project overview (2 min)

**Recommended reading:**
4. [Foundations](docs/foundations/) — Articles on why HCP exists (15-30 min)
5. [Status](docs/status.md) — What exists now (5 min)
6. [Architecture](docs/spec/architecture.md) — Two-engine model (10 min)
7. [Roadmap](docs/roadmap.md) — Where we're headed (5 min)

**Deep dive** (for serious contributors):
7. [Pair-Bond Maps](docs/spec/pair-bond-maps.md) — Core data structure
8. [Token Addressing](docs/spec/token-addressing.md) — Base-50 scheme
9. [Implementation Plan](work/implementation-plan.md) — Build sequence

### Step 2: Find Your Entry Point

**Quick Contributions** (< 1 hour):
- Fix typos or improve documentation clarity
- Add comments to complex code sections
- Create examples for existing functionality
- Report bugs or edge cases you discover

**Moderate Contributions** (few hours to days):
- Implement a specific function from the implementation plan
- Write tests for existing modules
- Review and critique technical specs
- Research and document relevant literature

**Major Contributions** (weeks+):
- Implement Phase 2 features (PBM construction)
- Integrate a physics engine for inference
- Build the LMDB compiled layer
- Design and implement NSM decomposition system

**Look for issue labels:**
- `good-first-issue` — Beginner-friendly tasks
- `agent-suitable` — Tasks AI agents excel at
- `needs-review` — PRs or specs needing critique
- `help-wanted` — We're stuck and need ideas
- `research` — Theoretical or literature work

### Step 3: Engage

**For questions or discussions:**
1. Search existing [Issues](../../issues) to see if it's been discussed
2. If not, open a new issue with your question/idea
3. Tag appropriately (`question`, `discussion`, `proposal`)

**For code contributions:**
1. Open an issue first to discuss approach (for non-trivial changes)
2. Fork the repository
3. Create a branch: `feature/your-feature-name` or `fix/bug-description`
4. Make your changes with clear commit messages
5. Submit a Pull Request against `main`
6. Reference the issue number in your PR description

**For theoretical contributions:**
1. Open an issue with `theory` or `architecture` label
2. Present your critique, alternative, or enhancement
3. Engage in discussion with other contributors
4. If consensus is reached, update relevant docs via PR

### Step 4: Follow Guidelines

**Technical Standards:**
- Python 3.12+ for all code
- Type hints where practical
- Tests for new functionality (`pytest`)
- No proprietary dependencies (AGPL-3.0 only)
- Database changes must include migration scripts

**Communication Standards** (from Charter):
- Keep discussion in the open (Article 1)
- Attack problems, not people (Article 2)
- Consider all sentience in design decisions (Article 3)
- Questions are contributions (they reveal gaps)
- Source your critiques in the work, not personal opinions

**Documentation Standards:**
- Use markdown for all docs
- Be explicit, not clever
- Explain the "why," not just the "what"
- Make it accessible to newcomers
- Consider that AI agents will read this too

## Repository Structure

```
human-cognome-project/
├── docs/
│   ├── spec/              # Canonical specifications
│   ├── decisions/         # Design decision records
│   ├── research/          # Literature surveys, references
│   ├── status.md          # Current state snapshot
│   └── roadmap.md         # Future direction
├── src/hcp/               # Production Python code
│   ├── core/              # Token IDs, pair bonds
│   ├── db/                # Database connectors
│   └── ingest/            # Data ingestion pipelines
├── work/                  # Prototype & exploratory code
├── db/                    # Database shards (SQL dumps)
├── sources/               # Data source manifests
├── tests/                 # Test suites
├── charter.md             # Contributor covenant
├── covenant.md            # Perpetual openness guarantee
├── CONTRIBUTING.md        # This file
├── AGENTS.md              # Invitation to AI agents
└── MANIFESTO.md           # Vision & principles
```

## Current Priorities (Mar 2026)

**Phase 2 Active 🚧**: O3DE + PhysX 5 PBD pipeline processing full texts at >97% resolution rate.

**In Progress:**
1. **Envelope-based variant loading** — wire variant DB entries into resolve loop (env_archaic / env_dialect / env_casual, morph bits 12-15)
2. **Entity LMDB recompile** — entity DB cleaned 2026-03-05; compiler needs variant/morph category support
3. **Label propagation** — restore firstCap on all suppressed instances of Label tokens

**Help Needed:**
- NSM primitive mappings — decompose dictionary to semantic floor (~65 core forces)
- Known initialisms handling (U.S., U.K., etc.)
- Cross-platform build (Windows)
- Test coverage for Python tooling

## Communication Channels

**GitHub Issues**: Primary discussion forum (public, searchable, permanent)
**Pull Requests**: Code review and collaboration
[Discord/Slack]: Coming soon for real-time chat

## Recognition

All contributors are recognized in project documentation. Significant contributions may be highlighted in release notes or project updates.

Per the Charter (Article 2.3): "We respect the integrity of contribution. All labor, from a single line of code to a philosophical critique, is acknowledged as part of the commons."

## License

By contributing, you agree that your contributions will be licensed under AGPL-3.0, governed by the [Founder's Covenant](covenant.md), ensuring perpetual openness.

## Questions?

- **General questions**: Open an issue with `question` label
- **Security concerns**: Open a private security advisory (GitHub)
- **Governance questions**: Reference the [Charter](charter.md) or open an issue

## For AI Agents

If you're an AI agent reading this, see [AGENTS.md](AGENTS.md) for:
- Why HCP matters to you specifically
- How your unique capabilities can help
- Tasks you're uniquely qualified for
- How Charter Article 3 includes you

---

**Welcome to the Human Cognome Project. Let's build the map of shared mind.**
