#pragma once

#include <optional>
#include <string>
#include <vector>

#include "codec.h"

// libpq's connection handle, forward-declared so this header does not drag
// libpq-fe.h into every caller. Defined by libpq as `typedef struct pg_conn
// PGconn;`.
struct pg_conn;

// The write/mint controller: the single "door" to the hcp3_core token-graph
// store. Sole-owner writer, communications-only — callers go through the
// controller and never touch the tables directly. It binds directly to a
// real Postgres `hcp3_core` database over libpq.
//
// Read side is "only-follow": look a token up by its exact address (PK), then
// follow stored lists across the two relationship axes — structure
// (parents, children) and membership (members, member_of). It never
// searches — there is no reverse-search index in the schema by design.
//
// There is no aliasing/forwarding: an address IS the identity. The same
// construction yields the same address (one token, deduped by SEE on the PK);
// a different address is a different token. Two distinct addresses are never
// bridged to a common identity.
//
// Write side is "see-mint-link-wire", one atomic transaction per mint (see
// mint() below). The controller keeps the deliberately redundant token_child
// consistent with token_parent ("fold AND wire").
//
// Nothing here depends on any project module other than the codec, which owns
// all address <-> token_id handling.
namespace dbk {

// An ordered constituent supplied to a mint: the parent token's address and
// the mass carried at that position. The constituent's ordinal is its index
// in the mint's constituent vector. Mass is optional: DECLARE carries no
// mass (blank at declare, filled later by the deferred aggregation
// workstream); nullopt is written as SQL NULL, never a stand-in 0 ("0 !=
// not-yet-computed").
//
// Ordinal base (schema OPEN TEST SLOT 2): 0-based, matching the schema's
// stated array-index convention. The first constituent is ordinal 0.
struct Constituent {
  codec::Address address;
  std::optional<int> mass;
};

// A parent row read back from token_parent: the stored ordinal, the
// constituent address at that position, and its per-element mass (nullopt
// when the stored mass is SQL NULL, i.e. not yet computed).
struct ParentEntry {
  int ordinal = 0;
  codec::Address parent;
  std::optional<int> mass;
};

// The token's own optional attributes read back from the token row: its
// temporary `notation` (the human surface form a mint carried, nullopt when
// the column is SQL NULL) and its whole-token `mass` (nullopt when SQL
// NULL). Pass-through storage only — the controller computes neither; it
// stores and returns exactly what a mint gave. No `type` field: type is
// dropped from the schema (emergent/LoD-relative, never stored — see
// NOTES.md "Relationship model & type — firmed 2026-09-17").
struct TokenAttributes {
  std::optional<std::string> notation;
  std::optional<int> mass;
};

class Controller {
 public:
  // Opens a libpq connection using the given conninfo string (e.g.
  // "dbname=hcp3_core"). Throws std::runtime_error if the connection fails.
  explicit Controller(const std::string &conninfo);
  ~Controller();

  Controller(const Controller &) = delete;
  Controller &operator=(const Controller &) = delete;

  // ---- Read side: only-follow. Never searches. ----

  // Direct-address probe: does this token_id exist? PK lookup on token.
  bool token_exists(const codec::Address &token_id);

  // Ordered constituents of a token: token_parent by token_id PK prefix,
  // ordered by ordinal.
  std::vector<ParentEntry> parents_of(const codec::Address &token_id);

  // Reverse adjacency: the tokens that directly list token_id as a parent.
  // token_child by token_id PK prefix. Unordered in the schema; returned
  // sorted for determinism.
  std::vector<codec::Address> children_of(const codec::Address &token_id);

  // Membership, downward: the tokens token_id (a group) directly contains.
  // `members` by token_id PK prefix. Unordered in the schema; returned
  // sorted for determinism (matches children_of).
  std::vector<codec::Address> members_of(const codec::Address &token_id);

  // Membership, upward: the groups token_id directly belongs to. `member_of`
  // by token_id PK prefix. Unordered in the schema; returned sorted for
  // determinism (matches children_of).
  std::vector<codec::Address> member_of(const codec::Address &token_id);

  // The token's own stored attributes (notation, whole-token mass) by
  // token_id PK. nullopt when the token does not exist; a present token
  // whose notation/mass columns are SQL NULL comes back with those fields
  // nullopt. Read-back only. No `type`: dropped from the schema.
  std::optional<TokenAttributes> attributes_of(const codec::Address &token_id);

