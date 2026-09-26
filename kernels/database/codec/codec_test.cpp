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
  check(codec::kAlphabetSize == 64, "alphabet has exactly 64 symbols");

  const std::string rfc4648_s5 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  bool matches_rfc = rfc4648_s5.size() == codec::kAlphabet.size();
  for (int i = 0; matches_rfc && i < codec::kAlphabetSize; ++i) {
    matches_rfc = codec::kAlphabet[i] == rfc4648_s5[std::size_t(i)] &&
                  codec::alphabet_index(rfc4648_s5[std::size_t(i)]) == i;
  }
  check(matches_rfc, "alphabet is RFC 4648 section 5 in RFC value order (index == value)");

  std::set<char> seen(codec::kAlphabet.begin(), codec::kAlphabet.end());
  check(seen.size() == 64, "alphabet symbols are unique");

  check(codec::is_valid_char('O') && codec::is_valid_char('o'), "O and o are valid symbols");
  check(codec::is_valid_char('0') && codec::is_valid_char('1'), "0 and 1 are valid symbols");
  check(codec::is_valid_char('-') && codec::is_valid_char('_'), "- and _ are valid symbols");
  check(!codec::is_valid_char('='), "= (octet-Base64 padding) is not an address symbol");
  check(!codec::is_valid_char('+') && !codec::is_valid_char('/'),
        "standard-Base64 + and / are not symbols (URL-safe alphabet only)");
  check(!codec::is_valid_char('*'), "the wildcard marker is not itself an alphabet character");
  check(!codec::is_valid_char('.'), "the delimiter is not an alphabet character");

  check(codec::alphabet_index('A') == 0, "A is value 0");
  check(codec::alphabet_index('a') == 26, "a is value 26");
  check(codec::alphabet_index('0') == 52, "0 is value 52");
  check(codec::alphabet_index('_') == 63, "_ is value 63");
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
  check(all_ok, "every couplet code in [0, 4096) round-trips through its two characters");

  check(codec::code_to_couplet(4096) == std::nullopt, "code 4096 is out of range");
  check(codec::code_to_couplet(65535) == std::nullopt, "code 65535 is out of range");
}

void test_couplet_rejection() {
  check(!codec::couplet_to_code('=', 'A').has_value(), "couplet with padding = is rejected");
  check(!codec::couplet_to_code('A', '+').has_value(), "couplet with standard-Base64 + is rejected");
  check(!codec::couplet_to_code('A', '*').has_value(), "couplet with the wildcard marker is rejected");
  check(!codec::is_valid_couplet('.', 'A'), "is_valid_couplet rejects the delimiter");
  check(codec::is_valid_couplet('O', 'o'), "is_valid_couplet accepts O/o");
  check(codec::is_valid_couplet('-', '_'), "is_valid_couplet accepts -/_");
  check(codec::couplet_to_code('A', 'A') == 0, "AA is code 0");
  check(codec::couplet_to_code('_', '_') == 4095, "__ is code 4095");
  check(codec::couplet_to_code('B', 'A') == 64, "BA is code 64 (first * 64 + second)");
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
    Address addr = {full(37), full(1234), full(4095)};
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
        "decode_token_id rejects padding (=)");
  check(!codec::decode_token_id("AA.A/").has_value(),
        "decode_token_id rejects standard-Base64 /");
  check(codec::decode_token_id("Oo.01.-_").has_value(),
        "decode_token_id accepts O, o, digits, - and _");
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
  AddressElement bad_first{64, 0, false};
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
// Storage key (pair codes) and ordering
// ---------------------------------------------------------------------

void test_pair_key() {
  const Address addr = {full(0), full(64), full(4095)};
  const auto key = codec::to_pair_key(addr);
  check(key.has_value() && *key == codec::PairKey({0, 64, 4095}),
        "to_pair_key gives one pair code per element, in order");
  const auto back = codec::from_pair_key(*key);
  check(back.has_value() && *back == addr, "from_pair_key recovers the address");

  check(!codec::to_pair_key({full(0), partial(3)}).has_value(),
        "a partial address has no single storage key");
  check(!codec::from_pair_key({0, 4096}).has_value(),
        "a code outside [0, 4096) is not a storage key");
  check(codec::to_pair_key(Address{}).has_value() && codec::to_pair_key(Address{})->empty(),
        "the empty address has the empty key");

  const auto range = codec::partial_code_range(partial(1));
  check(range.has_value() && range->first == 64 && range->second == 127,
        "a partial element is one contiguous code range [first*64, first*64+63]");
  check(!codec::partial_code_range(full(5)).has_value(),
        "a full element has no partial range");
}

void test_key_order_is_value_order() {
  // Every pair of symbols, ordered by value, must also be ordered by code.
  bool ordered = true;
  for (int code = 1; ordered && code < codec::kCoupletSpace; ++code) {
    const auto prev = codec::code_to_couplet(static_cast<uint16_t>(code - 1));
    const auto cur = codec::code_to_couplet(static_cast<uint16_t>(code));
    const int pv = codec::alphabet_index(prev->first) * 64 + codec::alphabet_index(prev->second);
    const int cv = codec::alphabet_index(cur->first) * 64 + codec::alphabet_index(cur->second);
    ordered = pv < cv;
  }
  check(ordered, "pair-code order is value order across all 4096 pairs");

  // The trap the pair key avoids: byte order of the token_id text is not
  // value order. 'Z' (value 25) < 'a' (26) < '0' (52) by value, but in
  // bytes '0' (0x30) < 'Z' (0x5A) < 'a' (0x61).
  const std::string by_value_low = *codec::encode_token_id({full(*codec::couplet_to_code('Z', 'Z'))});
  const std::string by_value_high = *codec::encode_token_id({full(*codec::couplet_to_code('0', '0'))});
  check(*codec::to_pair_key(*codec::decode_token_id(by_value_low)) <
            *codec::to_pair_key(*codec::decode_token_id(by_value_high)),
        "ZZ sorts before 00 by pair key (value order)");
  check(by_value_high < by_value_low,
        "...while the token_id text sorts 00 before ZZ (byte order): text is not the order authority");
}

}  // namespace

int main() {
  test_alphabet();
  test_couplet_round_trip();
  test_couplet_rejection();
  test_address_token_id_round_trip();
  test_invalid_input_rejection();
  test_delta_round_trip();
  test_pair_key();
  test_key_order_is_value_order();

  if (g_failures == 0) {
    std::printf("PASS codec_test\n");
    return 0;
  }
  std::printf("FAIL codec_test (%d failed)\n", g_failures);
  return 1;
}
