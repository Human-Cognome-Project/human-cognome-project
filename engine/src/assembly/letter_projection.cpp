#include "assembly/letter_projection.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace assembly {
namespace {
void need(bool condition, const char *message) {
  if (!condition) throw warm::Error(message);
}
using Tools = std::map<warm::ObjectKey, const warm::Tool *>;
Tools index(const warm::Bundle &b) {
  Tools tools;
  for (const auto &t : b.tools)
    need(tools.emplace(t.ref.object, &t).second,
         "duplicate definition identity");
  return tools;
}
const warm::Tool &get(const Tools &tools, const warm::ToolRef &ref) {
  auto it = tools.find(ref.object);
  need(it != tools.end() && it->second->ref == ref,
       "missing or inconsistent projection revision");
  return *it->second;
}
void core(const warm::Tool &t) {
  need(t.ref.object.scope.nil() &&
           warm::address_text(t.ref.object.address).substr(0, 2) == "00",
       "character/byte/nibble is not a shared core definition");
}
unsigned hex(char c) {
  if (c >= '0' && c <= '9') return unsigned(c - '0');
  if (c >= 'A' && c <= 'F') return unsigned(c - 'A' + 10);
  throw warm::Error("core notation is not canonical uppercase hex");
}
std::string bytes(const warm::Tool &t) {
  core(t);
  need(t.notation && !t.notation->empty() && t.notation->size() % 2 == 0,
       "missing/malformed core byte notation");
  std::string out;
  for (std::size_t i = 0; i < t.notation->size(); i += 2)
    out += char((hex((*t.notation)[i]) << 4) | hex((*t.notation)[i + 1]));
  return out;
}
void scalar(const std::string &s) {
  need(!s.empty() && s.size() <= 4, "source character is not one UTF-8 scalar");
  const auto first = static_cast<unsigned char>(s[0]);
  std::size_t width;
  std::uint32_t value, minimum;
  if (first < 0x80) {
    width = 1;
    value = first;
    minimum = 0;
  } else if (first >= 0xC2 && first <= 0xDF) {
    width = 2;
    value = first & 31;
    minimum = 0x80;
  } else if (first >= 0xE0 && first <= 0xEF) {
    width = 3;
    value = first & 15;
    minimum = 0x800;
  } else if (first >= 0xF0 && first <= 0xF4) {
    width = 4;
    value = first & 7;
    minimum = 0x10000;
  } else
    throw warm::Error("invalid UTF-8 leading byte");
  need(s.size() == width, "source character UTF-8 width mismatch");
  for (std::size_t i = 1; i < width; ++i) {
    const auto c = static_cast<unsigned char>(s[i]);
    need((c & 0xC0) == 0x80, "invalid UTF-8 continuation byte");
    value = (value << 6) | (c & 63);
  }
  need(value >= minimum && value <= 0x10FFFF &&
           !(value >= 0xD800 && value <= 0xDFFF),
       "overlong/surrogate/out-of-range UTF-8 scalar");
}
void byte_definition(const Tools &tools, const warm::Tool &t,
                     unsigned char value) {
  const auto encoded = bytes(t);
  need(encoded.size() == 1 && static_cast<unsigned char>(encoded[0]) == value &&
           t.mass == std::optional<std::uint64_t>(2) && t.parents.size() == 2,
       "byte definition disagrees with recorded UTF-8");
  for (std::size_t i = 0; i < 2; ++i) {
    const auto &p = t.parents[i];
    const auto &n = get(tools, p.target);
    core(n);
    need(p.ordinal == i && p.contribution == 1 &&
             n.mass == std::optional<std::uint64_t>(1) && n.parents.empty() &&
             n.notation && n.notation->size() == 1 &&
             hex((*n.notation)[0]) == (i == 0 ? value >> 4 : value & 15),
         "byte does not retain its two ordered nibble contributions");
  }
}
struct ByteRef {
  std::vector<std::uint32_t> path;
  warm::ToolRef character, byte;
};
std::vector<ByteRef> expand(const Tools &tools, const warm::Tool &letter) {
  need(letter.notation && !letter.notation->empty() &&
           !letter.parents.empty() && letter.parents.size() <= UINT32_MAX,
       "missing letter source form/construction");
  std::string exact_form;
  std::vector<ByteRef> out;
  for (std::size_t i = 0; i < letter.parents.size(); ++i) {
    const auto &p = letter.parents[i];
    const auto &c = get(tools, p.target);
    const auto encoded = bytes(c);
    scalar(encoded);
    need(p.ordinal == i && c.mass && *c.mass == encoded.size() * 2 &&
             p.contribution == *c.mass,
         "character ordinal/mass disagrees with UTF-8");
    const bool ascii = encoded.size() == 1;
    need(ascii || c.parents.size() == encoded.size(),
         "missing/malformed recorded byte construction");
    for (std::size_t j = 0; j < encoded.size(); ++j) {
      const auto &ref = ascii ? c.ref : c.parents[j].target;
      byte_definition(tools, get(tools, ref),
                      static_cast<unsigned char>(encoded[j]));
      need(out.size() < std::size_t(INT32_MAX),
           "byte occurrence count overflow");
      out.push_back({{std::uint32_t(i), std::uint32_t(j)}, c.ref, ref});
    }
    exact_form += encoded;
  }
  need(exact_form == *letter.notation,
       "letter source form differs from exact UTF-8 construction");
  need(letter.mass && *letter.mass == out.size() * std::uint64_t(2),
       "byte projection does not conserve exact letter mass");
  return out;
}
Point point(warm::Point p) { return {p.x, p.y, p.z}; }
ScaleRole role(warm::ScaleRole r) {
  switch (r) {
    case warm::ScaleRole::Root:
      return ScaleRole::Root;
    case warm::ScaleRole::PrimaryStep:
      return ScaleRole::PrimaryStep;
    case warm::ScaleRole::SecondaryCompression:
      return ScaleRole::SecondaryCompression;
    case warm::ScaleRole::NamingEquivalent:
      return ScaleRole::NamingEquivalent;
  }
  throw warm::Error("unknown scale role");
}
}  // namespace

