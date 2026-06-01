# DB Specialist: Test Prep for Resolution Pipeline
*Engine — 2026-03-10*

## The Problem

`BedManager::RebuildVocab()` reads vocab from the LMDB `w2t` sub-database.
`EnvelopeManager::ActivateEnvelope()` writes to the LMDB `env_vocab` sub-database.
These don't match — activating an envelope writes to a sub-db the engine never reads.

## Fix Required: Update envelope_queries lmdb_subdb

Change `lmdb_subdb` from `env_vocab` to `w2t` for all vocabulary envelope queries.

```sql
UPDATE envelope_queries SET lmdb_subdb = 'w2t';
```

If there are non-vocabulary envelope queries that legitimately write elsewhere, scope
the update to the vocab queries only. Currently all 24 rows have `lmdb_subdb = 'env_vocab'`
and they are all word→token vocab queries, so the blanket update is correct.

## Verify Envelope Queries Return Data

After the Kaikki rebuild (migrations 014–032), confirm the envelope queries actually
return rows. The queries use `tokens`, `token_pos`, and `token_morph_rules`.

Quick check:
```sql
-- Connect to hcp_english
-- Run one of the envelope queries directly, e.g. the len-5 common vocab query:
SELECT id, description, query_text
FROM envelope_queries
WHERE description LIKE '%5-letter%';
-- Then run that query_text on hcp_english to confirm it returns rows.
```

If any query returns 0 rows, the Kaikki population may be incomplete for that length
or the query references a column that changed. Report which ones fail.

## Activation Sequence for Test Run

Once the sub-db fix is in place, the test sequence is:

1. Start the engine daemon (HeadlessServerLauncher)
2. Issue console command: `SourceActivateEnvelope english_function_words`
   — this runs the Postgres queries, writes results to LMDB `w2t`, calls RebuildVocab
3. Check stderr log for: `[BedManager] RebuildVocab: N entries across M word lengths`
   — N should be several thousand, M should be 2–15
4. Issue: `source_phys_word_resolve` with a test text (e.g. first paragraph of Dracula)
5. Check resolution rate in stderr output

## Expected Log Output (healthy state)

```
[EnvelopeManager] Activated 'english_function_words': NNNN entries in XX.X ms
[BedManager] RebuildVocab: NNNN entries across NN word lengths
[BedManager] Word lengths (ascending, shortest-first): 2 3 4 5 6 7 8 9 10 ...
[BedManager] Length 2: N stream runs → phaseSize=M/M
...
[BedManager] Resolved: NNN/NNN runs (100.0%) in XX.Xs
```

## What Does NOT Need Doing

- The LMDB `data.mdb` file does not need to be regenerated — it is a runtime cache,
  not pre-compiled. Once the sub-db name is fixed and an envelope is activated,
  it populates automatically from Postgres.
- No engine code changes are needed for this fix — it is purely a DB-side correction.
