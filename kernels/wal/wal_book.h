#pragma once

#include <optional>
#include <string>
#include <vector>

#include "codec.h"
#include "wal_recognize.h"
#include "wal_report.h"

// libpq's connection handle, forward-declared so this header does not drag
// libpq-fe.h into every caller -- same convention as controller/controller.h.
struct pg_conn;

// The bookkeeping door (WAL-IMPL-PLAN.md W-4 / WAL-PLAN.md S4): the single
// libpq door over the WAL manager's OWN Postgres DB (`wal_manager` --
// wal_schema.sql, W-1). Mirrors controller/controller.h's "single door,
// communications-only" discipline, but writes go ONLY to the WAL DB -- this
// door never touches `hcp3_core` (WAL-IMPL-PLAN.md S0).
//
// Every access is a bounded PK follow, matching the schema's only-follow
// discipline (no reverse index, no predicate scan on a non-key column):
//   open        -- PK INSERT (idempotent on the identity PK)
//   close       -- PK DELETE (no-op on an absent identity)
//   is_open     -- PK lookup
//   list_open   -- unfiltered read, or filtered on a PK-leading-column
//                  prefix (kind, or kind+addr_a) -- NEVER on source or any
//                  other non-key column
//   record_seen -- the sole append to history, one row per report, keyed
//                  (source, lsn)
//
// `Obligation` (wal_recognize.h) is reused as the identity type: its shape
// -- (kind, addr_a, addr_b) -- IS the obligation table's PK, so no parallel
// identity struct is introduced here.
namespace wal {

class WalBook {
 public:
  // Opens a libpq connection using the given conninfo string (e.g.
  // "dbname=wal_manager"). Throws std::runtime_error if the connection
  // fails.
  explicit WalBook(const std::string &conninfo);
  ~WalBook();

  WalBook(const WalBook &) = delete;
  WalBook &operator=(const WalBook &) = delete;

  // open: INSERT the live obligation row for `identity`. Idempotent on the
  // identity PK (kind, addr_a, addr_b) -- a repeat open of the same
  // identity is a clean no-op, not a duplicate-key error. `source`/`lsn`
  // are the opening report's provenance (the obligation table's NOT NULL
  // `source`/`lsn` columns) -- provenance only, never a query filter (see
  // list_open below).
  void open(const Obligation &identity, const Source &source, Lsn lsn);

  // close: PK DELETE of `identity`'s row. Does NOT append to history --
  // the settling report's own record_seen() call is the sole history
  // append (W-1: a second append here would collide on history's
  // (source, lsn) PK). Closing an identity that is not currently open is
  // a no-op, not an error.
  void close(const Obligation &identity);

  // is_open: PK lookup -- true iff a row with this exact identity is
  // currently open (exists).
  bool is_open(const Obligation &identity);

  // list_open: reads the live obligation relation (open = membership, no
  // scan).
  //   - both omitted  -> unfiltered read of the whole relation.
  //   - `kind` only   -> filtered on the PK's leading column.
  //   - `kind` + `addr_a` -> filtered on the PK's two-column leading
  //     prefix.
  // Passing `addr_a` without `kind` throws std::invalid_argument: addr_a
  // alone is not a PK-leading prefix, and offering that path would be
  // exactly the forbidden non-PK predicate scan this door refuses to
  // expose. This door never filters on `source` or any other non-key
  // column.
  std::vector<Obligation> list_open(
      std::optional<ObligationKind> kind = std::nullopt,
      std::optional<codec::Address> addr_a = std::nullopt);

  // record_seen: the SOLE history append -- exactly one row per report,
  // keyed (source, lsn). `settled`, when present, names the single open
  // obligation identity this report's write settled (W-1's per-report
  // settlement model: at most one settlement per report); nullopt records
  // that the report opened work (or, for a DELETE, nothing at all) but
  // settled nothing. Throws std::runtime_error if (source, lsn) was
  // already recorded -- history's PK is unique, so a second append on the
  // same key is rejected, never silently merged or overwritten.
  void record_seen(const Report &report,
                    const std::optional<Obligation> &settled = std::nullopt);

 private:
  pg_conn *conn_;
};

}  // namespace wal
