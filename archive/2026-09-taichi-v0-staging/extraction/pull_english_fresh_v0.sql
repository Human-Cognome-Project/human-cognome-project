-- pull_english_fresh_v0.sql — English single-word drain onto the correct base
-- P strike (~1483): old addressing is drainage-only; "use what makes sense and
-- redo what doesn't." Used: char tokens carried verbatim (well-formed AB.AA);
-- namespace-reference word scheme (AB.AB, layer+sub p3, count in pairs 4-5).
-- Redone: word addresses re-minted (old = three-way-collided); counts assigned
-- DETERMINISTICALLY (sorted by word,pos,etym,old-id — never arrival order);
-- allocation rows AB.AA/AB.AB updated to match use, redo ledgered.
-- Every legacy id kept: clean ids -> address_forwarding aliases; O/o or null
-- ids -> legacy_registry with mapped_to (decision 4's kept mapping).
-- Run: driven by pull_english_fresh_v0.sh (COPY pipes between dbs).

-- ============ hcp2_english: staging DDL (UNLOGGED, dropped at end) ==========
-- stage_chars(seq int, address addr, character text)
-- stage_words(address addr, word text, pos text, etym smallint, old_tid text,
--             spelling smallint[])

-- ============ source-side: chars (run in hcp_english) =======================
-- COPY out: seq, decoded char_token -> addr text, character
--   excludes O/o-bearing char_token (none expected; excluded rows counted)

-- ============ source-side: words (run in hcp_english) =======================
-- pos -> p3 pair value (namespace-reference Layer table; P codec alphabet):
--   noun/name->100(CA) verb->101 adj->102 adv->103 prep->104 conj->105 det->106
--   pron->107 intj->108 num->109 symbol->110 particle->111 punct->112
--   article->113 postp->114(CP, unallocated sub - flagged) character->117(CR)
--   prefix->0(AA) suffix->1(AB) infix->2(AC) interfix->3(AD) affix->5(AF)
--   abbreviation->150(DA) initialism->151(DB) contraction->153(DD)
--   phrase->200(EA) prep_phrase->201(EB) proverb->202(EC)
-- count = row_number()-1 over (partition by p3val order by word, pos,
--   etymology_number nulls first, token_id nulls first) -> pairs 4,5
-- address = {1,1,p3val,count/2500,count%2500}

-- ============ hcp2_english: land ============================================
INSERT INTO tokens(address, name, category, subcategory, provenance, source_ref)
SELECT address, character, 'character', NULL,
       'p-loaded', 'pull:hcp_english.english_characters 2026-09-01'
FROM stage_chars;

INSERT INTO tokens(address, name, category, subcategory, provenance, source_ref)
SELECT address, word, 'word', pos,
       'p-loaded', 'pull:hcp_english.entries single-word re-mint 2026-09-01'
FROM stage_words;

-- forwarding aliases for clean legacy ids (decodable, no O/o)
INSERT INTO address_forwarding(alias, canonical, reason, provenance)
SELECT decode_tid(old_tid), address,
       'drain re-mint: legacy hcp_english id -> fresh scheme (P strike 2026-09-01)',
       ARRAY['pull:hcp_english.entries']
FROM stage_words
WHERE old_tid IS NOT NULL AND old_tid !~ '[Oo]'
      AND old_tid ~ '^[A-Za-z]{2}(\.[A-Za-z]{2}){4}$'
ON CONFLICT (alias) DO NOTHING;   -- old ids are NOT unique per entry row;
                                  -- first (deterministic sort) wins, rest to registry

-- O/o + null + duplicate legacy ids: kept mapping in legacy_registry
INSERT INTO legacy_registry(kind, legacy_form, mapped_to, note)
SELECT CASE WHEN old_tid IS NULL THEN 'null-legacy-id'
            WHEN old_tid ~ '[Oo]' THEN 'o-drift-remapped'
            ELSE 'duplicate-legacy-id' END,
       coalesce(old_tid, '(null):'||word||'/'||pos), address, word
FROM stage_words w
WHERE old_tid IS NULL OR old_tid ~ '[Oo]'
   OR NOT EXISTS (SELECT 1 FROM address_forwarding f
                  WHERE f.canonical = w.address);

-- word -> char atomization edges from spelling arrays
INSERT INTO atomizations(parent, ord, child, provenance)
SELECT w.address, s.ord, c.address, 'pull:spelling->english_characters.seq'
FROM stage_words w
CROSS JOIN LATERAL unnest(w.spelling) WITH ORDINALITY AS s(seq, ord)
JOIN stage_chars c ON c.seq = s.seq;

-- ============ hcp2_core: redo allocation rows to match use (ledgered) =======
UPDATE namespace_allocations SET name='Text Characters (as loaded)',
  description='Character tokens as carried from english_characters (ASCII + beyond); was: ASCII Text Characters. Redo per P strike 2026-09-01 (use/redo delegation), see event_ledger.'
  WHERE pattern='AB.AA';
UPDATE namespace_allocations SET name='English Language Family',
  description='English word tokens, namespace-reference layer scheme (A affixes/C words/D derivatives/E multi-word; p3=layer+sub, pairs 4-5=count). Was: Unicode future allocation. Redo per P strike 2026-09-01, see event_ledger.'
  WHERE pattern='AB.AB';
