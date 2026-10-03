// Check harness for the encoding populate driver (encoding_populate.*).
// Same tiny check-macro style as the controller tests.
//
// argv[1] = libpq conninfo (dbname must start with hcp_test_). REFUSES to run
// against anything but a throwaway database; run_populate_test.sh creates one
// from schema.sql (which includes the structural-mass trigger). The test
// seeds the floor it needs (256 byte codes of mass 2 with hex notation, and
// the overgroup token) through the Controller.
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "encoding_populate.h"

using namespace dbk;
using namespace dbk::bootstrap;

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) {
    ++g_failures;
  }
}

std::string text(const codec::Address &a) { return *codec::encode_token_id(a); }

codec::Address A(const std::string &token_id) { return *codec::decode_token_id(token_id); }

std::string hex(uint8_t b) {
  char buf[3];
  std::snprintf(buf, sizeof buf, "%02X", b);
  return buf;
}

void seed_floor(Controller &ctl) {
  ctl.mint(overgroup_address(), "Hex Code Patterns", {}, kLabelPlaceholderMass);
  for (int b = 0; b < 256; ++b) {
    ctl.mint(byte_code_address(static_cast<uint8_t>(b)), hex(static_cast<uint8_t>(b)), {}, 2);
  }
}

// Checks one character: parents in order, structural mass, membership, address.
void check_character(Controller &ctl, uint32_t cp, const char *name,
                     const std::vector<std::string> &bytes, int mass, Category cat,
                     const std::string &addr_text) {
  const auto id = character_address(cp);
  const std::string tag = std::string(name) + ": ";
  check(text(id) == addr_text, tag + "address " + addr_text);
  const auto parents = ctl.parents_of(id);
  bool order = parents.size() == bytes.size();
  for (size_t i = 0; order && i < parents.size(); ++i) {
    order = parents[i].ordinal == static_cast<int>(i) &&
            text(parents[i].parent) == text(byte_code_address(static_cast<uint8_t>(std::stoi(bytes[i], nullptr, 16))));
  }
  check(order, tag + "parents = byte codes " + bytes[0] + "..., in order");
  const auto attrs = ctl.attributes_of(id);
  check(attrs && attrs->mass == mass, tag + "structural mass " + std::to_string(mass));
  const auto groups = ctl.member_of(id);
  check(groups.size() == 1 && groups[0] == category_label_address(cat),
        tag + "member_of " + category_label_notation(cat));
  bool in_members = false;
  for (const auto &m : ctl.members_of(category_label_address(cat))) {
    in_members = in_members || m == id;
  }
  check(in_members, tag + "listed in the label's members");
  for (const auto &p : parents) {
    bool wired = false;
    for (const auto &c : ctl.children_of(p.parent)) {
      wired = wired || c == id;
    }
    check(wired, tag + "token_child wired from " + text(p.parent));
  }
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: %s <conninfo with dbname=hcp_test_*>\n", argv[0]);
    return 2;
  }
  const std::string conninfo = argv[1];
  if (conninfo.find("dbname=hcp_test_") == std::string::npos) {
    std::fprintf(stderr, "refusing: dbname must start with hcp_test_\n");
    return 2;
  }

  // ---- Pure functions: UTF-8, addresses, ranges. ----
  check(utf8_encode(0xE9) == std::vector<uint8_t>({0xC3, 0xA9}), "utf8 U+00E9 = C3 A9");
  check(utf8_encode(0x20AC) == std::vector<uint8_t>({0xE2, 0x82, 0xAC}), "utf8 U+20AC = E2 82 AC");
  check(utf8_encode(0x1F600) == std::vector<uint8_t>({0xF0, 0x9F, 0x98, 0x80}),
        "utf8 U+1F600 = F0 9F 98 80");
  check(utf8_encode(0xD800).empty() && utf8_encode(0xDFFF).empty(), "utf8 refuses surrogates");
  check(byte_code_address(0x00) == A("00.00.00.00.10") && byte_code_address(0xFF) == A("00.00.00.00.57"),
        "byte codes 0x00 and 0xFF map to ...10 and ...57");
  check(text(character_address(0x80)) == "00.00.00.01.00", "U+0080 first 2-byte slot");
  check(text(character_address(0x7FF)) == "00.00.00.01.Ux", "U+07FF last 2-byte slot (index 1919)");
  check(text(character_address(0x800)) == "00.00.00.03.00", "U+0800 first 3-byte slot");
  {
    // Surrogates skipped: U+E000 sits directly after U+D7FF.
    const auto a = character_address(0xD7FF), b = character_address(0xE000);
    const unsigned ia = (a[3].first * 62 + a[3].second) * 3844u + a[4].first * 62 + a[4].second;
    const unsigned ib = (b[3].first * 62 + b[3].second) * 3844u + b[4].first * 62 + b[4].second;
    check(ib == ia + 1, "U+E000 is the slot right after U+D7FF (surrogates skipped)");
  }
  check(text(character_address(0x10000)) == "00.00.00.10.00", "U+10000 first 4-byte slot");
  {
    const auto last3 = character_address(0xFFFF), last4 = character_address(0x10FFFF);
    check(last3[3].first == 0 && last3[3].second <= 61, "U+FFFF within 03.*-0z.*");
    check(codec::is_valid_address(last4), "U+10FFFF address valid and within range");
  }
  bool threw = false;
  try { character_address(0xD800); } catch (const std::invalid_argument &) { threw = true; }
  check(threw, "surrogate has no address");
  threw = false;
  try { character_address(0x7F); } catch (const std::invalid_argument &) { threw = true; }
  check(threw, "ASCII has no address (floor owns it)");

  // ---- Database: floor + trigger, via the Controller. ----
  Controller ctl(conninfo);
  seed_floor(ctl);
  verify_floor(ctl);
  check(true, "seeded floor verifies");

  // Category labels: created once, member_of the overgroup, idempotent.
  create_category_labels(ctl);
  create_category_labels(ctl);
  for (Category c : {Category::Two, Category::Three, Category::Four}) {
    const auto id = category_label_address(c);
    const auto attrs = ctl.attributes_of(id);
    check(attrs && attrs->notation == category_label_notation(c) &&
              attrs->mass == kLabelPlaceholderMass,
          std::string(category_label_notation(c)) + ": placeholder mass, notation");
    const auto groups = ctl.member_of(id);
    check(groups.size() == 1 && groups[0] == overgroup_address(),
          std::string(category_label_notation(c)) + ": member_of overgroup (once)");
    check(ctl.parents_of(id).empty(), std::string(category_label_notation(c)) + ": no parents");
  }
  check(text(category_label_address(Category::Two)) == "00.00.01.00.03" &&
            text(category_label_address(Category::Three)) == "00.00.01.00.04" &&
            text(category_label_address(Category::Four)) == "00.00.01.00.05",
        "label addresses 03, 04, 05");
  check(ctl.members_of(overgroup_address()).size() == 3, "overgroup has exactly the 3 labels");

  // Sample mode.
  for (uint32_t cp : sample_codepoints()) {
    check(populate_character(ctl, cp), "sample char freshly minted");
  }
  check_character(ctl, 0xE9, "U+00E9", {"C3", "A9"}, 4, Category::Two, text(character_address(0xE9)));
  check_character(ctl, 0x20AC, "U+20AC", {"E2", "82", "AC"}, 6, Category::Three,
                  text(character_address(0x20AC)));
  check_character(ctl, 0x1F600, "U+1F600", {"F0", "9F", "98", "80"}, 8, Category::Four,
                  text(character_address(0x1F600)));
  check(text(character_address(0xE9)).rfind("00.00.00.01.", 0) == 0 &&
            text(character_address(0x20AC)).rfind("00.00.00.0", 0) == 0 &&
            text(character_address(0x1F600)).rfind("00.00.00.", 0) == 0,
        "addresses in the reserved ranges");
  check(!populate_character(ctl, 0xE9), "re-run: existing character is a no-op");
  check(ctl.member_of(character_address(0xE9)).size() == 1, "re-run: membership not duplicated");

  // Rollback: membership forced to fail (label does not exist) -> nothing written.
  const uint32_t cp = 0xE8;
  const auto id = character_address(cp);
  threw = false;
  try {
    populate_character(ctl, cp, A("00.00.01.00.0z"));
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "forced membership failure: error propagates");
  check(!ctl.in_transaction(), "forced membership failure: scope closed");
  check(!ctl.token_exists(id), "forced membership failure: no token row");
  check(ctl.parents_of(id).empty() && ctl.member_of(id).empty(),
        "forced membership failure: no parent/membership rows");
  for (uint8_t b : {uint8_t{0xC3}, uint8_t{0xA8}}) {
    bool wired = false;
    for (const auto &c : ctl.children_of(byte_code_address(b))) {
      wired = wired || c == id;
    }
    check(!wired, "forced membership failure: no child row from " + hex(b));
  }

  std::printf("%s encoding_populate_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
