// Check harness for the encoding populate driver (encoding_populate.*).
// Same tiny check-macro style as the controller tests.
//
// argv[1] = libpq conninfo (dbname must start with hcp_test_). REFUSES to run
// against anything but a throwaway database; run_populate_test.sh creates one
// from schema.sql (which includes the structural-mass trigger). The test
// seeds the floor it needs (256 byte codes of mass 2 with hex notation, and
// the overgroup token) through the Controller.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

// ---- Raw-SQL helpers (throwaway DB only: invalid-fixture construction and
// row counting for the endpoint tests). ----

void exec(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  const bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
  if (!ok) {
    std::fprintf(stderr, "test bug: %s: %s\n", sql.c_str(), PQerrorMessage(conn));
  }
  PQclear(res);
  if (!ok) {
    std::exit(2);
  }
}

std::string scalar(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  std::string v;
  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1 && !PQgetisnull(res, 0, 0)) {
    v = PQgetvalue(res, 0, 0);
  } else {
    std::fprintf(stderr, "test bug: %s: %s\n", sql.c_str(), PQerrorMessage(conn));
    std::exit(2);
  }
  PQclear(res);
  return v;
}

// Row counts of the five tables, as one string.
std::string counts(PGconn *conn) {
  return scalar(conn,
                "SELECT (SELECT count(*) FROM token) || '/' || (SELECT count(*) FROM token_parent) "
                "|| '/' || (SELECT count(*) FROM token_child) || '/' || (SELECT count(*) FROM members) "
                "|| '/' || (SELECT count(*) FROM member_of)");
}

// Fingerprint of every row of the five tables that concerns one endpoint.
std::string fingerprint(PGconn *conn, const std::string &id) {
  const std::string a = "'{" + [&] {
    std::string s = id;
    for (char &c : s) {
      if (c == '.') c = ',';
    }
    return s;
  }() + "}'::text[]";
  return scalar(
      conn,
      "SELECT md5(concat_ws('#', "
      "(SELECT string_agg(concat_ws('|', token_id::text, token_text, notation, mass), ';') FROM token WHERE token_id = " + a + "), "
      "(SELECT string_agg(concat_ws('|', token_id::text, ordinal, parent_token_id::text, mass), ';' ORDER BY ordinal) FROM token_parent WHERE token_id = " + a + "), "
      "(SELECT string_agg(concat_ws('|', token_id::text, child_token_id::text), ';') FROM token_child WHERE child_token_id = " + a + "), "
      "(SELECT string_agg(concat_ws('|', token_id::text, member_token_id::text), ';') FROM members WHERE member_token_id = " + a + "), "
      "(SELECT string_agg(concat_ws('|', token_id::text, group_token_id::text), ';') FROM member_of WHERE token_id = " + a + ")))");
}

// SQL text[] literal of an address, e.g. '{00,00,02,00,3l}'.
std::string sqlarr(const codec::Address &a) {
  std::string s = text(a);
  for (char &c : s) {
    if (c == '.') c = ',';
  }
  return "'{" + s + "}'";
}

// Checks one endpoint: address, character, one parent (the combination), mass,
// reciprocal child, one membership (UTF-8) in both directions.
void check_endpoint(Controller &ctl, uint32_t cp, const char *name, int mass,
                    const std::string &addr_text) {
  const auto id = endpoint_address(cp);
  const auto combo = character_address(cp);
  const std::string tag = std::string(name) + " endpoint: ";
  check(text(id) == addr_text, tag + "address " + addr_text);
  const auto attrs = ctl.attributes_of(id);
  const auto bytes = utf8_encode(cp);
  check(attrs && attrs->notation == std::string(bytes.begin(), bytes.end()),
        tag + "notation is the character");
  check(attrs && attrs->mass == mass, tag + "structural mass " + std::to_string(mass));
  const auto parents = ctl.parents_of(id);
  check(parents.size() == 1 && parents[0].ordinal == 0 && parents[0].parent == combo,
        tag + "one parent, ordinal 0 = combination " + text(combo));
  bool wired = false;
  for (const auto &c : ctl.children_of(combo)) {
    wired = wired || c == id;
  }
  check(wired, tag + "token_child wired from the combination");
  const auto groups = ctl.member_of(id);
  check(groups.size() == 1 && groups[0] == utf8_label_address(), tag + "member_of UTF-8 only");
  const auto m = ctl.membership_present(id, utf8_label_address());
  check(m.in_member_of && m.in_members, tag + "membership in both directions");
  const auto cg = ctl.member_of(combo);
  check(cg.size() == 1 && cg[0] == category_label_address(category_of(cp)),
        tag + "combination still member_of its byte-width label only");
}

