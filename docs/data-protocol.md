# Data Protocol

How knowledge enters the field, what every entry must carry, and how the previous era's stores are
used. Prerequisites: [physics-basis.md](physics-basis.md), [architecture.md](architecture.md).

## Intake invariants

1. **No source filtering. It is ALL knowledge.** Every datum — true, false, popular, obscure, noise
   — is a read of some dimension of the field, and flows cannot be solved in dimensions that were
   refused measurement. Popularity is not truth, but it is a real frequency-side read of real
   connections; discard the noise and you delete the map of the samplers. A differential you refuse
   to read does not stop exerting force — it just balances against you from outside the model.
   Exclusion is the failure mode this system exists to end.
2. **Quality-first is an ordering, never a gate.** High-quality, well-measured sources are placed
   first as reference anchors so the field has stable structure before volume arrives; everything
   else accretes onto them, flagged, in its turn.
3. **Nothing is welded.** An element's sampling frequency and its amount are separate entries and
   are never converted into each other. A curated corpus measures its curator.
4. **Every entry carries flags, not judgements**: read-status (measured / assigned /
   model-produced / declared-derived / unread), and where an amount was read, the sampler's cycle
   and a continuity flag (continuous vs patterned), per the re-reading protocol
   (`ledger/RP_amount_ledger_rereading.md`).
5. **Provenance is a physical path.** Every element records the chain of singularities it came
   through. For every aggregator we ingest, we ingest its raw face too where recoverable — the pair
   is what makes its transformation solvable. Where the raw side is lost, that is a *declared*
   unread slot on the transformation. **Nothing happens for free**: every compression's cost appears
   on the ledger, including our own.

## First placement: the lexical landing lattice

**Kaikki/Wiktionary goes first** because it supplies the full range of words across time for most
known languages — so every element ever ingested afterward arrives already connected: each word in
it is a pinned endpoint whose partner near the present exists. Etymologies are pre-drafted temporal
flows. Reconstructed proto-forms are placed but flagged model-produced. The lattice arrives two
singularities deep (Wiktionary → Kaikki → us); the corpus that lands on it continuously audits it.

## The addressing precept (operational form)

Storage holds the **array of base-50 pairs**; the dotted string is display-only, generated on emit,
never persisted — enforced by column types and constraints, not discipline. Only as much of an
address as an operation requires is read (see [architecture.md](architecture.md)).

## Using the previous era's stores

The old databases are **read-only sources — never repaired in place**. The new structure is built
clean and content is *pulled* into it; not-yet-pulled is not excluded. The live-store probe
([../review/pass-04-data.md](../review/pass-04-data.md)) established:

- **The address convention held in content everywhere** — zero structural violations in 1.49M
  entries. The violation was storage form (TEXT blobs), not content.
- **Extraction pulls the arrayed form directly** from the decomposed `ns/p2–p5` columns (99.996%
  agreement with the blobs; the blobs serve as checksum). The enumerable mismatches are eyeballed,
  not automated over.
- **O/o alphabet drift is corrected at extraction** (decision 4 in
  [../review/decisions.md](../review/decisions.md)): ids minted with O/o are remapped into the
  canonical 50-letter space, with the mapping table kept. Addresses stay easily human-parsable;
  O reads as 0.
- **Sentinel conventions are flags, never parsed as addresses**: `UNK:*`, `BYTE_xx`, `[literal]`
  bracket fallbacks, and numeric var-ids (`00.00.00.00.NN`) form the declared legacy registry.
- The `source_*` databases (raw JSONB kept beside the indexed form) are the internal precedent for
  the path-through discipline — the raw face of the Kaikki singularity is intact and stays that way.

Toolkit: [`extraction/`](../extraction/) — the address codec and its tests, the shard connectors,
the Kaikki loader chain, and the Gutenberg fetcher. Legacy schema maps persist in
[legacy-data-maps/](legacy-data-maps/) until the new schema documentation replaces them.

## Honesty rules carried forward

- Slots stay empty until read; conversion never fills them.
- Destroyed or consumed data is *declared* (the archive ledger records, for example, that original
  text fields were deliberately consumed by the old `self_tokenize` pass).
- Dumps are point-in-time; the live store is the authority; migration filename order is not replay
  order — applied schema state is.
