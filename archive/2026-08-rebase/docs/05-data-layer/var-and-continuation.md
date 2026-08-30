# The Var DB and Continuation Index

How unresolved input is held, learned from, and promoted — and how multi-word constructs are
detected with a cheap forward walk.

Source: claim 232 (var and continuation-index mechanics). See also chamber output storage
(claim 187, [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md)).

---

## The var DB: unresolved sequences with a lifecycle

Unresolved sequences live in **`hcp_var`** with an explicit lifecycle (claim 232):

```
active ──▶ promoted        (the var resolved to a real token)
       ├──▶ retired         (dismissed — typo / one-off)
       └──▶ merged          (folded into another var)
```

A companion table, **`var_sources`**, records each `(doc_id, position)` usage. This is what makes
**promotion cheap and precise**: when a var is promoted, the position streams are patched directly —
no full-table scans. Promotion is a surgical patch, not a sweep.

This is the warm-cache "what I am doing" scratchpad (the var DB of the temporal triad, claim 50). The
chamber **writes** the var DB; the concept engine **reads** it and **writes** the dictionary — a
source-locked write-surface split (claim 114) with zero contention (see
[../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md#the-learning-loop-and-output-storage)).

---

## The continuation index: a three-valued forward walk

Boilerplate and multi-word detection use a **continuation index** (claim 232): submit a growing
prefix and get back a three-valued result:

| Result | Meaning |
|--------|---------|
| `true` | the prefix **continues** — keep extending |
| `false` | **stop** — no construct continues this prefix |
| `token_id` | **resolved** — the prefix is a known construct |

It is a simple boolean forward walk, **cached in LMDB**, and **source-scoped via Thing entities** (so
a construct list can be scoped to a document or corpus). This is the cheap machinery behind
multi-word construct detection — feed tokens forward one at a time, and the index tells you whether
you're still inside a construct, done, or have hit a known one.

This feeds the chamber's partial-match / var handling (claims 179, 187) and the planned higher-order
multi-word construct detection (claim 183).

---

## How it all composes into the learning loop

1. The [chamber](../04-engine/resolution-chamber.md) can't cleanly resolve a segment → it writes a
   **document var** (partial match) or an **atomized spelling + mint flag** (no match) to the var DB.
2. `var_sources` records where it occurred.
3. The **concept engine** reviews vars — eagerly (interactive mode) or batched (bulk-ingestion mode) —
   and decides: **promote** (mint a real token, patch the position streams), **retire** (dismiss), or
   **merge**.
4. The continuation index makes "does this growing sequence form a construct?" a cheap forward walk
   during that review.
5. Over passes, the var population **shrinks** as constructs are learned — the substrate gets better
   at resolving the same inputs cleanly. This is "good-and-adaptive, not perfect-first-pass" (claim
   184) realized in the data layer.

---

## See also

- [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md) — the producer of vars.
- [shards-and-schema.md](shards-and-schema.md) — the dictionary the concept engine promotes into.
