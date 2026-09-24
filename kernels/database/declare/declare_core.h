#pragma once

#include <optional>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"

// The DECLARE core (PLAN.md II.3): executes one DECLARE_RECORD statement --
// already structurally validated by command::validate_declare -- against the
// controller door, minting structure and authoring the members/member_of
// membership reciprocal.
//
// Scope: ONE DECLARE statement, including its N-member PARENTS set (PARENTS
// sets N; co-indexed NOTATION/MEMBER_OF) and its nesting -- any relationship
// field may be a full nested DECLARE, recursed here, building the connected
// sub-structure in one call. Cross-COMMAND arraying (a serial stream of
// several DECLARE statements sharing a scalar frame) is a later module
// (PLAN.md II.7) -- out of scope here.
//
// Spec anchors: PLAN.md I.B, I.E, I.F, I.G, II.3, II.6; NOTES.md
// "Relationship model & type -- firmed 2026-09-17", "The literal intake
// formula" (REVISED 2026-09-17), "DECLARE format" (REVISED 2026-09-17),
// "Build-phase rulings -- firmed 2026-09-17". See README.md in this
// directory for the semantic decisions made where the spec was silent.
namespace declare {

// One returned point (PLAN.md I.G "Return contract"):
//   - ingested: `address` is the ACTUAL assigned address (never an echo of
//     a requested `@`-pin -- there is no override mechanism built yet, so
//     today the two coincide, but the field always reports what mint()
//     actually placed).
//   - rejected: `reason` is populated, `address` is nullopt. No exception
//     is thrown for an ordinary rejection (a missing constituent, an
//     unresolvable naming literal, ...); see README.md for exactly which
//     paths reject vs. throw (a genuine store/connection error still
//     throws, via the controller).
struct Outcome {
  bool ingested = false;
  std::optional<codec::Address> address;
  std::string reason;
};

// The full result of one DECLARE_RECORD statement:
//   - PARENTS present (a structure node): one Outcome per PARENTS member,
//     in PARENTS order (N = parents->size()).
//   - PARENTS absent (a pure-grouping node): exactly one Outcome -- the
//     resolved naming-literal token's address (PLAN.md I.B "Grouping-node
//     identity"; I.G "a grouping-only node returns its naming-literal
//     token_id, not a freshly placed address").
struct Result {
  std::vector<Outcome> outcomes;
};

// Executes `node` against `ctl`. `node` is expected to already have passed
// command::validate_declare -- this function re-validates defensively (an
// already-valid node is unaffected) but that is not a substitute for
// validating untrusted input upstream.
Result execute(dbk::Controller &ctl, const command::DeclareRecord &node);

}  // namespace declare
