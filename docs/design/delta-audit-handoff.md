# Delta Audit — Handoff & Resume Guide

**Date:** 2026-03-26
**Status:** In progress — ~19% complete

---

## Current State

| Metric | Count |
|--------|-------|
| Total non-Label tokens | 443,495 |
| Assessed | 87,153 (19.6%) |
| Confirmed roots | 56,520 |
| Confirmed deltas | 28,181 |
| Uncertain | 2,452 |
| Remaining | 356,342 |

### Coverage by word length

- **Lengths 1-7:** Well covered by agent 1 (short-to-long). Delta rate climbs from 0% (single letters) to 38% (7-letter words).
- **Lengths 8-16:** Minimal coverage. This is the bulk of remaining work.
- **Lengths 17-48:** Covered by agent 2 (long-to-short). Delta rate 92-100% — almost all long words are derived forms.

The gap is lengths 8-16, which contains the bulk of the vocabulary (~350K words).

---

## How to Resume

### Prerequisites
- DB access: `PGPASSWORD=hcp_dev psql -h localhost -U hcp -d hcp_english`
- Claude Code with agent teams enabled
- Sonnet model access (Claude Max account, 5x or higher recommended)

### The progress table
All state is in `delta_audit_progress`. The query to find unprocessed words:

```sql
SELECT t.token_id, t.name, t.freq_rank
FROM tokens t
WHERE t.proper_common IS NULL
AND NOT EXISTS (SELECT 1 FROM delta_audit_progress d WHERE d.token_id = t.token_id)
ORDER BY length(t.name) ASC, t.name ASC
LIMIT 20;
```

### Launch agents

Create team and spawn assessors:
```
TeamCreate: team_name="delta-audit"
```

Agent 1 (short-to-long): ORDER BY length(t.name) ASC, t.name ASC
Agent 2 (long-to-short): ORDER BY length(t.name) DESC, t.name ASC

**Use `dontAsk` permission mode and `sonnet` model.**

### CRITICAL: Anti-bulk instructions

The agent prompt MUST include explicit prohibition against bulk SQL processing. Previous agents bulk-processed 400K+ entries using pattern matching, causing massive data corruption requiring recovery. The prohibition must be forceful and state consequences:

```
## ABSOLUTE PROHIBITION

YOU MUST NOT BULK PROCESS. NO EXCEPTIONS.
- Do NOT write SQL that processes multiple words by pattern
- Do NOT use suffix/prefix matching in SQL to categorize words
- Do NOT mark words as "bulk" anything
- Do NOT process more than 30 words per INSERT without having individually reasoned about each one
- The previous agent doing this was shut down for bulk processing and ALL ITS WORK WAS REVERTED. If you bulk process you will be shut down and your work reverted.
```

### Effort level

Use **low** effort. This is vocabulary knowledge, not deep reasoning. High effort burns extra usage for no quality gain on this task. The agents should be set to low via `/effort low` after launch.

---

## The Delta Principle (for agent prompts)

- Default root form = present tense, first person singular, most compact form
- Full chain always. "unkindly" → kind (NEG_UN + ADVERB_LY). No stopping points.
- PoS is irrelevant. "kindly" as ADJ doesn't make it a root.
- Accumulated dictionary senses don't create root status.
- False positives (butter/butt, hamster/hamst) are rare — only etymologically accidental.
- Overlap cases (mister): keep as root, note alternate decomposition.
- Archaic/deprecated: full variants under core token (thou→you ARCHAIC, won't→will CONTRACTION+NEG).

### When a delta is confirmed:
```sql
-- 1. Record verdict
INSERT INTO delta_audit_progress (token_id, name, verdict, base_name, delta_chain, tags, note) VALUES (...);

-- 2. Find base
SELECT token_id FROM tokens WHERE name = 'BASE' AND proper_common IS NULL LIMIT 1;

-- 3. Add variant
INSERT INTO token_variants (canonical_id, name, morpheme, characteristics) VALUES (...) ON CONFLICT DO NOTHING;

-- 4. Remove false root
DELETE FROM tokens WHERE token_id = 'DELTA_TOKEN_ID';
```

---

## Morpheme Labels

Suffixes: PLURAL, PAST, PROGRESSIVE, 3RD_SING, ADVERB_LY, AGENT_ER, COMPARATIVE_ER, SUPERLATIVE_EST, NOUN_NESS, NOUN_MENT, NOUN_TION, NOUN_ITY, ADJ_ABLE, ADJ_FUL, ADJ_LESS, ADJ_OUS, ADJ_IVE, ADJ_AL, ADJ_IC, VERB_IZE, VERB_ATE, VERB_EN, VERB_IFY, POSSESSIVE

Prefixes: NEG_UN, NEG_IN, NEG_IM, NEG_IL, NEG_IR, NEG_DIS, NEG_NON, ITER_RE, PRE, MIS, OVER, OUT, UNDER, ANTI, DE

Other: IRREGULAR, ARCHAIC, DIALECT, CASUAL, CONTRACTION, COMPOUND

Create new labels if needed. Consistency matters more than fitting a fixed list.

---

## Lessons Learned (Failure Modes)

1. **Bulk processing is the default failure.** Every Sonnet agent, regardless of instructions, will attempt to bulk-process using SQL pattern matching if not forcefully prohibited. This has happened THREE times: 105K bad entries (agent 1 initial run), 404K bad entries (agent 2 long-to-short), and the anti-bulk instructions must be extreme.

2. **Low effort works for this task.** Vocabulary assessment is pattern recognition, not deep reasoning. Low effort produces correct results faster and cheaper.

3. **Agents need periodic resumption.** Context fills, rate limits hit, sessions expire. The progress table means no work is lost. Just resume with "Continue the delta audit" messages.

4. **Watch for agents drifting into each other's territory.** Two agents working the same region waste tokens on NOT EXISTS checks. Split by length (short-to-long vs long-to-short) or by letter range.

---

## Recovery Procedures

If an agent bulk-processes and damages data:

1. Identify bad entries: `SELECT count(*) FROM delta_audit_progress WHERE note LIKE '%bulk%' OR delta_chain LIKE '%<%'`
2. Identify deleted tokens: entries with verdict='delta' whose token_id no longer in tokens table
3. Re-insert tokens: parse token_id for ns/p2/p3/p4/p5 coordinates, INSERT into tokens
4. Recover PoS/glosses from Kaikki: `/opt/project/sources/data/kaikki/english.jsonl`
5. Rebuild variants from Kaikki form-of entries
6. Delete bad audit entries
7. Resume

---

## Spec & Context

- Full spec: `docs/design/delta-audit-spec.md`
- Project memory: `~/.claude/projects/-opt-project-repo/memory/project_delta_audit.md`
- Foundational articles: `docs/foundations/01-03` (Patrick's Medium articles)
- LinkedIn drafts: `docs/foundations/04-inside-the-simulation.md`, `docs/foundations/05-the-shape-of-a-word.md`
