#pragma once

#include "wal_book.h"
#include "wal_report.h"

// The single-report ingest step (WAL-IMPL-PLAN.md W-5): links
// wal_recognize (owed()) and wal_book (WalBook) into the one-report
// bookkeeping step -- record the report exactly once, open what it owes,
// and close the obligation it settles when it is a settling write.
//
// A discrete, standalone-buildable module (its own wal_ingest_test.cpp,
// WAL-IMPL-PLAN.md S2 layout) -- not folded into wal_book or
// wal_recognize.
namespace wal {

// ingest(): the one-report step.
//   1. owed(report) -> open() each obligation the report opens (empty for
//      a settling write or a DELETE -- wal_recognize already returns
//      empty for those op kinds, so this loop is a no-op there).
//   2. If `report` IS a settling write -- kReturnTokenChild (identity
//      (parent, child)), kReturnMemberOf (identity (member, group)), or
//      kMassFill (identity (token_id, <empty>)) -- build that obligation's
//      identity FROM the report's own fields (only-follow: a PK
//      operation, never a reverse-index/scan) and close() it. close() is
//      a no-op when the identity was not open (W-4); this step does
//      nothing for a forward report or a DELETE, which settle nothing.
//   3. record_seen(report, settled) -- the SOLE history append, exactly
//      once per report. `settled` names the identity this report's write
//      actually settled: present only when the settling write closed an
//      obligation that was open immediately beforehand; nullopt otherwise
//      (a forward report, a DELETE, or a settling write whose identity
//      was not open -- still History-recorded, closing nothing).
void ingest(WalBook &book, const Report &report);

}  // namespace wal
