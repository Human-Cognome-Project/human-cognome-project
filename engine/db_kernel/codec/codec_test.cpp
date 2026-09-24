// Unit tests for the address / token_id codec. Standalone: standard
// library only, no test framework, no dependency outside this directory.
// A test prints one line per check and the process exits non-zero if any
// check failed.
#include <cctype>
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
  check(codec::kAlphabetSize == 50, "alphabet has exactly 50 characters");

  std::set<char> seen;
  bool all_letters = true;
  for (char c : codec::kAlphabet) {
    seen.insert(c);
    if (!std::isalpha(static_cast<unsigned char>(c))) {
      all_letters = false;
    }
  }
  check(seen.size() == 50, "alphabet characters are unique");
  check(all_letters, "alphabet characters are all letters");

  check(!codec::is_valid_char('o'), "lowercase o is excluded");
  check(!codec::is_valid_char('O'), "uppercase O is excluded");
  check(codec::alphabet_index('o') < 0, "lowercase o has no alphabet index");
  check(codec::alphabet_index('O') < 0, "uppercase O has no alphabet index");

  check(codec::is_valid_char('A'), "A is valid");
  check(codec::is_valid_char('z'), "z is valid");
  check(!codec::is_valid_char('0'), "digit 0 is invalid");
  check(!codec::is_valid_char('*'), "the wildcard marker is not itself an alphabet character");

  // A is index 0 by the documented ordering (uppercase block first).
  check(codec::alphabet_index('A') == 0, "A is alphabet index 0");
  // 'a' immediately follows the 25-character uppercase block.
  check(codec::alphabet_index('a') == 25, "a is alphabet index 25");
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
  check(all_ok, "every couplet code in [0, 2500) round-trips through its two characters");

  check(codec::code_to_couplet(2500) == std::nullopt, "code 2500 is out of range");
  check(codec::code_to_couplet(65535) == std::nullopt, "code 65535 is out of range");
}

void test_couplet_rejection() {
  check(!codec::couplet_to_code('o', 'A').has_value(), "couplet with lowercase o is rejected");
  check(!codec::couplet_to_code('A', 'O').has_value(), "couplet with uppercase O is rejected");
  check(!codec::couplet_to_code('0', 'A').has_value(), "couplet with a digit is rejected");
  check(!codec::couplet_to_code('A', '*').has_value(), "couplet with the wildcard marker is rejected");
  check(!codec::is_valid_couplet('o', 'o'), "is_valid_couplet rejects o/o");
  check(codec::is_valid_couplet('A', 'z'), "is_valid_couplet accepts A/z");
}

// ---------------------------------------------------------------------
// Address <-> token_id
// ---------------------------------------------------------------------

void test_address_token_id_round_trip() {
  // AA.AA.AA.AA.AA -- the 0x particle: five full couplets, all AA (code 0).
  {
    Address addr = {full(0), full(0), full(0), full(0), full(0)};
    const auto id = codec::encode_token_id(addr);
    check(id.has_value() && *id == "AA.AA.AA.AA.AA",
          "0x particle encodes to AA.AA.AA.AA.AA");
    const auto back = codec::decode_token_id("AA.AA.AA.AA.AA");
    check(back.has_value() && *back == addr,
          "AA.AA.AA.AA.AA decodes back to the 0x particle address");
  }

  // AA.AA.AA.AA.A* -- the 17-entry root: four full couplets plus a
  // partial trailing element (prefix/context node).
  {
    Address addr = {full(0), full(0), full(0), full(0), partial(0)};
    const auto id = codec::encode_token_id(addr);
    check(id.has_value() && *id == "AA.AA.AA.AA.A*",
          "root context encodes to AA.AA.AA.AA.A*");
    const auto back = codec::decode_token_id("AA.AA.AA.AA.A*");
    check(back.has_value() && *back == addr,
          "AA.AA.AA.AA.A* decodes back to the root context address");
  }

  // A general multi-couplet address, not tied to the base case.
  {
    Address addr = {full(37), full(1234), full(2499)};
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

  check(!codec::decode_token_id("AA.oA").has_value(),
        "decode_token_id rejects an excluded character (o)");
  check(!codec::decode_token_id("AA.AO").has_value(),
        "decode_token_id rejects an excluded character (O)");
  check(!codec::decode_token_id("A0").has_value(),
        "decode_token_id rejects a digit");
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
  AddressElement bad_first{60, 0, false};
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

}  // namespace

int main() {
  test_alphabet();
  test_couplet_round_trip();
  test_couplet_rejection();
  test_address_token_id_round_trip();
  test_invalid_input_rejection();
  test_delta_round_trip();

  if (g_failures == 0) {
    std::printf("PASS codec_test\n");
    return 0;
  }
  std::printf("FAIL codec_test (%d failed)\n", g_failures);
  return 1;
}
