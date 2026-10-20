#include "snapshot.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>

namespace viewport {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw std::invalid_argument(message);
}
bool same(const assembly::ExecutionHandle &a, const assembly::ExecutionHandle &b) {
  return a.session == b.session && a.study == b.study && a.generation == b.generation;
}
double number(double v) {
  need(std::isfinite(v), "viewport: non-finite report value");
  return v;
}
Json point(double x, double y, double z) {
  return Json::array({number(x), number(y), number(z)});
}
const char *kind(assembly::GroupKind k) {
  switch (k) {
    case assembly::GroupKind::SiblingDefinition: return "sibling_definition";
    case assembly::GroupKind::ParentValue: return "parent_value";
    case assembly::GroupKind::Membership: return "membership";
  }
  throw std::invalid_argument("viewport: unknown group kind");
}
}  // namespace

CaptureContext context(const assembly::ActiveAssembly &a,
                       const assembly::ExecutionHandle &h,
                       std::uint64_t ticks, std::string label) {
  need(h.study == a.study() && h.generation == a.generation() && !h.session.nil(),
       "viewport: capture assembly/handle mismatch");
  CaptureContext out{h, ticks, std::move(label), {}, Json::object()};
  std::map<warm::ObjectKey, std::string> labels;
  for (const auto &t : a.shared_recipe().tools)
    if (t.notation) labels.emplace(t.ref.object, *t.notation);
  std::map<std::string, DisplayContext> selected;
  for (const auto &s : a.shared_recipe().selections) {
    const auto aggregate = assembly::occurrence_text({a.study(), {s.entry, {}}});
    if (s.exposure == warm::Exposure::Aggregate)
      selected.emplace(aggregate, DisplayContext{"aggregate", aggregate, {}});
    else
      for (const auto &p : s.parts)
        selected.emplace(assembly::occurrence_text({a.study(), p}),
                         DisplayContext{"parts", aggregate, {}});
  }
  for (const auto &o : a.origins()) {
    const auto id = assembly::occurrence_text(o.occurrence);
    if (o.local) selected.emplace(id, DisplayContext{"standalone", id, {}});
    auto found = selected.find(id);
    if (found != selected.end()) {
      found->second.label = labels[o.definition];
      out.occurrences.emplace(id, found->second);
    }
  }
  return out;
}

