// Adversarial tests for the address / token_id codec. Standalone: standard
// library only. Does not modify codec.h/codec.cpp -- only exercises the
// public interface with edge/boundary/malformed input.
#include <cctype>
#include <cstdio>
#include <limits>
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

AddressElement full(uint16_t code) { return codec::make_full_element(code); }
AddressElement partial(uint8_t first_index) {
  return codec::make_partial_element(first_index);
}

// ---------------------------------------------------------------------
// Alphabet: exact membership, not just size.
// ---------------------------------------------------------------------

void test_alphabet_exact_membership() {
  // Build the expected Base62 set directly and compare against kAlphabet / is_valid_char for EVERY
  // byte value, not just a handful of samples.
  std::set<char> expected;
  for (char c = 'A'; c <= 'Z'; ++c) expected.insert(c);
  for (char c = 'a'; c <= 'z'; ++c) expected.insert(c);
  for (char c = '0'; c <= '9'; ++c) expected.insert(c);
  check(expected.size() == 62, "harness sanity: expected set has 62 chars");

  std::set<char> actual(codec::kAlphabet.begin(), codec::kAlphabet.end());
  check(actual == expected, "kAlphabet is EXACTLY 0-9,A-Z,a-z (no more, no less)");

  bool all_agree = true;
  int first_mismatch = -1;
  for (int b = 0; b < 256; ++b) {
    const char c = static_cast<char>(b);
    const bool should_be_valid = expected.count(c) > 0;
    const bool is_valid = codec::is_valid_char(c);
    if (should_be_valid != is_valid) {
      all_agree = false;
      if (first_mismatch < 0) first_mismatch = b;
    }
  }
  check(all_agree, "is_valid_char agrees with the exact alphabet for every byte 0..255" +
                        (first_mismatch >= 0 ? (" (first mismatch at byte " +
                                                 std::to_string(first_mismatch) + ")")
                                              : ""));

  // Symbols between the Base62 byte runs are not address symbols.
  check(!codec::is_valid_char('-'), "- is not a symbol");
  check(!codec::is_valid_char('_'), "_ is not a symbol");
  check(!codec::is_valid_char('+') && !codec::is_valid_char('/'), "+ and / are not symbols");
  check(!codec::is_valid_char('='), "= is not a symbol");
  check(!codec::is_valid_char('`'), "` is not a symbol");

  // Alphabet indices are a bijection onto [0, 62) in byte order.
  bool in_byte_order = true;
  for (int i = 0; i < codec::kAlphabetSize; ++i) {
    in_byte_order = in_byte_order && codec::alphabet_index(codec::kAlphabet[i]) == i &&
                    (i == 0 || codec::kAlphabet[i - 1] < codec::kAlphabet[i]);
  }
  check(in_byte_order, "alphabet_index and byte order agree for every symbol");
}

// ---------------------------------------------------------------------
// Couplet code: full range, both directions, boundary codes.
// ---------------------------------------------------------------------

void test_couplet_full_range_both_directions() {
  // char-pair -> code -> char-pair, for EVERY pair in the 62x62 grid,
  // checking the exact expected code (first*62+second), not just that
  // round-trip succeeds.
  bool all_ok = true;
  for (int a = 0; a < codec::kAlphabetSize && all_ok; ++a) {
    for (int b = 0; b < codec::kAlphabetSize && all_ok; ++b) {
      const char ca = codec::kAlphabet[a];
      const char cb = codec::kAlphabet[b];
      const auto code = codec::couplet_to_code(ca, cb);
      if (!code.has_value() || *code != static_cast<uint16_t>(a * codec::kAlphabetSize + b)) {
        all_ok = false;
      }
    }
  }
  check(all_ok, "couplet_to_code == first_index*62+second_index for all 3844 pairs");

  check(codec::couplet_to_code('0', '0') == 0, "00 is code 0 (minimum)");
  check(codec::couplet_to_code('z', 'z') == 3843, "zz is code 3843 (maximum)");
  check(codec::code_to_couplet(0) == std::make_pair('0', '0'), "code 0 decodes to 00");
  check(codec::code_to_couplet(3843) == std::make_pair('z', 'z'), "code 3843 decodes to zz");
  check(!codec::code_to_couplet(3844).has_value(), "code 3844 (one past max) is rejected");
  check(!codec::code_to_couplet(std::numeric_limits<uint16_t>::max()).has_value(),
        "uint16_t max is rejected");
}

