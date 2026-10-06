#include "encoding_populate.h"

#include <cstdio>
#include <stdexcept>

namespace dbk::bootstrap {

namespace {

codec::Address addr(std::initializer_list<unsigned> codes) {
  codec::Address a;
  for (unsigned c : codes) {
    a.push_back(codec::make_full_element(static_cast<uint16_t>(c)));
  }
  return a;
}

// Couplet code of the two-character literal "d1d2" in the Base62 alphabet.
unsigned couplet(char d1, char d2) {
  const auto c = codec::couplet_to_code(d1, d2);
  if (!c) {
    throw std::logic_error("bad couplet literal");
  }
  return *c;
}

const unsigned kFirstByteCouplet = 62;  // "10": byte 0x00 sits at ...10

std::string hex_notation(uint8_t b) {
  static const char *digits = "0123456789ABCDEF";
  return std::string{digits[b >> 4], digits[b & 0xF]};
}

}  // namespace

codec::Address byte_code_address(uint8_t byte) {
  return addr({0, 0, 0, 0, kFirstByteCouplet + byte});
}

codec::Address overgroup_address() {
  return addr({0, 0, couplet('0', '1'), 0, couplet('0', '2')});
}

codec::Address category_label_address(Category c) {
  const char last = c == Category::Two ? '3' : c == Category::Three ? '4' : '5';
  return addr({0, 0, couplet('0', '1'), 0, couplet('0', last)});
}

const char *category_label_notation(Category c) {
  switch (c) {
    case Category::Two: return "Two-Byte Codes";
    case Category::Three: return "Three-Byte Codes";
    default: return "Four-Byte Codes";
  }
}

bool is_populated_codepoint(uint32_t cp) {
  return cp >= 0x80 && cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);
}

std::vector<uint8_t> utf8_encode(uint32_t cp) {
  if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
    return {};
  }
  if (cp < 0x80) {
    return {static_cast<uint8_t>(cp)};
  }
  if (cp < 0x800) {
    return {static_cast<uint8_t>(0xC0 | (cp >> 6)),
            static_cast<uint8_t>(0x80 | (cp & 0x3F))};
  }
  if (cp < 0x10000) {
    return {static_cast<uint8_t>(0xE0 | (cp >> 12)),
            static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)),
            static_cast<uint8_t>(0x80 | (cp & 0x3F))};
  }
  return {static_cast<uint8_t>(0xF0 | (cp >> 18)),
          static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3F)),
          static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)),
          static_cast<uint8_t>(0x80 | (cp & 0x3F))};
}

Category category_of(uint32_t cp) {
  return cp < 0x800 ? Category::Two : cp < 0x10000 ? Category::Three : Category::Four;
}

codec::Address character_address(uint32_t cp) {
  if (!is_populated_codepoint(cp)) {
    throw std::invalid_argument("codepoint outside U+0080..U+10FFFF or a surrogate");
  }
  unsigned first_fourth;  // couplet code where the category range starts
  unsigned last_fourth;   // last couplet code the range may reach
  uint32_t index;         // sequential slot, in codepoint order, surrogates skipped
  switch (category_of(cp)) {
    case Category::Two:
      first_fourth = couplet('0', '1');
      last_fourth = couplet('0', '2');
      index = cp - 0x80;
      break;
    case Category::Three:
      first_fourth = couplet('0', '3');
      last_fourth = couplet('0', 'z');
      index = cp - 0x800 - (cp > 0xDFFF ? 0x800 : 0);
      break;
    default:
      first_fourth = couplet('1', '0');
      last_fourth = couplet('z', 'z');
      index = cp - 0x10000;
      break;
  }
  const unsigned fourth = first_fourth + index / codec::kCoupletSpace;
  if (fourth > last_fourth) {
    throw std::logic_error("character address overflows its reserved range");
  }
  return addr({0, 0, 0, fourth, index % codec::kCoupletSpace});
}

