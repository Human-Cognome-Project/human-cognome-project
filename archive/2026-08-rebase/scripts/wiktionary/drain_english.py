#!/usr/bin/env python3
"""Drain source_wiktionary.wiktextract_raw (English) into source_english.

Per-row atomic transaction. Delta rule applied: every duplicating element
(tag, category, gloss-path-prefix, etymology blob) becomes a row everyone
points to.

Idempotent at row level via entries.source_id natural key.
On restart: caches are warmed from existing rows; rows already drained
(have an entry with matching source_id) are skipped.
"""
import json
import hashlib
import os
import sys
import time
import psycopg

SRC_DSN = os.environ.get(
    "SRC_DSN", "host=127.0.0.1 port=5435 dbname=source_wiktionary user=hcp password=hcp_dev"
)
TGT_DSN = os.environ.get(
    "TGT_DSN", "host=127.0.0.1 port=5435 dbname=source_english user=hcp password=hcp_dev"
)
LOG_EVERY = 5_000
COMMIT_EVERY = 100  # batch this many source rows per (tgt + src) commit pair

REL_KEYS = (
    "synonyms", "antonyms", "hyponyms", "hypernyms",
    "meronyms", "holonyms", "coordinate_terms", "related", "derived",
)


class Drainer:
    def __init__(self, conn):
        self.conn = conn
        self.cur = conn.cursor()
        self.tag_cache = {}    # text -> tag_id
        self.cat_cache = {}    # name -> category_id
        self.gloss_cache = {}  # (parent_id_or_0, text) -> gloss_id
        self.etym_cache = {}   # hash bytes -> etymology_id

    def warm(self):
        self.cur.execute("SELECT tag_id, text FROM tags")
        for tid, t in self.cur.fetchall():
            self.tag_cache[t] = tid
        self.cur.execute("SELECT category_id, name FROM categories")
        for cid, n in self.cur.fetchall():
            self.cat_cache[n] = cid
        self.cur.execute("SELECT gloss_id, parent_id, text FROM gloss_nodes")
        for gid, pid, t in self.cur.fetchall():
            self.gloss_cache[(pid or 0, t)] = gid
        self.cur.execute("SELECT etymology_id, text_hash FROM etymology_blobs")
        for eid, h in self.cur.fetchall():
            self.etym_cache[bytes(h)] = eid
        sys.stderr.write(
            f"caches warmed: tags={len(self.tag_cache):,} cats={len(self.cat_cache):,} "
            f"gloss={len(self.gloss_cache):,} etym={len(self.etym_cache):,}\n"
        )

    def get_tag(self, text):
        if text in self.tag_cache:
            return self.tag_cache[text]
        self.cur.execute(
            "INSERT INTO tags(text) VALUES (%s) ON CONFLICT (text) "
            "DO UPDATE SET text=EXCLUDED.text RETURNING tag_id",
            (text,),
        )
        tid = self.cur.fetchone()[0]
        self.tag_cache[text] = tid
        return tid

    def get_cat(self, name):
        if name in self.cat_cache:
            return self.cat_cache[name]
        self.cur.execute(
            "INSERT INTO categories(name) VALUES (%s) ON CONFLICT (name) "
            "DO UPDATE SET name=EXCLUDED.name RETURNING category_id",
            (name,),
        )
        cid = self.cur.fetchone()[0]
        self.cat_cache[name] = cid
        return cid

    def get_gloss(self, parent_id, text, depth):
        key = (parent_id or 0, text)
        if key in self.gloss_cache:
            return self.gloss_cache[key]
        self.cur.execute(
            "INSERT INTO gloss_nodes(parent_id, text, depth) VALUES (%s, %s, %s) "
            "ON CONFLICT (COALESCE(parent_id, 0), text) "
            "DO UPDATE SET text=EXCLUDED.text RETURNING gloss_id",
            (parent_id, text, depth),
        )
        gid = self.cur.fetchone()[0]
        self.gloss_cache[key] = gid
        return gid

    def get_etym(self, text):
        h = hashlib.sha256(text.encode("utf-8")).digest()
        if h in self.etym_cache:
            return self.etym_cache[h]
        self.cur.execute(
            "INSERT INTO etymology_blobs(text_hash, text) VALUES (%s, %s) "
            "ON CONFLICT (text_hash) "
            "DO UPDATE SET text_hash=EXCLUDED.text_hash RETURNING etymology_id",
            (h, text),
        )
        eid = self.cur.fetchone()[0]
        self.etym_cache[h] = eid
        return eid

    def already_drained(self, source_id):
        self.cur.execute(
            "SELECT 1 FROM entries WHERE source_id = %s LIMIT 1", (source_id,)
        )
        return self.cur.fetchone() is not None

    def drain(self, source_id, raw):
        if self.already_drained(source_id):
            return False
        word = raw.get("word")
        pos = raw.get("pos")
        if not word or not pos:
            return False
        etym_num = raw.get("etymology_number")
        try:
            etym_num = int(etym_num) if etym_num is not None else None
        except (TypeError, ValueError):
            etym_num = None
        etym_id = None
        if raw.get("etymology_text"):
            etym_id = self.get_etym(raw["etymology_text"])

        self.cur.execute(
            "INSERT INTO entries(source_id, word, pos, etym_number, etymology_id) "
            "VALUES (%s, %s, %s, %s, %s) RETURNING entry_id",
            (source_id, word, pos, etym_num, etym_id),
        )
        entry_id = self.cur.fetchone()[0]

        # senses
        for sense_n, sense in enumerate(raw.get("senses", []) or [], 1):
            gloss_path = sense.get("glosses") or []
            parent_id = None
            leaf_id = None
            for depth, gtext in enumerate(gloss_path):
                if not gtext:
                    continue
                leaf_id = self.get_gloss(parent_id, gtext, depth)
                parent_id = leaf_id

            raw_gloss_arr = sense.get("raw_glosses") or []
            raw_gloss = raw_gloss_arr[-1] if raw_gloss_arr else None

            self.cur.execute(
                "INSERT INTO senses(entry_id, sense_n, gloss_leaf_id, raw_gloss) "
                "VALUES (%s, %s, %s, %s) RETURNING sense_id",
                (entry_id, sense_n, leaf_id, raw_gloss),
            )
            sense_id = self.cur.fetchone()[0]

            for tag in sense.get("tags") or []:
                if not tag:
                    continue
                tid = self.get_tag(tag)
                self.cur.execute(
                    "INSERT INTO sense_tags(sense_id, tag_id) VALUES (%s, %s) "
                    "ON CONFLICT DO NOTHING",
                    (sense_id, tid),
                )

            for cat in sense.get("categories") or []:
                name = cat.get("name") if isinstance(cat, dict) else str(cat)
                if not name:
                    continue
                cid = self.get_cat(name)
                self.cur.execute(
                    "INSERT INTO sense_categories(sense_id, category_id) VALUES (%s, %s) "
                    "ON CONFLICT DO NOTHING",
                    (sense_id, cid),
                )

            for ex in sense.get("examples") or []:
                if isinstance(ex, dict):
                    text = ex.get("text")
                    cite = ex.get("ref") or ex.get("source")
                    typ = ex.get("type") or "example"
                else:
                    text = str(ex)
                    cite = None
                    typ = "example"
                if not text:
                    continue
                self.cur.execute(
                    "INSERT INTO examples(sense_id, text, citation, type) "
                    "VALUES (%s, %s, %s, %s)",
                    (sense_id, text, cite, typ),
                )

            for rel_type in REL_KEYS:
                for rel in sense.get(rel_type) or []:
                    if not isinstance(rel, dict):
                        continue
                    target = rel.get("word")
                    if not target:
                        continue
                    self.cur.execute(
                        "INSERT INTO lex_relations"
                        "(sense_id, relation_type, target_word, target_sense, target_pos) "
                        "VALUES (%s, %s, %s, %s, %s)",
                        (sense_id, rel_type, target, rel.get("sense_index"), rel.get("pos")),
                    )

        # forms
        for form in raw.get("forms") or []:
            if not isinstance(form, dict):
                continue
            ftext = form.get("form")
            if not ftext:
                continue
            self.cur.execute(
                "INSERT INTO forms(entry_id, form_text) VALUES (%s, %s) "
                "ON CONFLICT (entry_id, form_text) "
                "DO UPDATE SET form_text=EXCLUDED.form_text RETURNING form_id",
                (entry_id, ftext),
            )
            fid = self.cur.fetchone()[0]
            for tag in form.get("tags") or []:
                if not tag:
                    continue
                tid = self.get_tag(tag)
                self.cur.execute(
                    "INSERT INTO form_tags(form_id, tag_id) VALUES (%s, %s) "
                    "ON CONFLICT DO NOTHING",
                    (fid, tid),
                )

        # entry-level lex relations
        for rel_type in REL_KEYS:
            for rel in raw.get(rel_type) or []:
                if not isinstance(rel, dict):
                    continue
                target = rel.get("word")
                if not target:
                    continue
                self.cur.execute(
                    "INSERT INTO lex_relations"
                    "(entry_id, relation_type, target_word, target_sense, target_pos) "
                    "VALUES (%s, %s, %s, %s, %s)",
                    (entry_id, rel_type, target, rel.get("sense_index"), rel.get("pos")),
                )

        # sounds
        for snd in raw.get("sounds") or []:
            if not isinstance(snd, dict):
                continue
            ipa = snd.get("ipa")
            enpr = snd.get("enpr")
            audio = snd.get("audio_url") or snd.get("audio")
            homophone = snd.get("homophone")
            if not (ipa or enpr or audio or homophone):
                continue
            self.cur.execute(
                "INSERT INTO sounds(entry_id, ipa, enpr, audio_url, homophone) "
                "VALUES (%s, %s, %s, %s, %s) RETURNING sound_id",
                (entry_id, ipa, enpr, audio, homophone),
            )
            snd_id = self.cur.fetchone()[0]
            for tag in snd.get("tags") or []:
                if not tag:
                    continue
                tid = self.get_tag(tag)
                self.cur.execute(
                    "INSERT INTO sound_tags(sound_id, tag_id) VALUES (%s, %s) "
                    "ON CONFLICT DO NOTHING",
                    (snd_id, tid),
                )

        return True