warm::Bundle normalize_letters(warm::Bundle b,
                               const std::vector<LetterRequest> &requests) {
  need(b.recipe_version == 1 && b.structural_occurrences.empty() &&
           b.focal_fields.empty(),
       "legacy normalization requires recipe v1");
  need(b.nodes.empty() && b.occurrences.empty() && b.selections.empty(),
       "normalization requires definitions, not an existing occurrence recipe");
  need(!requests.empty() && requests.size() <= std::size_t(INT32_MAX) / 2,
       "empty/oversized letter request");
  const auto tools = index(b);
  const bool parts = std::any_of(
      requests.begin(), requests.end(),
      [](const auto &r) { return r.exposure == warm::Exposure::Parts; });
  b.nodes = {{-1, warm::ScaleRole::Root}};
  if (parts) b.nodes.push_back({0, warm::ScaleRole::PrimaryStep});
  std::set<std::uint64_t> entries;
  for (const auto &r : requests) {
    need(entries.insert(r.entry).second, "duplicate request entry");
    const auto expected = expand(tools, get(tools, r.letter));
    need(r.bytes.size() == expected.size(),
         "missing/extraneous byte placement");
    need(expected.size() < std::size_t(INT32_MAX) - b.occurrences.size(),
         "native occurrence capacity overflow");
    b.occurrences.push_back({{r.entry, {}},
                             r.letter.object,
                             parts ? 1 : 0,
                             r.aggregate.position,
                             r.aggregate.velocity});
    warm::Selection selection{r.entry, r.exposure, {}};
    for (std::size_t i = 0; i < expected.size(); ++i) {
      const auto &v = expected[i];
      need(r.bytes[i].path == v.path, "missing/reordered/repeated byte path");
      const warm::OccurrenceId id{r.entry, v.path};
      const auto &state = r.bytes[i].initial;
      b.occurrences.push_back(
          {id, v.byte.object, 0, state.position, state.velocity});
      selection.parts.push_back(id);
    }
    b.selections.push_back(std::move(selection));
  }
  warm::validate(b);
  return b;
}

