#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "codec.h"

// The decoded WAL report (WAL-IMPL-PLAN.md W-2): the in-process stand-in
// for Postgres logical-decoding output. This is the ingest seam the rest
// of the WAL manager's kernel set (wal_recognize, wal_book, wal_ingest,
// wal_monitor) consumes -- the live wire form is not designed yet
// (WAL-IMPL-PLAN.md S1, bucket B), so every report here is built by a
// fixture, never decoded off a real replication slot.
//
// Charter (WAL-IMPL-PLAN.md S0 / WAL-PLAN.md S0): a report carries only
// what recognition needs to derive owed obligations -- it never carries
// the command string, and nothing here talks to a DB.
//
// Footprint representation: every address that crosses this seam is a
// `codec::Address` (reused, not re-decoded -- controller/README.md "all
// addresses cross the interface as codec::Address"). No parallel
// string/array token_id form is introduced here.
namespace wal {

// Which local instance a report came from (WAL-PLAN.md S1): core, a
// language shard, or the personality DB. Kept as an opaque label -- the
// multi-instance topology is context, not this seam's design -- rather
// than an enum, since the set of shards is not fixed here.
using Source = std::string;

// A source's native LSN. Per-source order only (WAL-PLAN.md S4, S8.3):
// no cross-source global order is assumed or representable. uint64_t is
// the simplest monotonic stand-in for a real pg_lsn; the wire encoding of
// a live LSN is bucket B (not designed), so fixtures just need something
// that orders within one source.
using Lsn = std::uint64_t;

// Address scope, read from the report's address range (WAL-PLAN.md S1,
// S4): local (the personality DB's private range) or global (shared,
// fabric-visible).
enum class Scope {
  kLocal,
  kGlobal,
};

// The op kind a report carries (WAL-IMPL-PLAN.md W-2). Mirrors the
// record-tier vocabulary (dispatch::Verb) where it overlaps, split where
// recognition (W-3) needs to tell forward writes from their reciprocal
// returns and from the mass-fill:
enum class OpKind {
  // A new token is created, given its parent_id(s) as part of its own
  // construction (WAL-PLAN.md S3). Each parent owes a token_child return.
  kDeclare,
  // An existing token gains one additional parent. Owes a token_child
  // return the same way a DECLARE's parents do; emits no mass obligation
  // (WAL-IMPL-PLAN.md W-3 -- an ADD_CONNECTION targets an existing,
  // already-massed token).
  kAddConnection,
  // A `members` declaration (group -> member). Owes a matching
  // `member_of` return per member.
  kMembershipWrite,
  // A removal (delete_token or delete_pair). History-only: opens no
  // obligation (WAL-PLAN.md S6).
  kDelete,
  // The reciprocal return settling a structure obligation: a token_child
  // write, identity (parent, child).
  kReturnTokenChild,
  // The reciprocal return settling a membership obligation: a member_of
  // write, identity (member, group).
  kReturnMemberOf,
  // UPDATE of an existing token's `token.mass` from NULL to non-NULL.
  // Distinct from the reciprocal return writes above: it settles a mass
  // obligation, identity (token_id). Has no record-tier emitter today
  // (WAL-IMPL-PLAN.md S1 bucket B -- there is no `token.mass` UPDATE
  // primitive); this fixture is the only way to exercise the mass-close
  // path, which is exactly what it is for.
  kMassFill,
};

// One decoded WAL report.
//
// Field meaning is per-op (see the `fixture::` builders below, which are
// the intended way to construct one):
//
//   kDeclare / kAddConnection:
//     primary     = the new (kDeclare) or existing (kAddConnection) token.
//     counterparts = the parent(s) this report gives `primary` -- one
//                    structure obligation per entry, identity
//                    (counterpart, primary).
//     mass        = nullopt for an ordinary DECLARE (token.mass = NULL,
//                   opens the mass obligation); present only for the
//                   seed-floor exception (e.g. 0x, mass 0); always
//                   nullopt for kAddConnection (it writes no token.mass).
//
//   kMembershipWrite:
//     primary      = the member.
//     counterparts = the group(s) it joins -- one membership obligation
//                    per entry, identity (primary, counterpart).
//     mass         = always nullopt (a membership write never touches
//                    token.mass).
//
//   kDelete:
//     primary      = the removed token (delete_token), or the member side
//                    of a removed pair (delete_pair).
//     counterparts = empty for delete_token; the removed group for
//                    delete_pair. Either way this settles nothing and
//                    opens no obligation -- History-only (WAL-PLAN.md S6).
//     mass         = always nullopt.
//
//   kReturnTokenChild:
//     primary      = the child (the new/connected token from the
//                    forward write).
//     counterparts = exactly one entry: the parent. Settles the structure
//                    obligation keyed (counterparts[0], primary).
//     mass         = always nullopt.
//
//   kReturnMemberOf:
//     primary      = the member.
//     counterparts = exactly one entry: the group. Settles the membership
//                    obligation keyed (primary, counterparts[0]).
//     mass         = always nullopt.
//
//   kMassFill:
//     primary      = the token_id whose mass obligation this settles.
//     counterparts = empty.
//     mass         = always present (the non-NULL fill).
struct Report {
  Source source;
  Lsn lsn = 0;
  Scope scope = Scope::kLocal;
  OpKind op = OpKind::kDeclare;

