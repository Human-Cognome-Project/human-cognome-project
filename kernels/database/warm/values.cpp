#include "values.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <tuple>

namespace warm {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw Error(message);
}
void identity(const Uuid &id) { need(!id.nil(), "nil non-scope identity"); }
void exact(std::uint64_t n) {
  need(n <= std::uint64_t(std::numeric_limits<std::int64_t>::max()),
       "exact integer overflow");
}
void point(Point p) {
  need(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z),
       "non-finite initial geometry/state");
}
bool text_value(const std::string &value) {
  return value.find('\0') == std::string::npos;
}
std::string ref_key(const ToolRef &r) {
  return key(r.object) + "/" + r.revision.str();
}
void cycles(const std::map<std::string, const Tool *> &tools, bool membership) {
  std::map<std::string, int> state;
  std::function<void(const std::string &)> visit = [&](const std::string &id) {
    need(state[id] != 1, "cyclic construction/membership");
    if (state[id] == 2) return;
    state[id] = 1;
    const auto &tool = *tools.at(id);
    if (membership)
      for (const auto &group : tool.member_of) visit(key(group.object));
    else
      for (const auto &parent : tool.parents) visit(key(parent.target.object));
    state[id] = 2;
  };
  for (const auto &[id, unused] : tools) visit(id);
}
}  // namespace