Json snapshot(const assembly::ActiveReport &r, const CaptureContext &c) {
  need(same(r.handle, c.handle) && !r.handle.session.nil() &&
           !r.handle.study.nil() && !r.handle.generation.nil(),
       "viewport: capture/report handle mismatch");
  need(c.source.is_object(), "viewport: source metadata must be an object");
  const auto &p = r.prepared;
  need(p.capacity >= 0 && !p.scales.empty(), "viewport: invalid prepared shape");
  const auto N = std::size_t(p.capacity), G = p.groups.size();
  need(r.particles.size() == std::size_t(field::kParticleFieldCount) * std::max(N, std::size_t(1)) &&
           r.groups.size() == std::size_t(field::kGroupFieldCount) * std::max(G, std::size_t(1)),
       "viewport: report scalar shape mismatch");
  Json scales = Json::array();
  std::vector<int> levels;
  for (std::size_t i = 0; i < p.scales.size(); ++i) {
    const auto &s = p.scales[i];
    int level = 0;
    std::string role;
    if (i == 0) {
      need(s.node.parent == -1 && s.node.role == assembly::ScaleRole::Root,
           "viewport: invalid conceptual root");
      role = "root";
    } else {
      need(s.node.parent >= 0 && std::size_t(s.node.parent) < i,
           "viewport: invalid conceptual parent");
      level = levels.at(s.node.parent);
      switch (s.node.role) {
        case assembly::ScaleRole::PrimaryStep: role = "primary_step"; ++level; break;
        case assembly::ScaleRole::SecondaryCompression: role = "secondary_compression"; break;
        case assembly::ScaleRole::NamingEquivalent: role = "naming_equivalent"; break;
        default: throw std::invalid_argument("viewport: invalid conceptual role");
      }
    }
    need(number(s.units) > 0, "viewport: invalid operative units");
    levels.push_back(level);
    scales.push_back({{"id", i}, {"parent", s.node.parent}, {"role", role},
                      {"units", s.units}, {"level", level}});
  }
  std::map<std::string, const assembly::Origin *> origins;
  for (const auto &o : r.origins)
    need(origins.emplace(assembly::occurrence_text(o.occurrence), &o).second,
         "viewport: duplicate occurrence origin");
  Json particles = Json::array();
  std::set<int> slots;
  std::set<std::string> ids;
  for (const auto &v : p.particles) {
    const auto &o = v.occurrence;
    need(o.slot >= 0 && std::size_t(o.slot) < N && slots.insert(o.slot).second &&
             ids.insert(o.id).second && origins.count(o.id) && c.occurrences.count(o.id) &&
             o.study_node >= 0 && std::size_t(o.study_node) < levels.size(),
         "viewport: invalid exposed occurrence mapping");
    const auto scalar = [&](int f) { return number(r.particles.at(std::size_t(f) * N + o.slot)); };
    const auto mass = scalar(field::kMass), radius = scalar(field::kRadius);
    need(mass > 0 && radius >= 0 && v.exact_mass > 0,
         "viewport: invalid particle mass/extent");
    const auto &d = c.occurrences.at(o.id);
    need(d.exposure == "aggregate" || d.exposure == "parts" || d.exposure == "standalone",
         "viewport: invalid supplied exposure");
    particles.push_back({{"id", o.id}, {"definition", warm::key(origins.at(o.id)->definition)},
      {"slot", o.slot}, {"position", point(scalar(field::kPosX), scalar(field::kPosY), scalar(field::kPosZ))},
      {"radius", radius}, {"mass", mass}, {"exact_mass", std::to_string(v.exact_mass)},
      {"binding", o.study_node}, {"level", levels.at(o.study_node)},
      {"exposure", d.exposure}, {"aggregate", d.aggregate}, {"label", d.label}});
  }
  need(c.occurrences.size() == particles.size(), "viewport: exposure coverage mismatch");
  Json focal = Json::array();
  std::array<double, 3> target{};
  double total = 0;
  std::set<int> focus_slots;
  for (const int g : p.focal_groups) {
    need(g >= 0 && std::size_t(g) < G && focus_slots.insert(g).second,
         "viewport: invalid focal field");
    const auto value = [&](int f) { return number(r.groups.at(std::size_t(f) * G + g)); };
    const auto mass = value(field::kCenM);
    need(mass > 0, "viewport: invalid focal mass");
    const std::array<double, 3> centre{value(field::kCenX), value(field::kCenY), value(field::kCenZ)};
    // Weighted offsets avoid multiplying an unnecessarily large coordinate.
    if (total == 0) target = centre;
    const double next = number(total + mass);
    for (int axis = 0; axis < 3; ++axis)
      target[axis] = number(target[axis] + (centre[axis] - target[axis]) * (mass / next));
    total = next;
    focal.push_back({{"kind", kind(p.groups[g].kind)}, {"value", p.groups[g].value},
                    {"position", point(centre[0], centre[1], centre[2])}, {"mass", mass}});
  }
  if (p.focal_groups.empty())
    for (int axis = 0; axis < 3; ++axis) target[axis] = -number(r.frame_anchor[axis]);
  return {{"schema", "hcp-study-snapshot"}, {"version", 1},
    {"capture", {{"session", r.handle.session.str()}, {"study", r.handle.study.str()},
      {"generation", r.handle.generation.str()}, {"tick", std::to_string(c.tick)},
      {"phase", c.tick == 0 ? "published_seeded" : "post_tick"}, {"label", c.label}}},
    {"frame", {{"coordinates", "reference_local"}, {"units", "operative_model_units"},
      {"reference", point(r.frame_anchor[0], r.frame_anchor[1], r.frame_anchor[2])},
      {"camera_target", point(target[0], target[1], target[2])},
      {"target_kind", p.focal_groups.empty() ? "legacy_origin" : "captured_focal_centroid"},
      {"focal_fields", focal}}}, {"bindings", scales}, {"particles", particles}, {"source", c.source}};
}
Json capture(assembly::ActiveSession &session, const warm::Uuid &owner, const CaptureContext &c) {
  return snapshot(session.report(c.handle, owner), c);
}
void write(const std::string &path, const Json &value) {
  const auto text = value.dump(2);  // Finite numbers, UTF-8 validation and proper escaping.
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.exceptions(std::ios::failbit | std::ios::badbit);
  out << text << '\n';
  out.close();
}
}  // namespace viewport
