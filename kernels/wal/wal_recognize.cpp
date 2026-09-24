#include "wal_recognize.h"

namespace wal {

std::vector<Obligation> owed(const Report &report) {
  std::vector<Obligation> obligations;

  switch (report.op) {
    case OpKind::kDeclare: {
      // One structure obligation per parent_id given at construction.
      for (const codec::Address &parent : report.counterparts) {
        obligations.push_back(
            Obligation{ObligationKind::kStructure, parent, report.primary});
      }
      // A mass obligation opens only for the ordinary case (token.mass
      // still NULL) -- the seed-floor exception carries a value and owes
      // no mass followup.
      if (!report.mass.has_value()) {
        obligations.push_back(
            Obligation{ObligationKind::kMass, report.primary, codec::Address{}});
      }
      break;
    }
    case OpKind::kAddConnection: {
      // Owes its token_child return the same way a DECLARE's parents do --
      // but an existing, already-massed token opens no mass obligation.
      for (const codec::Address &parent : report.counterparts) {
        obligations.push_back(
            Obligation{ObligationKind::kStructure, parent, report.primary});
      }
      break;
    }
    case OpKind::kMembershipWrite: {
      // One membership obligation per group joined.
      for (const codec::Address &group : report.counterparts) {
        obligations.push_back(
            Obligation{ObligationKind::kMembership, report.primary, group});
      }
      break;
    }
    case OpKind::kDelete:
      // History-only (WAL-PLAN.md S6) -- no obligation, no polarity bit.
      break;
    case OpKind::kReturnTokenChild:
    case OpKind::kReturnMemberOf:
    case OpKind::kMassFill:
      // Settling reports SETTLE an obligation (W-5); they open none.
      break;
  }

  return obligations;
}

}  // namespace wal
