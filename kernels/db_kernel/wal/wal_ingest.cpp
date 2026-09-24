#include "wal_ingest.h"

#include <optional>

#include "wal_recognize.h"

namespace wal {

namespace {

// The identity of the obligation `report` settles, built ONLY from the
// report's own fields -- only-follow, never a reverse index or scan.
// nullopt for anything that is not a settling write (a forward report or
// a DELETE settle nothing here).
std::optional<Obligation> settlement_identity(const Report &report) {
  switch (report.op) {
    case OpKind::kReturnTokenChild:
      // Settles the structure obligation (parent, child) = (counterparts[0],
      // primary) -- see wal_report.h's kReturnTokenChild field meaning.
      return Obligation{ObligationKind::kStructure, report.counterparts.at(0),
                        report.primary};
    case OpKind::kReturnMemberOf:
      // Settles the membership obligation (member, group) = (primary,
      // counterparts[0]).
      return Obligation{ObligationKind::kMembership, report.primary,
                        report.counterparts.at(0)};
    case OpKind::kMassFill:
      // Settles the mass obligation (token_id, <empty>) = (primary, {}).
      return Obligation{ObligationKind::kMass, report.primary, codec::Address{}};
    case OpKind::kDeclare:
    case OpKind::kAddConnection:
    case OpKind::kMembershipWrite:
    case OpKind::kDelete:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

void ingest(WalBook &book, const Report &report) {
  // A forward report opens what it owes. owed() already returns empty for
  // a settling write or a DELETE, so this loop is a no-op for those.
  for (const Obligation &ob : owed(report)) {
    book.open(ob, report.source, report.lsn);
  }

  // A settling write closes the matching open obligation by identity
  // equality, derived only from the report's own fields (only-follow: a
  // PK lookup + PK delete, never a reverse-index/scan).
  std::optional<Obligation> settled;
  const std::optional<Obligation> identity = settlement_identity(report);
  if (identity.has_value()) {
    if (book.is_open(*identity)) {
      settled = identity;
    }
    book.close(*identity);  // no-op if the identity was not open (W-4).
  }

  // record_seen is the sole history append -- exactly once per report,
  // whether it opened work, settled work, or (a DELETE) did neither.
  book.record_seen(report, settled);
}

}  // namespace wal
