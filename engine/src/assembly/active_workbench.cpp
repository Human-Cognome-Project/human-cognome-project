#include "assembly/projection_access.h"
#include "assembly/active_assembly.h"
#include "assembly/intrinsic_geometry.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace assembly {
namespace {
void require(bool ok) {
  if (!ok) throw warm::Error("invalid or incomplete active input");
}
Point point(warm::Point p) { return {p.x, p.y, p.z}; }
void local_ref(const LocalRef &r, const warm::Uuid &instance,
               const warm::Uuid &owner) {
  require(
      !r.object.scope.nil() && r.object.scope == instance && r.owner == owner &&
      !r.version.nil() && r.object.address.size() == 5 &&
      codec::decode_token_id(warm::address_text(r.object.address)).has_value());
  for (const auto &part : r.object.address) require(!part.partial);
}
std::string local_name(const LocalRef &r) {
  return warm::key(
      r.object);  // Version stays in provenance, not field identity.
}
std::string occurrence(const warm::Uuid &study, warm::OccurrenceId id) {
  return occurrence_text({study, std::move(id)});
}
}  // namespace

const warm::ObjectKey &Origin::definition() const {
  if (shared.has_value() == local.has_value())
    throw warm::Error("origin requires exactly one provenance pin");
  const auto &object = shared ? shared->object : local->object;
  const auto complete = [](const warm::Uuid &v) { return v.str().size() == 36 && !v.nil(); };
  if ((shared && (!object.scope.nil() || !complete(shared->revision))) ||
      (local && (!complete(object.scope) || !complete(local->version) || !complete(local->owner))) ||
      object.address.size() != 5)
    throw warm::Error("origin requires a complete provenance pin");
  for (const auto &part : object.address)
    if (part.partial || !codec::is_valid_element(part))
      throw warm::Error("origin requires a complete definition address");
  return object;
}