Uuid::Uuid() : value_("00000000-0000-0000-0000-000000000000") {}
Uuid::Uuid(std::string value) : value_(std::move(value)) {
  need(value_.size() == 36, "malformed UUID");
  for (std::size_t i = 0; i < value_.size(); ++i) {
    auto &c = value_[i];
    if (i == 8 || i == 13 || i == 18 || i == 23)
      need(c == '-', "malformed UUID");
    else {
      if (c >= 'A' && c <= 'F') c = char(c - 'A' + 'a');
      need((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'), "malformed UUID");
    }
  }
}
bool Uuid::nil() const { return *this == Uuid(); }
Uuid new_uuid() {
  std::random_device random;
  std::string value = "00000000-0000-4000-8000-000000000000";
  for (std::size_t i = 0; i < value.size(); ++i)
    if (value[i] != '-' && i != 14 && i != 19)
      value[i] = "0123456789abcdef"[random() & 15];
  return Uuid(value);
}
std::string address_text(const codec::Address &address) {
  need(!address.empty() && codec::is_valid_address(address) &&
           !address.back().partial,
       "invalid, empty or wildcard object address");
  return *codec::encode_token_id(address);
}
std::string address_array(const codec::Address &address) {
  const auto text = address_text(address);
  std::string out = "{";
  for (std::size_t i = 0; i < text.size(); i += 3) {
    if (i) out += ',';
    out += text.substr(i, 2);
  }
  return out + "}";
}
std::string key(const ObjectKey &o) {
  return o.scope.str() + "/" + address_text(o.address);
}
bool operator==(const ObjectKey &a, const ObjectKey &b) {
  return a.scope == b.scope && a.address == b.address;
}
bool operator<(const ObjectKey &a, const ObjectKey &b) {
  return key(a) < key(b);
}
bool operator==(const ToolRef &a, const ToolRef &b) {
  return a.object == b.object && a.revision == b.revision;
}
bool operator==(const OccurrenceId &a, const OccurrenceId &b) {
  return a.entry == b.entry && a.path == b.path;
}
bool operator<(const OccurrenceId &a, const OccurrenceId &b) {
  return std::tie(a.entry, a.path) < std::tie(b.entry, b.path);
}
bool contains(const ObjectKey &prefix, const ObjectKey &o) {
  key(prefix);
  key(o);
  return prefix.scope == o.scope && prefix.address.size() <= o.address.size() &&
         std::equal(prefix.address.begin(), prefix.address.end(),
                    o.address.begin());
}
std::string tool_body(const Tool &t) {
  std::ostringstream out;
  out << key(t.ref.object) << ' ' << t.epoch.str() << ' '
      << std::quoted(t.recipe) << ' ';
  out << bool(t.notation) << ' ' << std::quoted(t.notation.value_or("")) << ' ';
  out << bool(t.mass) << ' ' << t.mass.value_or(0) << '\n';
  for (const auto &p : t.parents)
    out << p.ordinal << ' ' << ref_key(p.target) << ' ' << bool(p.stored_mass)
        << ' ' << p.stored_mass.value_or(0) << ' ' << p.contribution << '\n';
  std::set<std::string> groups;
  for (const auto &g : t.member_of) groups.insert(ref_key(g));
  for (const auto &g : groups) out << g << '\n';
  return out.str();
}
void validate_directory(const std::vector<Binding> &bindings) {
  const Binding *core = nullptr, *ling = nullptr;
  for (const auto &b : bindings) {
    need(b.kind == BindingKind::Shared || b.kind == BindingKind::Personal ||
             b.kind == BindingKind::Temporary,
         "invalid binding kind");
    const auto address = address_text(b.prefix.address);
    identity(b.epoch);
    identity(b.policy);
    need(!b.source_handle.empty() && text_value(b.source_handle) &&
             !b.writers.empty(),
         "incomplete binding");
    std::set<Uuid> writers;
    for (const auto &w : b.writers) {
      identity(w);
      need(writers.insert(w).second, "duplicate writer");
    }
    if (b.kind == BindingKind::Shared) {
      need(b.prefix.scope.nil() && (address == "00" || address == "02"),
           "unassigned shared root");
      if (address == "00")
        core = &b;
      else
        ling = &b;
    } else {
      need(!b.prefix.scope.nil(),
           "personal/temporary binding needs owning instance");
      need(address[0] == (b.kind == BindingKind::Personal ? 'y' : 'z'),
           "wrong reserved prefix family");
    }
    if (b.kind != BindingKind::Temporary) {
      need(b.snapshot_hash && b.snapshot_hash->size() == 64,
           "missing validated snapshot hash");
      for (char c : *b.snapshot_hash)
        need((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'),
             "invalid snapshot hash");
    } else
      need(!b.snapshot_hash, "temporary lifetime is not a source snapshot");
    for (const auto &other : bindings)
      if (&b != &other)
        need(!contains(b.prefix, other.prefix), "overlapping root bindings");
  }
  need(core && ling && core->epoch == ling->epoch,
       "incompatible or incomplete source pair");
}
void validate(const Bundle &b) {
  identity(b.ref.selection);
  identity(b.ref.revision);
  identity(b.owner);
  identity(b.creator);
  need(b.recipe_version == 1, "unsupported recipe version");
  std::map<std::string, const Tool *> tools;
  for (const auto &t : b.tools) {
    identity(t.ref.revision);
    identity(t.epoch);
    identity(t.author);
    need(!t.recipe.empty() && text_value(t.recipe) &&
             (!t.notation || text_value(*t.notation)) &&
             tools.emplace(key(t.ref.object), &t).second,
         "duplicate/incomplete tool or NUL text");
    if (t.mass) exact(*t.mass);
  }
  auto target = [&](const ToolRef &r) -> const Tool & {
    auto it = tools.find(key(r.object));
    need(it != tools.end() && it->second->ref == r,
         "missing or inconsistent dependency revision");
    return *it->second;
  };
  for (const auto &[unused, ptr] : tools) {
    const auto &t = *ptr;
    auto dependency = [&](const ToolRef &r) -> const Tool & {
      need(r.object.scope.nil() || r.object.scope == t.ref.object.scope,
           "private dependency escape");
      return target(r);
    };
    std::uint64_t total = 0;
    for (std::size_t i = 0; i < t.parents.size(); ++i) {
      const auto &p = t.parents[i];
      const auto &parent = dependency(p.target);
      exact(p.contribution);
      need(p.ordinal == i && p.contribution > 0 && parent.mass &&
               *parent.mass == p.contribution,
           "invalid ordinal or resolved contribution");
      if (p.stored_mass) {
        exact(*p.stored_mass);
        need(*p.stored_mass == p.contribution, "conflicting parent-row mass");
      }
      need(p.contribution <= std::uint64_t(INT64_MAX) - total,
           "mass sum overflow");
      total += p.contribution;
    }
    if (!t.parents.empty())
      need(t.mass && total == *t.mass, "construction mass mismatch");
    std::set<std::string> groups;
    for (const auto &g : t.member_of) {
      dependency(g);
      need(groups.insert(key(g.object)).second, "duplicate direct membership");
    }
  }
  cycles(tools, false);
  cycles(tools, true);
  std::set<std::string> geometry;
  for (const auto &g : b.geometry) {
    need(tools.count(key(g.object)) && geometry.insert(key(g.object)).second &&
             !g.spheres.empty(),
         "invalid geometry owner/layout");
    point(g.centroid);
    for (const auto &s : g.spheres) {
      point(s.centre);
      need(std::isfinite(s.radius) && s.radius >= 0, "invalid sphere");
    }
  }
  need(!b.occurrences.empty() && b.occurrences.size() <= std::size_t(INT32_MAX),
       "empty/oversized recipe");
  std::map<OccurrenceId, const Occurrence *> occurrences;
  for (const auto &o : b.occurrences) {
    need((o.id.path.empty() || o.id.path.size() == 2) &&
             occurrences.emplace(o.id, &o).second,
         "invalid/duplicate occurrence path");
    const auto it = tools.find(key(o.definition));
    need(it != tools.end() && it->second->mass && *it->second->mass > 0 &&
             geometry.count(key(o.definition)),
         "missing live mass/geometry");
    point(o.position);
    point(o.velocity);
  }
  std::set<OccurrenceId> selected, active;
  bool parts_exposed = false;
  for (const auto &s : b.selections) {
    need(s.exposure == Exposure::Aggregate || s.exposure == Exposure::Parts,
         "invalid exposure");
    const OccurrenceId aggregate{s.entry, {}};
    need(occurrences.count(aggregate) && selected.insert(aggregate).second &&
             !s.parts.empty(),
         "missing/overlapping aggregate");
    const auto &letter = *tools.at(key(occurrences.at(aggregate)->definition));
    std::vector<OccurrenceId> expected;
    for (const auto &character : letter.parents) {
      const auto &c = target(character.target);
      const bool ascii_byte = c.mass && *c.mass == 2;
      const std::size_t count = ascii_byte ? 1 : c.parents.size();
      need(count > 0 && count <= UINT32_MAX, "missing byte construction");
      for (std::size_t byte = 0; byte < count; ++byte) {
        const OccurrenceId id{s.entry,
                              {character.ordinal, std::uint32_t(byte)}};
        need(occurrences.count(id) && selected.insert(id).second,
             "missing/overlapping byte part");
        const auto &byte_tool = *tools.at(key(occurrences.at(id)->definition));
        const auto &byte_ref = ascii_byte ? c.ref : c.parents[byte].target;
        need(byte_tool.ref == byte_ref &&
                 byte_tool.mass == std::optional<std::uint64_t>(2) &&
                 byte_tool.parents.size() == 2 &&
                 byte_tool.parents[0].contribution == 1 &&
                 byte_tool.parents[1].contribution == 1,
             "part is not the recorded byte position");
        expected.push_back(id);
      }
    }
    need(expected == s.parts, "missing, reordered or extraneous recipe parts");
    if (s.exposure == Exposure::Aggregate)
      active.insert(aggregate);
    else {
      parts_exposed = true;
      active.insert(s.parts.begin(), s.parts.end());
    }
  }
  need(selected.size() == occurrences.size(),
       "stray occurrence outside selections");
  need(b.nodes.size() == (parts_exposed ? 2u : 1u) && b.nodes[0].parent == -1 &&
           b.nodes[0].role == ScaleRole::Root,
       "invalid normalized root layout");
  if (parts_exposed)
    need(b.nodes[1].parent == 0 && b.nodes[1].role == ScaleRole::PrimaryStep,
         "invalid primary letter binding");
  for (const auto &o : b.occurrences)
    need(o.node == (parts_exposed && o.id.path.empty() ? 1 : 0),
         "binding inconsistent with supplied exposure");
  std::set<std::pair<OccurrenceId, OccurrenceId>> contacts;
  for (const auto &c : b.contacts) {
    need(active.count(c.a) && active.count(c.b) && !(c.a == c.b),
         "inactive/missing/self contact");
    need(contacts.emplace(std::min(c.a, c.b), std::max(c.a, c.b)).second,
         "duplicate contact");
  }
}
}  // namespace warm
