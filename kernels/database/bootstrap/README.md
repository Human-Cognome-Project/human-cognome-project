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

## Running

The driver checks the floor first (256 byte codes with hex notation, the
overgroup) and refuses to proceed if it is missing. A full run is about 1.1M
commands; per-commit fsync dominates, so a bootstrap run may set
`PGOPTIONS="-c synchronous_commit=off"` (about 6x faster here; a crash can lose
only the last few commits, and a re-run completes them).
