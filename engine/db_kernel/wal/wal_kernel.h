#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "endpoint.h"
#include "scheduler.h"
#include "wal_monitor.h"
#include "wal_recognize.h"
#include "wal_report.h"

// The WAL manager as a monitored-endpoint kernel (WAL-INTEGRATION-PLAN.md,
// part 1): the reaction body registered as a scheduler::Handler on the WAL
// manager's per-source in-box(es). Reused bookkeeping logic only --
// wal_recognize / wal_book / wal_ingest / wal_monitor do not change here;
// this class is the box coupling wired around them.
//
// Source-blind (the plan's governing reframe): the handler never reads
// Report::source to route or branch -- source is ordering data the reused
// WalMonitor guard consumes, nothing this class inspects. One out-box; no
// per-originator destination lookup.
namespace wal {

// Owns nothing. The driver/test owns the in-arena, out-arena, WalBook,
// WalMonitor, Registry, Scheduler and every Box, and must keep every one of
// them alive across each run_until_idle() call this kernel's handler runs
// under (WAL-INTEGRATION-PLAN.md "Ownership shape, priority, and the
// driver").
class WalKernel {
 public:
  // `in_arena` is seeded (fully, before the run starts) by the driver/test
  // with the fixture Reports a Message's payload handle indexes into.
  // `out_arena` is where this kernel appends owed-work items; `monitor`
  // does the actual booking (unchanged); `out_box_id` is the single
  // standing reciprocal-work box every pushed handle is sent to.
  WalKernel(const std::vector<Report> &in_arena, std::vector<Obligation> &out_arena,
            WalMonitor &monitor, endpoint::EndpointId out_box_id)
      : in_arena_(in_arena),
        out_arena_(out_arena),
        monitor_(monitor),
        out_box_id_(out_box_id) {}

  WalKernel(const WalKernel &) = delete;
  WalKernel &operator=(const WalKernel &) = delete;

  // Bound to this object's state, so every in-box this is registered on
  // (one per WAL source) shares the same out-arena/monitor.
  scheduler::Handler make_handler();

  // The reaction body itself: resolve the in-box Message's arena handle to
  // a Report, compute owed(), book it via the monitor (unchanged, may
  // throw -- see wal_kernel.cpp), then push one out-box handle per owed
  // obligation.
  void handle(const box::Message &message, scheduler::Sender &sender);

 private:
  const std::vector<Report> &in_arena_;
  std::vector<Obligation> &out_arena_;
  WalMonitor &monitor_;
  endpoint::EndpointId out_box_id_;
};

}  // namespace wal