warm::Bundle normalize_structural_letters(
    warm::Bundle b,
    const std::vector<StructuralLetterRequest> &requests) {
  need(b.nodes.empty() && b.occurrences.empty() &&
           b.structural_occurrences.empty() && b.selections.empty(),
       "structural normalization requires definitions only");
  need(b.recipe_version == 1 || b.recipe_version == 2,
       "unsupported recipe version");
  need(!requests.empty() && requests.size() <= std::size_t(INT32_MAX) / 2,
       "empty/oversized letter request");
  b.recipe_version = 2;
  const auto tools = index(b);
  const bool parts = std::any_of(
      requests.begin(), requests.end(),
      [](const auto &r) { return r.exposure == warm::Exposure::Parts; });
  b.nodes = {{-1, warm::ScaleRole::Root}};
  if (parts)
    b.nodes.push_back({0, warm::ScaleRole::PrimaryStep});
  std::set<std::uint64_t> entries;
  for (const auto &r : requests) {
    need(entries.insert(r.entry).second, "duplicate request entry");
    const auto expected = expand(tools, get(tools, r.letter));
    need(expected.size() <
             std::size_t(INT32_MAX) - b.structural_occurrences.size(),
         "native occurrence capacity overflow");
    b.structural_occurrences.push_back(
        {{r.entry, {}}, r.letter.object, parts ? 1 : 0});
    warm::Selection selection{r.entry, r.exposure, {}};
    for (const auto &v : expected) {
      const warm::OccurrenceId id{r.entry, v.path};
      b.structural_occurrences.push_back({id, v.byte.object, 0});
      selection.parts.push_back(id);
    }
    b.selections.push_back(std::move(selection));
  }
  warm::validate(b);
  return b;
}

bool operator==(const OccurrenceKey &a, const OccurrenceKey &b) {
  return a.study == b.study && a.source == b.source;
}
std::string occurrence_text(const OccurrenceKey &k) {
  need(!k.study.nil() && (k.source.path.empty() || k.source.path.size() == 2),
       "invalid occurrence key");
  auto text = k.study.str() + "/" + std::to_string(k.source.entry);
  for (auto ordinal : k.source.path) text += "/" + std::to_string(ordinal);
  return text;
}

