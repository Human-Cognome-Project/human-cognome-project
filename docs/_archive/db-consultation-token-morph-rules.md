# DB Consultation: token_morph_rules Table
*Linguist specialist — 2026-03-10*

## Context

During Kaikki population planning, Patrick identified that the inflection rule that applies to each (token, morpheme) pair is **characterizing data about the word**, not just a performance shortcut.

> "past tense = in essenced or ed or double letter ed"

The specific rule a word uses for each morpheme it accepts is intrinsic to the word — it's its **inflection class**. Two words sharing the same rule_id for PAST are in the same conjugation class. This is linguistically meaningful (analogous to Latin conjugation classes, German strong/weak verbs).

---

## The Problem with the Current Approach

The current `envelope_queries` (migration 030) call `apply_inflection(t.name, 'PAST')` for every verb at assembly time. This plpgsql function:
1. Opens a cursor over `inflection_rules`
2. Tests each rule's regex condition against the root
3. Returns on first match

For 5,000 verbs × 3 morphemes = 15,000 function calls per envelope activation, each doing regex evaluation. Correct but slow.

---

## Proposed Solution: token_morph_rules

A new table that pre-assigns the inflection rule for each (token, morpheme) pair **during Pass 2 of Kaikki population**.

### Schema

```sql
CREATE TABLE token_morph_rules (
    id           SERIAL PRIMARY KEY,
    token_id     TEXT    NOT NULL REFERENCES tokens(token_id) ON DELETE CASCADE,
    morpheme     TEXT    NOT NULL,   -- 'PAST', 'PLURAL', 'PROGRESSIVE', '3RD_SING', etc.
    rule_id      INTEGER NOT NULL REFERENCES inflection_rules(id),

    -- Pre-computed assembly values (avoids join to inflection_rules at query time)
    -- For __DOUBLING__ rules: strip_suffix='', add_suffix = doubled_char + suffix
    -- e.g. 'tap' PAST → strip='', add='ped' (not 'ed') — doubling absorbed into add
    -- e.g. 'run' PROGRESSIVE → strip='', add='ning'
    strip_suffix TEXT    NOT NULL DEFAULT '',
    add_suffix   TEXT    NOT NULL DEFAULT '',

    UNIQUE (token_id, morpheme)
);

CREATE INDEX idx_morph_rules_token    ON token_morph_rules (token_id);
CREATE INDEX idx_morph_rules_morpheme ON token_morph_rules (morpheme);
```

### Key design points

1. **Only regular tokens get rows here.** Tokens with an explicit irregular variant in `token_variants` for a given morpheme do NOT get a `token_morph_rules` row for that morpheme. The irregular IS the inflection.

2. **`__DOUBLING__` pre-computed at population time.** The `apply_doubling_rule()` function is called once per token during Pass 2. The doubled consonant is absorbed into `add_suffix`:
   - `tap` PAST → strip=`''`, add=`'ped'` → assembly produces `tap` + `ped` = `tapped` ✓
   - `run` PROGRESSIVE → strip=`''`, add=`'ning'` → `run` + `ning` = `running` ✓
   - `big` COMPARATIVE → strip=`''`, add=`'ger'` → `big` + `ger` = `bigger` ✓

3. **rule_id is the inflection class identifier.** Words sharing rule_id for a given morpheme are in the same inflection class. Queryable: "all nouns whose PLURAL uses rule X" = all consonant+y nouns.

---

## Assembly Query Change

Replace all `apply_inflection()` calls in `envelope_queries` with a JOIN:

### Before (migration 030 pattern):
```sql
SELECT apply_inflection(t.name, 'PAST') AS name, t.token_id
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = 'V_MAIN'
WHERE length(t.name) BETWEEN 2 AND 4
  AND t.freq_rank IS NOT NULL
  AND (tp.morpheme_accept & 2) != 0
  AND NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PAST'
  )
ORDER BY t.freq_rank ASC
```

### After:
```sql
SELECT left(t.name, length(t.name) - length(tmr.strip_suffix)) || tmr.add_suffix AS name,
       t.token_id
FROM tokens t
JOIN token_pos tp  ON tp.token_id = t.token_id AND tp.pos = 'V_MAIN'
JOIN token_morph_rules tmr ON tmr.token_id = t.token_id AND tmr.morpheme = 'PAST'
WHERE length(t.name) BETWEEN 2 AND 4
  AND t.freq_rank IS NOT NULL
ORDER BY t.freq_rank ASC
```

The `morpheme_accept` bit check and `NOT EXISTS` on `token_variants` are replaced by the presence of a `token_morph_rules` row — if the row exists, it's a regular form. If only `token_variants` has it, that's the irregular.

---

## Population (Pass 2)

During Pass 2 (token_pos insertion), for each (token, morpheme) in `morpheme_accept`:

```python
import re

def find_inflection_rule(root, morpheme, rules):
    """Find the first matching rule for (root, morpheme). Returns (rule_id, strip, add)."""
    for rule in rules[morpheme]:  # sorted by priority ASC
        if re.search(rule['condition'], root):
            if rule['strip_suffix'] == '__DOUBLING__':
                # Apply doubling logic at population time
                doubled = apply_doubling(root, rule['add_suffix'])
                return rule['id'], '', doubled[len(root):]  # strip='', add=doubled_char+suffix
            else:
                return rule['id'], rule['strip_suffix'], rule['add_suffix']
    return None

def apply_doubling(root, suffix):
    """Python port of apply_doubling_rule() from migration 030."""
    doubable = set('bdfgmnprt')
    vowels = set('aeiou')
    if len(root) < 2:
        return root + suffix
    c_last = root[-1]
    c_vowel = root[-2]
    c_prev = root[-3] if len(root) >= 3 else ''
    if c_last not in doubable:
        return root + suffix
    if c_vowel not in vowels:
        return root + suffix
    if c_prev and c_prev in vowels:
        return root + suffix  # digraph
    if len(root) >= 4 and root[-2:] in ('en','on','an','er','or'):
        return root + suffix  # unstressed suffix
    return root + c_last + suffix  # double
```

Insert into `token_morph_rules` alongside `token_pos` in the same transaction.

---

## Request

Please add a new migration (031) that:
1. Creates the `token_morph_rules` table (schema above)
2. Updates the `envelope_queries` rows in hcp_core to use the JOIN pattern instead of `apply_inflection()` — or adds a note that they'll be updated after Pass 2 populates the table

The table needs to exist before Pass 2 starts. Pass 1 (root token insertion) can proceed without it.

---

## Related Tasks

- Task #20 (Pass 2): Will populate `token_morph_rules` alongside `token_pos`
- Task #16 (LMDB compiler): Will use `token_morph_rules` for assembly when building vbed entries
