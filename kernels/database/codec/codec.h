#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The address / token_id codec: pure functions that convert between the
// base-50 couplet alphabet, address sequences, and their canonical string
// (token_id) form, plus a position-based delta transform used for
// prefix-relative (context) addressing.
//
// Nothing here talks to storage or holds state. Every function is a pure
// transform over its arguments.
namespace codec {

// ---------------------------------------------------------------------
// Alphabet: the single source of truth.
//
// Base-50: A-Z and a-z (52 characters) minus 'o' and 'O' (50 characters).
// 'o'/'O' are excluded (easily confused with '0' in a handwritten or
// spoken token_id) and every function below rejects them as invalid
// input, along with any character outside the alphabet.
//
// Ordering is the plain documented order: the 25 surviving uppercase
// letters (A-N, P-Z) followed by the 25 surviving lowercase letters
// (a-n, p-z). This order fixes each character's alphabet index
// [0, kAlphabetSize) and is what every code/index conversion below uses.
// ---------------------------------------------------------------------

constexpr int kAlphabetSize = 50;
extern const std::array<char, kAlphabetSize> kAlphabet;

// Couplet codes span [0, kCoupletSpace) -- 50*50 possible couplets.
constexpr int kCoupletSpace = kAlphabetSize * kAlphabetSize;  // 2500

// The character's alphabet index, or -1 if it is not in the alphabet
// (this includes 'o' and 'O').
int alphabet_index(char c);

bool is_valid_char(char c);

// ---------------------------------------------------------------------
// Couplets: one address element is a pair of base-50 characters, packed
// into a code in [0, kCoupletSpace).
// ---------------------------------------------------------------------

bool is_valid_couplet(char first, char second);

// Encodes a two-character couplet to its code. Returns nullopt if either
// character is outside the alphabet.
std::optional<uint16_t> couplet_to_code(char first, char second);

// Decodes a couplet code back to its two characters. Returns nullopt if
// the code is outside [0, kCoupletSpace).
std::optional<std::pair<char, char>> code_to_couplet(uint16_t code);

// ---------------------------------------------------------------------
// Addresses: an ordered sequence of address elements.
//
// Representation choice: vector<AddressElement> rather than a flat
// vector<uint16_t> of couplet codes. Reason: one element -- and only the
// address's LAST element -- may be PARTIAL: a single alphabet character
// with no second character, addressing a prefix/context node (the up to
// 50 couplets it could still extend to) rather than one leaf token. A
// flat couplet-code vector (range [0, kCoupletSpace)) has no room to
// represent that case; a small element struct does, without inventing a
// second, parallel address type just for contexts.
// ---------------------------------------------------------------------

struct AddressElement {
  // Alphabet index [0, kAlphabetSize) of the element's first character.
  uint8_t first = 0;
  // Alphabet index [0, kAlphabetSize) of the second character. Meaningful
  // only when !partial.
  uint8_t second = 0;
  // True => this element specifies only `first`; the second character is
  // the wildcard marker '*'. Valid only as an address's last element.
  bool partial = false;
};

using Address = std::vector<AddressElement>;

inline bool operator==(const AddressElement &a, const AddressElement &b) {
  return a.first == b.first && a.partial == b.partial &&
         (a.partial || a.second == b.second);
}
inline bool operator!=(const AddressElement &a, const AddressElement &b) {
  return !(a == b);
}

// A full (non-partial) element built from a couplet code. Does not
// validate the code; pair with is_valid_element / is_valid_address.
AddressElement make_full_element(uint16_t couplet_code);

// A partial (wildcard-tail) element built from a single character's
// alphabet index. Does not validate the index.
AddressElement make_partial_element(uint8_t first_index);

// An element is valid if its character index (or indices) are in range.
bool is_valid_element(const AddressElement &e);

// An address is valid if every element is individually valid and at most
// the LAST element is partial.
bool is_valid_address(const Address &address);

// ---------------------------------------------------------------------
// token_id: the canonical string form of an address.
//
// OPEN: delimiter is '.' between elements (matches the worked examples,
// e.g. "AA.AA.AA.AA.AA"). A full element serializes as its two
// characters; a partial (last-element-only) element serializes as its one
// character followed by the literal wildcard marker '*' (e.g. "A*").
// Chosen as the simplest form that round-trips losslessly and reads
// directly off the worked examples in the spec -- no other delimiter or
// marker scheme is implied, so nothing more elaborate is introduced. The
// empty address encodes to the empty string, and vice versa.
// ---------------------------------------------------------------------

constexpr char kDelimiter = '.';
constexpr char kPartialMarker = '*';

// Encodes an address to its token_id. Returns nullopt if the address is
// not valid (see is_valid_address).
std::optional<std::string> encode_token_id(const Address &address);

// Decodes a token_id back to its address. Returns nullopt if the string is
// malformed: an element that isn't exactly two characters, an invalid
// (non-alphabet, non-marker) character, a partial marker on any element
// but the last, or a stray/duplicated/leading/trailing delimiter.
std::optional<Address> decode_token_id(const std::string &token_id);

// ---------------------------------------------------------------------
// Delta-by-position: prefix-relative (context) addressing.
//
// A "context" address is the shared prefix chain another address was
// resolved against. Per spec the shared prefix is assumed BY POSITION --
// this transform does not compare context's and full's element values,
// it only trusts that context.size() elements are shared and slices the
// tail. This is the simplest correct reading of "the shared prefix is
// assumed by position": a positional split, not a content-verifying
// diff. No elaborate longest-common-prefix search is introduced.
// ---------------------------------------------------------------------

// The differing tail: full's elements from index context.size() onward.
// Returns nullopt if context is longer than full (context cannot be a
// positional prefix of a shorter address).
std::optional<Address> compute_delta(const Address &context,
                                      const Address &full);

// The inverse: context followed by delta.
Address reconstruct_address(const Address &context, const Address &delta);

}  // namespace codec
