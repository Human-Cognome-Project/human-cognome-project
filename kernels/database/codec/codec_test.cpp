// Unit tests for the address / token_id codec. Standalone: standard
// library only, no test framework, no dependency outside this directory.
// A test prints one line per check and the process exits non-zero if any
// check failed.
#include <cstdio>
#include <set>
#include <string>

#include "codec.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s\n", what.c_str());
    ++g_failures;
  }
}

using codec::Address;
using codec::AddressElement;

AddressElement full(uint16_t code) {
  return codec::make_full_element(code);
}

AddressElement partial(uint8_t first_index) {
  return codec::make_partial_element(first_index);
}

// ---------------------------------------------------------------------
// Alphabet
// ---------------------------------------------------------------------

void test_alphabet() {
  check(codec::kAlphabetSize == 62, "alphabet has exactly 62 symbols");

  const std::string byte_order =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  bool matches_bytes = byte_order.size() == codec::kAlphabet.size();
  for (int i = 0; matches_bytes && i < codec::kAlphabetSize; ++i) {
    matches_bytes = codec::kAlphabet[i] == byte_order[std::size_t(i)] &&
                    codec::alphabet_index(byte_order[std::size_t(i)]) == i;
  }
  check(matches_bytes, "Base62 alphabet/index is 0-9 A-Z a-z in byte order");

  std::set<char> seen(codec::kAlphabet.begin(), codec::kAlphabet.end());
  check(seen.size() == 62, "alphabet symbols are unique");

  check(codec::is_valid_char('O') && codec::is_valid_char('o'), "O and o are valid symbols");
  check(codec::is_valid_char('0') && codec::is_valid_char('1'), "0 and 1 are valid symbols");
  check(!codec::is_valid_char('-') && !codec::is_valid_char('_'),
        "- and _ are not Base62 address symbols");
  check(!codec::is_valid_char('=') && !codec::is_valid_char('+') &&
            !codec::is_valid_char('/'), "punctuation outside Base62 is rejected");
  check(!codec::is_valid_char('*'), "the wildcard marker is not itself an alphabet character");
  check(!codec::is_valid_char('.'), "the delimiter is not an alphabet character");

  check(codec::alphabet_index('0') == 0, "0 is index 0");
  check(codec::alphabet_index('A') == 10, "A is index 10");
  check(codec::alphabet_index('a') == 36, "a is index 36");
  check(codec::alphabet_index('z') == 61, "z is index 61");
}

// ---------------------------------------------------------------------
// Couplet <-> code
// ---------------------------------------------------------------------

void test_couplet_round_trip() {
  bool all_ok = true;
  for (int code = 0; code < codec::kCoupletSpace; ++code) {
    const auto chars = codec::code_to_couplet(static_cast<uint16_t>(code));
    if (!chars.has_value()) {
      all_ok = false;
      break;
    }
    const auto back = codec::couplet_to_code(chars->first, chars->second);
    if (!back.has_value() || *back != code) {
      all_ok = false;
      break;
    }
  }
  check(all_ok, "every couplet code in [0, 3844) round-trips through its two characters");

  check(codec::code_to_couplet(3844) == std::nullopt, "code 3844 is out of range");
  check(codec::code_to_couplet(65535) == std::nullopt, "code 65535 is out of range");
}

void test_couplet_rejection() {
  check(!codec::couplet_to_code('=', 'A').has_value(), "couplet with = is rejected");
  check(!codec::couplet_to_code('A', '+').has_value(), "couplet with + is rejected");
  check(!codec::couplet_to_code('A', '*').has_value(), "couplet with the wildcard marker is rejected");
  check(!codec::is_valid_couplet('.', 'A'), "is_valid_couplet rejects the delimiter");
  check(codec::is_valid_couplet('O', 'o'), "is_valid_couplet accepts O/o");
  check(!codec::is_valid_couplet('-', '_'), "is_valid_couplet rejects -/_");
  check(codec::couplet_to_code('0', '0') == 0, "00 is transient code 0");
  check(codec::couplet_to_code('z', 'z') == 3843, "zz is transient code 3843");
  check(codec::couplet_to_code('B', 'A') == 692, "BA is 11*62 + 10");
}

// ---------------------------------------------------------------------
// Address <-> token_id
// ---------------------------------------------------------------------

void test_address_token_id_round_trip() {
  // The minimal five-pair address is five 00 couplets.
  {
    Address addr = {full(0), full(0), full(0), full(0), full(0)};
    const auto id = codec::encode_token_id(addr);
    check(id.has_value() && *id == "00.00.00.00.00",
          "the minimal five-pair address encodes to 00.00.00.00.00");
    const auto back = codec::decode_token_id("00.00.00.00.00");
    check(back.has_value() && *back == addr,
          "00.00.00.00.00 decodes back to the minimal address");
  }

  // Four full couplets plus a partial trailing element (query prefix).
  {
    Address addr = {full(0), full(0), full(0), full(0), partial(0)};
    const auto id = codec::encode_token_id(addr);
    check(id.has_value() && *id == "00.00.00.00.0*",
          "root context encodes to 00.00.00.00.0*");
    const auto back = codec::decode_token_id("00.00.00.00.0*");
    check(back.has_value() && *back == addr,
          "00.00.00.00.0* decodes back to the root context address");
  }

  // A general multi-couplet address, not tied to the base case.
  {
    Address addr = {full(37), full(1234), full(3843)};
    const auto id = codec::encode_token_id(addr);
    check(id.has_value(), "a general full-couplet address encodes");
    const auto back = codec::decode_token_id(*id);
    check(back.has_value() && *back == addr,
          "a general full-couplet address round-trips through its token_id");
  }

  // Empty address <-> empty string.
  {
    const auto id = codec::encode_token_id(Address{});
    check(id.has_value() && id->empty(), "the empty address encodes to the empty string");
    const auto back = codec::decode_token_id("");
    check(back.has_value() && back->empty(), "the empty string decodes to the empty address");
  }
}

