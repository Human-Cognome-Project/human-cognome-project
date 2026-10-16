#include <algorithm>
#include <iostream>
#include <limits>
#include <set>

#include "harness.h"
#include "letter_projection_fixture.h"

namespace {
using namespace letter_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  CHECK_TRUE(ok, label);
}
template <class F>
void rejects(F run, const char *label) {
  bool refused = false;
  try {
    run();
  } catch (const warm::Error &) {
    refused = true;
  }
  check(refused, label);
}
}  // namespace

int main() {
  using namespace letter_fixture;
  try {
    Fixture f;
    auto requests = std::vector<LetterRequest>{
        f.request(f.english, 9, warm::Exposure::Aggregate),
        f.request(f.french, 3, warm::Exposure::Aggregate),
        f.request(f.accented, 5, warm::Exposure::Aggregate),
        f.request(f.dutch, 4, warm::Exposure::Aggregate),
        f.request(f.repeated, 6, warm::Exposure::Aggregate)};
    auto b = normalize_letters(f.base, requests);
    warm::validate(b);
    auto p = project_letters(b);
    check(p.occurrences().size() == 18,
          "five aggregates and thirteen byte alternatives; no character/nibble "
          "particles");
    check(b.nodes.size() == 1 &&
              std::all_of(b.occurrences.begin(), b.occurrences.end(),
                          [](const auto &o) { return o.node == 0; }),
          "letter-only normalization ignores dormant byte detail");
    const auto &en = b.tools.at(f.tools.at(f.english)),
               &fr = b.tools.at(f.tools.at(f.french));
    check(!(en.ref.object == fr.ref.object) &&
              en.parents[0].target == fr.parents[0].target,
          "English/French A retain distinct language identities with one "
          "shared core parent");
    bool exact_source = true;
    for (const auto &t : f.base.tools)
      exact_source &= warm::tool_body(t) ==
                      warm::tool_body(b.tools.at(f.tools.at(t.ref.object)));
    check(exact_source && !en.parents[0].stored_mass,
          "all source rows, revisions and unknown stored masses preserved");
    std::vector<std::uint64_t> dutch_counts;
    std::vector<std::uint32_t> repeated_ordinals;
    for (const auto &edge : p.parents()) {
      if (edge.occurrence.source.entry == 4) dutch_counts.push_back(edge.count);
      if (edge.occurrence.source.entry == 6 &&
          edge.field.object == f.ref(object(floor_address('a' + 17))).object)
        repeated_ordinals.push_back(edge.ordinal);
    }
    check(dutch_counts == std::vector<std::uint64_t>{4, 2, 4},
          "Dutch aggregate retains 4+2+4 ordered scalar contributions");
    check(repeated_ordinals == std::vector<std::uint32_t>{0, 1},
          "repeated a parents remain distinct contributions to the same field");
    std::vector<std::string> dutch_bytes;
    std::vector<std::vector<std::uint32_t>> dutch_paths;
    bool provenance = true, equivalence = true, slots = true;
    std::set<std::string> ids;
    for (std::size_t i = 0; i < p.occurrences().size(); ++i) {
      const auto &o = p.occurrences()[i];
      slots &= o.slot == int(i) && ids.insert(occurrence_text(o.key)).second;
      if (o.key.source.path.empty())
        equivalence &= bool(o.complete_character) ==
                       (o.key.source.entry != 4 && o.key.source.entry != 6);
      if (o.byte) {
        provenance &= o.byte->owner ==
                          OccurrenceKey{p.study(), {o.key.source.entry, {}}} &&
                      o.byte->character_ordinal == o.key.source.path[0] &&
                      o.byte->byte_ordinal == o.key.source.path[1];
        if (o.key.source.entry == 4) {
          dutch_bytes.push_back(
              *b.tools.at(f.tools.at(o.definition.object)).notation);
          dutch_paths.push_back(o.key.source.path);
        }
      }
    }
    check(dutch_bytes == std::vector<std::string>{"C3", "AD", "6A", "CC", "81"},
          "Dutch exact five-byte sequence conserves mass 10");
    check(dutch_paths ==
                  std::vector<std::vector<std::uint32_t>>{
                      {0, 0}, {0, 1}, {1, 0}, {2, 0}, {2, 1}} &&
              provenance,
          "character/byte ordinals round-trip with full owner and pinned "
          "source character");
    check(equivalence,
          "whole-value character equivalence exists only for single-scalar "
          "letters");
    check(slots && p.occurrences().front().key.source.entry == 3,
          "slots include inactive alternatives in entry/path order");
    check(f.base.occurrences.empty() && f.base.nodes.empty(),
          "normalization leaves supplied definition inputs unchanged");

    auto both = std::vector<LetterRequest>{
        f.request(f.english, UINT64_MAX, warm::Exposure::Parts),
        f.request(f.english, 0, warm::Exposure::Aggregate)};
    b = normalize_letters(f.base, both);
    b.contacts = {{{0, {}}, {UINT64_MAX, {0, 0}}}};
    p = project_letters(b);
    check(p.occurrences().size() == 4 && p.parents().size() == 3 &&
              std::count_if(p.occurrences().begin(), p.occurrences().end(),
                            [](const auto &o) { return o.active; }) == 2,
          "two uses of A have distinct alternatives, one active aggregate and "
          "one active byte");
    check(b.nodes.size() == 2 && b.occurrences[0].node == 1 &&
              b.occurrences[1].node == 0,
          "mixed normalization assigns root bytes and primary-step letters "
          "before native prepare");
    check(
        occurrence_text(p.occurrences()[2].key).find("18446744073709551615") !=
            std::string::npos,
        "native occurrence identity retains full uint64 entry");
    const auto second = project_letters(b);
    check(p.study() != second.study() &&
              !(p.occurrences()[0].key == second.occurrences()[0].key),
          "every new projection has a distinct study identity, including "
          "equal-count exposure changes");
    both[1].exposure = warm::Exposure::Parts;
    const auto bytes_only = normalize_letters(f.base, both);
    check(bytes_only.nodes.size() == 2 && bytes_only.occurrences[0].node == 1 &&
              bytes_only.occurrences[1].node == 0,
          "byte-only keeps inactive letter alternatives at primary binding");
    const auto changed = project_letters(bytes_only);
    check(changed.study() != p.study() &&
              changed.occurrences().size() == p.occurrences().size(),
          "equal-count exposure change creates a new study rather than "
          "refreshing live identity");
    auto dutch_parts = normalize_letters(
        f.base, {f.request(f.dutch, 1, warm::Exposure::Parts)});
    const auto dp = project_letters(dutch_parts);
    check(dp.parents().size() == 10 &&
              std::all_of(dp.parents().begin(), dp.parents().end(),
                          [](const auto &e) { return e.count == 1; }),
          "exposed bytes retain ten ordered nibble contributions; no active "
          "letter contribution");
    for (const auto &form : {std::string(u8"€"), std::string(u8"𐐀")}) {
      Fixture wide;
      const auto c = wide.character(object("00.00.00.02.00"), form);
      const auto id = object("02.00.20.00.1Z");
      wide.add(id, form, form.size() * 2, {c});
      for (const auto &t : wide.base.tools)
        if (t.mass && *t.mass >= 2 &&
            std::none_of(
                wide.base.geometry.begin(), wide.base.geometry.end(),
                [&](const auto &g) { return g.object == t.ref.object; }))
          wide.base.geometry.push_back({t.ref.object, {}, {{{}, 0.25}}});
      const auto w = project_letters(normalize_letters(
          wide.base, {wide.request(id, 0, warm::Exposure::Parts)}));
      check(w.occurrences().size() == form.size() + 1 &&
                w.parents().size() == form.size() * 2,
            "valid three/four-byte source scalar projects exact byte and "
            "nibble occurrences");
    }

    auto bad = b;
    bad.selections[0].parts.clear();
    rejects([&] { project_letters(bad); }, "missing persisted parts rejected");
    bad = b;
    bad.selections[0].parts.push_back(bad.selections[0].parts[0]);
    rejects([&] { project_letters(bad); }, "repeated persisted part rejected");
    bad = dutch_parts;
    std::swap(bad.selections[0].parts[0], bad.selections[0].parts[1]);
    rejects([&] { project_letters(bad); },
            "reordered persisted parts rejected");
    bad = b;
    bad.occurrences.push_back(bad.occurrences[0]);
    rejects([&] { project_letters(bad); },
            "duplicate occurrence identity rejected");
    bad = b;
    bad.occurrences[1].id.path = {0};
    rejects([&] { project_letters(bad); },
            "malformed occurrence path rejected");
    bad = dutch_parts;
    bad.occurrences[1].definition =
        f.ref(object(floor_address('A' + 17))).object;
    rejects([&] { project_letters(bad); },
            "wrong definition at a recorded byte position rejected");
    bad = b;
    f.tool(bad, object(floor_address('A' + 17))).parents[0].ordinal =
        UINT32_MAX;
    rejects([&] { project_letters(bad); }, "invalid source ordinal rejected");
    bad = dutch_parts;
    f.tool(bad, object("00.00.00.01.1l")).notation = "C3ad";
    rejects([&] { project_letters(bad); }, "malformed source hex rejected");
    bad = b;
    auto &byte = f.tool(bad, object(floor_address('A' + 17)));
    std::swap(byte.parents[0].target, byte.parents[1].target);
    rejects([&] { project_letters(bad); },
            "mass-preserving wrong ordered nibble values rejected");
    bad = b;
    bad.nodes.resize(1);
    rejects([&] { project_letters(bad); },
            "unnormalized mixed layout rejected");
    bad = b;
    bad.contacts = {{{0, {}}, {UINT64_MAX, {}}}};
    rejects([&] { project_letters(bad); },
            "inactive contact endpoint rejected");
    bad = b;
    f.tool(bad, f.english).parents[0].target.revision = new_uuid();
    rejects([&] { project_letters(bad); },
            "inconsistent pinned dependency revision rejected");
    bad = b;
    bad.tools.push_back(bad.tools.front());
    bad.tools.back().ref.revision = new_uuid();
    rejects([&] { project_letters(bad); },
            "two revisions of one object in one study rejected");
    bad = b;
    f.tool(bad, f.english).parents[0].stored_mass = 0;
    rejects([&] { project_letters(bad); },
            "stored zero is not substituted for unknown parent mass");
    bad = b;
    f.tool(bad, f.english).mass.reset();
    rejects([&] { project_letters(bad); }, "unknown live mass rejected");
    bad = b;
    f.tool(bad, f.english).mass = UINT64_MAX;
    rejects([&] { project_letters(bad); }, "exact integer overflow rejected");
    bad = b;
    bad.geometry.clear();
    rejects([&] { project_letters(bad); },
            "missing supplied geometry rejected");
    bad = b;
    bad.occurrences[0].position.x = std::numeric_limits<double>::infinity();
    rejects([&] { project_letters(bad); },
            "non-finite supplied initial state rejected");
    bad = b;
    f.tool(bad, f.english).notation = "a";
    rejects([&] { project_letters(bad); },
            "source-form case mismatch rejected without casefolding");
    bad = dutch_parts;
    f.tool(bad, f.dutch).notation = u8"íj";
    rejects([&] { project_letters(bad); },
            "source form missing combining scalar rejected");
    bad = normalize_letters(
        f.base, {f.request(f.accented, 0, warm::Exposure::Aggregate)});
    f.tool(bad, f.accented).notation = u8"í";
    rejects([&] { project_letters(bad); },
            "canonically equivalent spelling rejected without normalization");
    auto r = f.request(f.dutch, 0, warm::Exposure::Parts);
    r.bytes.pop_back();
    rejects([&] { normalize_letters(f.base, {r}); },
            "missing explicit byte placement rejected");
    r = f.request(f.dutch, 0, warm::Exposure::Parts);
    r.bytes[1].path = r.bytes[0].path;
    rejects([&] { normalize_letters(f.base, {r}); },
            "repeated input byte path rejected");
    r = f.request(f.dutch, 0, warm::Exposure::Parts);
    r.bytes.push_back(r.bytes[0]);
    rejects([&] { normalize_letters(f.base, {r}); },
            "extraneous byte placement rejected");
    r = f.request(f.dutch, 0, warm::Exposure::Parts);
    r.bytes[0].path = {0, 0, 0};
    rejects([&] { normalize_letters(f.base, {r}); },
            "malformed input byte path rejected");
    r = f.request(f.english, 0, warm::Exposure::Aggregate);
    rejects(
        [&] {
          normalize_letters(f.base, {r, r});
        },
        "duplicate request entry rejected");
    rejects([&] { normalize_letters(b, {r}); },
            "normalization cannot silently replace an existing recipe");
    r.letter.revision = new_uuid();
    rejects([&] { normalize_letters(f.base, {r}); },
            "stale requested letter revision rejected");
    check(
        warm::key(object("02.00.00.00.03", new_uuid())) != warm::key(f.english),
        "opaque definition identity preserves instance scope");
    // Structurally valid invalid UTF-8 fixtures isolate source-form validation.
    for (const std::string form :
         {std::string("\xC0\xAF", 2), std::string("\xED\xA0\x80", 3),
          std::string("\xF4\x90\x80\x80", 4), std::string("\xE2\x28\xA1", 3)}) {
      Fixture invalid;
      const auto c = invalid.character(object("00.00.00.02.00"), form);
      const auto id = object("02.00.20.00.1Z");
      invalid.add(id, form, form.size() * 2, {c});
      rejects(
          [&] {
            normalize_letters(
                invalid.base,
                {invalid.request(id, 0, warm::Exposure::Aggregate)});
          },
          "invalid/overlong/surrogate/out-of-range source UTF-8 rejected");
    }
    std::cout << checks << " projection checks\n";
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  return harness::report("letter_projection_test");
}
