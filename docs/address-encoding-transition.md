# Primary address alphabet: Base64url transition

**Decision (2026-09-26):** Replace the project's custom base-50 address
alphabet with the URL and filename safe Base64 alphabet defined in
[RFC 4648 §5](https://www.rfc-editor.org/rfc/rfc4648#section-5). Retain the
existing concept of an ordered array of two-character address elements for
the primary namespace. This page records the intended address format and
implementation work; the current C++ codec and PostgreSQL record tier still
use base-50. The previous choice prioritized human legibility; a standard
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

## Migration seams in the current repository

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
Change address producers and consumers together. No running codec or live
store was converted by this documentation decision.

The governing [system guide](napier-system-guide.md) and
[architecture](architecture.md) describe the intended system. Existing
codec/API/schema notes remain authoritative for what the current binaries
actually accept until the implementation and data transition are complete.
