#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"

// The address span planner (PLAN.md II.2): resolves an ADDRESS span
// expression (I.E) against a slot count N into a concrete per-slot
// address plan, or reports why it cannot.
//
// Pure/parse-level, standard library + the codec only. Does NOT resolve
// open seams G4 (next-slot mechanism) or G5 (block boundaries past "hex
// couplets") -- see PlanStatus::kPendingSeam below. Fill uses the
// analyst-supplied-start form (FROM:x, given directly): sequential fill
// needs no cursor, per PLAN.md Part V.
namespace command {

// ---------------------------------------------------------------------
// Radix-64 address successor -- the codec exposes encode/decode and delta
// transforms but no increment; this is that increment, built purely from
// the codec's alphabet/couplet primitives.
// ---------------------------------------------------------------------

// The address immediately following `address`, treating it as a
// mixed-radix (base = codec::kCoupletSpace) counter, most-significant
// element first (matching an address's own left-to-right ordering; the
// LAST element is the fastest-varying "leaf" digit). Carries leftward on
// per-element overflow.
//
// Returns nullopt when:
//   - `address` is empty (nothing to increment);
//   - any element is partial (a wildcard/context element has no single
//     successor -- it addresses a range, not a point);
//   - the carry propagates out of the leftmost element, i.e. the address
//     is already the last one representable at its current depth.
//     Growing a new ring to keep counting is a depth-expansion decision
//     gated on the trunk map (PLAN.md Part V, G5) and is deliberately
//     NOT made here -- nullopt is the honest answer, not a guessed
//     boundary.
std::optional<codec::Address> successor(const codec::Address &address);

// The number of slots from `from` to `to` inclusive, i.e.
// |{from, successor(from), successor(successor(from)), ..., to}|,
// computed by exact digit-wise arithmetic over the same radix-64 couplet
// representation `successor` uses (not by stepping `successor` in a
// loop). Returns nullopt when:
//   - `from` and `to` have different lengths (different address depth --
//     not a single run; resolving across a depth change is a G5
//     question, not answered here);
//   - either address contains a partial element;
//   - `to` precedes `from` (not a forward run);
//   - the count would overflow std::size_t.
std::optional<std::size_t> span_length(const codec::Address &from,
                                        const codec::Address &to);

// ---------------------------------------------------------------------
// plan(): resolve an ADDRESS span against N slots.
// ---------------------------------------------------------------------

enum class PlanStatus {
  // Every slot has a concrete resolution (a concrete address, an inert
  // undeclared-hook marker, or a nested-declare occupant) and the span
  // covers exactly N.
  kValid,
  // Malformed: does not cover N, an elastic origin is not last, a
  // required value is missing, or a FROM..TO run cannot be walked
  // (backwards, cross-depth, or exceeds representable depth).
  kInvalid,
  // Structurally well-formed, but contains an AFTER segment whose
  // concrete origin ("the next block after b") depends on the deferred
  // trunk map (PLAN.md Part V, G5). Per NOTES.md "Open decisions", the
  // next-slot mechanism itself is also open (G4); this module uses the
  // analyst-supplied-start form (FROM) for anything it actually fills,
  // and reports AFTER as pending rather than inventing a boundary.
  kPendingSeam,
};

struct PlannedSlot {
  // The slot's concrete address, when resolvable now. Nullopt when the
  // owning segment is a kAfter origin (G5-pending; see PlanStatus) or a
  // kNestedDeclare (its address is only known once that nested statement
  // actually mints -- an execution-time fact, not a planning gap).
  std::optional<codec::Address> address;

  // True for a kUndeclared slot: no address is assigned here at all; it
  // is filed via the inert undeclared -> cross-connection hook.
  bool is_undeclared_hook = false;

  // Set for a kNestedDeclare slot: which nested statement occupies it.
  // Non-owning -- the AddressSpan (and its segments' shared_ptr<DeclareRecord>)
  // outlives the PlanResult in every call shape this module supports.
  const DeclareRecord *nested = nullptr;
};

struct PlanResult {
  PlanStatus status = PlanStatus::kInvalid;
  // Populated iff status != kInvalid: one entry per slot, in order.
  // (kPendingSeam may still return a partial prefix planned before the
  // pending segment was reached -- see plan()'s doc comment.)
  std::vector<PlannedSlot> slots;
  // Populated when status == kInvalid: why. Empty otherwise.
  std::string reason;
};

// Resolves `span` against `n` slots.
//
// Rules enforced (PLAN.md I.E / II.2):
//   - segments lay end to end; validity requires them to cover exactly N;
//   - an open (un-TO'd) FROM or AFTER origin is elastic and may be the
//     LAST segment only;
//   - every other segment (direct, pin, a closed FROM/AFTER run, a
//     nested declare, the undeclared hook) is self-delimiting and legal
//     anywhere, including interior;
//   - a nested declare is always exactly one slot.
//
// An AFTER segment is always structurally legal wherever a FROM would be,
// but its concrete origin cannot be computed without the trunk map (G5).
// A closed `AFTER:b TO:y` therefore cannot even have its SLOT COUNT
// verified (count = distance(resolved-start, y), and resolved-start is
// unknown) -- plan() reports kPendingSeam as soon as it reaches such a
// segment, with the slots already planned before it. An open (elastic,
// last-segment) AFTER's slot COUNT is known (it is simply "whatever
// remains"), so plan() still fills that many slots, each with a nullopt
// address (kPendingSeam), rather than failing validation outright.
PlanResult plan(const AddressSpan &span, std::size_t n);

// ---------------------------------------------------------------------
// N-independent span shape validation.
// ---------------------------------------------------------------------

struct SpanShapeResult {
  bool ok = true;
  // Populated iff !ok.
  std::string reason;
};

// The grammar rules from `plan()` that do NOT require knowing N: every
// address value is a valid codec address, a nested declare recurses into
// full structural validation, a closed FROM..TO run is a forward,
// same-depth walk, and an open (un-TO'd) FROM/AFTER origin is the last
// segment. Does NOT check cover-N -- there being no N to cover here.
//
// Used where N genuinely is not statically known at IR-validation time
// -- e.g. MOVE_RECORD's destination, once its source may be a range or
// wildcard whose resolved unit count depends on the live store (PLAN.md
// I.D "Ranged/arrayable"; Patrick's F-1 resolution defers destination
// cover-N to the UPDATE core, PLAN.md II.5).
SpanShapeResult validate_span_shape(const AddressSpan &span);

}  // namespace command
