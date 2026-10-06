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

}  // namespace dbk::bootstrap
