#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "codec.h"

// The record-tier Command IR: the parsed, in-process request shape the
// record-tier cores (DECLARE / READ / UPDATE) consume, plus its structural
// validation. Pure/parse-level -- standard library and the codec only, no
// DB, no libpq. Text/wire framing into this IR is a separate, unbuilt
// layer (PLAN.md I.H, seam G6); this module starts from already-parsed
// C++ values.
//
// Spec anchors (see PLAN.md Part I-II and NOTES.md "Relationship model &
// type -- firmed 2026-09-17" / "The literal intake formula" / "DECLARE
// format" / "UPDATE -- record-tier ops" / "READ -- record-tier exploratory
// read"): TYPE is dropped everywhere below. Validation is structural --
// which downward field is present -- never a declared/type-gated branch.
namespace command {

// ---------------------------------------------------------------------
// DECLARE_RECORD (PLAN.md I.B / II.1; NOTES.md "The literal intake
// formula", REVISED 2026-09-17 marker).
// ---------------------------------------------------------------------

struct DeclareRecord;  // forward: a Reference may nest a full declare.

// One entry in PARENTS / MEMBERS / MEMBER_OF: either a plain reference to
// an already-existing (or sibling-in-this-statement) address, or a full
// nested DECLARE_RECORD of the same shape ("grouping by nesting" /
// "recursion" -- NOTES.md). Exactly one of the two must be set; a
// Reference that is neither or both is a malformed IR node, not a valid
// unresolved state.
struct Reference {
  std::optional<codec::Address> address;
  std::shared_ptr<DeclareRecord> nested;

  bool is_nested() const { return nested != nullptr; }

  static Reference ToAddress(codec::Address address) {
    Reference r;
    r.address = std::move(address);
    return r;
  }
  static Reference ToNested(std::shared_ptr<DeclareRecord> declare) {
    Reference r;
    r.nested = std::move(declare);
    return r;
  }
};

// One PARENTS member: an ordered list of constituent references. The
// anti-alias floor (>= 2) is enforced by validate_declare, not by this
// type -- the IR carries whatever was parsed so a too-short list can be
// reported as a validation failure rather than silently rejected at
// construction.
using ConstituentList = std::vector<Reference>;

// ---------------------------------------------------------------------
// ADDRESS span expression (PLAN.md I.E; NOTES.md "The literal intake
// formula" ADDRESS paragraph, "DECLARE addressing modes"). Also reused
// (unchanged grammar) by MOVE_RECORD's destination span.
// ---------------------------------------------------------------------

struct AddressSegment {
  enum class Kind {
    kDirect,        // one explicit address value -- one slot.
    kPin,           // '@'-pin: a *requested* address, the manager may
                    // override it at execution -- one slot here.
    kFrom,          // FROM:x -- an origin. Elastic (consumes whatever
                    // slots remain) unless closed by a TO bound, in which
                    // case it is fixed-count and self-delimiting.
    kAfter,         // AFTER:b -- an origin keyed to the next block after
                    // block b (trunk-shift). Same elastic/closed shape as
                    // kFrom, but see span_planner.h: resolving the
                    // concrete start address needs the deferred trunk map
                    // (PLAN.md Part V, G5) and is NOT done here.
    kNestedDeclare, // a nested DECLARE_RECORD -- mints one item, one
                    // self-delimiting slot, legal anywhere (interior or
                    // last) EXCEPT in a DeclareRecord's own ADDRESS when
                    // that node's PARENTS is present (ruling #3, Patrick,
                    // build-phase; rejected -- it would collide with
                    // that slot's own PARENTS composition). Unrestricted
                    // elsewhere, e.g. MOVE_RECORD's destination.
    kUndeclared,    // the inert undeclared -> cross-connection hook: a
                    // left-undeclared slot. No address is assigned by
                    // this module; the hook is exact-method-TBD, per
                    // NOTES.md "DECLARE addressing modes".
  };

  Kind kind = Kind::kDirect;

  // kDirect / kPin: the segment's single address value.
  // kFrom / kAfter: the origin address (kAfter: the referenced block).
  std::optional<codec::Address> origin;

  // Present iff this kFrom/kAfter origin is closed by a TO:y bound. TO
  // is "a condition on the preceding origin, never standalone" (I.E), so
  // it is modelled as part of the origin segment rather than as its own
  // free-standing segment kind -- a standalone TO is simply not a
  // representable AddressSegment, which enforces the "never standalone"
  // rule by construction.
  std::optional<codec::Address> to_bound;

  // Set iff kind == kNestedDeclare.
  std::shared_ptr<DeclareRecord> nested;

