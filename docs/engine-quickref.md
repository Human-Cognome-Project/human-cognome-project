# HCP Engine Quick Reference

Operational reference for the engine daemon, ingestion pipeline, and DB operations.

---

## Engine Daemon

**Binary**: `hcp-engine/build/linux/bin/profile/HCPEngine.HeadlessServerLauncher`
**Gem library**: `hcp-engine/build/linux/bin/profile/libHCPEngine.so`
**Socket**: `localhost:9720` (length-prefixed, big-endian u32 + JSON)
**Working directory**: must be `hcp-engine/build/linux/bin/profile/` when launching

**Check if running:**
```bash
ps aux | grep HeadlessServerLauncher | grep -v grep
```

**Start daemon:**
```bash
cd /opt/project/repo/hcp-engine/build/linux/bin/profile
./HCPEngine.HeadlessServerLauncher --engine-path=. --project-path=. --headless &
```

**Stop daemon:**
```bash
kill $(pgrep -f HeadlessServerLauncher)
```

**Health check:**
```python
import socket, struct, json
s = socket.socket(); s.connect(('127.0.0.1', 9720))
msg = b'{"action":"health"}'; s.sendall(struct.pack('>I', len(msg)) + msg)
hdr = b''
while len(hdr)<4: hdr += s.recv(4-len(hdr))
n = struct.unpack('>I', hdr)[0]; buf = b''
while len(buf)<n: buf += s.recv(n-len(buf))
print(json.loads(buf)); s.close()
```

**IMPORTANT**: After rebuilding `libHCPEngine.so`, restart the daemon to pick up changes.
The launcher (HeadlessServerLauncher) is a thin shell — all logic is in the gem `.so`.
Launcher binary date ≠ gem date. Check both:
```bash
ls -la hcp-engine/build/linux/bin/profile/HCPEngine.HeadlessServerLauncher \
       hcp-engine/build/linux/bin/profile/libHCPEngine.so
```

---

## Build

```bash
cd /opt/project/repo/hcp-engine

# Runtime gem (fast, what the daemon uses)
cmake --build build/linux --config profile --target HCPEngine 2>&1 | tail -20

# Editor gem (includes Qt widgets — has pre-existing moc redefinition in HCPEngineWidget.cpp, ignore)
cmake --build build/linux --config profile --target HCPEngine.Editor 2>&1 | tail -40
```

---

## Ingestion

**Script**: `scripts/ingest_texts.py`
**Text files**: `data/gutenberg/texts/*.txt`

```bash
# Ingest specific files
python3 scripts/ingest_texts.py "data/gutenberg/texts/01952_The Yellow Wallpaper.txt"
python3 scripts/ingest_texts.py "data/gutenberg/texts/00244_A Study in Scarlet.txt"

# Ingest multiple
python3 scripts/ingest_texts.py "data/gutenberg/texts/00244_A Study in Scarlet.txt" \
                                "data/gutenberg/texts/01952_The Yellow Wallpaper.txt"

# Ingest first N
python3 scripts/ingest_texts.py --first 5

# Ingest all
python3 scripts/ingest_texts.py --all
```

Response fields: `status`, `tokens`, `unique`, `slots`, `doc_id`, `ms` (engine time)

**Socket API — list documents:**
```bash
python3 - <<'EOF'
import socket, struct, json
def sr(d):
    s = socket.socket(); s.connect(('127.0.0.1', 9720))
    msg = json.dumps(d).encode(); s.sendall(struct.pack('>I', len(msg)) + msg)
    hdr = b''
    while len(hdr)<4: hdr += s.recv(4-len(hdr))
    n = struct.unpack('>I', hdr)[0]; buf = b''
    while len(buf)<n: buf += s.recv(n-len(buf))
    s.close(); return json.loads(buf)
print(json.dumps(sr({"action":"list"}), indent=2))
EOF
```

---

## Database

**Connection**: `dbname=hcp_fic_pbm user=hcp password=hcp_dev host=localhost port=5432`