void test_invalid_input_rejection() {
  // A partial element before the last position is invalid, both when
  // building an Address directly and when parsing a token_id.
  {
    Address addr = {partial(0), full(0)};
    check(!codec::is_valid_address(addr), "a partial element before the last position is invalid");
    check(!codec::encode_token_id(addr).has_value(),
          "encode_token_id rejects a non-trailing partial element");
  }
  check(!codec::decode_token_id("A*.AA").has_value(),
        "decode_token_id rejects a wildcard marker before the last element");

  check(!codec::decode_token_id("AA.=A").has_value(),
        "decode_token_id rejects =");
  check(!codec::decode_token_id("AA.A/").has_value(),
        "decode_token_id rejects standard-Base64 /");
  check(codec::decode_token_id("Oo.01.9z").has_value(),
        "decode_token_id accepts O, o, digits and lowercase letters");
  check(!codec::decode_token_id("Oo.01.-_").has_value(),
        "decode_token_id rejects - and _");
  check(!codec::decode_token_id("A").has_value(),
        "decode_token_id rejects a one-character element");
  check(!codec::decode_token_id("AAA").has_value(),
        "decode_token_id rejects a three-character element");
  check(!codec::decode_token_id(".AA").has_value(),
        "decode_token_id rejects a leading delimiter");
  check(!codec::decode_token_id("AA.").has_value(),
        "decode_token_id rejects a trailing delimiter");
  check(!codec::decode_token_id("AA..AA").has_value(),
        "decode_token_id rejects a doubled delimiter");

  Address bad_partial_second = {AddressElement{0, 99, true}};
  // second is unused when partial, so validity turns on `first` alone.
  check(codec::is_valid_element(bad_partial_second[0]),
        "a partial element's unused second field does not affect validity");
  AddressElement bad_first{62, 0, false};
  check(!codec::is_valid_element(bad_first), "an out-of-range first index is invalid");
}

// ---------------------------------------------------------------------
// Delta-by-position
// ---------------------------------------------------------------------

void test_delta_round_trip() {
  const Address context = {full(0), full(0), full(0), full(0)};
  const Address full_addr = {full(0), full(0), full(0), full(0), full(7)};

  const auto delta = codec::compute_delta(context, full_addr);
  check(delta.has_value() && delta->size() == 1 && (*delta)[0] == full(7),
        "compute_delta yields the differing tail for the base-case root/leaf pair");

  const Address rebuilt = codec::reconstruct_address(context, *delta);
  check(rebuilt == full_addr, "reconstruct_address(context, delta) recovers the full address");

  // Delta of an address against itself is empty.
  const auto self_delta = codec::compute_delta(full_addr, full_addr);
  check(self_delta.has_value() && self_delta->empty(),
        "an address's delta against itself is empty");
  check(codec::reconstruct_address(full_addr, Address{}) == full_addr,
        "reconstructing with an empty delta returns the context unchanged");

  // Delta against the empty context is the whole address.
  const auto whole_delta = codec::compute_delta(Address{}, full_addr);
  check(whole_delta.has_value() && *whole_delta == full_addr,
        "delta against an empty context is the whole address");

  // A context longer than the address it's compared to has no delta.
  check(codec::compute_delta(full_addr, context) == std::nullopt,
        "a context longer than the address has no delta");

  // Positional, not content-verifying: the spec assumes the shared prefix
  // by position, so a context that disagrees with full's actual prefix
  // still yields a delta -- the tail past context's length.
  const Address mismatched_context = {full(9), full(9), full(9), full(9)};
  const auto positional_delta = codec::compute_delta(mismatched_context, full_addr);
  check(positional_delta.has_value() && positional_delta->size() == 1 &&
            (*positional_delta)[0] == full(7),
        "delta is taken by position, not verified against context content");
}

// ---------------------------------------------------------------------
// Literal pair ordering matches the PostgreSQL C-collated text[] key
// ---------------------------------------------------------------------

void test_pair_order_matches_bytes() {
  // In byte order, every successive pair is greater than its predecessor;
  // one pair per text[] element preserves this order in PostgreSQL's C collation.
  bool ordered = true;
  for (int code = 1; ordered && code < codec::kCoupletSpace; ++code) {
    const auto prev = codec::code_to_couplet(static_cast<uint16_t>(code - 1));
    const auto cur = codec::code_to_couplet(static_cast<uint16_t>(code));
    const std::string previous{prev->first, prev->second};
    const std::string current{cur->first, cur->second};
    ordered = previous < current;
  }
  check(ordered, "all 3844 literal pairs sort in codec order by byte value");
  check(std::string("09") < "0A" && std::string("0Z") < "0a" &&
            std::string("0z") < "10",
        "digit, uppercase and lowercase boundaries sort directly");
}

}  // namespace

int main() {
  test_alphabet();
  test_couplet_round_trip();
  test_couplet_rejection();
  test_address_token_id_round_trip();
  test_invalid_input_rejection();
  test_delta_round_trip();
  test_pair_order_matches_bytes();

  if (g_failures == 0) {
    std::printf("PASS codec_test\n");
    return 0;
  }
  std::printf("FAIL codec_test (%d failed)\n", g_failures);
  return 1;
}
