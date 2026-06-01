# Decision Records

Historical design-decision records, retained for provenance. Each captures the context, the
decision, and (where relevant) its current standing against the
[orchestrator claim-graph](../../../README.md). These are **records**, not living specs — when a
decision's specifics are superseded, the record carries an annotation rather than being deleted
(auditable supersession; cf. the native-erasure ethos, claim 248).

| # | Decision | Standing |
|---|----------|----------|
| [001](001-token-id-decomposition.md) | Token ID decomposition into `ns/p2/p3/p4/p5` + generated `token_id` + compound B-tree index | **Current** — live across `hcp_english` and all 6 entity shards (claim 207). |
| [002](002-names-shard-elimination.md) | Names are Proper-Noun *constructs*, not a token property; `hcp_names` shard eliminated | **Current** — `hcp_names` is gone; capitalized forms are spelling variants (claim 208). |
| [005](005-decompose-all-token-refs.md) | Decompose *every* token-reference array via junction tables ("zero TEXT[]") | **Partially superseded** — junction-table approach abandoned for native ARRAY columns (claim 207). Address-decomposition half (= 001) still current. See the banner in the record. |

> **Numbering gap (003, 004):** there is no decision 003 or 004 in the record set. The gap is left
> as-is rather than renumbered — decision numbers are historical identifiers, not a contiguous
> sequence, and rewriting them would break provenance references.

For the current live schema these decisions describe, see
[../../05-data-layer/shards-and-schema.md](../../05-data-layer/shards-and-schema.md). For tracked
debt, see [../deferred-and-open.md](../deferred-and-open.md).