  codec::Address primary;
  std::vector<codec::Address> counterparts;

  // token.mass as this report carries it. See per-op meaning above;
  // nullopt stands for SQL NULL ("not yet computed"), never a real 0.
  std::optional<std::int64_t> mass;
};

// ---------------------------------------------------------------------
// Fixture builders -- one per op kind, so downstream tests (wal_recognize,
// wal_book, wal_ingest, wal_monitor) construct reports and streams
// without a live DB or a real replication slot (WAL-IMPL-PLAN.md S4).
//
// `wal_report.h` is a single header (WAL-IMPL-PLAN.md S2 layout -- no
// paired .cpp, unlike wal_recognize/wal_book/wal_ingest/wal_monitor), so
// these are defined inline here rather than declared for a separate
// translation unit.
// ---------------------------------------------------------------------
namespace fixture {

// A new token `child`, declared with `parents` as its parent_id(s).
// token.mass = NULL (the ordinary case: mass not yet computed, opens the
// mass obligation). Use declare_with_mass for the seed-floor exception.
inline Report declare(Source source, Lsn lsn, Scope scope,
                       codec::Address child,
                       std::vector<codec::Address> parents) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kDeclare;
  r.primary = std::move(child);
  r.counterparts = std::move(parents);
  r.mass = std::nullopt;
  return r;
}

// The seed-floor exception (e.g. 0x, mass 0): a DECLARE whose token.mass
// is given, not NULL, so it opens no mass obligation.
inline Report declare_with_mass(Source source, Lsn lsn, Scope scope,
                                 codec::Address child,
                                 std::vector<codec::Address> parents,
                                 std::int64_t mass) {
  Report r = declare(std::move(source), lsn, scope, std::move(child),
                      std::move(parents));
  r.mass = mass;
  return r;
}

// An existing token `existing_token` gains one additional parent
// `new_parent`.
inline Report add_connection(Source source, Lsn lsn, Scope scope,
                              codec::Address existing_token,
                              codec::Address new_parent) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kAddConnection;
  r.primary = std::move(existing_token);
  r.counterparts = {std::move(new_parent)};
  r.mass = std::nullopt;
  return r;
}

// A `members` declaration: `member` joins each of `groups`.
inline Report membership_write(Source source, Lsn lsn, Scope scope,
                                codec::Address member,
                                std::vector<codec::Address> groups) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kMembershipWrite;
  r.primary = std::move(member);
  r.counterparts = std::move(groups);
  r.mass = std::nullopt;
  return r;
}

// delete_token: `removed` is deleted outright (no counterpart).
inline Report delete_token(Source source, Lsn lsn, Scope scope,
                            codec::Address removed) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kDelete;
  r.primary = std::move(removed);
  r.mass = std::nullopt;
  return r;
}

// delete_pair: the (member, group) membership is removed atomically.
inline Report delete_pair(Source source, Lsn lsn, Scope scope,
                           codec::Address member, codec::Address group) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kDelete;
  r.primary = std::move(member);
  r.counterparts = {std::move(group)};
  r.mass = std::nullopt;
  return r;
}

// The reciprocal token_child return: settles the structure obligation
// (parent, child).
inline Report return_token_child(Source source, Lsn lsn, Scope scope,
                                  codec::Address parent,
                                  codec::Address child) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kReturnTokenChild;
  r.primary = std::move(child);
  r.counterparts = {std::move(parent)};
  r.mass = std::nullopt;
  return r;
}

// The reciprocal member_of return: settles the membership obligation
// (member, group).
inline Report return_member_of(Source source, Lsn lsn, Scope scope,
                                codec::Address member,
                                codec::Address group) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kReturnMemberOf;
  r.primary = std::move(member);
  r.counterparts = {std::move(group)};
  r.mass = std::nullopt;
  return r;
}

// The mass-fill: token.mass for `token_id` goes from NULL to `mass`. The
// only way to exercise the mass-close path (see kMassFill above).
inline Report mass_fill(Source source, Lsn lsn, Scope scope,
                         codec::Address token_id, std::int64_t mass) {
  Report r;
  r.source = std::move(source);
  r.lsn = lsn;
  r.scope = scope;
  r.op = OpKind::kMassFill;
  r.primary = std::move(token_id);
  r.mass = mass;
  return r;
}

}  // namespace fixture

}  // namespace wal
