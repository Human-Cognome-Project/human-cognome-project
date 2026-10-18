#include "assembly/study.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace assembly {
namespace {

void require(bool condition, const char *message) {
  if (!condition) throw std::invalid_argument(message);
}

void finite(Point p) {
  require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z),
          "assembly: non-finite coordinates");
}

float numerical(double value) {
  require(std::isfinite(value) && std::abs(value) <= std::numeric_limits<float>::max(),
          "assembly: value outside finite f32 range");
  return static_cast<float>(value);
}

Point convert(Point p, double units) {
  finite(p);
  return {numerical(p.x * units), numerical(p.y * units), numerical(p.z * units)};
}

Point placed(Point p, double units) {
  finite(p);
  Point result{p.x * units, p.y * units, p.z * units};
  finite(result);
  return result;
}

int count(std::size_t size) {
  require(size <= std::size_t(std::numeric_limits<int>::max()),
          "assembly: logical count exceeds i32 axis");
  return static_cast<int>(size);
}

std::uint64_t add_mass(std::uint64_t a, std::uint64_t b) {
  require(b <= std::numeric_limits<std::uint64_t>::max() - a,
          "assembly: exact mass overflow");
  return a + b;
}

std::vector<ScaleBinding> scale_bindings(const std::vector<StudyNode> &nodes) {
  std::vector<ScaleBinding> scales;
  std::set<const taichi::lang::SNode *> seen;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const auto &node = nodes[i];
    require(node.layout && seen.insert(node.layout).second,
            "assembly: missing or duplicate SNode binding");
    double units = 1;
    if (i == 0) {
      require(node.parent == -1 && node.role == ScaleRole::Root &&
                  node.layout->type == taichi::lang::SNodeType::root,
              "assembly: first binding must be the study root");
    } else {
      require(node.parent >= 0 && std::size_t(node.parent) < i,
              "assembly: scale parent must precede child");
      const auto &parent = scales[node.parent];
      require(std::any_of(parent.node.layout->ch.begin(), parent.node.layout->ch.end(),
                          [&](const auto &child) { return child.get() == node.layout; }),
              "assembly: binding does not follow the SNode structure");
      units = parent.units;
      switch (node.role) {
        case ScaleRole::PrimaryStep: units *= 64; break;
        case ScaleRole::SecondaryCompression:
        case ScaleRole::NamingEquivalent: break;
        default: throw std::invalid_argument("assembly: invalid child scale role");
      }
      numerical(units);
    }
    scales.push_back({node, units});
  }
  return scales;
}

}  // namespace

double enclosing_radius(Point centroid, const std::vector<Sphere> &components) {
  finite(centroid);
  require(!components.empty(), "assembly: supplied component geometry required");
  double radius = 0;
  for (const auto &component : components) {
    finite(component.centre);
    require(std::isfinite(component.radius) && component.radius >= 0,
            "assembly: invalid component radius");
    const double distance = std::hypot(component.centre.x - centroid.x,
                                       component.centre.y - centroid.y,
                                       component.centre.z - centroid.z);
    const double extent = distance + component.radius;
    require(std::isfinite(extent), "assembly: enclosure overflow");
    radius = std::max(radius, extent);
  }
  return radius;
}