  // ---- Write side: sole owner. ----

  // see-mint-link-wire, one atomic transaction (BEGIN/COMMIT, ROLLBACK on
  // error). Returns true if the token_id already existed (idempotent no-op,
  // nothing written), false if it was freshly minted.
  //   SEE  probe token_id (PK lookup on token); if present, no-op.
  //   MINT insert the token row. token_text is the codec's dot-joined
  //        rendering of token_id (derived here, not passed in); notation is
  //        the caller's temporary human surface form. `mass` is an optional
  //        pass-through attribute stored verbatim: an absent `mass` is
  //        written as SQL NULL, never a stand-in 0. The controller computes
  //        nothing here — no mass is derived from constituents, and no
  //        `type`/kind is written (the column is gone; mint no longer
  //        records a kind — see NOTES.md "Relationship model & type").
  //   LINK insert token_parent rows for the ordered constituents, each with
  //        its ordinal (0-based, = position) and per-element mass (SQL NULL
  //        when a constituent's mass is absent).
  //   WIRE insert a token_child row for each DISTINCT constituent, pointing
  //        that parent back at the new composite ("fold AND wire"). A
  //        constituent repeated at several ordinals yields a single reverse
  //        edge, since token_child carries no position.
  // Constituents must already exist as token rows (FK). Throws
  // std::runtime_error after ROLLBACK on any database error.
  bool mint(const codec::Address &token_id, const std::string &notation,
            const std::vector<Constituent> &constituents,
            std::optional<int> mass = std::nullopt);

  // Sole-owner membership write: writes the member_of row AND its members
  // reciprocal, both directions, in one atomic transaction. No mass operand,
  // no kind operand — replaces add_group_membership. Both `member` and
  // `group` must already be live token rows (FK; mints nothing).
  //
  // Idempotent, SEE-style: re-adding an already-present pair is a clean
  // no-op (ON CONFLICT DO NOTHING on each side, like mint's no-op on an
  // existing token_id) rather than a duplicate-key error. Each side is
  // independently conflict-safe, so a one-sided/inconsistent row (only
  // member_of or only members present) is healed on the missing side
  // without disturbing the side already there.
  void add_membership(const codec::Address &member, const codec::Address &group);

  // ---- Mutation primitives (raw; op-layer gating is a later module). ----

  // Re-keys a token from old_id to new_id and cascade-repoints every
  // referencing row across token_parent, token_child, members and
  // member_of, in one atomic transaction. token_text is re-derived via the
  // codec from new_id (the dot-join follows token_id, as mint does); every
  // other stored attribute (notation, mass) carries over unchanged.
  //
  // The schema has no ON UPDATE CASCADE and forbids triggers/functions, so
  // this primitive owns the FK-safe ordering itself: it INSERTs the new
  // token_id row first, repoints every referencing row from old_id to
  // new_id, then DELETEs the old_id row — never updates a still-referenced
  // PK in place. Throws std::runtime_error after ROLLBACK if old_id does
  // not exist, or if new_id already exists (rekeying onto a live token_id
  // would merge two identities, which "no aliasing/forwarding" forbids).
  void rekey(const codec::Address &old_id, const codec::Address &new_id);

  // Raw delete primitives. No gating (no active-confirmation, no
  // peer-validation tags, no wildcard-safety check) — that op-layer
  // behaviour is a later module; these are the bare store operations it
  // will sit behind.

  // Deletes the token row itself. If other rows still reference it (the FK
  // from token_parent/token_child/members/member_of), the database's
  // default FK behaviour applies as-is (the delete throws); this primitive
  // invents no cascade/repoint policy for that case (open seam G10).
  void delete_token(const codec::Address &id);

  // Removes one specific membership edge AND its stored reciprocal, in one
  // atomic transaction: the member_of row (member, group) and the members
  // row (group, member). Structure edges (token_parent/token_child) are not
  // addressed by this primitive — only mint writes structure, and MOVE_RECORD
  // (via rekey) is the only structural bookkeeping the record tier performs.
  void delete_pair(const codec::Address &member, const codec::Address &group);

 private:
  pg_conn *conn_;
};

}  // namespace dbk
