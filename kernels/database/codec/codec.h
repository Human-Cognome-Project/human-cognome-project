#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The address / token_id codec: pure functions that convert between the
// RFC 4648 section 5 (URL- and filename-safe Base64) symbol alphabet,
// address sequences, their canonical string (token_id) form, and their
// storage key (one numeric pair code per element), plus a position-based
// delta transform used for prefix-relative (context) addressing.
//
// Nothing here talks to storage or holds state. Every function is a pure
// transform over its arguments.
namespace codec {

// ---------------------------------------------------------------------
// Alphabet: the single source of truth.
//
// The 64 symbols of RFC 4648 section 5, in RFC value order:
//   A-Z (0-25), a-z (26-51), 0-9 (52-61), '-' (62), '_' (63).
// A symbol's alphabet index IS its RFC value, and every code/index
// conversion below uses that order. '=' (octet-Base64 padding), the
// delimiter '.' and the partial marker '*' are not address symbols.
//
// Addresses are sequences of independent radix-64 digits grouped into
// pairs. They use the RFC alphabet and value mapping, not the RFC's
// octet-to-symbol encoding: an address is not the output of a generic
// Base64url byte encoder (see docs/address-encoding-transition.md).
// ---------------------------------------------------------------------

constexpr int kAlphabetSize = 64;
extern const std::array<char, kAlphabetSize> kAlphabet;

// Couplet (pair) codes span [0, kCoupletSpace) -- 64*64 possible pairs,
// i.e. twelve bits: first_value * 64 + second_value.
constexpr int kCoupletSpace = kAlphabetSize * kAlphabetSize;  // 4096

// The character's alphabet index (RFC value), or -1 if it is not in the
// alphabet.
int alphabet_index(char c);

bool is_valid_char(char c);

// ---------------------------------------------------------------------
// Couplets: one address element is a pair of alphabet symbols, packed
// into a code in [0, kCoupletSpace). Code order is value order: comparing
// two codes numerically compares the pairs digit by digit.
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
// token_id is the human-facing and interchange rendering. Storage keys
// use the pair-code form below, because byte order of token_id strings
// is NOT value order once digits, '-' and '_' are in the alphabet.
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
// Storage key: one pair code per element, in address order.
//
// The record tier stores token_id as this array (PostgreSQL smallint[];
// codes fit in [0, 4096)). Lexicographic comparison of two keys is value
// order of the addresses, so an ordered index over the key follows the
// codec's digit order exactly, whatever symbols are involved. token_id
// strings are rendered from the key at the edges; they are never the
// ordering authority.
// ---------------------------------------------------------------------

using PairKey = std::vector<uint16_t>;

// The storage key of a full address. Returns nullopt if the address is
// invalid or contains a partial element (a partial names a range of keys,
// not one key; see partial_code_range).
std::optional<PairKey> to_pair_key(const Address &address);

// The address a storage key names. Returns nullopt if any code is outside
// [0, kCoupletSpace).
std::optional<Address> from_pair_key(const PairKey &key);

// The inclusive range of pair codes a partial element stands for: every
// second symbol after its first, i.e. [first*64, first*64 + 63]. One
// contiguous interval in key order. Returns nullopt if `e` is not a valid
// partial element.
std::optional<std::pair<uint16_t, uint16_t>> partial_code_range(
    const AddressElement &e);

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