void create_category_labels(Controller &ctl) {
  for (Category c : {Category::Two, Category::Three, Category::Four}) {
    ctl.with_transaction([&] {
      ctl.mint(category_label_address(c), category_label_notation(c), {},
               kLabelPlaceholderMass);
      ctl.add_membership(category_label_address(c), overgroup_address());
    });
  }
}

void verify_floor(Controller &ctl) {
  if (!ctl.token_exists(overgroup_address())) {
    throw std::runtime_error("floor: overgroup token 00.00.01.00.02 missing");
  }
  for (int b = 0; b < 256; ++b) {
    const auto id = byte_code_address(static_cast<uint8_t>(b));
    const auto attrs = ctl.attributes_of(id);
    if (!attrs) {
      throw std::runtime_error("floor: byte-code token missing for byte " +
                               hex_notation(static_cast<uint8_t>(b)));
    }
    if (attrs->notation != hex_notation(static_cast<uint8_t>(b))) {
      throw std::runtime_error("floor: byte-code notation mismatch for byte " +
                               hex_notation(static_cast<uint8_t>(b)));
    }
  }
}

bool populate_character(Controller &ctl, uint32_t cp, const codec::Address &label) {
  if (!is_populated_codepoint(cp)) {
    throw std::invalid_argument("codepoint outside U+0080..U+10FFFF or a surrogate");
  }
  const auto id = character_address(cp);
  std::vector<Constituent> parents;
  for (uint8_t b : utf8_encode(cp)) {
    parents.push_back({byte_code_address(b), std::nullopt});
  }
  // Temporary notation: the UTF-8 bytes as hex, in order (not the character).
  std::string notation;
  for (uint8_t b : utf8_encode(cp)) {
    notation += hex_notation(b);
  }
  return ctl.with_transaction([&] {
    const bool existed = ctl.mint(id, notation, parents);
    if (existed) {
      // Guard against an address collision with a different token.
      const auto stored = ctl.parents_of(id);
      bool same = stored.size() == parents.size();
      for (size_t i = 0; same && i < stored.size(); ++i) {
        same = stored[i].parent == parents[i].address;
      }
      if (!same) {
        throw std::runtime_error("existing token at character address has different parents");
      }
    }
    ctl.add_membership(id, label);
    return !existed;
  });
}

bool populate_character(Controller &ctl, uint32_t cp) {
  return populate_character(ctl, cp, category_label_address(category_of(cp)));
}

void populate_codepoint(Controller &ctl, uint32_t cp) { populate_character(ctl, cp); }

std::vector<uint32_t> sample_codepoints() { return {0x00E9, 0x20AC, 0x1F600}; }

// ---- Endpoint tier. ----

