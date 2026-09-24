#pragma once

#include <vector>

#include "codec.h"
#include "wal_report.h"

// Recognition (WAL-IMPL-PLAN.md W-3 / WAL-PLAN.md S3): a PURE function from
// a decoded WAL report to the obligations it opens. No DB, no store read,
// no command-string access -- the report's own initial data is the whole
// input, matching the "return paths carried in the initial data" charter.
//
// `Obligation` is the SHARED type the rest of the kernel set (wal_book,
// wal_ingest, wal_monitor -- W-4/W-5/W-6) consumes; its shape mirrors the
// W-1 schema's `obligation` PK exactly: (kind, addr_a, addr_b).
namespace wal {

// The axis an obligation sits on -- read directly, never inferred, same
// discipline as the `obligation.kind` schema column it mirrors.
enum class ObligationKind {
  kStructure,   // (parent, child)
  kMembership,  // (member, group)
  kMass,        // (token_id, <empty>)
};

// One owed obligation. Identity is the FIXED two-slot (addr_a, addr_b)
// pair, matching wal_schema.sql's `obligation` PK shape:
//   structure  -> addr_a = parent,   addr_b = child
//   membership -> addr_a = member,   addr_b = group
//   mass       -> addr_a = token_id, addr_b = <empty> (mirrors the
//                 schema's `'{}'` sentinel -- codec::Address is already a
//                 vector, so an empty Address IS that sentinel, not a
//                 separate representation of it).
struct Obligation {
  ObligationKind kind;
  codec::Address addr_a;
  codec::Address addr_b;
};

inline bool operator==(const Obligation &a, const Obligation &b) {
  return a.kind == b.kind && a.addr_a == b.addr_a && a.addr_b == b.addr_b;
}
inline bool operator!=(const Obligation &a, const Obligation &b) {
  return !(a == b);
}

// The obligations `report` opens (Spec 3):
//   - kDeclare: one `structure` obligation (parent, child) per parent in
//     `counterparts`; PLUS, only when `report.mass` is nullopt (an
//     ordinary new-token DECLARE, not the seed-floor exception), one
//     `mass` obligation (token_id, <empty>) keyed on `primary`.
//   - kAddConnection: one `structure` obligation (new_parent, token) for
//     its single counterpart -- same as a DECLARE's parents -- but NEVER a
//     mass obligation (it targets an existing, already-massed token;
//     recompute is NEEDS-PATRICK, not opened here).
//   - kMembershipWrite: one `membership` obligation (member, group) per
//     group in `counterparts`.
//   - kDelete: no obligation at all -- History-only (WAL-PLAN.md S6). The
//     axis identity carries no INSERT/DELETE polarity bit, so no removal
//     obligation is opened here.
//   - kReturnTokenChild / kReturnMemberOf / kMassFill: these are settling
//     reports (they SETTLE an obligation, handled in W-5) -- they open
//     nothing, so `owed()` returns empty for them.
std::vector<Obligation> owed(const Report &report);

}  // namespace wal
