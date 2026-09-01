# FRESH-DB SPEC v0 — for P's strikes; nothing is created until struck

**Plan lines:** `data-protocol.md` §The-addressing-precept + §Using-the-previous-era's-stores
(the ruling was already in the doc: *"The old databases are read-only sources — never repaired
in place. The new structure is built clean and content is pulled into it"*; P's 2026-09-01
message activates it) + data-plan §2b (current dbs = trainable source) + §6b (same-method
addressing, no single-writer assumptions) + namespace-reference §Ingestion Rules + **live
`hcp_core.namespace_allocations` as canonical** (60 rows read 2026-09-01).

**Organizing principle (P, recurring):** ONE structure per the plan's architecture. Writers are
distinguished by **provenance columns** — never by schema, database, or table forks. The
`engine.*` side-schema was the named error; its contents fold into the places below.

## 1. Enforcement by column types (the addressing precept, literal)

```sql
CREATE DOMAIN pairval AS smallint CHECK (VALUE >= 0 AND VALUE < 2500);
-- an address IS an array of base-50 pair values; depth 1..5 (LoD-dependent);
-- the dotted string does not exist in storage anywhere.
CREATE DOMAIN addr AS pairval[] CHECK (array_length(VALUE, 1) BETWEEN 1 AND 5);
CREATE FUNCTION render_dotted(a addr) RETURNS text IMMUTABLE ...;  -- EMIT-ONLY
```

No `token_id text` column exists in any fresh table. Rendering is a function call at the edge.

## 2. Core tables (one structure; provenance inside, not around)

```sql
CREATE TABLE tokens (
  address       addr PRIMARY KEY,
  name          text,                      -- surface form where the source gives one
  status        text NOT NULL DEFAULT 'evidenced',   -- evidenced|hypothesized|discharged
  provenance    text NOT NULL,             -- p-loaded | system-derived | engine-minted | foreign-derived
  source_ref    text,                      -- which drain/run/event produced it
  origin_node   text,                      -- §6b: metadata BESIDE the address, never in it
  observed_time timestamptz NOT NULL DEFAULT now()
);
CREATE TABLE atomizations (                -- composition edges, variable-depth safe
  parent addr NOT NULL, ord int NOT NULL, child addr NOT NULL,
  provenance text NOT NULL, PRIMARY KEY (parent, ord)
);
CREATE TABLE namespace_allocations (LIKE hcp_core.namespace_allocations INCLUDING ALL);
  -- carried over row-for-row, then P strikes the divergences (see §5)
CREATE TABLE address_forwarding (          -- supersession first-class (item-3 machinery, plan-native now)
  alias addr PRIMARY KEY, canonical addr NOT NULL, reason text NOT NULL,
  weight real, provenance text[] NOT NULL, status text NOT NULL DEFAULT 'active',
  created timestamptz NOT NULL DEFAULT now()
);
CREATE TABLE event_ledger (                -- ledger lives IN the db (P ruling); append-only
  event_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  event_class text NOT NULL, status text NOT NULL DEFAULT 'evidenced',
  observed_time timestamptz NOT NULL DEFAULT now(), emission_stamp date,
  content_sha256 text, n_bytes bigint, source text, provenance text NOT NULL,
  supersedes bigint REFERENCES event_ledger(event_id), detail text
);
CREATE TABLE legacy_registry (             -- sentinels/O-o remaps: flags, never parsed as addresses
  kind text NOT NULL, legacy_form text NOT NULL, mapped_to addr, note text
);
```

Layered content (senses, relations, sounds, kx/cx/nsm, bonds) gets the same treatment
per source as it is PULLED — typed columns, `addr` columns for references, provenance
inside — specified drain-by-drain, not invented ahead (pbm storage per
`spec/pbm-storage-schema.md`, re-based on `addr`).

## 3. Minting = the item-3 one op, now plan-native

Every new address is derived by the same method (archive-resolve → injective pack →
content digest; `engine/storage/derive_address_v0.py`): address = deterministic fn of
composition, never arrival order; occupied-by-same-structure IS resolution; split names
heal through `address_forwarding`. Namespace routing per `namespace_allocations`
(working/variable ranges `AA.AA.AB.*.*` for operational tokens; new token one LoD below
its source per the ingestion rules).

## 4. The pull (current dbs → fresh), per data-protocol

- Extraction reads the **decomposed `ns/p2–p5` columns** (99.996% agreement; TEXT blobs
  serve as checksum, never as source). **O/o drift remapped** at extraction, mapping kept
  in `legacy_registry`. Sentinels (`UNK:*`, `BYTE_xx`, numeric var-ids) land in
  `legacy_registry`, never in `tokens`.
- Old dbs are READ-ONLY sources beside Kaikki — drained from, never extended, never
  repaired in place. Not-yet-pulled is not excluded.
- Order: **Kaikki goes first** (data-protocol) — the fresh substrate's first mass is the
  language grid; the old dbs' content pulls in against it on P's word. Scoping of each
  drain (e.g. whether higher-order forms ride in) = P's call at drain time; the loaded
  extract's single-word scoping is a condition of the original work, not a standing rule.
- Every pull run = an `event_ledger` ingestion-initiation event (the one-door routine).

## 5. Flags for P (strike or confirm; the build waits on these)

a. **AB.AB divergence:** live allocation table says `AB.AB` = Unicode Text Characters
   (future); namespace-reference.md and the english db's content say `AB.AB` = English
   Language Family. Which rules the fresh table? (Everything downstream — including
   fragment addresses — follows the answer.)
b. **Database naming/sharding:** fresh names (e.g. `hcpf_core`, `hcpf_<lang>` …) and
   whether the bootstrap hosting split (token-addressing §Bootstrap) carries over as-is.
c. **Ledger home:** one `event_ledger` in the fresh core db (proposed), or per-shard.
d. **Existing `engine.*` rows** (11 ledger events, condensation reads, empty fragment/
   forwarding tables in old hcp_english): fold their content into the fresh structure's
   tables at creation, then drop the side-schema — confirm.

## 6. Candidate test (P suggestion ~1473 — noted, not queued)

Addressing multi-word entries "might not be a bad test": MWEs are compositions of words,
so deriving their addresses exercises the same-method one-op a rung above fragments
(phrase address = deterministic fn of its word composition; resolve-don't-remint against
the ~338K MWE entries already in hcp_english as the referee). Separate decision from any
re-download; fires only on our call after the fresh core stands.