def main():
    # Two source connections: one for the long-running named cursor (iterator,
    # must stay in a transaction), and a separate non-autocommit one whose
    # drained=true UPDATEs are committed in lockstep with tgt commits.
    src_read = psycopg.connect(SRC_DSN)
    src_write = psycopg.connect(SRC_DSN, autocommit=False)
    tgt = psycopg.connect(TGT_DSN, autocommit=False)

    drainer = Drainer(tgt)
    drainer.warm()

    src_cur = src_read.cursor(name="wikt_iter")
    src_cur.itersize = 200
    src_cur.execute(
        "SELECT id, raw FROM wiktextract_raw "
        "WHERE NOT drained AND lang_code = 'en' ORDER BY id"
    )

    tgt.execute(
        "UPDATE drain_progress SET started_at = COALESCE(started_at, now())"
    )
    tgt.commit()

    t0 = time.time()
    n_done = 0
    n_skipped = 0
    n_err = 0

    upd_cur = src_write.cursor()
    batch_n = 0  # rows accumulated since last commit pair

    def commit_pair():
        # Order matters: target first, source second. If we crash after tgt
        # commit but before src commit, restart will see those rows undrained
        # in source, re-iterate, drain() returns False (already_drained),
        # UPDATE re-applies. Self-healing.
        tgt.commit()
        src_write.commit()

    try:
        for source_id, raw in src_cur:
            try:
                drained_now = drainer.drain(source_id, raw)
            except Exception as e:
                tgt.rollback()
                src_write.rollback()
                n_err += 1
                batch_n = 0
                sys.stderr.write(f"ERR id={source_id} word={raw.get('word')!r}: {e}\n")
                continue
            if drained_now:
                n_done += 1
            else:
                n_skipped += 1
            upd_cur.execute(
                "UPDATE wiktextract_raw SET drained=true, drained_at=now(), "
                "drain_target='source_english' WHERE id=%s",
                (source_id,),
            )
            batch_n += 1
            if batch_n >= COMMIT_EVERY:
                commit_pair()
                batch_n = 0
            total = n_done + n_skipped
            if total % LOG_EVERY == 0:
                el = time.time() - t0
                rate = total / el if el else 0
                sys.stderr.write(
                    f"[{total:>8,}] new={n_done:,} skip={n_skipped:,} err={n_err} "
                    f"{el:>6.0f}s {rate:>5.0f}/s tags={len(drainer.tag_cache):,} "
                    f"cats={len(drainer.cat_cache):,} gloss={len(drainer.gloss_cache):,} "
                    f"etym={len(drainer.etym_cache):,}\n"
                )
                sys.stderr.flush()
        # final partial batch
        if batch_n:
            commit_pair()
    finally:
        src_cur.close()

    tgt.execute(
        "UPDATE drain_progress SET completed_at=now(), rows_drained=%s",
        (n_done + n_skipped,),
    )
    tgt.commit()

    el = time.time() - t0
    sys.stderr.write(
        f"DONE new={n_done:,} skip={n_skipped:,} err={n_err} time={el:.0f}s\n"
    )


if __name__ == "__main__":
    main()
