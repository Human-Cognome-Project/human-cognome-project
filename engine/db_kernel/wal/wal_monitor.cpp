#include "wal_monitor.h"

#include <stdexcept>
#include <string>

#include "wal_ingest.h"

namespace wal {

void WalMonitor::feed(const Report &report) {
  const auto it = high_water_.find(report.source);
  if (it != high_water_.end() && !(it->second < report.lsn)) {
    throw std::invalid_argument(
        "WalMonitor::feed: out-of-order lsn for source '" + report.source +
        "': high-water is " + std::to_string(it->second) +
        ", report.lsn is " + std::to_string(report.lsn));
  }

  // Everything that actually books the report goes through W-5's single-
  // report step -- this loop adds only the per-source sequencing below.
  ingest(book_, report);

  high_water_[report.source] = report.lsn;
}

void WalMonitor::feed_source(const std::vector<Report> &ordered_reports) {
  for (const Report &r : ordered_reports) {
    feed(r);
  }
}

void WalMonitor::feed_all(const std::vector<Report> &reports) {
  for (const Report &r : reports) {
    feed(r);
  }
}

std::optional<Lsn> WalMonitor::high_water(const Source &source) const {
  const auto it = high_water_.find(source);
  if (it == high_water_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace wal