// Builds an endpoint, applies one raw-SQL corruption, and requires
// populate_endpoint to throw and leave counts and the fingerprint unchanged.
void check_rejected(Controller &ctl, PGconn *raw, uint32_t cp, const std::string &what,
                    const std::string &corrupt_sql) {
  populate_character(ctl, cp);
  populate_endpoint(ctl, cp);
  exec(raw, corrupt_sql);
  const std::string id = text(endpoint_address(cp));
  const std::string n0 = counts(raw), f0 = fingerprint(raw, id);
  bool threw = false;
  try {
    populate_endpoint(ctl, cp);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "invalid whole (" + what + "): throws");
  check(!ctl.in_transaction(), "invalid whole (" + what + "): scope closed");
  check(counts(raw) == n0 && fingerprint(raw, id) == f0,
        "invalid whole (" + what + "): nothing written (counts + fingerprint)");
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
  for (const auto &[c, want] : std::vector<std::pair<uint32_t, std::string>>{
           {0xE9, "C3A9"}, {0x20AC, "E282AC"}, {0x1F600, "F09F9880"}}) {
    const auto attrs = ctl.attributes_of(character_address(c));
    check(attrs && attrs->notation == want, "notation is byte hex " + want);
  }
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

  // ======== Endpoint tier. ========

  // ---- Pure: endpoint_address. ----
  for (const auto &[c, want] : std::vector<std::pair<uint32_t, std::string>>{
           {0x41, "00.00.02.00.13"},   {0x80, "00.00.02.00.24"},   {0xE9, "00.00.02.00.3l"},
           {0x4E2D, "00.00.02.05.Cn"}, {0x20AC, "00.00.02.02.Au"}, {0x1F600, "00.00.02.0X.Qm"},
           {0x10FFFF, "00.00.02.4f.pX"}}) {
    char name[32];
    std::snprintf(name, sizeof name, "endpoint_address(U+%04X)", c);
    check(text(endpoint_address(c)) == want, std::string(name) + " = " + want);
  }
  {
    bool mono = true, own_trunk = true;
    std::string prev;
    auto step = [&](uint32_t c) {
      if (c >= 0xD800 && c <= 0xDFFF) {
        return;
      }
      const std::string s = text(endpoint_address(c));
      mono = mono && (prev.empty() || prev < s);  // fixed-width + "C" order = byte order
      own_trunk = own_trunk && s.rfind("00.00.02.", 0) == 0;
      prev = s;
    };
    for (uint32_t c = 0; c <= 0x10FFFF; c += 97) {
      step(c);
    }
    check(mono && own_trunk, "endpoint_address strictly increasing over a stride of cp");
    mono = true;
    prev.clear();
    for (uint32_t c : {0x7Fu, 0x80u, 0x7FFu, 0x800u, 0xD7FFu, 0xE000u, 0xFFFFu, 0x10000u, 0x10FFFFu}) {
      step(c);
    }
    check(mono && own_trunk, "endpoint_address strictly increasing over every width boundary");
    bool disjoint = true;
    for (uint32_t c : {0x80u, 0x7FFu, 0x800u, 0xFFFFu, 0x10000u, 0x10FFFFu}) {
      disjoint = disjoint && endpoint_address(c)[2] != character_address(c)[2] &&
                 endpoint_address(c)[2] != utf8_label_address()[2];
    }
    check(disjoint, "endpoint trunk 02 disjoint from the combination (00) and label (01) trunks");
    check(text(utf8_label_address()) == "00.00.01.00.06", "UTF-8 label address 00.00.01.00.06");
  }
  {
    bool ok = true;
    for (uint32_t c : {0xD800u, 0xDBFFu, 0xDFFFu, 0x110000u}) {
      bool th = false;
      try { endpoint_address(c); } catch (const std::invalid_argument &) { th = true; }
      ok = ok && th;
    }
    check(ok, "endpoint_address refuses surrogates and values above U+10FFFF");
    check(text(endpoint_address(0)) == "00.00.02.00.00", "endpoint_address(0) defined (no special case)");
  }

  // ---- Database. ----
  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "cannot connect: %s\n", PQerrorMessage(raw));
    return 2;
  }
  const std::vector<uint32_t> combos = {0xE9,   0x80,   0x7FF,  0x800,  0x4E2D,  0xD7FF,
                                        0xE000, 0xFFFF, 0x10000, 0x1F600, 0x10FFFF, 0x20AC};
  for (uint32_t c : combos) {
    populate_character(ctl, c);
  }
  threw = false;
  try { verify_utf8_label(ctl); } catch (const std::runtime_error &) { threw = true; }
  check(threw, "verify_utf8_label throws before the label exists");

  // Rollback: label absent, so add_membership fails after the mint -> nothing written.
  {
    const std::string n0 = counts(raw);
    threw = false;
    try { populate_endpoint(ctl, 0xE9); } catch (const std::runtime_error &) { threw = true; }
    check(threw, "label absent: error propagates");
    check(!ctl.in_transaction(), "label absent: scope closed");
    check(!ctl.token_exists(endpoint_address(0xE9)) && counts(raw) == n0,
          "label absent: no endpoint rows (token/parent/child/membership counts unchanged)");
  }

  // The UTF-8 label: one command, idempotent, no parents, member of nothing.
  create_utf8_label(ctl);
  create_utf8_label(ctl);
  {
    const auto attrs = ctl.attributes_of(utf8_label_address());
    check(attrs && attrs->notation == "UTF-8" && attrs->mass == kLabelPlaceholderMass,
          "UTF-8 label: notation, placeholder mass");
    check(ctl.parents_of(utf8_label_address()).empty() && ctl.member_of(utf8_label_address()).empty(),
          "UTF-8 label: no parents, member of nothing");
    verify_utf8_label(ctl);
    check(true, "verify_utf8_label passes once the label exists");
  }

  // Model-anchored endpoints: 2-, 3-, 4-byte, and the width boundaries.
  const std::string n_before_mint = counts(raw);
  for (uint32_t c : combos) {
    check(populate_endpoint(ctl, c), "endpoint freshly minted");
  }
  {
    std::string n = n_before_mint;
    (void)n;
    check(scalar(raw, "SELECT count(*) FROM token WHERE token_id >= '{00,00,02}' AND token_id < '{00,00,03}'") ==
              std::to_string(combos.size()),
          "one endpoint token per codepoint minted");
    check(scalar(raw, "SELECT count(*) FROM token_parent WHERE token_id >= '{00,00,02}'") ==
              std::to_string(combos.size()),
          "one token_parent row per endpoint");
  }
  check_endpoint(ctl, 0xE9, "U+00E9", 4, "00.00.02.00.3l");
  check_endpoint(ctl, 0x4E2D, "U+4E2D", 6, "00.00.02.05.Cn");
  check_endpoint(ctl, 0x1F600, "U+1F600", 8, "00.00.02.0X.Qm");
  check_endpoint(ctl, 0x20AC, "U+20AC", 6, "00.00.02.02.Au");
  check_endpoint(ctl, 0x80, "U+0080", 4, "00.00.02.00.24");
  check_endpoint(ctl, 0x7FF, "U+07FF", 4, text(endpoint_address(0x7FF)));
  check_endpoint(ctl, 0x800, "U+0800", 6, text(endpoint_address(0x800)));
  check_endpoint(ctl, 0xD7FF, "U+D7FF", 6, text(endpoint_address(0xD7FF)));
  check_endpoint(ctl, 0xE000, "U+E000", 6, text(endpoint_address(0xE000)));
  check_endpoint(ctl, 0xFFFF, "U+FFFF", 6, text(endpoint_address(0xFFFF)));
  check_endpoint(ctl, 0x10000, "U+10000", 8, text(endpoint_address(0x10000)));
  check_endpoint(ctl, 0x10FFFF, "U+10FFFF", 8, "00.00.02.4f.pX");
  check(scalar(raw, "SELECT notation = chr(20013) FROM token WHERE token_id = '{00,00,02,05,Cn}'") == "t",
        "U+4E2D notation = chr(20013) in SQL");
  check(scalar(raw, "SELECT encode(convert_to(notation,'UTF8'),'hex') FROM token WHERE token_id = '{00,00,02,0X,Qm}'") ==
            "f09f9880",
        "U+1F600 notation bytes f09f9880");
  check(ctl.members_of(utf8_label_address()).size() == combos.size(),
        "UTF-8 label members = the endpoints minted");

  // Re-run: complete endpoints are a no-op (nothing written).
  {
    const std::string n0 = counts(raw);
    bool all_noop = true;
    for (uint32_t c : combos) {
      all_noop = all_noop && !populate_endpoint(ctl, c);
    }
    check(all_noop && counts(raw) == n0, "re-run: complete endpoints accepted, nothing written");
  }

  // Combination guards: absent, NULL mass, wrong notation -> throw, nothing written.
  {
    const std::string n0 = counts(raw);
    threw = false;
    try { populate_endpoint(ctl, 0xE7); } catch (const std::runtime_error &) { threw = true; }
    check(threw && !ctl.token_exists(endpoint_address(0xE7)) && counts(raw) == n0,
          "absent combination: throws, nothing written");
    // NULL-mass combination (raw fixture; no trigger fires on a token insert).
    exec(raw, "INSERT INTO token (token_id, token_text, notation, mass) VALUES (" +
                  sqlarr(character_address(0xE8)) + ", '" + text(character_address(0xE8)) + "', 'C3A8', NULL)");
    const std::string n1 = counts(raw);
    threw = false;
    try { populate_endpoint(ctl, 0xE8); } catch (const std::runtime_error &) { threw = true; }
    check(threw && !ctl.token_exists(endpoint_address(0xE8)) && counts(raw) == n1,
          "NULL-mass combination: pre-check throws, nothing written");
    // The trigger's own arm (#109, direct-mint path) with the pre-check bypassed:
    // the mint's NULL-parent RAISE rolls the whole command back.
    threw = false;
    try {
      ctl.with_transaction([&] {
        ctl.mint(endpoint_address(0xE8), "x", {{character_address(0xE8), std::nullopt}});
      });
    } catch (const std::runtime_error &) { threw = true; }
    check(threw && !ctl.in_transaction() && !ctl.token_exists(endpoint_address(0xE8)) && counts(raw) == n1,
          "NULL-mass parent: trigger RAISE rolls the command back");
    // Wrong notation on the combination.
    exec(raw, "INSERT INTO token (token_id, token_text, notation, mass) VALUES (" +
                  sqlarr(character_address(0xE6)) + ", '" + text(character_address(0xE6)) + "', 'C3A7', 4)");
    const std::string n2 = counts(raw);
    threw = false;
    try { populate_endpoint(ctl, 0xE6); } catch (const std::runtime_error &) { threw = true; }
    check(threw && counts(raw) == n2, "wrong-notation combination: throws, nothing written");
    // Outside the populated range.
    threw = false;
    try { populate_endpoint(ctl, 0x41); } catch (const std::invalid_argument &) { threw = true; }
    check(threw, "U+0041 is outside this build: refused");
  }

  // Required negative tests: the five invalid wholes (plus a sixth: the
  // combination not among the parents). Each throws and writes nothing.
  {
    const auto id = [](uint32_t c) { return sqlarr(endpoint_address(c)); };
    const auto cb = [](uint32_t c) { return sqlarr(character_address(c)); };
    check_rejected(ctl, raw, 0xEA, "missing reciprocal token_child edge",
                   "DELETE FROM token_child WHERE token_id = " + cb(0xEA) + " AND child_token_id = " + id(0xEA));
    check_rejected(ctl, raw, 0xEB, "member_of row present, members row absent",
                   "DELETE FROM members WHERE member_token_id = " + id(0xEB));
    check_rejected(ctl, raw, 0xEC, "members row present, member_of row absent",
                   "DELETE FROM member_of WHERE token_id = " + id(0xEC));
    check_rejected(ctl, raw, 0xED, "mass != sum of parents' masses",
                   "UPDATE token SET mass = 99 WHERE token_id = " + id(0xED));
    check_rejected(ctl, raw, 0xEE, "notation != chr(cp)",
                   "UPDATE token SET notation = 'x' WHERE token_id = " + id(0xEE));
    check_rejected(ctl, raw, 0xEF, "combination not among the parents",
                   "UPDATE token_parent SET parent_token_id = " + cb(0xE9) + " WHERE token_id = " + id(0xEF));
  }

  // Schema-permits-N proof (raw SQL): a second parent and a second group on an
  // endpoint insert cleanly, the trigger sums both, and the re-run still accepts.
  {
    const uint32_t cp2 = 0xE9;  // a 2-byte endpoint, mass 4
    exec(raw, "INSERT INTO token_parent (token_id, ordinal, parent_token_id) VALUES (" + sqlarr(endpoint_address(cp2)) +
                  ", 1, " + sqlarr(character_address(0x80)) + ")");
    exec(raw, "INSERT INTO token_child (token_id, child_token_id) VALUES (" + sqlarr(character_address(0x80)) + ", " +
                  sqlarr(endpoint_address(cp2)) + ")");
    check(scalar(raw, "SELECT mass FROM token WHERE token_id = " + sqlarr(endpoint_address(cp2))) == "8",
          "second parent: the trigger sums both parents (4 + 4 = 8)");
    exec(raw, "INSERT INTO member_of (token_id, group_token_id) VALUES (" + sqlarr(endpoint_address(cp2)) + ", " +
                  sqlarr(category_label_address(Category::Two)) + ")");
    check(ctl.parents_of(endpoint_address(cp2)).size() == 2 && ctl.member_of(endpoint_address(cp2)).size() == 2,
          "schema permits a second parent and a second group on an endpoint");
    const std::string n0 = counts(raw), f0 = fingerprint(raw, text(endpoint_address(cp2)));
    check(!populate_endpoint(ctl, cp2), "re-run: endpoint with a second parent (mass = sum) still accepted");
    check(counts(raw) == n0 && fingerprint(raw, text(endpoint_address(cp2))) == f0,
          "re-run with a second parent: nothing written");
  }

  PQfinish(raw);
  std::printf("%s encoding_populate_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
