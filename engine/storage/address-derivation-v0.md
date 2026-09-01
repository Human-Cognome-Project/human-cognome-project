# Same-method address derivation + supersession edge — v0 spec

**Plan line (data-plan §7 rule 2):** serves §6b (addressing reconciles by METHOD, not by
partition — P 1435/1437) + §2 (two-way db) + namespace-reference.md §Ingestion Rules.
Role: the addressing law for every engine mint step, starting at the next mint rung
(cross-byte fragment condensations → `AB.AB.B*`). Supersedes the two-step
resolve-before-mint in `write_back_v0.py` with ONE op.

## The law (P, Zulip ~1436/1440)

An address is a **deterministic function of the structure itself under shared rules** —
never of arrival order, never of which node minted it. Consequences, in force here:

1. **Independent discovery converges on one name.** Any node, any epoch, any arrival
   order: the same composition derives the same address. Sequential-index allocation
   (`next_id++`) is a single-source shortcut and is banned from every mint path.
2. **Collision = agreement.** Deriving an address and finding it occupied **by the same
   structure** IS resolution — identity, not conflict. Merge is UNION (provenance
   accrues). Resolve-before-mint and minting are therefore ONE op: compute the address
   by method; what you find there is the answer.
3. **An address means WHAT-IT-IS, never where-it-came-from.** Node-of-origin is metadata
   beside the address, never part of it. Foreign derived rows land `hypothesized` until
   locally confirmed.

## The method (v0)

Input: a **composition** `C` = ordered array of element values at the rung below
(cross-byte fragments: byte values 0–255, length ≥ 2). All stored forms are
`smallint[]` from birth (data-protocol.md; the dotted string is emit-only).

**R0 — archive resolution.** If the identical structure is already named in the
substrate, that address IS the derivation. A length-1 "fragment" is not a fragment: a
single byte resolves to its given byte-code token `{0,0,0,0,b}` (`AA.AA.AA.AA.{b}`),
exactly as write-back v0 proved. A composition matching an existing fragment row
resolves to that row (clause 2 makes this the same op as minting). Archive lookups
against P-loaded word/char tokens join at the next rung (entries.spelling match) — noted,
not implemented here.

**Fragment addresses live in `AB.AB.B*`** (namespace-reference Layer B, reserved for
fragments; one LoD below the word source per §Ingestion Rules). Derivation:

- pair1 = `AB` = 1, pair2 = `AB` = 1 (English family — cross-language fragments need
  P's word; the byte layer is universal but Layer B is defined under AB.AB).
- pair3 = `B` + length-class: value `50 + min(len(C) − 2, 49)` (len 2 → `BA`,
  len 3 → `BB`, …). Length is structure, so this stays a pure function of `C`.
- pairs 4–5 (6,250,000 slots per length class), by whichever rung the structure admits:
  - **R1 — injective pack**, when the composition space fits the slot space
    (`256^len ≤ 6.25M`, i.e. byte bigrams): `v = b₀·256 + b₁`;
    pairs = `(v ÷ 2500, v mod 2500)`. The address IS the composition — packing, not
    hashing; collision between distinct structures is impossible, so clause 2 holds by
    construction.
  - **R2 — content digest**, otherwise: `v = BLAKE2b(be32(C) ∥ be32(k), person="hcp-addr-v0",
    8 bytes) mod 6.25M`, probe counter `k = 0, 1, …`. Deriving at `k` and finding a
    DIFFERENT structure there (true hash collision, rare) steps to `k+1`. The probe
    sequence is a pure function of `C`, but which `k` a structure lands on can depend on
    local occupancy — that residual order-dependence is exactly the divergence case the
    forwarding edge exists for.

## The one op

```
derive(C) → address A (by R0/R1/R2)
upsert (A, C):
  empty                      → MINTED        (status per origin: evidenced locally)
  occupied by same C         → RESOLVED      (agreement; n_evidence++, provenance ∪)
  occupied by different C    → DIVERGENT     (R2 only: re-derive at k+1; flag)
```

Set-based (write in sets, read in sets, loop over nothing — KISS bench rule).

## Supersession edge (P 1440: never a hole)

When the same structure is found under two names — cross-node merge after an R2 probe
split, or a method-version migration — the better name **normalizes by incorporated-
function weight** (selection by use, not arbitration; explicit negotiation is the
minimized fallback). The normalized-away name leaves a **forwarding edge**:

`engine.address_forwarding_v0(alias smallint[] PK, canonical smallint[], reason,
weight real, provenance text[], status active|retired, created)`

- Rows are never deleted; supersession is first-class, provenance kept.
- Resolution follows edges transitively (bounded hops, cycle-checked): a superseded
  name still resolves — never a hole.
- `weight` is the observed incorporated-function measure (use-accrual); v0 records the
  column and the mechanism, measurement rides future epochs.

## Standing tables (durable, additive; `DROP SCHEMA engine CASCADE` removes)

- `engine.fragments_v0(address smallint[] PK, composition smallint[] NOT NULL,
  method text, status text, n_evidence int, provenance text[], created)`
- `engine.address_forwarding_v0` as above.

Created empty by `derive_address_v0.py`; first real rows arrive with the kernel's
cross-byte bond epoch. The self-test exercises the whole mechanism in session-local
TEMP tables on real Moby byte sequences (provenance `method-selftest`, explicitly NOT a
condensation claim) — determinism across arrival orders, agreement-union, forced-collision
probing, split-name reconciliation through the forwarding edge.
