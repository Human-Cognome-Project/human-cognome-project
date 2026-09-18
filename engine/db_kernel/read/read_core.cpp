#include "read_core.h"

namespace dbread {

namespace {

// A wildcard is a terminal partial address (codec's wildcard-tail marker
// on the last element) -- same test command/command_ir.cpp uses.
bool is_wildcard(const codec::Address &address) {
  return !address.empty() && address.back().partial;
}

// True iff `address` falls under the tree region a terminal-wildcard
// `prefix` denotes: every element strictly before the wildcard's own
// position matches exactly, `address` has at least one more element than
// that (it is genuinely UNDER the prefix, not equal to its boundary), and
// its element at the wildcard's position shares the wildcard's leading
// character (the only character a partial element carries).
//
// This is a pure comparison between two addresses already in hand -- no
// store access. Wildcard EXCLUSIONS are resolved this way: every node
// this predicate is ever run against was already reached by a stored
// follow, and the predicate just decides whether to prune it. A wildcard
// ANCHOR is different in kind -- there is no node in hand yet to test --
// so it is resolved separately, via the door's gather() primitive (see
// read() below and README.md), not by this comparison.
bool address_under_prefix(const codec::Address &address,
                           const codec::Address &prefix) {
  if (prefix.empty()) return false;  // malformed prefix; matches nothing.
  const std::size_t cut = prefix.size() - 1;
  if (address.size() <= cut) return false;  // not even reaching the
                                             // wildcard position, or
                                             // exactly at the boundary --
                                             // "under" requires more.
  for (std::size_t i = 0; i < cut; ++i) {
    if (address[i] != prefix[i]) return false;
  }
  return address[cut].first == prefix[cut].first;
}

// Exclusions prune named branches -- specific token_ids, or (per NOTES.md
// "Build-phase rulings", READ's terminal-wildcard exclusions) a whole
// tree region named by a terminal wildcard. Applied uniformly to every
// node the traversal reaches, anchor included: an excluded node is
// dropped from the result AND not expanded (the whole branch beneath it
// is pruned, matching "prune named branches").
bool excluded(const codec::Address &address,
              const std::vector<codec::Address> &exclusions) {
  for (const codec::Address &ex : exclusions) {
    if (is_wildcard(ex)) {
      if (address_under_prefix(address, ex)) return true;
    } else if (address == ex) {
      return true;
    }
  }
  return false;
}

// The follows active at every level of the traversal, in a fixed,
// deterministic order: structure axis before membership axis, forward
// (down) before reverse (up) within an axis. NOTES.md/PLAN.md do not
// state a linearization order for combined axes (RADIAL or kBoth); this
// order is a decision, flagged in the handoff report, not a stated rule.
// RADIAL (direction.axis unset) walks all four regardless of `reverse`:
// "reverse selectable... on those axes" (PLAN.md I.C) reads as an
// orientation on a NARROWED axis, so it has nothing to select when
// nothing is narrowed -- RADIAL already means every direction outward.
std::vector<ReachedVia> active_follows(const command::ReadDirection &direction) {
  if (!direction.axis.has_value()) {
    return {ReachedVia::kParent, ReachedVia::kChild, ReachedVia::kMembers,
            ReachedVia::kMemberOf};
  }
  switch (*direction.axis) {
    case command::ReadAxis::kParents:
      return direction.reverse ? std::vector<ReachedVia>{ReachedVia::kChild}
                                : std::vector<ReachedVia>{ReachedVia::kParent};
    case command::ReadAxis::kMembers:
      return direction.reverse
                 ? std::vector<ReachedVia>{ReachedVia::kMemberOf}
                 : std::vector<ReachedVia>{ReachedVia::kMembers};
    case command::ReadAxis::kBoth:
      return direction.reverse
                 ? std::vector<ReachedVia>{ReachedVia::kChild, ReachedVia::kMemberOf}
                 : std::vector<ReachedVia>{ReachedVia::kParent, ReachedVia::kMembers};
  }
  return {};  // unreachable; silences -Wreturn-type on some compilers.
}

// The one-level neighbour addresses reached via `follow` from `address`,
// in the order the controller returns them. parents_of's ParentEntry
// list is unpacked to its addresses in ordinal order, WITHOUT
// deduplicating a repeated constituent (mirroring mint's own token_parent
// rows: a constituent used twice keeps two ordinal rows) -- consistent
// with the no-dedup return rule.
std::vector<codec::Address> follow_addresses(dbk::Controller &ctl,
                                              const codec::Address &address,
                                              ReachedVia follow) {
  switch (follow) {
    case ReachedVia::kParent: {
      std::vector<codec::Address> out;
      for (const dbk::ParentEntry &pe : ctl.parents_of(address)) {
        out.push_back(pe.parent);
      }
      return out;
    }
    case ReachedVia::kChild:
      return ctl.children_of(address);
    case ReachedVia::kMembers:
      return ctl.members_of(address);
    case ReachedVia::kMemberOf:
      return ctl.member_of(address);
    case ReachedVia::kAnchor:
      break;  // never a follow target; see visit() below.
  }
  return {};
}

// Depth-first pre-order walk: emit the node, then -- unless the depth
// budget is spent -- recurse into each active follow's neighbours in
// order. `max_depth` bounds recursion on its own (no visited-set is
// used, since "no dedup" extends to not collapsing a structural cycle
// either; a finite depth budget is what keeps this terminating, exactly
// as it would need to for a DAG walked to a fixed LoD).
void visit(dbk::Controller &ctl, const codec::Address &address, unsigned depth,
           ReachedVia via, unsigned max_depth,
           const std::vector<codec::Address> &exclusions,
           const std::vector<ReachedVia> &follows, std::vector<ReadNode> &out) {
  if (excluded(address, exclusions)) return;
  out.push_back(ReadNode{address, depth, via});
  if (depth >= max_depth) return;
  for (ReachedVia follow : follows) {
    for (const codec::Address &next : follow_addresses(ctl, address, follow)) {
      visit(ctl, next, depth + 1, follow, max_depth, exclusions, follows, out);
    }
  }
}

}  // namespace

ReadResult read(dbk::Controller &ctl, const command::ReadRecord &op) {
  ReadResult result;
  result.status = ReadStatus::kOk;

  const std::vector<ReachedVia> follows = active_follows(op.direction);

  // A terminal-wildcard anchor names a tree region, not a single token
  // (NOTES.md "READ -- terminal wildcards permitted": "a nominal,
  // tree-constrained read"). It is resolved via the door's gather
  // primitive -- a contiguous PK-range walk over the EXISTING tokens
  // under that prefix, in PK/address order (NOTES.md "Gather primitive")
  // -- and each resolved token is then read exactly as a concrete anchor
  // is below: its own raw-radial visit(), depth 0 at the resolved token
  // itself. The overall return is the concatenation of those per-member
  // reads, in gather's order. This stays linearized, once-per-path,
  // no-dedup: nothing collapses a node reachable from two different
  // gathered members, or from the same member via two paths, exactly as
  // a concrete-anchor read never collapses a repeat (see visit() above).
  // An empty gathered region (no existing token under the prefix)
  // produces an empty, still-kOk result -- a well-formed read of nothing,
  // not an error.
  if (is_wildcard(op.anchor)) {
    for (const codec::Address &member : ctl.gather(op.anchor)) {
      visit(ctl, member, 0, ReachedVia::kAnchor, op.depth, op.exclusions, follows,
            result.nodes);
    }
    return result;
  }

  // A nonexistent anchor simply has nothing to read: the request is
  // well-formed, the traversal runs, and it produces no nodes. Not
  // stated explicitly in NOTES.md/PLAN.md; the plain, non-inventive
  // reading of "only-follow from the anchor" when there is no such
  // token to follow from.
  if (!ctl.token_exists(op.anchor)) {
    return result;
  }

  visit(ctl, op.anchor, 0, ReachedVia::kAnchor, op.depth, op.exclusions, follows,
        result.nodes);
  return result;
}

}  // namespace dbread