ActiveAssembly::ActiveAssembly(LetterProjection p, warm::Uuid owner,
                               bool required, std::optional<LocalInput> local)
    : projection_(std::move(p)),
      memberships_(membership_fields(projection_)),
      owner_(std::move(owner)),
      generation_(warm::new_uuid()),
      requires_local_(required),
      local_(std::move(local)) {
  require(!owner_.nil());
  for (const auto &t : projection_.recipe().tools)
    require(t.ref.object.scope.nil());
  if (!local_) return;
  local_ref(local_->source, instance(), owner_);
  std::map<warm::ObjectKey, LocalRef> refs;
  const auto register_ref = [&](const LocalRef &r) {
    local_ref(r, instance(), owner_);
    const auto found = refs.emplace(r.object, r);
    require(found.second || (found.first->second.version == r.version &&
                             found.first->second.owner == r.owner));
  };
  register_ref(local_->source);
  std::set<std::uint64_t> entries;
  for (const auto &o : detail::ProjectionAccess::rows(projection_))
    entries.insert(o.source.entry);
  std::set<warm::ObjectKey> definitions;
  for (const auto &v : local_->values) {
    register_ref(v.ref);
    require(v.mass > 0 &&
            v.mass <= std::uint64_t(std::numeric_limits<std::int64_t>::max()) &&
            definitions.insert(v.ref.object).second &&
            entries.insert(v.entry).second);
    detail::intrinsic_enclosing_radius(v.centroid, v.components);
  }
  for (const auto &e : local_->context) register_ref(e.target);
}
ActiveAssembly ActiveAssembly::assemble(warm::Bundle shared, warm::Uuid owner,
                                        bool required,
                                        std::optional<LocalInput> local, const warm::Uuid &study) {
  return ActiveAssembly(project_letters(shared,study), std::move(owner), required,
                        std::move(local));
}
ActiveAssembly ActiveAssembly::revise(std::optional<LocalInput> local) const {
  auto revised = ActiveAssembly(projection_, owner_, requires_local_, std::move(local));
  revised.focal_fields_ = focal_fields_;
  revised.focus_override_ = focus_override_;
  return revised;
}
ActiveAssembly ActiveAssembly::expose_all(warm::Exposure target) const {
  require(!local_ && inputs_available() && shared_recipe().recipe_version == 2 &&
      (target == warm::Exposure::Aggregate || target == warm::Exposure::Parts));
  detail::PreparationWork work(UINT64_MAX);
  auto current=LetterProjection(projection_,{},LetterProjection::OwnerSpare{});
  auto next=LetterProjection(projection_,{},LetterProjection::OwnerSpare{});
  current.reserve_exposure(work); next.reserve_exposure(work);
  for(const auto &selection:shared_recipe().selections) {
    if(current.exposure(selection.entry)==target) continue;
    current.prepare_exposure(next,{study(),{selection.entry,{}}},target,work);
    std::swap(current,next);
  }
  auto out=ActiveAssembly(std::move(current),owner_,requires_local_,{});
  out.focal_fields_=focal_fields_;out.focus_override_=focus_override_;
  return out;
}
ActiveAssembly ActiveAssembly::restart() const {
  auto rebuilt = assemble(projection_.recipe(), owner_, requires_local_, local_);
  rebuilt.focal_fields_ = focal_fields_;
  rebuilt.focus_override_ = focus_override_;
  return rebuilt;
}
ActiveAssembly ActiveAssembly::focus(std::vector<FocalField> fields) const {
  auto focused = revise(local_);
  focused.focal_fields_ = std::move(fields);
  focused.focus_override_ = true;
  return focused;
}
std::vector<Origin> ActiveAssembly::origins() const {
  std::vector<Origin> out;
  for (std::size_t i=0;i<projection_.occurrence_count();++i) {
    const auto &o=detail::ProjectionAccess::row(projection_,i);
    auto study=projection_.study();
    auto source=o.source;
    auto definition=detail::ProjectionAccess::definition(projection_,i);
    out.push_back({{std::move(study),std::move(source)},std::move(definition),{}});
  }
  if (local_)
    for (const auto &v : local_->values)
      out.push_back({{study(), {v.entry, {}}}, {}, v.ref});
  return out;
}
StructuralStudyInput ActiveAssembly::bind_structure(const std::vector<StudyNode> &nodes) const {
  require(inputs_available());
  if (local_)
    throw warm::Error("structural active binding does not yet support local overlays");
  auto input = memberships_.bind_structure(projection_, nodes);
  // An explicit empty override clears recipe focus. No override preserves it.
  if (focus_override_)
    input.focal_fields.assign(focal_fields_.begin(), focal_fields_.end());
  return input;
}
StudyInput ActiveAssembly::bind(const std::vector<StudyNode> &nodes) const {
  require(inputs_available());
  auto input = memberships_.bind(projection_, nodes);
  input.focal_fields.assign(focal_fields_.begin(),focal_fields_.end());
  if (!local_) return input;
  for (const auto &v : local_->values) {
    require(input.capacity < std::numeric_limits<int>::max());
    const auto id = occurrence(study(), {v.entry, {}});
    input.definitions.push_back(
        {InputText(local_name(v.ref)), v.mass, v.centroid, {v.components.begin(),v.components.end()}, {}});
    input.occurrences.push_back({InputText(id), InputText(local_name(v.ref)), input.capacity++, 0,
                                 point(v.initial.position),
                                 point(v.initial.velocity)});
  }
  for (const auto &e : local_->context)
    input.memberships.push_back(
        {InputText(occurrence(study(), e.occurrence)), InputText(local_name(e.target))});
  std::set<warm::OccurrenceId> moved;
  for (const auto &p : local_->placements) {
    require(moved.insert(p.occurrence).second);
    const auto id = occurrence(study(), p.occurrence);
    auto found =
        std::find_if(input.occurrences.begin(), input.occurrences.end(),
                     [&](const auto &o) { return std::string(o.id) == id; });
    require(found != input.occurrences.end());
    found->position = point(p.initial.position);
    found->velocity = point(p.initial.velocity);
  }
  std::set<warm::ObjectKey> resized;
  for (const auto &g : local_->geometry) {
    require(g.shared_definition.scope.nil() &&
            resized.insert(g.shared_definition).second);
    auto found = std::find_if(
        input.definitions.begin(), input.definitions.end(),
        [&](const auto &d) { return std::string(d.id) == warm::key(g.shared_definition); });
    require(found != input.definitions.end());
    found->centroid = g.centroid;
    found->components.assign(g.components.begin(),g.components.end());
  }
  return input;
}

}  // namespace assembly
