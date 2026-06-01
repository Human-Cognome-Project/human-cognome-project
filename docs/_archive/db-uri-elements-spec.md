# DB Specialist: URI Elements Namespace + Vocab Gaps

Date: 2026-03-11
From: Engine specialist
Priority: High

---

## 1. Rename / Establish the URI Elements Namespace in hcp_core

What is currently called **"labels"** in the core DB needs to be split:

- **Labels** (engine meaning) = proper nouns, titles — language-specific capitalized words. These live in `hcp_english` (and equivalent language shards).
- **URI Elements / Programming namespace** = language-invariant tokens — URLs, web protocols, ASCII identifiers, structured codes. These belong in `hcp_core` because they appear identically regardless of the language of the surrounding text. A Chinese reader still sees `www.gutenberg.org`.

The name collision between "labels" (core) and "Labels" (engine proper-noun tier) is causing confusion. Recommend renaming the core namespace to **"Programming"** or **"URI"** — a short, unambiguous token namespace prefix for language-invariant identifiers.

---

## 2. Tokens to Move FROM hcp_english INTO the URI/Programming Namespace in Core

These were found as lingo vars in The Yellow Wallpaper, indicating they either leaked into the English shard or are missing entirely. They are language-invariant and belong in core:

| Token | Reason |
|-------|--------|
| `www` | Web prefix — appears in any language context |
| `ascii` | Technical standard name — language-invariant |

**Broader audit needed**: Check hcp_english for other tokens that are URI-ish or programming codes — web protocols (`http`, `https`, `ftp`), TLDs (`.org`, `.com`, `.net`), file extensions (`.txt`, `.pdf`, `.epub`, `.html`, `.xml`, `.zip` etc.), common initialisms that have no language affiliation. Move these to the URI/Programming namespace in core.

The URI/Programming namespace is also the source of truth for **file extension recognition** — when the engine encounters a token like `.txt` or `.epub`, it looks here to classify it as a file reference rather than a punctuation sequence.

Note: ASCII character tokens (the individual character→token mapping) also currently sit under what was called "labels." These are language-invariant by definition and should move to the URI/Programming namespace alongside `www`, `ascii`, etc.

---

## 3. Missing Tokens in hcp_english

These appeared as lingo vars in The Yellow Wallpaper (a short 47KB Victorian text). If they're missing here they'll be missing everywhere. These should be in hcp_english with appropriate Labels tier marking:

**Common given names** (Labels — intrinsically capitalized):
- `charlotte`, `david`, `michael`, `jennie`, `julia`

**Common surnames** (Labels):
- `gilman`, `perkins`, `widger`

**Common English words** (should be in general vocab):
- `november` — month name
- `mississippi` — US state name (also a Labels entry)
- `romanesque` — architectural/style term
- `tremens` — as in *delirium tremens* (medical Latin, used in English)

These are not obscure — if they're missing from a 19th-century English text corpus they will be missing from most texts. Worth doing a broader sweep of other ingested documents' var lists once more texts are re-ingested.

---

## 4. Note for Engine Specialist (for awareness only — engine is handling this)

`i'll`, `i'm`, `i've` are appearing as lingo vars. The engine inflection stripper correctly strips `'ll`/`'m`/`'ve` to produce base `i`, but length-1 bases currently can't enter the physics resolution beds. Engine fix is in progress. No DB action needed for these — `i` is already in vocab.

---

## 5. Priority Order

1. URI/Programming namespace rename + ASCII token migration (unblocks clean namespace architecture)
2. `www`, `ascii` moved to core URI namespace
3. Broader hcp_english audit for other URI-ish leakage
4. Common given names + words added to hcp_english