PreparedStudy prepare(const StudyInput &input) {
  require(input.capacity >= 0, "assembly: negative capacity");
  PreparedStudy target;
  target.capacity = input.capacity;
  target.scales = scale_bindings(input.nodes);
  std::map<std::string, const Definition *> definitions;
  std::map<std::string, double> radii;
  for (const auto &definition : input.definitions) {
    require(!definition.id.empty() && definitions.emplace(definition.id, &definition).second,
            "assembly: empty or duplicate definition identity");
    require(definition.mass > 0, "assembly: active definitions need positive exact mass");
    radii.emplace(definition.id, enclosing_radius(definition.centroid, definition.components));
    std::set<std::uint32_t> ordinals;
    std::uint64_t parent_mass = 0;
    for (const auto &parent : definition.parents) {
      require(!parent.value.empty() && parent.count > 0 &&
                  ordinals.insert(parent.ordinal).second,
              "assembly: invalid ordered parent occurrence");
      parent_mass = add_mass(parent_mass, parent.count);
    }
    require(definition.parents.empty() || parent_mass == definition.mass,
            "assembly: parent counts must account for exact structural mass");
  }

  std::map<std::string, const Occurrence *> occurrences;
  std::set<int> slots;
  for (const auto &occurrence : input.occurrences) {
    require(!occurrence.id.empty() && occurrences.emplace(occurrence.id, &occurrence).second,
            "assembly: empty or duplicate occurrence identity");
    require(definitions.count(occurrence.definition) != 0,
            "assembly: unknown definition");
    require(occurrence.slot >= 0 && occurrence.slot < input.capacity &&
                slots.insert(occurrence.slot).second,
            "assembly: invalid or shared particle slot");
    require(occurrence.study_node >= 0 &&
                std::size_t(occurrence.study_node) < target.scales.size(),
            "assembly: missing occurrence scale binding");
    finite(occurrence.position);
    finite(occurrence.velocity);
  }
  auto occurrence_at = [&](const std::string &id) -> const Occurrence & {
    auto it = occurrences.find(id);
    require(it != occurrences.end(), "assembly: unknown occurrence reference");
    return *it->second;
  };
  std::set<std::string> selected_members, inactive;
  for (const auto &selection : input.selections) {
    const auto &aggregate = occurrence_at(selection.aggregate);
    require(!selection.parts.empty() && selected_members.insert(aggregate.id).second,
            "assembly: empty or overlapping exposure selection");
    require(selection.exposure == Exposure::Aggregate || selection.exposure == Exposure::Parts,
            "assembly: invalid exposure choice");
    std::uint64_t part_mass = 0;
    for (const auto &id : selection.parts) {
      const auto &part = occurrence_at(id);
      require(selected_members.insert(id).second,
              "assembly: occurrence selected in more than one exposure");
      part_mass = add_mass(part_mass, definitions.at(part.definition)->mass);
      if (selection.exposure == Exposure::Aggregate) inactive.insert(id);
    }
    require(part_mass == definitions.at(aggregate.definition)->mass,
            "assembly: aggregate/part exact mass mismatch");
    if (selection.exposure == Exposure::Parts) inactive.insert(aggregate.id);
  }

  std::map<std::pair<GroupKind, std::string>, int> groups;
  auto group_for = [&](GroupKind kind, const std::string &value) {
    auto result = groups.emplace(std::make_pair(kind, value), count(target.groups.size()));
    if (result.second) target.groups.push_back({kind, value});
    return result.first->second;
  };
  std::map<std::string, int> active_slots;
  for (const auto &occurrence : input.occurrences) {
    if (inactive.count(occurrence.id)) continue;
    const auto &definition = *definitions.at(occurrence.definition);
    const double units = target.scales[occurrence.study_node].units;
    const double extent = radii.at(definition.id) * units;
    float radius = numerical(extent);
    // Outward rounding preserves enclosure when storing a prepared f32 radius.
    if (radius < extent) radius = std::nextafter(radius, std::numeric_limits<float>::infinity());
    require(std::isfinite(radius), "assembly: radius outside finite f32 range");
    target.particles.push_back({occurrence, definition.mass,
                               placed(occurrence.position, units),
                               convert(occurrence.velocity, units),
                               numerical(double(definition.mass)), radius});
    active_slots.emplace(occurrence.id, occurrence.slot);
    const int sibling = group_for(GroupKind::SiblingDefinition, definition.id);
    target.participation.push_back({occurrence.id, occurrence.slot, sibling, "", 0,
                                   definition.mass, 1.0f});
    for (const auto &parent : definition.parents) {
      const int group = group_for(GroupKind::ParentValue, parent.value);
      const float share = numerical(double(parent.count) / double(definition.mass));
      target.participation.push_back({occurrence.id, occurrence.slot, group, parent.value,
                                     parent.ordinal, parent.count, share});
    }
  }
  std::set<std::pair<std::string, std::string>> memberships;
  for (const auto &membership : input.memberships) {
    require(!membership.group.empty() && active_slots.count(membership.occurrence),
            "assembly: membership needs a named field and active occurrence");
    if (!memberships.emplace(membership.occurrence, membership.group).second)
      continue;
    const auto &occurrence = occurrence_at(membership.occurrence);
    const auto mass = definitions.at(occurrence.definition)->mass;
    const int group = group_for(GroupKind::Membership, membership.group);
    target.participation.push_back({occurrence.id, occurrence.slot, group, "", 0,
                                   mass, 1.0f});
  }
  std::set<std::pair<int, int>> pairs;
  for (const auto &contact : input.contacts) {
    require(active_slots.count(contact.a) && active_slots.count(contact.b),
            "assembly: contact must reference active occurrences");
    const int a = active_slots.at(contact.a), b = active_slots.at(contact.b);
    require(a != b && pairs.emplace(std::min(a, b), std::max(a, b)).second,
            "assembly: self or duplicate contact");
    target.contacts.push_back({a, b});
  }
  count(target.participation.size());
  count(target.contacts.size());
  std::set<int> focal;
  for (const auto &field : input.focal_fields) {
    const auto found = groups.find({field.kind, field.value});
    require(found != groups.end() && focal.insert(found->second).second,
            "assembly: focus requires distinct exposed fields");
    target.focal_groups.push_back(found->second);
  }
  if (!target.focal_groups.empty()) {
    const auto member = std::find_if(target.participation.begin(), target.participation.end(),
        [&](const auto &e) { return e.group == target.focal_groups.front(); });
    const auto particle = std::find_if(target.particles.begin(), target.particles.end(),
        [&](const auto &p) { return p.occurrence.slot == member->slot; });
    target.frame_anchor = particle->position;
  }
  // Choose the numerical reference before f32 conversion. Original supplied
  // placements remain in Occurrence; no universal absolute 3D map is needed.
  for (auto &p : target.particles) {
    p.position = {numerical(p.position.x - target.frame_anchor.x),
                  numerical(p.position.y - target.frame_anchor.y),
                  numerical(p.position.z - target.frame_anchor.z)};
  }
  return target;
}

