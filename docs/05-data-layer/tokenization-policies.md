# Tokenization and Storage Policies

What gets stored, what gets derived, and what gets minted. These policies decide the *shape* of the
substrate.

Sources: claims 56 (see-it-mint-it), 229 (inflections at runtime), 231 (prefix stripping),
210 (everything gets a token). Inflection-rule count verified live (`inflection_rules` = 39).

---

## See it, mint it

> *Every surface form seen mints its own token — including misspellings, variants, inflections,
> dialects, coinages. No runtime morphological reconstruction.* — claim 56

Morphology lives in a column; NSM mapping is via a stored decomposition chain. The key property:

- the **token table grows unbounded** (every surface form seen becomes a token), **but**
- the **concept space does not grow unbounded**, because all tokens distill to the same finite prime
  set.

This **supersedes** earlier morph-bit / delta-storage designs (any doc describing runtime morph
reconstruction in the recording flow is stale). The scope marker (claim 56): see-it-mint-it as stated
is for English / Latin-script space-separated input.

---

## Inflections are assembled at runtime, not stored

> *Regular inflections are not stored; the engine assembles them at runtime from an
> `inflection_rules` table.* — claim 229

The live `inflection_rules` table has **39 rules** (24 suffix + 15 prefix — confirmed live):
morpheme-keyed, with priority, a POSIX condition, `strip_suffix`, and `add_suffix`; CVC
consonant-doubling is handled by a stored function.

**Per-PoS morpheme-acceptance defaults** govern which inflections a word can take:

| PoS | Accepts |
|-----|---------|
| `N_COMMON` | PLURAL, POSSESSIVE |
| `V_MAIN` | PAST, PROGRESSIVE, 3RD_SING, GERUND |
| `ADJ` | COMPARATIVE, SUPERLATIVE, ADVERB_LY |
| `N_PROPER` | POSSESSIVE only |
| `ADV` / `PREP` / `DET` / `CONJ` | none |

These are overridable per entry. **Only delta forms** (archaic / dialectal / nonstandard-tagged) get
stored as variant records; standard rule-derivable forms are **skipped.** This is the storage-side
complement to see-it-mint-it: *mint the observed surfaces, derive the regulars.*

---

## Prefix stripping: 12 bound prefixes, working-bit only

> *Prefix stripping validates against exactly **12 bound prefixes**.* — claim 231

The 12: `un, re, pre, mis, dis, de, non, in, im, il, ir, anti` — each mapped to a morpheme class.
Free-morpheme prefixes (`over` / `under` / `out`) are **excluded** as too false-positive-prone.

The stripped prefix class is a **transient working bit** — *not* stored to the DB. Concept space later
handles the prefix as a **force vector on the root.** This parallels the V-1 / V-3 variant-normalize
step in the engine resolve loop.

---

## Everything gets a token

No exclusion at mint time (claim 210): categories filter via broadphase rather than gatekeeping what
exists. The token graph itself encodes knowledge; categories are query-optimization tags, not the
meaning. (Full classification policies are in
[shards-and-schema.md](shards-and-schema.md#classification-policies).)

---

## See also

- [kaikki-pipeline.md](kaikki-pipeline.md) — where the stored surfaces come from.
- [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md) — the mint/learning loop:
  unresolved input → atomized + mint flag → concept engine → dictionary.
