# Primary address encoding: paired Base62

**Decision (2026-09-26, correcting PR #77):** The canonical stored address remains an ordered PostgreSQL `text[]` of literal two-character pairs, one pair per array element. The alphabet is exactly
`0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz` (Base62 in byte order). All address columns use PostgreSQL's built-in `COLLATE "C"`, as they already did before PR #77. The dotted form, such as `0A.Bz`, is for display and interchange; its pairs are the same literal values as the array elements.

## Why the integer-key decision was reversed

PR #77 chose the RFC 4648 §5 Base64url alphabet in RFC value order and proposed a subsequent change from `text[]` to `smallint[]` pair-code keys. **That storage change was proposed, not implemented.** Its motivation was the mismatch between RFC value order (`A–Z a–z 0–9 - _`) and PostgreSQL's `C` byte order (`- 0–9 A–Z _ a–z`): a one-range wildcard gather would otherwise miss members. The merged PR temporarily restricted the `text[]` store to letters while waiting for numeric keys.

That decision treated the paired characters as a number to pack and unpack instead of using the paired array as the directly ordered, subdividable address. The pairs and their shared roots are part of the storage design: they locate nearby elements together, permit lossless tree compression, and define the same ranges that wildcards traverse. Encoding each pair as an integer would require translation to use its character-level trunk and place divisions, without solving any problem the existing byte-ordered key cannot solve. A generic Base64url encoder also does not directly encode these independent address pairs.

With the byte-ordered Base62 alphabet, the built-in `COLLATE "C"` compares literal pairs in exactly the codec's order. PostgreSQL's existing primary-key index can follow addresses and scan contiguous wildcard/range bounds with no numeric stored key, custom collation, or extra index. The smallint-key proposal and the temporary letter-only guard are withdrawn. Numeric pair codes can still be used *inside* address arithmetic; they are not stored identity.

This corrects the **addressing decision and codec work in PR #77**; it does not erase history or undo its separate, valid rule that a partial/wildcard address is query-only and cannot be stored as an identity. Earlier Base64url decision notes in PR #74 are superseded as well.

## Pair boundaries, compression and wildcard ranges

The first character of a pair selects a trunk; the second selects a position within it. The array position extends the address tree one level. Common roots group related elements in contiguous address areas. For example, a tree recording text made from English words and punctuation can identify their common address area, record that shared value once for the element tree, and record only the differing portions for its members. Combining the root and each continuation must reconstruct every full value exactly. Parent storage is intended to make substantial use of this structure.

A wildcard is the inverse traversal: fix the shared address area and leave the continuation open to select its range. The same pair-aligned boundaries that make shared-root compression possible also make bounded primary-key `gather()` scans possible. At each array level, a full pair has `62² = 3,844` possibilities; a terminal partial such as `A*` covers the 62 pairs with first character `A`, including deeper addresses beneath them. A five-pair address has `62^10 = 839,299,365,868,340,224` possible values (about 839.3 quadrillion), compared with `50^10 = 97,656,250,000,000,000` under the earlier base-50 alphabet.

**Built boundary:** The present PostgreSQL schema stores full `text[]` addresses per row, and the codec includes a positional prefix/delta transform. Recording a shared root only once per compressed element tree and expanding it on read is a storage/composition objective, not a claim that ordinary PostgreSQL rows already share physical address bytes. This decision preserves the exact pair representation required to implement that objective, including parent storage, without specifying an unreviewed compression format.

## What is built and what remains

| Component | State after correction |
|---|---|
| C++ codec | Base62 in `C` byte order; independent two-character pairs, dotted rendering, partial validation, positional delta. Numeric couplet codes are transient arithmetic only. |
| PostgreSQL record and WAL schemas | Existing `text[] COLLATE "C"` address columns remain; no `smallint[]` conversion. |
| Controller and span planner | Use all 62 symbols; prefix, inclusive ranges and successor steps follow byte order. Partial addresses remain query-only. |
| Compressed tree/parent recording | Design intent described above; full address arrays remain the currently built storage form. |
| Legacy base-50 extraction | Historical provenance remains unchanged; no old identifiers are silently reinterpreted. |

Changing the alphabet changes address *interpretation* even where the printed characters overlap. Data creation must follow the agreed derivation and seed plan; this PR does not migrate, delete or reseed any database. Existing database contents are not assumed to be a canonical dataset.
