# Encoding bootstrap driver

Offline bootstrap that populates the multi-byte character tokens of the
encoding floor through the write/mint controller (`../controller/`). C++ only;
no SQL is issued here except through the `Controller`.

- `encoding_populate.{h,cpp}` — address layout, UTF-8 encoding, category labels,
  the per-character command.
- `populate_encoding.cpp` — the driver: `populate_encoding <conninfo> sample|full`.
- `encoding_populate_test.cpp`, `run_populate_test.sh` — model-anchored tests
  against a throwaway `hcp_test_<pid>` database (schema + trigger, floor seeded
  via the Controller), dropped afterwards.

## What it writes

- **Category labels** `Two-Byte Codes`, `Three-Byte Codes`, `Four-Byte Codes` at
  `00.00.01.00.03`/`04`/`05`, each `member_of` the overgroup `00.00.01.00.02`,
  with a manually set temporary placeholder mass of 10 (the floor's provisional
  label convention). No parents, so the structural-mass trigger never fires.
- **Characters**, U+0080..U+10FFFF minus the surrogates U+D800..U+DFFF. Each is
  minted over its UTF-8 bytes as ordered parents (byte `b` is the floor's byte
  code at `00.00.00.00.<couplet 62+b>`, i.e. `…10`–`…57`), with no mass (the
  trigger fills the sum: 4, 6, 8), and added as a member of its category label —
  one `with_transaction` command per character.
- **Addresses** are the next sequential slot in the category range, counting
  codepoints in order with surrogates skipped; the slot is a pure function of
  the codepoint, so a sample run lands where a full run would. 2-byte
  `00.00.00.01.*`–`02.*`; 3-byte `…03.*`–`0z.*`; 4-byte `…10.*`–`zz.*`.
- `notation` (temporary) carries the byte hex (UTF-8 bytes as uppercase hex, in order, e.g. `C3A9`).

Additive and idempotent: re-running mints nothing new; an existing token at a
character address whose parents differ aborts that character.

## Endpoint tier (code BUILT and tested on throwaway databases; not yet run on `hcp_core`)

Design: `../ENCODING-ENDPOINT-TIER-PLAN.md` (on `docs/encoding-endpoint-tier-plan`).
Driver modes `endpoints-label` (creates the `UTF-8` label `00.00.01.00.06`),
`endpoints-sample` (U+00E9, U+20AC, U+1F600) and `endpoints` (U+0080..U+10FFFF);
the endpoint modes require the label and the byte-couplet tier to exist.

- **Endpoint** per codepoint at `endpoint_address(cp)` = `00.00.02.<cp/3844>.<cp%3844>`
  (couplets), notation = the character, one parent (the byte-couplet token,
  `character_address(cp)`, resolved by direct PK read and cross-checked), no
  mass passed (the trigger sets it: 4, 6, 8), `member_of` the `UTF-8` label.
- **Re-run** accepts an existing endpoint only if the whole validates
  (combination in its parents; mass = sum of all parents' token masses; the
  combination lists it as a child; both membership directions by
  `Controller::membership_present`; notation = the character). Otherwise it
  throws and writes nothing; it never heals, and only a fresh mint adds
  membership. The parent count is never required to be 1.

## Running

The driver checks the floor first (256 byte codes with hex notation, the
overgroup) and refuses to proceed if it is missing. A full run is about 1.1M
commands; per-commit fsync dominates, so a bootstrap run may set
`PGOPTIONS="-c synchronous_commit=off"` (about 6x faster here; a crash can lose
only the last few commits, and a re-run completes them).