  static AddressSegment Direct(codec::Address a) {
    AddressSegment s;
    s.kind = Kind::kDirect;
    s.origin = std::move(a);
    return s;
  }
  static AddressSegment Pin(codec::Address a) {
    AddressSegment s;
    s.kind = Kind::kPin;
    s.origin = std::move(a);
    return s;
  }
  static AddressSegment From(codec::Address origin,
                              std::optional<codec::Address> to = std::nullopt) {
    AddressSegment s;
    s.kind = Kind::kFrom;
    s.origin = std::move(origin);
    s.to_bound = std::move(to);
    return s;
  }
  static AddressSegment After(codec::Address block,
                               std::optional<codec::Address> to = std::nullopt) {
    AddressSegment s;
    s.kind = Kind::kAfter;
    s.origin = std::move(block);
    s.to_bound = std::move(to);
    return s;
  }
  static AddressSegment Nested(std::shared_ptr<DeclareRecord> declare) {
    AddressSegment s;
    s.kind = Kind::kNestedDeclare;
    s.nested = std::move(declare);
    return s;
  }
  static AddressSegment Undeclared() {
    AddressSegment s;
    s.kind = Kind::kUndeclared;
    return s;
  }
};

// Segments lay end to end; validity (see span_planner.h) requires them to
// cover exactly N slots.
using AddressSpan = std::vector<AddressSegment>;

// ---------------------------------------------------------------------
// DeclareRecord: one DECLARE_RECORD statement, in its native arrayed/
// serial form (an array IS an ordered serial stream sharing a scalar
// frame -- NOTES.md "Arraying is universal"; PLAN.md I.F). This IR does
// NOT expand an arrayed declare into N single-item declares -- that
// broadcast/zip is the arraying executor's job (PLAN.md II.7), out of
// this module's scope.
// ---------------------------------------------------------------------

struct DeclareRecord {
  // ADDRESS -- required for a plain structure node (PARENTS present,
  // MEMBERS absent). For a grouping node (MEMBERS present), ADDRESS
  // establishes/references its naming literal (ruling #1, Patrick,
  // build-phase -- SUPERSEDES the earlier "ADDRESS forbidden on a
  // grouping node" rule): PARENTS present => ADDRESS, if given, is the
  // mint's placement target (else manager-placed); PARENTS absent =>
  // ADDRESS is REQUIRED and references the pre-existing naming literal
  // (the "use-provided-ID" form). A grouping node with neither PARENTS
  // nor ADDRESS establishes no naming literal and is rejected. See
  // validate_declare.
  std::optional<AddressSpan> address;

  // NOTATION -- positional-partial. When present its length must equal N
  // (see validate_declare); blank slots (nullopt entries, "successive
  // commas") are legal and are NOT a co-index violation.
  std::vector<std::optional<std::string>> notation;

  // PARENTS -- structure constituents; THE member array. Present <=>
  // this node is a structure node. Outer length sets N. Each entry's
  // list must carry >= 2 constituent references (anti-alias floor).
  std::optional<std::vector<ConstituentList>> parents;

  // MEMBERS -- grouping roster. Present <=> this node is a grouping
  // node. Must be non-empty (floor >= 1) when present. A node may carry
  // both PARENTS and MEMBERS (structure and grouping at once -- the
  // "mint form" of the naming literal, see ADDRESS above); the
  // literal-vs-label reading is emergent, never asserted here. When both
  // are present, PARENTS must declare exactly one literal (N=1) -- a
  // naming-literal mint is a single literal; N>1 broadcasting one
  // MEMBERS roster across several literals is unspecified and rejected
  // (ruling #2, Patrick, build-phase; fail-fast default).
  std::optional<std::vector<Reference>> members;

  // MEMBER_OF -- membership, up. Section 1 (`shared`) broadcasts to all
  // N members; section 2 (`per_member`) is a SPARSE positional map
  // (member index -> that member's additional memberships), size <= N,
  // gaps legal. Neither section is a co-index array.
  struct MemberOf {
    std::vector<Reference> shared;
    std::map<std::size_t, std::vector<Reference>> per_member;
  };
  std::optional<MemberOf> member_of;
};

// ---------------------------------------------------------------------
// READ_RECORD (PLAN.md I.C; NOTES.md "READ -- record-tier exploratory
// read").
// ---------------------------------------------------------------------

enum class ReadAxis {
  kParents,  // structure-down.
  kMembers,  // membership-down.
  kBoth,
};

struct ReadDirection {
  // nullopt => RADIAL (the default): no narrowing, all directions.
  std::optional<ReadAxis> axis;
  // An orientation on the narrowed axis/axes above, not a peer value of
  // `axis` -- NOTES.md: "reverse selectable... (not a peer value)".
  bool reverse = false;
};

struct ReadRecord {
  codec::Address anchor;                  // a single token_id.
  ReadDirection direction;                // optional; default = radial.
  unsigned depth = 0;                     // LoD dial; depth 0 = rolled-up.
  std::vector<codec::Address> exclusions; // specific token_ids only.
  // Named, OPTIONAL; deferred with the cache tier (PLAN.md I.C) -- the
  // record-tier core function is raw-radial only regardless of this
  // flag's value.
  bool cache_shaped = false;
};

// ---------------------------------------------------------------------
// UPDATE_RECORD sub-ops (PLAN.md I.D; NOTES.md "UPDATE -- record-tier
// ops").
// ---------------------------------------------------------------------

// MOVE_RECORD's source selector. A range or prefix/context wildcard is
// MOVE's characteristic mechanism -- its primary use is BULK CORRECTION:
// relocating a whole misclassified branch/region in one instruction. This
// is NOT stated as MOVE-exclusive machinery (the spec does not restrict
// wildcards to MOVE alone); it is simply the form MOVE needs and the
// other record-tier ops do not. The one op with a STATED no-wildcard
// rule is DELETE (NOTES.md UPDATE ops: "specific IDs and connections
// ONLY -- no wildcards"); see validate_delete_record/
// validate_delete_connection. Elsewhere the spec is silent, so no
// restriction is invented for those ops either. A range or prefix
// wildcard is still only-follow, not a search: it is a contiguous
// address region or a trunk/block followed deterministically as a
// whole, never a scan.
struct MoveSource {
  enum class Kind {
    kExplicit,  // one concrete, already-existing address -- the
                // degenerate single-token case.
    kRange,     // a contiguous FROM..TO range, both ends concrete.
    kPrefix,    // a prefix/context wildcard -- a codec partial address
                // (e.g. "A*") selecting the whole block/trunk beneath it.
  };