namespace {

// The character as UTF-8 text (what `chr(cp)` stores in a UTF8 database).
std::string character_text(uint32_t cp) {
  const auto bytes = utf8_encode(cp);
  return std::string(bytes.begin(), bytes.end());
}

std::string utf8_hex(uint32_t cp) {
  std::string hex;
  for (uint8_t b : utf8_encode(cp)) {
    hex += hex_notation(b);
  }
  return hex;
}

std::string cp_label(uint32_t cp) {
  char buf[16];
  std::snprintf(buf, sizeof buf, "U+%04X", cp);
  return buf;
}

std::string text_of(const codec::Address &a) {
  const auto t = codec::encode_token_id(a);
  return t ? *t : std::string("<invalid address>");
}

// The whole-endpoint integrity check of populate_endpoint's re-run path.
// Reads only: PK points and PK-prefix follows of this endpoint's own rows.
void validate_endpoint(Controller &ctl, uint32_t cp, const codec::Address &id,
                       const codec::Address &combination) {
  const std::string who = cp_label(cp) + " endpoint " + text_of(id) + ": ";
  const auto attrs = ctl.attributes_of(id);
  if (!attrs) {
    throw std::runtime_error(who + "vanished during validation");
  }
  if (attrs->notation != character_text(cp)) {
    throw std::runtime_error(who + "notation is not the character");
  }
  // (i) the combination is among its parents (never size() == 1).
  const auto parents = ctl.parents_of(id);
  bool linked = false;
  long sum = 0;
  for (const auto &p : parents) {
    linked = linked || p.parent == combination;
    // (ii) the parent TOKENS' own masses; token_parent.mass is NULL here.
    const auto pa = ctl.attributes_of(p.parent);
    if (!pa || !pa->mass) {
      throw std::runtime_error(who + "parent " + text_of(p.parent) +
                               " is missing or has NULL mass");
    }
    sum += *pa->mass;
  }
  if (!linked) {
    throw std::runtime_error(who + "combination " + text_of(combination) +
                             " is not among its parents");
  }
  if (!attrs->mass || *attrs->mass != sum) {
    throw std::runtime_error(who + "mass is not the sum of its parents' masses");
  }
  // (iii) the reciprocal token_child edge, from the combination's children.
  bool wired = false;
  for (const auto &c : ctl.children_of(combination)) {
    wired = wired || c == id;
  }
  if (!wired) {
    throw std::runtime_error(who + "combination does not list it as a child");
  }
  // (iv) both membership directions, by point-probe on the full pair.
  const auto m = ctl.membership_present(id, utf8_label_address());
  if (!m.in_member_of || !m.in_members) {
    throw std::runtime_error(who + "UTF-8 membership is missing or one-sided");
  }
}

}  // namespace

codec::Address endpoint_address(uint32_t cp) {
  if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
    throw std::invalid_argument("codepoint above U+10FFFF or a surrogate");
  }
  return addr({0, 0, couplet('0', '2'), cp / codec::kCoupletSpace, cp % codec::kCoupletSpace});
}

codec::Address utf8_label_address() {
  return addr({0, 0, couplet('0', '1'), 0, couplet('0', '6')});
}

void create_utf8_label(Controller &ctl) {
  ctl.with_transaction(
      [&] { ctl.mint(utf8_label_address(), kUtf8LabelNotation, {}, kLabelPlaceholderMass); });
}

void verify_utf8_label(Controller &ctl) {
  const auto attrs = ctl.attributes_of(utf8_label_address());
  if (!attrs || attrs->notation != kUtf8LabelNotation) {
    throw std::runtime_error("UTF-8 label 00.00.01.00.06 missing or wrong notation");
  }
}

codec::Address resolve_combination(Controller &ctl, uint32_t cp) {
  if (!is_populated_codepoint(cp)) {
    throw std::invalid_argument("codepoint outside U+0080..U+10FFFF or a surrogate");
  }
  const auto combination = character_address(cp);
  const std::string who = cp_label(cp) + " combination " + text_of(combination) + ": ";
  const auto attrs = ctl.attributes_of(combination);
  if (!attrs) {
    throw std::runtime_error(who + "missing");
  }
  if (!attrs->mass) {
    throw std::runtime_error(who + "mass is NULL");
  }
  if (attrs->notation != utf8_hex(cp)) {
    throw std::runtime_error(who + "notation is not the UTF-8 byte hex " + utf8_hex(cp));
  }
  return combination;
}

void verify_combinations(Controller &ctl, const std::vector<uint32_t> &codepoints) {
  for (uint32_t cp : codepoints) {
    resolve_combination(ctl, cp);
  }
}

bool populate_endpoint(Controller &ctl, uint32_t cp) {
  const auto combination = resolve_combination(ctl, cp);
  const auto id = endpoint_address(cp);
  if (ctl.token_exists(id)) {
    validate_endpoint(ctl, cp, id, combination);
    return false;
  }
  return ctl.with_transaction([&] {
    if (ctl.mint(id, character_text(cp), {{combination, std::nullopt}})) {
      throw std::runtime_error(cp_label(cp) + " endpoint " + text_of(id) +
                               ": appeared during the mint");
    }
    ctl.add_membership(id, utf8_label_address());
    return true;
  });
}

}  // namespace dbk::bootstrap