// ---------------------------------------------------------------------
// token_id round trip over the FULL couplet code range, via the
// address/string path (not just the raw couplet functions).
// ---------------------------------------------------------------------

void test_token_id_full_range_round_trip() {
  bool all_ok = true;
  int bad_code = -1;
  for (int code = 0; code < codec::kCoupletSpace; ++code) {
    Address addr = {full(static_cast<uint16_t>(code))};
    const auto id = codec::encode_token_id(addr);
    if (!id.has_value() || id->size() != 2) {
      all_ok = false;
      bad_code = code;
      break;
    }
    const auto back = codec::decode_token_id(*id);
    if (!back.has_value() || back->size() != 1 || (*back)[0] != addr[0]) {
      all_ok = false;
      bad_code = code;
      break;
    }
  }
  check(all_ok, "every couplet code 0..3843 round-trips through a single-element token_id" +
                    (bad_code >= 0 ? (" (first failure at code " + std::to_string(bad_code) + ")")
                                   : ""));
}

// ---------------------------------------------------------------------
// Malformed token_id strings: things beyond the base test's coverage.
// ---------------------------------------------------------------------

void test_malformed_token_id_rejection() {
  check(!codec::decode_token_id("*A").has_value(),
        "decode_token_id rejects wildcard-marker-as-first-char element");
  check(!codec::decode_token_id("**").has_value(),
        "decode_token_id rejects a double wildcard-marker element");
  check(!codec::decode_token_id("A*.B*").has_value(),
        "decode_token_id rejects two partial elements (only the last may be partial)");
  check(!codec::decode_token_id("AA.A*.AA").has_value(),
        "decode_token_id rejects a partial element in the middle");

  // Non-alphabet bytes beyond the letters: whitespace, control, high-bit.
  check(!codec::decode_token_id("A A").has_value(),
        "decode_token_id rejects an embedded space");
  {
    std::string with_nul = "A";
    with_nul.push_back('\0');
    check(!codec::decode_token_id(with_nul).has_value(),
          "decode_token_id rejects an embedded NUL byte");
  }
  {
    std::string high_bit = "A";
    high_bit.push_back(static_cast<char>(0x80));
    check(!codec::decode_token_id(high_bit).has_value(),
          "decode_token_id rejects a high-bit byte (0x80)");
  }
  check(codec::decode_token_id("Az").has_value(),
        "sanity: a plain valid couplet like Az is accepted (control case)");

  // Delimiter-only and marker-only strings.
  check(!codec::decode_token_id(".").has_value(), "decode_token_id rejects a lone delimiter");
  check(!codec::decode_token_id("*").has_value(),
        "decode_token_id rejects a lone wildcard marker (wrong length, no first char)");

  // Multiple trailing/leading delimiters and an all-delimiter string.
  check(!codec::decode_token_id("...").has_value(),
        "decode_token_id rejects an all-delimiter string");
  check(!codec::decode_token_id("AA.AA.").has_value(),
        "decode_token_id rejects trailing delimiter after otherwise-valid content");

  // Case sensitivity: uppercase and lowercase are distinct alphabet
  // entries, not folded together.
  check(codec::alphabet_index('A') != codec::alphabet_index('a'),
        "uppercase and lowercase letters map to distinct alphabet indices");
  check(codec::couplet_to_code('A', 'a') != codec::couplet_to_code('a', 'A'),
        "couplet order matters: Aa != aA");

  // A valid-looking element using a delimiter as one of its two
  // characters (can't happen post-split, but guard the couplet-level
  // primitive directly).
  check(!codec::couplet_to_code('.', 'A').has_value(),
        "couplet_to_code rejects the delimiter character itself");
}

// ---------------------------------------------------------------------
// AddressElement / is_valid_address edge cases beyond the base test.
// ---------------------------------------------------------------------

