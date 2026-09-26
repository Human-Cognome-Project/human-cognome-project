// endpoint -- identity plus the local registry (name -> box::Box*).
//
// See box.h for where EndpointId is physically defined and why. This
// header builds the registry semantics around it: disjoint standing /
// ephemeral slot ranges (F3), a standing-generation sentinel (F7),
// allocate/recycle with the ONE generation bump-site at recycle (F2), and
// stale-generation drop on resolve. Only-follow: resolve() is a direct
// lookup, never a scan.
//
// The node_addr half of endpoint identity (cross-node / token_id-space
// addressing) is a deferred seam -- this registry is local-process only.
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "box.h"

namespace endpoint {

class Registry {
 public:
  // Fixed sentinel generation carried by every standing EndpointId (F7).
  // resolve() never checks it against anything -- a standing slot
  // resolves unconditionally, regardless of the generation the caller
  // passes. The sentinel exists only so a standing id's provenance is
  // visually distinct from a real ephemeral generation counter (which
  // starts at 0 and counts up from ordinary recycles).
  static constexpr uint32_t kStandingGeneration = 0xFFFFFFFFu;

  // standing_capacity and ephemeral_capacity define two DISJOINT slot
  // ranges (F3): standing = [0, standing_capacity), ephemeral =
  // [standing_capacity, standing_capacity + ephemeral_capacity).
  // allocate() therefore can never return a slot register_standing could
  // also claim -- a same-generation collision between the two kinds is
  // structurally impossible, not left to the generation stamp to catch.
  Registry(uint32_t standing_capacity, uint32_t ephemeral_capacity);

  // Register a fixed, well-known endpoint (a component input box, the
  // pinned reconcile box). `slot` must lie in the standing range; `box`
  // is owned by the CALLER (the setup/topology code), not the registry,
  // and must outlive it. Stable for the process lifetime. Returns the
  // canonical EndpointId (generation == kStandingGeneration) for the
  // caller to hold onto. Throws std::invalid_argument if `slot` is
  // outside the standing range.
  EndpointId register_standing(uint32_t slot, box::Box *box);

  // Draw an ephemeral return endpoint from the freelist. The Box itself
  // is owned BY THE REGISTRY (ephemeral endpoints are anonymous return
  // mailboxes, not part of the wired topology). Does NOT bump generation
  // (F2) -- returns the slot at its current generation. Throws
  // std::runtime_error if the ephemeral pool is exhausted.
  EndpointId allocate();

  // Direct lookup, never a scan. A standing slot always resolves,
  // regardless of the generation carried in `id` (F7). An ephemeral slot
  // resolves only on an exact generation match; a stale generation (the
  // slot has been recycled since) yields nullptr, so a late or duplicate
  // ack is dropped, never misdelivered. Returns nullptr for a slot in
  // neither range.
  box::Box *resolve(EndpointId id);

  // Return an ephemeral slot to the freelist and bump its generation --
  // the ONE and ONLY generation bump-site (F2). A late ack addressed to
  // the pre-recycle id fails resolve() from this point on. Any messages
  // still queued in the slot's box are discarded, so the next allocate()
  // always hands out an empty return mailbox. Throws
  // std::invalid_argument if `id` does not name a currently-allocated
  // ephemeral slot at its current generation.
  void recycle(EndpointId id);

  bool is_standing_slot(uint32_t slot) const;
  bool is_ephemeral_slot(uint32_t slot) const;

 private:
  uint32_t standing_capacity_;
  uint32_t ephemeral_capacity_;

  std::unordered_map<uint32_t, box::Box *> standing_;  // slot -> caller-owned box

  std::vector<box::Box> ephemeral_boxes_;       // index = slot - standing_capacity_
  std::vector<uint32_t> ephemeral_generation_;  // parallel: current generation
  std::vector<uint32_t> freelist_;              // available ephemeral slot numbers
};

}  // namespace endpoint