```bash
PGPASSWORD=hcp_dev psql -U hcp -h localhost -d hcp_fic_pbm
```

**List ingested documents:**
```sql
SELECT id, doc_id, name FROM pbm_documents ORDER BY id;
```

**Check var categories for a document (pk=N):**
```sql
SELECT var_category, COUNT(*) FROM pbm_docvars WHERE doc_id = N GROUP BY var_category;
SELECT surface, var_category FROM pbm_docvars WHERE doc_id = N ORDER BY var_category, surface;
```

**Delete a document and all its data (replace N with the doc pk):**
```sql
BEGIN;
DELETE FROM pbm_word_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = N);
DELETE FROM pbm_char_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = N);
DELETE FROM pbm_marker_bonds WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = N);
DELETE FROM pbm_var_bonds    WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id = N);
DELETE FROM pbm_starters     WHERE doc_id = N;
DELETE FROM pbm_docvars      WHERE doc_id = N;
DELETE FROM document_provenance WHERE doc_id = N;
DELETE FROM pbm_documents    WHERE id = N;
COMMIT;
```

**Delete multiple documents (e.g., pks 18 and 19):**
```sql
BEGIN;
DELETE FROM pbm_word_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id IN (18,19));
DELETE FROM pbm_char_bonds   WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id IN (18,19));
DELETE FROM pbm_marker_bonds WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id IN (18,19));
DELETE FROM pbm_var_bonds    WHERE starter_id IN (SELECT id FROM pbm_starters WHERE doc_id IN (18,19));
DELETE FROM pbm_starters     WHERE doc_id IN (18,19);
DELETE FROM pbm_docvars      WHERE doc_id IN (18,19);
DELETE FROM document_provenance WHERE doc_id IN (18,19);
DELETE FROM pbm_documents    WHERE id IN (18,19);
COMMIT;
```

**Note**: No cascade on foreign keys — must delete in the order above (bonds → starters → docvars → provenance → document).

---

## Socket API Actions

| Action | Key fields | Notes |
|--------|-----------|-------|
| `health` | — | Liveness check |
| `list` | — | Lists all documents |
| `ingest` | `name`, `text`, `century` | Full tokenize+store pipeline |
| `phys_ingest` | `name`, `text`, `century` | Physics-based resolution + store |
| `retrieve` | `doc_id` | Load positions, reconstruct text |
| `tokenize` | `text` | Tokenize only, no store |
| `phys_resolve` | `text` | Physics resolution only |
| `info` | `doc_id` | Document detail + provenance + vars |
| `bonds` | `doc_id`, `token_id` | Bond data for a token |
| `update_meta` | `doc_id`, `set`, `remove` | Update metadata fields |
| `activate_envelope` | | LMDB envelope activation |
| `deactivate_envelope` | | LMDB envelope deactivation |

---

## Gotchas

- **No delete action** in socket API — delete via Postgres directly (see above).
- **Document PKs increment** on re-ingest — old pk is gone, new pk is different.
- **`doc_id` string** (`vA.AB.AS.AA.AA`) stays stable across re-ingest if you're the only writer.
  PKs do not. Use `doc_id` string for stable references.
- **Engine not restarted after rebuild** = old code still running. Always restart after `cmake --build`.
- **HCPEngine.Editor build** has a pre-existing Qt moc error in `HCPEngineWidget.cpp` (duplicate
  `moc_HCPEngineWidget` symbol in unity build). Does not affect the runtime `HCPEngine` target.
  Use `--target HCPEngine` for runtime builds; `--target HCPEngine.Editor` for tool builds.
- **WS_BUFFER_CAPACITY = 131072** per workspace. `setNbActiveParticles` must be set to actual
  particle count, not buffer capacity (was a critical bug fixed 2026-03-01).
- **Workstation UI blocks ingestion** — the UI holds a DB write lock while connected. Close the
  Workstation before running `ingest_texts.py` or any `phys_ingest` calls, or ingestion will stall
  until the UI disconnects.
