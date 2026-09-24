# ingestion -- superseded (2026-09-18)

This directory held the pre-rebase "path A" entry point: `dbk::Ingestor`
(a provisional-address-or-inert-extrapolation-hook intake over
`Controller::mint`/`add_group_membership`), its one-shot CLI (`db_ingest`),
and the persistent one-request-per-line verb dispatcher (`db_runtime`).

All of it called door surface the schema/type rebase removed --
`add_group_membership` (replaced by `Controller::add_membership`, no mass/
kind operand) and `mint`'s `type` parameter (the `token.type` column is
gone; type is emergent, never stored -- see `../NOTES.md` "Relationship
model & type -- firmed 2026-09-17"). It could not be patched back to
compiling without reintroducing that removed surface, and patching it
properly would mean rebuilding, in parallel, everything the command IR +
DECLARE core now do better:

- `Ingestor`'s provisional-address/collision/inert-extrapolation logic is
  superseded by `command::DeclareRecord` + the address span planner
  (`../command/span_planner.h`) and `../declare/declare_core.h` -- a
  richer, spec-traced ADDRESS grammar (`FROM`/`AFTER`/`TO`/`@`-pin/nested),
  structural (not type-gated) validation, and the `PARENTS`/`MEMBERS`/
  `MEMBER_OF` fields this old struct had no room for.
- `db_runtime`'s one-request-per-line text dispatch was **never the
  intent** (`../NOTES.md` "Request transport -- firmed 2026-09-18": "The
  request was NEVER one-request-per-line -- the nested and arrayed nature
  was intended from the start"). Rebuilding it to parse the firmed,
  multi-field, nestable, arrayable `DECLARE_RECORD` grammar as text lines
  would mean building the external wire/framing parser -- seam G6,
  explicitly deferred (PLAN.md I.H). Its verb-dispatch ROLE (routing a
  parsed request to the right core) is superseded by `../dispatch/`,
  which dispatches over the in-process IR instead, with a real testable
  surface and no wire format invented ahead of G6.
- `db_ingest`'s one-shot-CLI role has no replacement here yet, for the
  same reason: any CLI front end would need to parse text into
  `command::DeclareRecord`, i.e. it would need G6.

**What replaces this directory:** `../command/` (the IR + validation +
span planner), `../declare/` (DECLARE), `../read/` (READ), `../update/`
(the four UPDATE ops), and `../dispatch/` (the verb dispatcher + arraying
executor, PLAN.md II.7/II.8) -- test it there. `../seed/seed_0x.cpp` seeds
the floor directly through `Controller::mint`'s bootstrap mass channel,
no longer through this directory's `Ingestor`.

Nothing here is rebuilt in place: the files that called the removed door
surface (`ingestion.h`, `ingestion.cpp`, `ingestion_test.cpp`,
`ingestion_cli.cpp`, `db_runtime.cpp`, `db_runtime_test.sh`) are deleted,
not patched, since every semantic they carried either has a better home
above or depends on the still-deferred external wire format (G6). This
README is left as the breadcrumb from the old path to the new one.
