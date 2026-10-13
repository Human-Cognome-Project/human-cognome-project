# Isolated letter seed population, 2026-10-13

These paired PostgreSQL 16.15 exports preserve the **owned isolated validation
stores**, not an existing user installation. The complete retained October 3
UTF-8 core was restored into a newly created empty fixture; a separate new
Linguistics database used the reviewed passive schema. The bounded writer then
populated the accepted 20-language initial batch. Existing user stores and the
October 3 source export remain unchanged.

| Export | SHA256 |
|---|---|
| `hcp_core.sql.gz` | `f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121` |
| `hcp_linguistics.sql.gz` | `9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296` |

Both are gzip-compressed plain SQL from `pg_dump --no-owner --no-privileges`;
Git LFS stores the compressed payloads. The source core export SHA256 is
`4408fd55e8884efa6a1b575b7e317aea17bdea922b09fab193cf9f7f65456dc8`.
[COUNTS.json](COUNTS.json) records seed/map/ledger/payload hashes, counts and
order-independent row digest sums. [The retained ledger](../../../linguistics/letter-seed-ledger.json)
maps every symbolic seed identity to its accepted address; the
[acceptance report](../../../linguistics/population-validation.json) covers all checks.

| Table | Core before | Core after | Linguistics |
|---|---:|---:|---:|
| token | 1,112,215 | 1,112,215 | 2,772 |
| token_parent | 4,382,976 | 4,382,976 | 2,741 |
| token_child | 4,333,104 | 4,335,845 | 0 |
| members | 1,112,213 | 1,112,213 | 5,460 |
| member_of | 1,112,213 | 1,112,213 | 5,460 |

Linguistics contains 2,707 language-specific component endpoints and 65 group
anchors. Components have exact source notation, verified whole mass and ordered
core character parents; their parent-row mass cells remain NULL. Group whole
masses remain NULL, with no naming constructions or centroid assembly. Single
Unicode ellipses retain one character parent. Repeated parent occurrences are
covered by a separate test-only sequence, without altering the retained inventory.

Exactly two destination foreign keys differ from the original schema:
`hcp_core.token_child.child_token_id` and
`hcp_linguistics.token_parent.parent_token_id` may now point to the other store.
Owner-token and all membership keys remain. The core's exact original mass
trigger is retained; Linguistics has none. Core tokens, parents, memberships and
every pre-existing child row were preserved. The identical rerun added zero rows.

Read-only reproduction can restore the pair into **new empty databases**. Do not
restore over existing data or use the unchanged native mutation paths on these
cross-linked fixtures. These dumps are not an instruction to modify a user store.
New target population still awaits concrete implementation/schema review and
explicit target selection. The private execution journal is intentionally not
published: it belongs to its original database identities. Preserve the original
journal for recovery there; a restored copy is evidence, not the same transaction.

See [writer and validation instructions](../../../../docs/letter-population-writer.md).
