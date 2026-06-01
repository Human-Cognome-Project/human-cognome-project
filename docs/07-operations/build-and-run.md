# Build and Run

How to build the engine Gem and talk to the running daemon.

> This page documents the **operational surface** verified against the repo (`hcp-engine/`,
> `scripts/hcp_client.py`) and the engine baseline (claim 201). Engine *internals* are deferred
> pending rework (claims 201/239) — see
> [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) — so treat build specifics
> as current-state, not a frozen contract. Engine-internal build notes live under
> [`hcp-engine/`](../../hcp-engine/) (its `AGENTS.md`, `ROADMAP.md`, `CMakeLists.txt`).

---

## The engine

The NAPIER inference engine is an **O3DE 25.10.2 C++ Gem** using **PhysX 5 GPU-accelerated PBD**
(claim 201). It runs headless as a daemon.

### Build

The engine builds through O3DE's CMake/ninja flow. The Gem source is under
[`hcp-engine/Gem/Source/`](../../hcp-engine/Gem/Source/); build targets are defined in
`hcp-engine/Gem/CMakeLists.txt` (`hcpengine_files.cmake` is the runtime source list). Build output
lands in `hcp-engine/build/linux/` (ninja).

See the engine's own [`hcp-engine/AGENTS.md`](../../hcp-engine/AGENTS.md) for the current O3DE
build invocation and prerequisites — that is the authoritative, engine-local build reference, and it
tracks the active build configuration as the Gem evolves.

### Run the daemon

The headless daemon is `HCPEngine.HeadlessServerLauncher`. It listens on **TCP port 9720** with a
JSON socket API.

---

## The socket API

The protocol is **length-framed JSON**: a 4-byte big-endian length prefix followed by a UTF-8 JSON
payload (verified in `scripts/hcp_client.py`).

API verbs (claim 201): `health`, `ingest`, `retrieve`, `list`, `tokenize`, `phys_ingest`.

### Using the reference client

[`scripts/hcp_client.py`](../../scripts/hcp_client.py) is the reference client (Python — front-end
tooling only; it is not an engine component, per the language policy in
[../../CONTRIBUTING.md](../../CONTRIBUTING.md)).

```bash
# Ingest a single file
python scripts/hcp_client.py ingest <file>

# Ingest all .txt files in a directory
python scripts/hcp_client.py ingest <dir>

# List all documents
python scripts/hcp_client.py list

# Delete a document by doc_id
python scripts/hcp_client.py delete <doc_id>

# Send a raw JSON action
python scripts/hcp_client.py raw '{"action":"ping"}'
```

Options: `--host` (default `127.0.0.1`), `--port` (default `9720`), `--name` (document name for
ingest, default = filename stem).

---

## Benchmarking

[`scripts/run_benchmark.py`](../../scripts/run_benchmark.py) runs the ingestion benchmark. For
reference, the document-storage milestone (claim 204) ingested a 9-document Gutenberg set cleanly at
>98% reconstruction accuracy; a full 890 KB novel (Dracula) ingests in seconds. (Treat any specific
wall-time numbers in older docs as hardware-and-build-specific historical figures, not current
guarantees.)

---

## See also

- [database-access.md](database-access.md) — connecting to the shards and the claim-graph.
- [quickref.md](quickref.md) — the command + query cheat-sheet.
- [../04-engine/implementation-baseline.md](../04-engine/implementation-baseline.md) — what the
  engine is.
