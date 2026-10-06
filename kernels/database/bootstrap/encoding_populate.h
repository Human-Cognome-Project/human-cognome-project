#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "codec.h"
#include "controller.h"

// Bootstrap driver logic: populates the multi-byte character tokens of the
// encoding floor through the Controller. Offline bootstrap only — it walks
// Unicode U+0080..U+10FFFF, UTF-8-encodes each codepoint, and mints the
// character as a composite over its ordered byte-code tokens (the floor's
// 256 hex couplets), as a member of its byte-length category label.
//
// Masses: a character is minted with NO mass; the structural-mass trigger
// (schema/structural_mass_trigger.sql) fills it as the sum of its parents.
// The three category labels carry a manually set temporary placeholder mass
// (same convention as the floor's provisional labels).
namespace dbk::bootstrap {

enum class Category { Two, Three, Four };

// Floor addresses (see NOTES.md "Addressing re-based to the 00 root").
codec::Address byte_code_address(uint8_t byte);      // 00.00.00.00.<10..57>
codec::Address overgroup_address();                  // 00.00.01.00.02
codec::Address category_label_address(Category c);   // 00.00.01.00.03..05
const char *category_label_notation(Category c);

// Temporary placeholder mass of the category labels (as the floor's labels).
constexpr int kLabelPlaceholderMass = 10;

// UTF-8 bytes of a codepoint. Empty for surrogates and values > 0x10FFFF.
std::vector<uint8_t> utf8_encode(uint32_t codepoint);

// Category by UTF-8 length; only for codepoints U+0080..U+10FFFF, not
// surrogates (callers must check is_populated_codepoint).
Category category_of(uint32_t codepoint);
bool is_populated_codepoint(uint32_t codepoint);

// The character's address: the next sequential slot of its category's
// reserved range, counting codepoints in order with surrogates skipped.
// The slot is a pure function of the codepoint, so a sample run lands on the
// same address a full run would.
//   2-byte 00.00.00.01.* – 02.*   3-byte 00.00.00.03.* – 0z.*
//   4-byte 00.00.00.10.* – zz.*
codec::Address character_address(uint32_t codepoint);

// Creates the three category labels (placeholder mass, members of the
// overgroup), idempotently, each in its own command transaction.
void create_category_labels(Controller &ctl);

// Verifies the floor the driver relies on: the 256 byte-code tokens exist
// with notation = two hex digits, and the overgroup exists. Throws
// std::runtime_error naming the first fault.
void verify_floor(Controller &ctl);

// One atomic command for one character: mint (parents = byte-code tokens in
// order, no mass) + add_membership to `label`, inside with_transaction. A
// failure rolls the whole character back. Re-running an existing character
// is a no-op provided its stored parents match; a mismatch throws.
// Returns true if the character was freshly minted.
bool populate_character(Controller &ctl, uint32_t codepoint,
                        const codec::Address &label);
bool populate_character(Controller &ctl, uint32_t codepoint);

// Populates one codepoint (checks is_populated_codepoint).
void populate_codepoint(Controller &ctl, uint32_t codepoint);

// The sample set: U+00E9, U+20AC, U+1F600.
std::vector<uint32_t> sample_codepoints();

// ---- Endpoint tier (ENCODING-ENDPOINT-TIER-PLAN.md). ----
//
// An endpoint is the actual-character token: one per codepoint, with exactly
// one structural parent (the byte-couplet token the UTF-8 mapping links it to)
// and one membership (the UTF-8 label). Minted through the same direct
// Controller channel as the byte tiers (DECLARE's >= 2-constituent floor
// cannot express a one-parent token). No mass is passed: the structural-mass
// trigger sets it as the sum of the parents' masses.

// The codepoint written as four Base62 digits grouped in two couplets, under
// the next free trunk:  00.00.02.<cp / 3844>.<cp % 3844>. Uses nothing but the
// codepoint (no encoding, no byte-couplet layout, no special cases); PK order
// is codepoint order. Throws std::invalid_argument for a value > U+10FFFF or a
// surrogate (never minted); U+0000..U+007F have addresses though this build
// mints only U+0080 and up.
codec::Address endpoint_address(uint32_t codepoint);

// The UTF-8 label (character-level encoding membership): 00.00.01.00.06.
codec::Address utf8_label_address();
constexpr const char *kUtf8LabelNotation = "UTF-8";

// Creates the UTF-8 label (placeholder mass, no parents, member of nothing),
// idempotently, in its own command transaction.
void create_utf8_label(Controller &ctl);

// Throws std::runtime_error unless the UTF-8 label exists with its notation.
void verify_utf8_label(Controller &ctl);

// The UTF-8 mapping's link-in: the byte-couplet token of `codepoint`
// (character_address, a direct PK read). Throws std::runtime_error, writing
// nothing, unless that token exists, has a non-NULL mass, and its notation is
// the upper-case hex of utf8_encode(codepoint). The address is the identity;
// the notation is only a cross-check. Returns the combination's address.
codec::Address resolve_combination(Controller &ctl, uint32_t codepoint);

// Reads and resolves the combinations of the given codepoints (the sample
// mode's pre-check). Throws on the first fault.
void verify_combinations(Controller &ctl, const std::vector<uint32_t> &codepoints);

// One atomic command for one endpoint: resolve_combination, then
// with_transaction { mint (token_text derived, notation = the character as
// UTF-8 text, no mass, one constituent = the combination) ; add_membership to
// the UTF-8 label }. Returns true if freshly minted.
//
// An existing endpoint is accepted (returns false, nothing written) only if
// the WHOLE validates: the combination is among its parents; its mass equals
// the sum of attributes_of(parent).mass over ALL its parents; the combination
// lists it in children_of; BOTH membership directions are present
// (membership_present); and its notation is the character. Anything else
// throws std::runtime_error and writes nothing -- never healed, and
// add_membership is run only for a fresh mint. The parent count is never
// required to be 1.
bool populate_endpoint(Controller &ctl, uint32_t codepoint);

}  // namespace dbk::bootstrap
