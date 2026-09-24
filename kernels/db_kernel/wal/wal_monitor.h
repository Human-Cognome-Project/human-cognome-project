#pragma once

#include <map>
#include <optional>
#include <vector>

#include "wal_book.h"
#include "wal_report.h"

// The ordered per-source monitor loop (WAL-IMPL-PLAN.md W-6): drives
// wal::ingest (W-5) over a fixture stream, one source at a time, in that
// source's own lsn order, tracking each source's independent progress.
//
// Charter (WAL-PLAN.md S4/S6, Spec 6): per-source lsn order only -- NO
// cross-source global order is assumed, built, or even representable here.
// Interleaving two sources' reports must not couple their progress: this
// module tracks one lsn high-water mark PER source, keyed only by that
// source's own label, and advancing one source's mark never reads or
// writes another source's entry.
//
// Everything that actually does bookkeeping goes through wal::ingest (W-5)
// -> wal_book's door (W-4); this module adds nothing but the per-source
// sequencing and the progress map -- it does not re-implement open/close/
// record_seen (WAL-IMPL-PLAN.md S0).
namespace wal {

// Per-source lsn high-water mark: the lsn of the last report the monitor
// has fed for that source. A source absent from the map has had no report
// fed for it yet.
using HighWaterMap = std::map<Source, Lsn>;

class WalMonitor {
 public:
  // `book` is owned by the caller and must outlive this monitor -- same
  // non-owning-reference convention wal_ingest::ingest uses for WalBook.
  explicit WalMonitor(WalBook &book) : book_(book) {}

  WalMonitor(const WalMonitor &) = delete;
  WalMonitor &operator=(const WalMonitor &) = delete;

  // Feed one report: wal::ingest(book, report), then advance report.source's
  // high-water mark to report.lsn. Only report.source's OWN entry in the
  // progress map is read or written -- another source's entry is untouched,
  // however the caller interleaves reports across sources.
  //
  // Throws std::invalid_argument if report.lsn is not strictly greater than
  // the high-water mark already recorded for report.source: within one
  // source, delivery must already be in that source's lsn order (the
  // per-source charter this module exists to keep, WAL-PLAN.md S4/S6) --
  // an out-of-order same-source report is a feeder bug, not something to
  // silently accept or reorder.
  void feed(const Report &report);

  // Feed an ordered stream of reports belonging to ONE source, in order.
  // Equivalent to calling feed() on each report in sequence; a convenience
  // for handing over one source's whole ordered stream at once.
  void feed_source(const std::vector<Report> &ordered_reports);

  // Feed a stream that may interleave multiple sources' reports, e.g. a
  // scripted multi-source test scenario built by hand. Each report is fed
  // via feed(), in the given order. Cross-source interleaving in `reports`
  // has no bearing on the result -- there is no cross-source order to keep
  // or violate (Spec 6) -- only each source's OWN reports must already
  // appear, among themselves, in that source's lsn order.
  void feed_all(const std::vector<Report> &reports);

  // The high-water lsn recorded for `source`, or nullopt if no report has
  // been fed for it yet. This is the per-source progress a real consumer
  // would persist for rebase (WAL-PLAN.md S4, "per-source lsn... for
  // rebase").
  std::optional<Lsn> high_water(const Source &source) const;

  // Read-only view of the whole per-source progress map.
  const HighWaterMap &high_water_map() const { return high_water_; }

 private:
  WalBook &book_;
  HighWaterMap high_water_;
};

}  // namespace wal
