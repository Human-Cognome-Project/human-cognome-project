#!/usr/bin/env python3
"""
Self-tokenization: the database consumes its own text content into token_id arrays.
Destructive — original text fields are cleared after successful tokenization.

Process:
1. Build word→token_id lookup from kk_entries
2. For each text field, tokenize into array of token_ids
3. Store tokenized array
4. Clear original text field

Fields tokenized:
- kk_senses.gloss → tokenized_gloss
- kk_sense_examples.text → tokenized_text
- kk_entries.etymology_text → tokenized_etymology
- kk_relations.target_word → target_token_id (single lookup, not array)
- kk_relations.sense_context → tokenized_context
"""

import re
import psycopg2
import sys

def main():
    print("="*60, flush=True)
    print("SELF-TOKENIZATION", flush=True)
    print("="*60, flush=True)

    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # --- Build lookup ---
    print("Building word→token_id lookup...", flush=True)
    cur.execute("SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL")

    # For words with multiple entries (different PoS), take first
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        # Also store lowercase version for case-insensitive fallback
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid

    print(f"  Lookup entries: {len(word_to_tid)}", flush=True)

    # Also load character tokens for punctuation
    cur.execute("SELECT character, char_token FROM english_characters")
    char_to_tid = {row[0]: row[1] for row in cur.fetchall()}
    print(f"  Character tokens: {len(char_to_tid)}", flush=True)

    # --- Add tokenized columns ---
    print("Adding tokenized columns...", flush=True)
    cur.execute("""
        ALTER TABLE kk_senses ADD COLUMN IF NOT EXISTS tokenized_gloss TEXT[];
        ALTER TABLE kk_sense_examples ADD COLUMN IF NOT EXISTS tokenized_text TEXT[];
        ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS tokenized_etymology TEXT[];
        ALTER TABLE kk_relations ADD COLUMN IF NOT EXISTS target_token_id TEXT;
        ALTER TABLE kk_relations ADD COLUMN IF NOT EXISTS tokenized_context TEXT[];
    """)
    conn.commit()

    # --- Tokenizer ---
    # Split on whitespace and punctuation, keeping punctuation as separate tokens
    TOKEN_RE = re.compile(r"([a-zA-ZÀ-ÿ'''-]+|[0-9]+|[^\s])")

    def tokenize_text(text):
        """Split text into tokens, resolve each to token_id. Return (token_ids[], unresolved_count)."""
        if not text or not text.strip():
            return [], 0

        tokens = TOKEN_RE.findall(text)
        token_ids = []
        unresolved = 0

        for tok in tokens:
            # Try exact match
            tid = word_to_tid.get(tok)
            if not tid:
                # Try lowercase
                tid = word_to_tid.get(tok.lower())
            if not tid:
                # Try as single character
                tid = char_to_tid.get(tok)
            if not tid:
                # Try each character individually for punctuation/symbols
                if len(tok) == 1:
                    tid = char_to_tid.get(tok)

            if tid:
                token_ids.append(tid)
            else:
                # Unresolved — store the raw text in brackets as marker
                token_ids.append(f"[{tok}]")
                unresolved += 1

        return token_ids, unresolved

    # --- Process glosses ---
    print("\nTokenizing glosses...", flush=True)
    cur.execute("SELECT id, gloss FROM kk_senses WHERE gloss IS NOT NULL AND tokenized_gloss IS NULL")
    senses = cur.fetchall()
    print(f"  Glosses to process: {len(senses)}", flush=True)

    total_unresolved = 0
    processed = 0
    batch = []

    for sense_id, gloss in senses:
        tids, unres = tokenize_text(gloss)
        total_unresolved += unres
        batch.append((tids if tids else None, sense_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_senses SET tokenized_gloss = %s, gloss = NULL WHERE id = %s", batch)
            conn.commit()
            processed += len(batch)
            batch = []
            if processed % 100000 == 0:
                print(f"  Glosses: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    if batch:
        cur.executemany("UPDATE kk_senses SET tokenized_gloss = %s, gloss = NULL WHERE id = %s", batch)
        conn.commit()
        processed += len(batch)

    print(f"  Glosses done: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    # --- Process examples ---
    print("\nTokenizing examples...", flush=True)
    cur.execute("SELECT id, text FROM kk_sense_examples WHERE text IS NOT NULL AND tokenized_text IS NULL")
    examples = cur.fetchall()
    print(f"  Examples to process: {len(examples)}", flush=True)

    total_unresolved = 0
    processed = 0
    batch = []

    for ex_id, text in examples:
        tids, unres = tokenize_text(text)
        total_unresolved += unres
        batch.append((tids if tids else None, ex_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_sense_examples SET tokenized_text = %s, text = NULL WHERE id = %s", batch)
            conn.commit()
            processed += len(batch)
            batch = []
            if processed % 100000 == 0:
                print(f"  Examples: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    if batch:
        cur.executemany("UPDATE kk_sense_examples SET tokenized_text = %s, text = NULL WHERE id = %s", batch)
        conn.commit()
        processed += len(batch)

    print(f"  Examples done: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    # --- Process etymology ---
    print("\nTokenizing etymology...", flush=True)
    cur.execute("SELECT id, etymology_text FROM kk_entries WHERE etymology_text IS NOT NULL AND etymology_text != '' AND tokenized_etymology IS NULL")
    entries = cur.fetchall()
    print(f"  Etymology entries to process: {len(entries)}", flush=True)

    total_unresolved = 0
    processed = 0
    batch = []

    for entry_id, etym in entries:
        tids, unres = tokenize_text(etym)
        total_unresolved += unres
        batch.append((tids if tids else None, entry_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_entries SET tokenized_etymology = %s, etymology_text = NULL WHERE id = %s", batch)
            conn.commit()
            processed += len(batch)
            batch = []
            if processed % 100000 == 0:
                print(f"  Etymology: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    if batch:
        cur.executemany("UPDATE kk_entries SET tokenized_etymology = %s, etymology_text = NULL WHERE id = %s", batch)
        conn.commit()
        processed += len(batch)

    print(f"  Etymology done: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    # --- Process relation targets ---
    print("\nResolving relation targets...", flush=True)
    cur.execute("SELECT id, target_word FROM kk_relations WHERE target_word IS NOT NULL AND target_token_id IS NULL")
    relations = cur.fetchall()
    print(f"  Relations to resolve: {len(relations)}", flush=True)

    resolved = 0
    unresolved = 0
    batch = []

    for rel_id, target_word in relations:
        tid = word_to_tid.get(target_word)
        if not tid:
            tid = word_to_tid.get(target_word.lower())

        if tid:
            batch.append((tid, rel_id))
            resolved += 1
        else:
            # Mark as unresolved but don't nuke — keep target_word for retry
            unresolved += 1

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_relations SET target_token_id = %s WHERE id = %s", batch)
            conn.commit()
            batch = []
            if resolved % 100000 == 0:
                print(f"  Relations resolved: {resolved}, unresolved: {unresolved}", flush=True)

    if batch:
        cur.executemany("UPDATE kk_relations SET target_token_id = %s WHERE id = %s", batch)
        conn.commit()

    # Nuke resolved target_words
    cur.execute("UPDATE kk_relations SET target_word = NULL WHERE target_token_id IS NOT NULL")
    conn.commit()

    print(f"  Relations done: resolved {resolved}, unresolved {unresolved}", flush=True)

    # --- Process sense_context ---
    print("\nTokenizing relation sense contexts...", flush=True)
    cur.execute("SELECT id, sense_context FROM kk_relations WHERE sense_context IS NOT NULL AND sense_context != '' AND tokenized_context IS NULL")
    contexts = cur.fetchall()
    print(f"  Contexts to process: {len(contexts)}", flush=True)

    total_unresolved = 0
    processed = 0
    batch = []

    for rel_id, ctx in contexts:
        tids, unres = tokenize_text(ctx)
        total_unresolved += unres
        batch.append((tids if tids else None, rel_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_relations SET tokenized_context = %s, sense_context = NULL WHERE id = %s", batch)
            conn.commit()
            processed += len(batch)
            batch = []

    if batch:
        cur.executemany("UPDATE kk_relations SET tokenized_context = %s, sense_context = NULL WHERE id = %s", batch)
        conn.commit()
        processed += len(batch)

    print(f"  Contexts done: {processed}, unresolved tokens: {total_unresolved}", flush=True)

    # --- Summary ---
    print(f"\n{'='*60}", flush=True)
    print("SELF-TOKENIZATION COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)

    # Count remaining unconsumed text
    cur.execute("SELECT count(*) FROM kk_senses WHERE gloss IS NOT NULL")
    print(f"Unconsumed glosses: {cur.fetchone()[0]}", flush=True)
    cur.execute("SELECT count(*) FROM kk_sense_examples WHERE text IS NOT NULL")
    print(f"Unconsumed examples: {cur.fetchone()[0]}", flush=True)
    cur.execute("SELECT count(*) FROM kk_entries WHERE etymology_text IS NOT NULL AND etymology_text != ''")
    print(f"Unconsumed etymology: {cur.fetchone()[0]}", flush=True)
    cur.execute("SELECT count(*) FROM kk_relations WHERE target_word IS NOT NULL")
    print(f"Unconsumed relation targets: {cur.fetchone()[0]}", flush=True)
    cur.execute("SELECT count(*) FROM kk_relations WHERE sense_context IS NOT NULL AND sense_context != ''")
    print(f"Unconsumed sense contexts: {cur.fetchone()[0]}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