LetterProjection project_letters(const warm::Bundle &b) {
  warm::validate(b);
  LetterProjection out;
  out.study_ = warm::new_uuid();
  out.recipe_ = b;
  const auto tools = index(b);
  std::map<warm::OccurrenceId, warm::StructuralOccurrence> ordered;
  std::map<warm::OccurrenceId, const warm::Occurrence *> legacy;
  std::set<warm::ObjectKey> used;
  for (const auto &o : b.occurrences) {
    ordered.emplace(o.id,
                    warm::StructuralOccurrence{o.id, o.definition, o.node});
    legacy.emplace(o.id, &o);
    used.insert(o.definition);
  }
  for (const auto &o : b.structural_occurrences) {
    ordered.emplace(o.id, o);
    used.insert(o.definition);
  }
  std::map<warm::OccurrenceId, ByteProvenance> provenance;
  std::map<warm::OccurrenceId, warm::ToolRef> equivalent;
  std::set<warm::OccurrenceId> active;
  for (const auto &s : b.selections) {
    const warm::OccurrenceId aggregate{s.entry, {}};
    const auto &letter = *tools.at(ordered.at(aggregate).definition);
    const auto expanded = expand(tools, letter);
    if (letter.parents.size() == 1)
      equivalent.emplace(aggregate, letter.parents[0].target);
    for (std::size_t i = 0; i < expanded.size(); ++i) {
      const auto &v = expanded[i];
      need(s.parts[i] == warm::OccurrenceId{s.entry, v.path},
           "invalid persisted byte path");
      provenance.emplace(
          s.parts[i],
          ByteProvenance{
              {out.study_, aggregate}, v.character, v.path[0], v.path[1]});
    }
    if (s.exposure == warm::Exposure::Aggregate)
      active.insert(aggregate);
    else
      active.insert(s.parts.begin(), s.parts.end());
  }
  if (b.recipe_version == 1)
    out.input_.capacity = int(ordered.size());
  for (const auto &id : used) {
    const auto &t = *tools.at(id);
    const auto &g =
        *std::find_if(b.geometry.begin(), b.geometry.end(),
                      [&](const auto &v) { return v.object == id; });
    Definition d{warm::key(id), *t.mass, point(g.centroid), {}, {}};
    for (const auto &s : g.spheres)
      d.components.push_back({point(s.centre), s.radius});
    for (const auto &p : t.parents)
      d.parents.push_back(
          {warm::key(p.target.object), p.ordinal, p.contribution});
    out.structural_input_.definitions.push_back(std::move(d));
  }
  for (const auto &[id, o] : ordered) {
    const auto &t = *tools.at(o.definition);
    const OccurrenceKey k{out.study_, id};
    const auto slot = b.recipe_version == 1 ? int(out.occurrences_.size()) : -1;
    ProjectedOccurrence record{k,  t.ref, slot, o.node, active.count(id) != 0,
                               {}, {}};
    if (provenance.count(id)) record.byte = provenance.at(id);
    if (equivalent.count(id)) record.complete_character = equivalent.at(id);
    out.occurrences_.push_back(record);
    out.structural_input_.occurrences.push_back(
        {occurrence_text(k), warm::key(o.definition), o.node});
    if (b.recipe_version == 1) {
      const auto &initial = *legacy.at(id);
      out.input_.occurrences.push_back(
          {occurrence_text(k), warm::key(o.definition), slot, o.node,
           point(initial.position), point(initial.velocity)});
    }
    if (record.active)
      for (const auto &p : t.parents)
        out.parents_.push_back({k,
                                {GroupKind::ParentValue, p.target.object},
                                p.ordinal,
                                p.contribution});
  }
  for (const auto &s : b.selections) {
    Selection selection{occurrence_text({out.study_, {s.entry, {}}}),
                        {},
                        s.exposure == warm::Exposure::Aggregate
                            ? Exposure::Aggregate
                            : Exposure::Parts};
    for (const auto &id : s.parts)
      selection.parts.push_back(occurrence_text({out.study_, id}));
    out.structural_input_.selections.push_back(std::move(selection));
  }
  for (const auto &c : b.contacts)
    out.structural_input_.contacts.push_back(
        {occurrence_text({out.study_, c.a}),
         occurrence_text({out.study_, c.b})});
  for (const auto &f : b.focal_fields)
    out.structural_input_.focal_fields.push_back(
        {static_cast<GroupKind>(f.kind), warm::key(f.object)});
  if (b.recipe_version == 1) {
    out.input_.definitions = out.structural_input_.definitions;
    out.input_.selections = out.structural_input_.selections;
    out.input_.contacts = out.structural_input_.contacts;
  }
  return out;
}

StudyInput LetterProjection::bind(const std::vector<StudyNode> &nodes) const {
  need(recipe_.recipe_version == 1,
       "legacy numerical binding requires recipe v1");
  need(nodes.size() == recipe_.nodes.size(),
       "native layout differs from normalized recipe");
  for (std::size_t i = 0; i < nodes.size(); ++i)
    need(nodes[i].layout && nodes[i].parent == recipe_.nodes[i].parent &&
             nodes[i].role == role(recipe_.nodes[i].role),
         "native binding differs from normalized exposure");
  auto input = input_;
  input.nodes = nodes;
  return input;
}
StructuralStudyInput LetterProjection::bind_structure(
    const std::vector<StudyNode> &nodes) const {
  need(
      recipe_.recipe_version == 2,
      "structural binding requires recipe v2; explicitly adapt legacy recipes");
  need(nodes.size() == recipe_.nodes.size(),
       "native layout differs from normalized recipe");
  for (std::size_t i = 0; i < nodes.size(); ++i)
    need(nodes[i].layout && nodes[i].parent == recipe_.nodes[i].parent &&
             nodes[i].role == role(recipe_.nodes[i].role),
         "native binding differs from normalized exposure");
  auto input = structural_input_;
  input.nodes = nodes;
  return input;
}
}  // namespace assembly
