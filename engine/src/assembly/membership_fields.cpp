#include "assembly/membership_fields.h"

#include <functional>
#include <map>
#include <set>

namespace assembly {
namespace {
void need(bool condition, const char *message) {
  if (!condition) throw warm::Error(message);
}
}  // namespace

MembershipFields membership_fields(const LetterProjection &projection) {
  return MembershipFields::collect(projection, false);
}

MembershipFields MembershipFields::collect(const LetterProjection &projection, bool alternatives) {
  return collect(projection,alternatives,{});
}

MembershipFields MembershipFields::collect(const LetterProjection &projection, bool alternatives,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  MembershipFields out(domain);
  out.study_ = projection.study();
  out.recipe_ = projection.recipe().ref;
  out.alternatives_ = alternatives;
  using ToolAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,const warm::Tool *>>;
  std::map<warm::ObjectKey, const warm::Tool *,std::less<warm::ObjectKey>,ToolAllocator>
      tools{ToolAllocator(domain)};
  for (const auto &t : projection.recipe().tools)
    tools.emplace(t.ref.object, &t);
  auto get = [&](const warm::ToolRef &ref) -> const warm::Tool & {
    const auto it = tools.find(ref.object);
    need(it != tools.end() && it->second->ref == ref,
         "membership: missing or inconsistent pinned group reference");
    return *it->second;
  };
  using Targets = std::set<warm::ObjectKey,std::less<warm::ObjectKey>,
      detail::SharedReservationAllocator<warm::ObjectKey>>;
  using StateAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,int>>;
  using ReachedAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,Targets>>;
  std::map<warm::ObjectKey,int,std::less<warm::ObjectKey>,StateAllocator> state{StateAllocator(domain)};
  std::map<warm::ObjectKey,Targets,std::less<warm::ObjectKey>,ReachedAllocator> reached{ReachedAllocator(domain)};
  std::function<const Targets &(const warm::ToolRef &)>
      follow =
          [&](const warm::ToolRef &ref) -> const Targets & {
    const auto &t = get(ref);
    auto &status = state[ref.object];
    need(status != 1, "membership: cyclic upward membership");
    // operator[] would default-construct a detached nested set. Construct its
    // allocator explicitly in the node before any target insertion.
    auto &targets = reached.try_emplace(ref.object,
        detail::SharedReservationAllocator<warm::ObjectKey>(domain)).first->second;
    if (status == 2) return targets;
    status = 1;
    for (const auto &group : t.member_of) {
      const auto &ancestors = follow(group);
      targets.insert(group.object);
      targets.insert(ancestors.begin(), ancestors.end());
    }
    status = 2;
    return targets;
  };
  using OwnerAllocator = detail::SharedReservationAllocator<std::pair<const std::uint64_t,const ProjectedOccurrence *>>;
  std::map<std::uint64_t,const ProjectedOccurrence *,std::less<std::uint64_t>,OwnerAllocator>
      owners{OwnerAllocator(domain)};
  for (const auto &o : projection.occurrences())
    if (o.key.source.path.empty()) owners.emplace(o.key.source.entry, &o);
  for (const auto &o : projection.occurrences()) {
    if (!alternatives && !o.active) continue;
    const auto it = owners.find(o.key.source.entry);
    need(it != owners.end(), "membership: missing owning letter occurrence");
    const auto &owner = *it->second;
    need(o.key.study == out.study_ && (o.key.source.path.empty() ||
                                       (o.byte && o.byte->owner == owner.key)),
         "membership: inconsistent byte ownership");
    const auto &live = get(o.definition);
    need(live.mass && *live.mass > 0, "membership: unknown live mass");
    for (const auto &group : follow(owner.definition)) {
      need(out.contributions_.size() < std::size_t(INT32_MAX),
           "membership: native contribution count overflow");
      out.contributions_.push_back({o.key,
                                    {GroupKind::Membership, group},
                                    tools.at(group)->ref,
                                    *live.mass});
    }
  }
  return out;
}

StudyInput MembershipFields::bind(const LetterProjection &projection,
                                  const std::vector<StudyNode> &nodes) const {
  need(!alternatives_, "membership: alternative catalogue is not a live participation set");
  need(study_ == projection.study(),
       "membership: fields belong to another study");
  auto input = projection.bind(nodes);
  for (const auto &edge : contributions_)
    input.memberships.push_back(
        {occurrence_text(edge.occurrence), warm::key(edge.field.object)});
  return input;
}
StructuralStudyInput MembershipFields::bind_structure(
    const LetterProjection &projection,
    const std::vector<StudyNode> &nodes) const {
  need(!alternatives_, "membership: alternative catalogue is not a live participation set");
  need(study_ == projection.study(),
       "membership: fields belong to another study");
  auto input = projection.bind_structure(nodes);
  for (const auto &edge : contributions_)
    input.memberships.push_back(
        {occurrence_text(edge.occurrence), warm::key(edge.field.object)});
  return input;
}
}  // namespace assembly
