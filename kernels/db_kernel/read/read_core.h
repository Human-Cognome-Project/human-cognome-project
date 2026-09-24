#pragma once

#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"

// The READ core, raw radial (PLAN.md II.4; NOTES.md "READ -- record-tier
// exploratory read"). Only-follow traversal from a validated
// command::ReadRecord's anchor, over the controller door. Cache-shaped
// mode is out of scope regardless of ReadRecord::cache_shaped -- this
// module always executes the raw-radial (cache-free) core.
//
// This module CONSUMES command/ (the IR + validate_read) and controller/
// (the door's only-follow reads) unmodified. It adds no new storage
// access of its own; every result comes from parents_of / children_of /
// members_of / member_of, called once per node visited.
namespace dbread {

// Which stored follow produced a given result node, in traversal order.
// kAnchor marks the anchor itself (depth 0), reached by no follow.
enum class ReachedVia {
  kAnchor,
  kParent,    // structure axis, forward (down): Controller::parents_of.
  kChild,     // structure axis, reverse (up): Controller::children_of.
  kMembers,   // membership axis, forward (down): Controller::members_of.
  kMemberOf,  // membership axis, reverse (up): Controller::member_of.
};

// One entry of the linearized, once-per-path READ return (NOTES.md
// "Re-convergence -- RULED"). `depth` is the hop count from the anchor
// (0 = the anchor). Nothing here deduplicates: a token_id reached at two
// distinct points in the traversal produces two distinct ReadNode
// entries at their own positions in the enclosing vector -- position,
// depth and repetition together are the return's encoding of structure,
// per spec ("repetition + position encode the structure losslessly").
struct ReadNode {
  codec::Address address;
  unsigned depth = 0;
  ReachedVia via = ReachedVia::kAnchor;
};

enum class ReadStatus {
  // The traversal ran (possibly to an empty result, e.g. a nonexistent
  // anchor, a wildcard anchor whose region is empty, or a fully-excluded
  // anchor -- see read_core.cpp).
  kOk,
};

struct ReadResult {
  ReadStatus status = ReadStatus::kOk;
  std::vector<ReadNode> nodes;  // meaningful iff status == kOk.
};

// Executes op's raw-radial read against ctl. The caller is responsible
// for having already run op through command::validate_read(op) --
// this function assumes a structurally valid ReadRecord and does not
// re-check codec address well-formedness.
//
// A terminal-wildcard anchor (op.anchor's last element partial) is
// resolved via ctl.gather(op.anchor) -- the door's contiguous PK-range
// walk -- to the existing tokens under that prefix (NOTES.md "Gather
// primitive" / "READ -- terminal wildcards permitted"), and each
// resolved token is then read exactly as a concrete anchor would be, in
// gather's PK/address order, concatenated into one linearized,
// once-per-path, no-dedup return (see read_core.cpp).
ReadResult read(dbk::Controller &ctl, const command::ReadRecord &op);

}  // namespace dbread
