#pragma once

#include <map>

#include "assembly/letter_projection.h"

// Pure, bounded constructions using the retained seed's hex-notation contract.
// Additional addresses below are fixture identifiers, never DB allocations.
namespace letter_fixture {
using namespace warm;
using namespace assembly;
inline ObjectKey object(const std::string &text, Uuid scope = {}) {
  const auto address = codec::decode_token_id(text);
  if (!address) throw Error("fixture address");
  return {scope, *address};
}
inline std::string floor_address(unsigned slot) {
  const auto couplet = *codec::code_to_couplet(std::uint16_t(slot));
  return "00.00.00.00." + std::string{couplet.first, couplet.second};
}
struct Fixture {
  Bundle base;
  std::map<ObjectKey, std::size_t> tools;
  const ObjectKey english = object("02.00.00.00.03");
  const ObjectKey french = object("02.01.00.00.03");
  const ObjectKey dutch = object("02.00.20.00.1W");
  const ObjectKey accented = object("02.00.20.00.1X");
  const ObjectKey repeated = object("02.00.20.00.1Y");
  const Uuid epoch = new_uuid(), actor = new_uuid();

  ToolRef add(ObjectKey id, std::string notation, std::uint64_t mass,
              const std::vector<ToolRef> &parents = {}) {
    if (tools.count(id)) return base.tools.at(tools.at(id)).ref;
    Tool t{{id, new_uuid()},
           epoch,
           "projection-fixture-v1",
           notation,
           mass,
           {},
           {},
           actor};
    for (const auto &ref : parents) {
      const auto &p = base.tools.at(tools.at(ref.object));
      t.parents.push_back({ref, std::uint32_t(t.parents.size()), {}, *p.mass});
    }
    tools.emplace(id, base.tools.size());
    base.tools.push_back(t);
    return t.ref;
  }
  ToolRef byte(unsigned value) {
    const char *hex = "0123456789ABCDEF";
    const auto hi = add(object(floor_address(value >> 4)),
                        std::string(1, hex[value >> 4]), 1);
    const auto lo = add(object(floor_address(value & 15)),
                        std::string(1, hex[value & 15]), 1);
    return add(object(floor_address(value + 17)),
               std::string{hex[value >> 4], hex[value & 15]}, 2, {hi, lo});
  }
  ToolRef character(ObjectKey id, const std::string &form) {
    std::string encoded;
    std::vector<ToolRef> parents;
    for (const unsigned char c : form) {
      auto ref = byte(c);
      encoded += *base.tools.at(tools.at(ref.object)).notation;
      parents.push_back(ref);
    }
    if (form.size() == 1) return parents[0];
    return add(id, encoded, form.size() * 2, parents);
  }
  Fixture() {
    base.ref = {new_uuid(), new_uuid()};
    base.owner = new_uuid();
    base.creator = actor;
    const auto a = byte('A'), lower_a = byte('a'), j = byte('j');
    const auto acute_i = character(object("00.00.00.01.1l"), u8"í");
    const auto accent = character(object("00.00.00.01.AL"), u8"́");
    add(english, "A", 2, {a});
    add(french, "A", 2, {a});
    add(dutch, u8"íj́", 10, {acute_i, j, accent});
    add(accented, u8"í", 4, {acute_i});
    add(repeated, u8"aá", 8, {lower_a, lower_a, accent});
    // Explicit caller-supplied geometry for every possible whole/byte object.
    for (const auto &t : base.tools)
      if (t.mass && *t.mass >= 2)
        base.geometry.push_back({t.ref.object, {}, {{{}, 0.5}}});
  }
  ToolRef ref(ObjectKey id) const { return base.tools.at(tools.at(id)).ref; }
  Tool &tool(Bundle &b, ObjectKey id) const { return b.tools.at(tools.at(id)); }
  LetterRequest request(ObjectKey id, std::uint64_t entry,
                        warm::Exposure exposure) const {
    LetterRequest r{entry, ref(id), exposure, {{2, 3, 4}, {0.25, -0.5, 1}}, {}};
    const auto &t = base.tools.at(tools.at(id));
    for (const auto &p : t.parents) {
      const auto &c = base.tools.at(tools.at(p.target.object));
      const auto size = *c.mass == 2 ? 1 : c.parents.size();
      for (std::size_t j = 0; j < size; ++j)
        r.bytes.push_back(
            {{p.ordinal, std::uint32_t(j)}, {{2, -3, 1}, {-0.5, 0.25, -1}}});
    }
    return r;
  }
};
}  // namespace letter_fixture
