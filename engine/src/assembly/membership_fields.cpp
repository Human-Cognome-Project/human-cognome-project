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
  MembershipFields out;
  out.study_ = projection.study();
  std::map<warm::ObjectKey, const warm::Tool *> tools;
  for (const auto &t : projection.recipe().tools)
    tools.emplace(t.ref.object, &t);
  auto get = [&](const warm::ToolRef &ref) -> const warm::Tool & {
    const auto it = tools.find(ref.object);
    need(it != tools.end() && it->second->ref == ref,
         "membership: missing or inconsistent pinned group reference");
    return *it->second;
  };
  std::map<warm::ObjectKey, int> state;
  std::map<warm::ObjectKey, std::set<warm::ObjectKey>> reached;
  std::function<const std::set<warm::ObjectKey> &(const warm::ToolRef &)>
      follow =
          [&](const warm::ToolRef &ref) -> const std::set<warm::ObjectKey> & {
    const auto &t = get(ref);
    auto &status = state[ref.object];
    need(status != 1, "membership: cyclic upward membership");
    auto &targets = reached[ref.object];
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
  std::map<std::uint64_t, const ProjectedOccurrence *> owners;
  for (const auto &o : projection.occurrences())
    if (o.key.source.path.empty()) owners.emplace(o.key.source.entry, &o);
  for (const auto &o : projection.occurrences()) {
    if (!o.active) continue;
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
  need(study_ == projection.study(),
       "membership: fields belong to another study");
  auto input = projection.bind_structure(nodes);
  for (const auto &edge : contributions_)
    input.memberships.push_back(
        {occurrence_text(edge.occurrence), warm::key(edge.field.object)});
  return input;
}
}  // namespace assembly