FixedStudy::FixedStudy(engine::Runtime &runtime, const StudyInput &input,
                       const field::Parameters &parameters, field::HostPath path)
    : prepared_(prepare(input)),
      fields_(std::make_unique<field::Harness>(runtime, prepared_.capacity,
              count(prepared_.groups.size()), count(prepared_.participation.size()),
              count(prepared_.contacts.size()), path)) {
  stage(prepared_, parameters);
}

void FixedStudy::stage(const PreparedStudy &target, const field::Parameters &parameters) {
  require(target.capacity == fields_->particle_count() &&
              count(target.groups.size()) == fields_->group_count() &&
              count(target.participation.size()) == fields_->edge_count() &&
              count(target.contacts.size()) == fields_->contact_count(),
          "assembly: refresh changes fixed-study counts");
  std::fill(fields_->particle_data.begin(), fields_->particle_data.end(), 0.0f);
  std::fill(fields_->group_data.begin(), fields_->group_data.end(), 0.0f);
  for (const auto &particle : target.particles) {
    const int slot = particle.occurrence.slot;
    fields_->particle(field::kPosX, slot) = numerical(particle.position.x);
    fields_->particle(field::kPosY, slot) = numerical(particle.position.y);
    fields_->particle(field::kPosZ, slot) = numerical(particle.position.z);
    fields_->particle(field::kVelX, slot) = numerical(particle.velocity.x);
    fields_->particle(field::kVelY, slot) = numerical(particle.velocity.y);
    fields_->particle(field::kVelZ, slot) = numerical(particle.velocity.z);
    fields_->particle(field::kMass, slot) = particle.mass;
    fields_->particle(field::kRadius, slot) = particle.radius;
  }
  for (std::size_t i = 0; i < target.participation.size(); ++i) {
    const auto &edge = target.participation[i];
    fields_->edge_int(field::kEdgeParticle, int(i)) = edge.slot;
    fields_->edge_int(field::kEdgeGroup, int(i)) = edge.group;
    fields_->edge_float(field::kShare, int(i)) = edge.share;
  }
  for (std::size_t i = 0; i < target.contacts.size(); ++i) {
    fields_->contact_int(field::kContactA, int(i)) = target.contacts[i].a;
    fields_->contact_int(field::kContactB, int(i)) = target.contacts[i].b;
  }
  fields_->publish(parameters, {target.frame_anchor.x, target.frame_anchor.y,
                                target.frame_anchor.z});
  fields_->set_focus(target.focal_groups);
}

void FixedStudy::publish(const StudyInput &input, const field::Parameters &parameters) {
  error_.clear();
  try {
    fields_->invalidate();
    auto target = prepare(input);
    stage(target, parameters);
    prepared_ = std::move(target);
  } catch (const std::exception &failure) {
    error_ = failure.what();
    throw;
  }
}

}  // namespace assembly
