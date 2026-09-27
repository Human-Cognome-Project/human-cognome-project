#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The address / token_id codec: pure functions that convert between the
// byte-ordered Base62 symbol alphabet, address sequences, their dotted
// string rendering (token_id), plus a position-based
// delta transform used for prefix-relative (context) addressing.
//
// Nothing here talks to storage or holds state. Every function is a pure
// transform over its arguments.
namespace codec {

// ---------------------------------------------------------------------
// Alphabet: the single source of truth.
//
// The 62 symbols, in PostgreSQL's built-in COLLATE "C" byte order:
//   0-9 (0-9), A-Z (10-35), a-z (36-61).
// Each address element is a literal two-character pair stored in text[].
// The delimiter '.' and partial marker '*' are rendering/query syntax,
// not address symbols. See docs/address-encoding-transition.md.
// ---------------------------------------------------------------------

constexpr int kAlphabetSize = 62;
extern const std::array<char, kAlphabetSize> kAlphabet;

// Transient pair codes span [0, kCoupletSpace). Stored keys remain text[].
constexpr int kCoupletSpace = kAlphabetSize * kAlphabetSize;  // 3844

// The character's byte-ordered alphabet index, or -1 if it is not in the
// alphabet.
int alphabet_index(char c);

bool is_valid_char(char c);

// ---------------------------------------------------------------------
// Couplets: one address element is a pair of alphabet symbols, packed
// into a code in [0, kCoupletSpace). Code order is value order: comparing
// two codes numerically compares the pairs digit by digit. Pair codes
// support address arithmetic; they are not the PostgreSQL storage key.
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
// 64 couplets it could still extend to) rather than one leaf token. A
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
// token_id is the dotted human-facing rendering. PostgreSQL stores each
// full pair literally as one element of a text[] key with COLLATE "C".
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
