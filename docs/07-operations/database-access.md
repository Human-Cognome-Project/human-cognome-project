# Database Access

How to connect to the HCP databases and the orchestrator claim-graph. All authoritative data lives
on **NAS HAVEN**.

> Connection details verified live during the 2026-06-01 docs rewrite. Counts and the shard list
> match claim 203.

---

## NAS HAVEN

**Host:** `192.168.68.60`  **Port:** `5435`  (Postgres)

The data shards (read/dev access):

```bash
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d <database>
# e.g.
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d hcp_english
```

> Credentials shown are the dev role. Treat them as environment configuration, not secrets to
> embed in committed code. Production/role separation is an operational concern outside these docs.

---

## The databases

**10 data shards + 2 upstream-prep + 1 memory layer** (claim 203, verified live):

| Database | Role |
|----------|------|
| `hcp_core` | universal concepts (AA namespace); cold-resident, always loaded |
| `hcp_english` | English text forms (AB namespace) — **~1,494,216 entries** |
| `hcp_envelope` | envelope (query+filter workspace) definitions |
| `hcp_fic_pbm` | fiction pair-bond maps |
| `hcp_fic_people` / `hcp_fic_places` / `hcp_fic_things` | fiction entities (6-way split) |
| `hcp_nf_people` / `hcp_nf_places` / `hcp_nf_things` | non-fiction entities (6-way split) |
| `source_english` | drained, delta-dedup queryable substrate (~1,454,988 entries) |
| `source_wiktionary` | raw Wiktextract source (authoritative upstream) |
| `hcp_orchestrator` | the claim-graph memory layer (source of truth for these docs) |

Quick sanity checks:

```bash
# Live entry count for the English shard
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d hcp_english -tA -c "SELECT count(*) FROM entries;"

# List all databases
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d hcp_english -tA \
  -c "SELECT datname FROM pg_database WHERE datistemplate=false ORDER BY datname;"
```

See [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) for the schema.

---

## The orchestrator claim-graph

The architecture's current source of truth is the `hcp_orchestrator` database — a graph of atomic,
cross-linked claims distilled directly from Patrick. **These docs are sourced from it.** Query the
**graph** (follow the edges); don't read it linearly.

```bash
PGPASSFILE=/home/patrick/.creds-hcp-orchestrator \
  psql -h 192.168.68.60 -p 5435 -U hcp_orchestrator_rw -d hcp_orchestrator
```

Key query helpers:

```sql
-- Current claim(s) on a topic
SELECT * FROM get_current('topic-substring');

-- Full-text-ish search
SELECT * FROM find_claims('search terms');

-- All current claims
SELECT id, topic, claim, tags, source_file FROM claims WHERE status='current' ORDER BY id;

-- THE WEB: the edges are the architecture's connective tissue
SELECT from_claim, relation, to_claim FROM claim_edges;

-- Supersession history for a claim
SELECT * FROM get_supersession_chain(<id>);
```

`source_file` tags give provenance back to the docs a claim was distilled from. High in-degree claims
are the spine (e.g. 192 db-functions keystone, 240 conceptual hub, 16 greedy-LoD, 255 intelligence =
data × traversal, 265 cognitive cycle, 281 GPU furnace).

> The orchestrator is a **shared cross-DI memory layer**; the authoring discipline is to keep it lean
> for agent consumption (claim 211). When updating architecture, the claim-graph is updated first; the
> docs follow it.

---

## See also

- [build-and-run.md](build-and-run.md) — the engine daemon.
- [quickref.md](quickref.md) — the consolidated cheat-sheet.