void test_address_validity_edges() {
  // Multiple partial elements anywhere, even if the last one is also
  // partial -- only ONE partial element total is allowed, and only in
  // last position.
  Address two_partials = {partial(0), partial(1)};
  check(!codec::is_valid_address(two_partials),
        "an address with two partial elements (first non-last) is invalid");

  // A single-element address that is partial: valid (it's trivially last).
  Address single_partial = {partial(5)};
  check(codec::is_valid_address(single_partial), "a lone partial element is a valid address");

  // first index exactly at the boundary: 61 valid, 62 invalid.
  check(codec::is_valid_element(AddressElement{61, 0, false}),
        "first index 61 (max valid) is valid");
  check(!codec::is_valid_element(AddressElement{62, 0, false}),
        "first index 62 (one past max) is invalid");
  check(!codec::is_valid_element(AddressElement{255, 0, false}),
        "first index 255 (uint8_t max) is invalid");
  check(codec::is_valid_element(AddressElement{0, 61, false}),
        "second index 61 (max valid) is valid");
  check(!codec::is_valid_element(AddressElement{0, 62, false}),
        "second index 62 (one past max) is invalid");
  check(!codec::is_valid_element(AddressElement{0, 255, false}),
        "second index 255 (uint8_t max) is invalid");

  // encode_token_id must refuse an address containing an out-of-range
  // element rather than silently emitting a garbage character.
  Address bad = {AddressElement{62, 0, false}};
  check(!codec::encode_token_id(bad).has_value(),
        "encode_token_id refuses an address with an out-of-range element");
}

// ---------------------------------------------------------------------
// Delta-by-position: additional adversarial cases.
// ---------------------------------------------------------------------

void test_delta_adversarial() {
  const Address root4 = {full(0), full(0), full(0), full(0)};       // 00.00.00.00
  const Address leaf5 = {full(0), full(0), full(0), full(0), full(7)};

  // Context longer than full by exactly one -- boundary of the rejection.
  check(codec::compute_delta(leaf5, root4) == std::nullopt,
        "context one element longer than full is rejected (boundary)");

  // Context == full length, identical content -> empty delta (already in
  // base test); context == full length, DIFFERENT content -> still empty
  // delta, since the transform is positional only (never compares
  // values even when lengths match).
  const Address root4_other = {full(9), full(9), full(9), full(9)};
  const auto d = codec::compute_delta(root4_other, root4);
  check(d.has_value() && d->empty(),
        "equal-length positional delta is empty even when content differs");

  // Delta/reconstruct with partial elements in the tail.
  const Address ctx = {full(0), full(0)};
  const Address with_partial_tail = {full(0), full(0), full(3), partial(9)};
  const auto delta_partial = codec::compute_delta(ctx, with_partial_tail);
  check(delta_partial.has_value() && delta_partial->size() == 2 &&
            (*delta_partial)[1] == partial(9),
        "compute_delta correctly carries a trailing partial element into the delta");
  check(codec::reconstruct_address(ctx, *delta_partial) == with_partial_tail,
        "reconstruct_address restores a partial-tailed address from its delta");

  // Context itself contains a partial element (context is a context/root
  // node with a wildcard tail) -- still purely positional slicing, no
  // validation of the context's own shape is performed.
  const Address partial_ctx = {full(0), partial(1)};
  const Address longer = {full(0), partial(1), full(2)};
  const auto delta_from_partial_ctx = codec::compute_delta(partial_ctx, longer);
  check(delta_from_partial_ctx.has_value() && delta_from_partial_ctx->size() == 1 &&
            (*delta_from_partial_ctx)[0] == full(2),
        "compute_delta slices correctly when context's own last element is partial");

  // reconstruct_address does not mutate its inputs (context passed by
  // const ref, delta copied in) -- verify context is unchanged after use.
  Address ctx_copy = ctx;
  codec::reconstruct_address(ctx_copy, *delta_partial);
  check(ctx_copy == ctx, "reconstruct_address does not mutate its context argument");

  // Chained delta: delta of a delta's reconstruction against a longer
  // context recovers the correct further tail.
  const Address a = {full(0)};
  const Address ab = {full(0), full(1)};
  const Address abc = {full(0), full(1), full(2)};
  const auto delta_a_ab = codec::compute_delta(a, ab);
  const auto rebuilt_ab = codec::reconstruct_address(a, *delta_a_ab);
  check(rebuilt_ab == ab, "chained delta step 1: a -> ab round-trips");
  const auto delta_ab_abc = codec::compute_delta(rebuilt_ab, abc);
  const auto rebuilt_abc = codec::reconstruct_address(rebuilt_ab, *delta_ab_abc);
  check(rebuilt_abc == abc, "chained delta step 2: ab -> abc round-trips");
}

}  // namespace

int main() {
  test_alphabet_exact_membership();
  test_couplet_full_range_both_directions();
  test_token_id_full_range_round_trip();
  test_malformed_token_id_rejection();
  test_address_validity_edges();
  test_delta_adversarial();

  if (g_failures == 0) {
    std::printf("PASS codec_advtest\n");
    return 0;
  }
  std::printf("FAIL codec_advtest (%d failed)\n", g_failures);
  return 1;
}
