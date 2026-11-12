#include "assembly/membership_fields.h"
#include "assembly/preparation_work.h"

#include <map>
#include <set>

namespace assembly {
namespace {
void need(bool condition, const char *message) {
  if (!condition) throw warm::Error(message);
}
warm::ObjectKey catalogue_object(const warm::ObjectKey &key,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
  work.copy_key(key);
  return {warm::Uuid(key.scope,domain),{key.address.begin(),key.address.end(),
      detail::SharedReservationAllocator<codec::AddressElement>(domain)}};
}
struct MembershipKeyLess {
  detail::PreparationWork *work;
  bool operator()(const warm::ObjectKey &a, const warm::ObjectKey &b) const {
    work->copy_key(a); work->copy_key(b);
    return a < b;
  }
};
struct MembershipOwnerLess {
  detail::PreparationWork *work;
  bool operator()(std::uint64_t a, std::uint64_t b) const {
    work->charge(1); return a < b;
  }
};
}  // namespace

MembershipFields membership_fields(const LetterProjection &projection) {
  return MembershipFields::collect(projection, false);
}
MembershipFields MembershipFields::collect(const LetterProjection &projection, bool alternatives) {
  return collect(projection,alternatives,{});
}
MembershipFields MembershipFields::collect(const LetterProjection &projection, bool alternatives,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter) {
  detail::PreparationWork standalone(UINT64_MAX);
  auto &work = meter ? *meter : standalone;
  work.charge(1);
  work.copy_key(projection.study());
  work.copy_key(projection.recipe().ref.selection);
  work.copy_key(projection.recipe().ref.revision);
  MembershipFields out(projection.study(),projection.recipe().ref,domain);
  out.alternatives_ = alternatives;
  using ToolAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,const warm::Tool *>>;
  std::map<warm::ObjectKey,const warm::Tool *,MembershipKeyLess,ToolAllocator>
      tools{MembershipKeyLess{&work},ToolAllocator(domain)};
  for (const auto &t : projection.recipe().tools) {
    work.charge(1);
    tools.emplace(catalogue_object(t.ref.object,domain,work), &t);
  }
  auto get = [&](const warm::ToolRef &ref) -> const warm::Tool & {
    work.charge(1);
    const auto it = tools.find(ref.object);
    need(it != tools.end() && work.equal(it->second->ref,ref),
         "membership: missing or inconsistent pinned group reference");
    return *it->second;
  };
  using Targets = std::set<warm::ObjectKey,MembershipKeyLess,
      detail::SharedReservationAllocator<warm::ObjectKey>>;
  using StateAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,int>>;
  using ReachedAllocator = detail::SharedReservationAllocator<std::pair<const warm::ObjectKey,Targets>>;
  std::map<warm::ObjectKey,int,MembershipKeyLess,StateAllocator>
      state{MembershipKeyLess{&work},StateAllocator(domain)};
  std::map<warm::ObjectKey,Targets,MembershipKeyLess,ReachedAllocator>
      reached{MembershipKeyLess{&work},ReachedAllocator(domain)};
  struct Frame {
    const warm::Tool *tool;
    int *status;
    Targets *targets;
    std::size_t next{};
    const Targets *pending{};
  };
  detail::SharedReservedVector<Frame> stack{detail::SharedReservationAllocator<Frame>(domain)};
  auto visit = [&](const warm::ToolRef &ref) -> Targets * {
    work.charge(1);
    const auto &t = get(ref); // Validate pins even on memoized visits.
    auto status_it = state.find(ref.object);
    if (status_it == state.end())
      status_it = state.try_emplace(catalogue_object(ref.object,domain,work),0).first;
    auto &status = status_it->second;
    need(status != 1, "membership: cyclic upward membership");
    auto reached_it = reached.find(ref.object);
    if (reached_it == reached.end())
      reached_it = reached.try_emplace(catalogue_object(ref.object,domain,work),
          MembershipKeyLess{&work},detail::SharedReservationAllocator<warm::ObjectKey>(domain)).first;
    auto &targets = reached_it->second;
    if (status == 2) return &targets;
    need(stack.size() < tools.size(), "membership: traversal depth exceeds tool inventory");
    work.charge(1);
    if (stack.size() == stack.capacity()) work.elements(stack.size(),sizeof(Frame));
    stack.push_back({&t,&status,&targets});
    status = 1;
    return &targets;
  };
  auto follow = [&](const warm::ToolRef &ref) -> const Targets & {
    auto *root = visit(ref);
    while (!stack.empty()) {
      work.charge(1);
      auto &frame = stack.back();
      if (frame.next == frame.tool->member_of.size()) {
        *frame.status = 2;
        stack.pop_back();
        continue;
      }
      const auto &group = frame.tool->member_of[frame.next];
      if (frame.pending) {
        const auto insert = [&](const warm::ObjectKey &key) {
          work.charge(1);
          if (frame.targets->find(key) == frame.targets->end())
            frame.targets->insert(catalogue_object(key,domain,work));
        };
        insert(group.object);
        for (const auto &ancestor : *frame.pending) insert(ancestor);
        ++frame.next;
        frame.pending = nullptr;
      } else {
        const auto parent = stack.size()-1;
        const auto *ancestors = visit(group);
        // visit may grow stack; reacquire the parent instead of using frame.
        stack[parent].pending = ancestors;
      }
    }
    return *root;
  };
  using OwnerAllocator = detail::SharedReservationAllocator<std::pair<const std::uint64_t,const ProjectedOccurrence *>>;
  std::map<std::uint64_t,const ProjectedOccurrence *,MembershipOwnerLess,OwnerAllocator>
      owners{MembershipOwnerLess{&work},OwnerAllocator(domain)};
  for (const auto &o : projection.occurrences()) {
    work.charge(1);
    if (o.key.source.path.empty()) owners.emplace(o.key.source.entry, &o);
  }
  for (const auto &o : projection.occurrences()) {
    work.charge(1);
    if (!alternatives && !o.active) continue;
    const auto it = owners.find(o.key.source.entry);
    need(it != owners.end(), "membership: missing owning letter occurrence");
    const auto &owner = *it->second;
    need(work.equal(o.key.study,out.study_) && (o.key.source.path.empty() ||
                                       (o.byte && work.equal(o.byte->owner,owner.key))),
         "membership: inconsistent byte ownership");
    const auto &live = get(o.definition);
    need(live.mass && *live.mass > 0, "membership: unknown live mass");
    for (const auto &group : follow(owner.definition)) {
      work.charge(1);
      work.copy_key(o.key);
      need(out.contributions_.size() < std::size_t(INT32_MAX),
           "membership: native contribution count overflow");
      OccurrenceKey occurrence{warm::Uuid(o.key.study,domain),{o.key.source.entry,
          {o.key.source.path.begin(),o.key.source.path.end(),
           detail::SharedReservationAllocator<std::uint32_t>(domain)}}};
      const auto &target=tools.at(group)->ref;
      work.copy_key(target.revision);
      if (out.contributions_.size() == out.contributions_.capacity()) {
        work.elements(out.contributions_.size(),sizeof(MembershipContribution));
        // Conservatively admit nested payload relocation even if a move is used.
        for (const auto &retained : out.contributions_) {
          work.copy_key(retained.occurrence);
          work.copy_key(retained.field.object);
          work.copy_key(retained.target);
        }
      }
      out.contributions_.push_back({std::move(occurrence),
                                    {GroupKind::Membership, catalogue_object(group,domain,work)},
                                    {catalogue_object(target.object,domain,work),warm::Uuid(target.revision,domain)},
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
        {InputText(occurrence_text(edge.occurrence)), InputText(warm::key(edge.field.object))});
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
        {InputText(occurrence_text(edge.occurrence)), InputText(warm::key(edge.field.object))});
  return input;
}
}  // namespace assembly
