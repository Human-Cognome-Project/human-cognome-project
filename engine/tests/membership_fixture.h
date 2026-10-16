#pragma once

#include "assembly/membership_fields.h"
#include "letter_projection_fixture.h"

// Only explicit fixture ancestry is followed. These labels/extra addresses do
// not allocate production groups or infer additional source-language ancestry.
namespace membership_fixture {
using namespace letter_fixture;
struct Fixture : letter_fixture::Fixture {
  const ObjectKey en = object("02.00.00"),
                  en_alphabet = object("02.00.00.00.00"),
                  family = object("02.00"), fr = object("02.01.00"),
                  fr_alphabet = object("02.01.00.00.00"),
                  nl = object("02.00.20"),
                  nl_alphabet = object("02.00.20.00.00"),
                  core_class = object("00.00.01.00.01");
  ToolRef group(ObjectKey id, const std::string &label) {
    auto r = add(id, label, 0);
    tool(base, id).mass.reset();
    return r;
  }
  Fixture() {
    const auto e = group(en, "English"),
               ea = group(en_alphabet, "English alphabet"),
               fam = group(family, "Fixture ancestor"), f = group(fr, "French"),
               fa = group(fr_alphabet, "French alphabet"),
               d = group(nl, "Dutch"),
               da = group(nl_alphabet, "Dutch alphabet"),
               classification = group(core_class, "Core classification");
    tool(base, en).member_of = {fam};
    tool(base, en_alphabet).member_of = {e};
    tool(base, fr_alphabet).member_of = {f};
    tool(base, nl_alphabet).member_of = {d};
    tool(base, english).member_of = {e, ea};  // Direct and mediated English.
    tool(base, french).member_of = {f, fa};
    for (const auto &id : {dutch, accented, repeated})
      tool(base, id).member_of = {d, da};
    // A byte's own classification is never inherited from another context.
    tool(base, object(floor_address('A' + 17))).member_of = {classification};
  }
};
inline std::map<ObjectKey, std::uint64_t> totals(
    const MembershipFields &fields) {
  std::map<ObjectKey, std::uint64_t> out;
  for (const auto &edge : fields.contributions())
    out[edge.field.object] += edge.count;
  return out;
}
}  // namespace membership_fixture