  Kind kind = Kind::kExplicit;
  // kExplicit: the token. kRange: the range start. kPrefix: the partial
  // (wildcard-tail) address itself.
  codec::Address from;
  // kRange only: the inclusive range end.
  std::optional<codec::Address> to;

  static MoveSource Explicit(codec::Address a) {
    MoveSource s;
    s.kind = Kind::kExplicit;
    s.from = std::move(a);
    return s;
  }
  static MoveSource Range(codec::Address from, codec::Address to) {
    MoveSource s;
    s.kind = Kind::kRange;
    s.from = std::move(from);
    s.to = std::move(to);
    return s;
  }
  static MoveSource Prefix(codec::Address wildcard) {
    MoveSource s;
    s.kind = Kind::kPrefix;
    s.from = std::move(wildcard);
    return s;
  }
};

// MOVE_RECORD -- bookkeeping-only relocation. `sources` is the arrayable
// side (PLAN.md I.D "Ranged/arrayable"): one or more source selectors,
// coexisting freely -- an explicit token, a range, and a prefix wildcard
// may all appear in the same MOVE. `destination` is an ADDRESS span
// (I.E), the SAME grammar and open/elastic rules as DECLARE's ADDRESS --
// no MOVE-specific restriction.
//
// Cover-N (validate_move_record): when every source is kExplicit, N =
// sources.size() is known statically, and the destination is checked to
// cover it exactly, the same way DECLARE's ADDRESS is. When ANY source
// is a kRange/kPrefix wildcard, the resolved unit count depends on which
// addresses in that region are actually populated in the live store --
// not known at IR-validation time -- so cover-N is skipped entirely for
// the whole statement and left to the UPDATE core (PLAN.md II.5), which
// has DB access. This module always validates that each source selector
// and the destination span are individually well-formed, regardless of
// which cover-N path applies.
struct MoveRecord {
  std::vector<MoveSource> sources;
  AddressSpan destination;
};

// ADD_CONNECTION <group> <element...> -- assert membership. `group` is
// the scalar frame (must lead); `elements` is the arrayable side (>= 1).
// No connection-kind operand; mints nothing (endpoints must pre-exist).
struct AddConnection {
  codec::Address group;
  std::vector<codec::Address> elements;
};

// DELETE_RECORD -- single instruction only, never arrayed; specific id
// only, no wildcard. The active-confirmation / peer-validation-tag gates
// (PLAN.md I.D, G7) are op-layer behaviour, not represented as IR data
// here.
struct DeleteRecord {
  codec::Address token;
};

// DELETE_CONNECTION -- single, specific pair only. Removes the named
// edge and its stored reciprocal (both-direction removal).
struct DeleteConnection {
  codec::Address a;
  codec::Address b;
};

// ---------------------------------------------------------------------
// Structural validation.
//
// No TYPE anywhere: every check below is "which field is present", never
// a declared-type branch. Applies per node -- a Reference's nested
// DeclareRecord is validated recursively, so a mixed nested statement
// (structure nodes and grouping nodes at different depths) is checked
// node by node.
// ---------------------------------------------------------------------

enum class ValidationStatus { kValid, kInvalid };

struct ValidationResult {
  ValidationStatus status = ValidationStatus::kInvalid;
  // Populated on kInvalid: a human-readable reason for the first failure
  // found. Empty on kValid.
  std::string reason;

  bool ok() const { return status == ValidationStatus::kValid; }
};

ValidationResult validate_declare(const DeclareRecord &node);
ValidationResult validate_read(const ReadRecord &op);
ValidationResult validate_move_record(const MoveRecord &op);
ValidationResult validate_add_connection(const AddConnection &op);
ValidationResult validate_delete_record(const DeleteRecord &op);
ValidationResult validate_delete_connection(const DeleteConnection &op);

}  // namespace command
