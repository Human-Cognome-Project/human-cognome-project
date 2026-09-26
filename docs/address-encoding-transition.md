# Primary address alphabet: Base64url transition

**Decision (2026-09-26):** Replace the project's custom base-50 address
alphabet with the URL and filename safe Base64 alphabet defined in
[RFC 4648 §5](https://www.rfc-editor.org/rfc/rfc4648#section-5). Retain the
existing concept of an ordered array of two-character address elements for
the primary namespace. This page records the address format, the storage
ordering decision and the implementation state. **The C++ codec now
implements the RFC §5 alphabet** (see [Implementation status](#implementation-status));
the PostgreSQL record tier is in an interim state until the pair-code
storage step lands. The previous choice prioritized human legibility; a standard
alphabet and easier interchange now take precedence.

**Why this was chosen:** This is a newly made design decision, not a change
already reflected in the running database. A conventional 64-symbol alphabet
and six-bit digit values should make the address path easier to implement and
use from C++, with less project-specific base conversion to maintain. It is
also more familiar to external tooling and coding collaborators. Those are
reasons for choosing the alphabet, not a claim that ordinary byte-oriented
Base64url library calls already encode the project's pair-array addresses.

## Alphabet, shape and capacity

The planned 64 symbols, **in RFC value order**, are
`ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_`.
`O`, `o`, `0`, `1`, `-` and `_` are valid symbols; `=` is padding in
octet-oriented Base64url, **not** an address symbol. A full address element
uses two symbols (`64² = 4,096` possibilities), in the same outer-to-inner
array layout as the current two-character couplets. A partial final
element, the dot-separated human rendering, and `*` as the partial marker
can retain their present roles; the marker and separator are outside the
64-symbol alphabet.

| Five full elements (ten symbols) | Possibilities |
|---|---:|
| Current base-50 | `50^10 = 97,656,250,000,000,000` (97.65625 quadrillion) |
| Planned 64-symbol alphabet | `64^10 = 1,152,921,504,606,846,976` (about 1.153 quintillion) |

This is the *combinatorial maximum at five full elements*, not a prediction
of how many tokens will be allocated. No change to the level count is
required for the capacity increase.

**Encoding distinction:** RFC 4648 describes converting *octets* to
six-bit symbols and specifies canonical padding rules. The primary address
space is instead a sequence of independent radix-64 symbols grouped into
pairs; ten such symbols span 60 bits of address choices. Using the RFC §5
**alphabet and value mapping** does not by itself make a dotted array of
couplets the output of an ordinary Base64url byte encoder. A future byte
serialization must define an explicit mapping and its padding convention
before generic Base64url encode/decode libraries can be used to round-trip
these addresses. Document and test this distinction at every interchange
boundary; do not silently reinterpret an address as a seven- or eight-octet
payload. The address API's logical array and lossless identities remain the
authority during this transition.

## Storage key ordering (decided 2026-09-26)

**Decision:** The stored identity of an address is an array of numeric
**pair codes**, one per element (`first_value × 64 + second_value`, range
`0..4095`), held in PostgreSQL as `smallint[]`. Base64url `token_id` text is
the rendering used at the edges (display, interchange, the stored
`token_text` debug column). It is never the ordering authority.

**The problem this solves.** The record tier depends on one property: the
primary-key index order must equal address order, so that a terminal
wildcard (`AB.C*`) or a `FROM..TO` span is **one contiguous indexed range
scan**. That is what keeps range reads to a bounded index walk rather than a
search, under the only-follow rule. Under base-50, `text[] COLLATE "C"`
byte order happened to equal digit order (upper-case letters before
lower-case letters, both in ASCII order). Under RFC §5 it does not:

| | Symbol order |
|---|---|
| RFC value order (address order) | `A–Z` (0–25), `a–z` (26–51), `0–9` (52–61), `-` (62), `_` (63) |
| `COLLATE "C"` byte order | `-` (0x2D), `0–9` (0x30–39), `A–Z` (0x41–5A), `_` (0x5F), `a–z` (0x61–7A) |

A text key would therefore misplace every address containing a digit, `-`
or `_`. For example, a gather over `Z*` scans `[ZA, aA)` in bytes. `Z-` and
`Z0`…`Z9` sort *before* `ZA` in bytes, so those eleven members would be
silently missed. `Z_` sorts after `ZA` (and before `aA`), so it is
returned, but in the wrong position relative to its siblings.

**Options considered:**

1. **Pair-code key (`smallint[]`), Base64url rendered at the edges** —
   chosen. Integer array comparison is lexicographic by value, so index
   order equals address order for every symbol with no collation involved.
   A partial final element becomes one contiguous code interval
   (`[first×64, first×64+63]`), and deeper addresses under a prefix still
   sort inside that prefix's range, exactly as with `text[]` today. Pair
   codes are the codec's own `couplet_to_code` values, so the codec and
   the key cannot drift apart. The key is compact (2 bytes per element).
   The cost is that raw SQL shows numbers; `token_text` keeps the readable
   form alongside.
2. **Keep `text[]` and accept byte order ≠ value order** — rejected.
   Exact follows and fixed-prefix lookups would still work, but numeric
   spans and wildcard bounds would have to be split into several byte
   ranges. That spreads the ordering exception through the span planner
   and controller permanently.
3. **Keep `text[]` with a custom collation that sorts in value order** —
   rejected. It depends on a non-default collation surviving PostgreSQL
   upgrades, dumps/restores and every deployment, and a missing or changed
   collation fails silently as wrong range results. That is the same
   failure mode the `COLLATE "C"` pin was introduced to prevent.

**Partial addresses are query-only.** A partial (wildcard) address such
as `AB.C*` names a range of tokens and is resolved by `gather()`; it is
never a token identity. The pair key therefore has no form for it
(`to_pair_key` refuses it), and the record tier refuses partials at every
key boundary: mint, constituents, membership, rekey, delete and exact
follows, and WAL obligation/history identities. This was already the
documented role of wildcards ("a GATHER, never a search"); it is now
enforced and tested so the `smallint[]` step cannot silently drop an
accepted identity shape. If a context node ever needs to be a token in its
own right, that needs its own full address.

**Consequences.** The codec exposes `to_pair_key` / `from_pair_key` (the
storage form) and `partial_code_range` (a wildcard's code interval).
Base64url strings remain the interchange format and are always produced
from, and parsed back to, the key through the codec.

**Development store.** The current development rows are not necessarily
the desired contents of the new store. It is rebuilt cleanly from the
intended schema and data derivation under the pair-code key, rather than
rekeyed in place.

## Implementation status

| Step | State |
|---|---|
| Codec: RFC §5 alphabet, value-order pair codes `0..4095`, token_id parsing/rendering, pair-key form, partial code range, tests | **Built** |
| Address successor / span planner: radix-64 carry in value order (`Az → A0`, `A9 → A-`, `AA.__ → AB.AA`) | **Built** (generic over the codec's radix; tests updated) |
| Record tier (controller) on `text[]` | **Interim:** accepts only the letter subset `A–Z a–z` (values 0–51), for which byte order equals value order, so every range stays exact. Digits, `-` and `_` are refused loudly (mint, follow and gather throw). Bound arithmetic treats `z` as the last stored symbol. |
| Partial addresses query-only: refused at every record-tier and WAL key boundary | **Built** (controller and WAL tests) |
| Pair-code storage: `smallint[]` PK/FK columns in `schema.sql` and `wal_schema.sql`, controller/WAL rendering and parsing, seeder, verify script, DB tests | **Next step**; removes the interim letter limit |
| WAL bookkeeper | Uses equality only, so it is correct under either order; moves to pair-code columns with the storage step |
| Legacy extraction | Unchanged. Remains the base-50 provenance record |

## Migration seams in the current repository

The table below is the original seam inventory made with the decision.
See [Implementation status](#implementation-status) for what has since
been built.

| Component | Current assumption | Work needed before cutover |
|---|---|---|
| [`codec/`](../kernels/database/codec/README.md) | `kAlphabetSize = 50`; letter-only symbols, couplet codes `0..2499`, tests for rejecting `O/o/0/1`. | Move to the RFC §5 symbol/value table, 4,096 pair codes, updated parsing/validation, partials, exact round trips and invalid-symbol tests. |
| [Address successor and planner](../kernels/database/command/README.md) | Radix-50 carry and a maximum symbol that also sorts last in byte order. | Update `FROM`, `AFTER`, `TO`, prefix bounds, sequential fill and overflow behaviour for the new radix and physical order. |
| [PostgreSQL schema](../kernels/database/schema/README.md) | `text[] COLLATE "C"` PK/FK columns; byte order equals the old codec's digit order, making some numeric spans one indexed range. | Preserve exact addressed follows; explicitly redesign/test range and trunk bounds against the new alphabet before claiming one PK interval still means the same numeric span. |
| [WAL bookkeeper](../kernels/wal/README.md) and [endpoints](../network/ENDPOINT-ACTIVATION-NOTES.md) | Reports, obligation identities, and node-address notes assume base-50 tokens. | Carry the selected address version/identity consistently through reports, reciprocal work, history and future bridge/network records. |
| [Legacy extraction](../tools/legacy-extraction/README.md) | Old source IDs and O/o-remapping belong to the earlier 50-symbol convention. | Preserve the legacy mapping as provenance. Decide and test lossless translation for existing references without conflating old and new code-point meanings. |

**Ordering is the immediate trap.** `COLLATE "C"` sorts bytes with `-`
before digits and uppercase letters, and `_` before lowercase letters.
RFC value order puts letters first, then digits, then `-` and `_`.
Therefore the current equation “PK byte order = codec digit order” stops
holding. A fixed-character prefix can remain a contiguous prefix in a
byte-sorted index, but the current *numeric* successor, wildcard boundary,
and `FROM..TO` assumptions cannot simply be carried over. Choose and
verify the physical-order strategy in the implementation PR, including
bounded PK scans; keep the record tier's only-follow rule.

The old alphabet's strings overlap with the new one, while their symbol
indices differ. Treat the conversion as an **identity change**, not an
unchecked literal substitution or an unversioned mixed decoder. The working
expectation is to **rebuild or rewrite the present development DB data as the
new address path and data structures are constructed**, where that is simpler
than an in-place rekey. The exact reconstruction process is still to be
worked out during implementation. Retain the source data and provenance
needed to reconstruct or relate existing `token_id` keys, reciprocal PK/FK
references, WAL obligations, snapshots and content-addressed manifests;
decide explicitly what must be translated and what can be regenerated.
Change address producers and consumers together. The codec has since been
converted (see [Implementation status](#implementation-status)); no live
store has been rekeyed. The development store is to be rebuilt cleanly
under the pair-code key.

The governing [system guide](napier-system-guide.md) and
[architecture](architecture.md) describe the intended system. Existing
codec/API/schema notes remain authoritative for what the current binaries
actually accept until the implementation and data transition are complete.
