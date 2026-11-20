#include "assembly/projection_access.h"
#include "assembly/membership_fields.h"
#include "assembly/preparation_work.h"

#include <map>
#include <set>
#include <new>

namespace assembly {
namespace {
void need(bool condition, const char *message) {
  if (!condition) throw warm::Error(message);
}
warm::ObjectKey catalogue_object(const warm::ObjectKey &key,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
  work.copy_key(key);
  // Keep each allocation independently owned across later refusal points.
  // GCC 11 does not fully unwind these nested aggregate temporaries.
  auto scope = warm::Uuid(key.scope,domain);
  codec::Address address(key.address.begin(),key.address.end(),
      detail::SharedReservationAllocator<codec::AddressElement>(domain));
  return {std::move(scope),std::move(address)};
}
struct MembershipKeyLess {
  detail::PreparationWork *work;
  bool operator()(const warm::ObjectKey &a, const warm::ObjectKey &b) const {
    work->copy_key(a); work->copy_key(b);
    return a < b;
  }
};
}  // namespace

MembershipFields membership_fields(const LetterProjection &projection) {
  return MembershipFields::collect(projection, false);
}
MembershipFields::MembershipFields(const MembershipFields &other,
    const std::shared_ptr<detail::ReservationDomain> &domain)
    : MembershipFields(other.study_,other.recipe_,domain) {
  alternatives_=other.alternatives_;
  targets_.reserve(other.targets_.size());
  for (const auto &pin:other.targets_) {
    auto scope=warm::Uuid(pin.object.scope,domain);
    codec::Address address(pin.object.address.begin(),pin.object.address.end(),
        detail::SharedReservationAllocator<codec::AddressElement>(domain));
    auto revision=warm::Uuid(pin.revision,domain);
    warm::ToolRef copy{{std::move(scope),std::move(address)},std::move(revision)};
    targets_.push_back(std::move(copy));
  }
  contributions_.reserve(other.contributions_.size());
  for (const auto &row:other.contributions_) {
    (void)other.target(row);
    warm::OccurrencePath path(row.occurrence.path.begin(),row.occurrence.path.end(),
        detail::SharedReservationAllocator<std::uint32_t>(domain));
    Row copy{{row.occurrence.entry,std::move(path)},row.kind,row.target_index,row.count};
    contributions_.push_back(std::move(copy));
  }
}
MembershipFields &MembershipFields::operator=(const MembershipFields &other) {
  if (this!=&other) {
    MembershipFields copy(other,contributions_.get_allocator().domain);
    *this=std::move(copy);
  }
  return *this;
}
MembershipFields &MembershipFields::operator=(MembershipFields &&other) noexcept {
  if (this!=&other) {
    // Transfer both correlated tables by construction, with displaced allocators
    // intact through destruction, as for LetterProjection and exchange values.
    this->~MembershipFields();
    ::new (static_cast<void *>(this)) MembershipFields(std::move(other));
  }
  return *this;
}
const warm::ToolRef &MembershipFields::target(const Row &row,detail::PreparationWork *meter) const {
  if (meter) meter->charge(1);
  need(row.target_index<targets_.size(),"membership: target index out of range");
  return targets_[row.target_index];
}
std::uint32_t MembershipFields::retain_target(const warm::ToolRef &target,detail::PreparationWork &work) {
  std::size_t index=0;
  for (;index<targets_.size();++index) if (work.equal(targets_[index],target)) break;
  if (index==targets_.size()) {
    work.charge(1);
    need(index<std::size_t(INT32_MAX),"membership: target count overflow");
    if (targets_.size()==targets_.capacity()) {
      work.elements(targets_.size(),sizeof(warm::ToolRef));
      for (const auto &retained:targets_) work.copy_key(retained);
    }
    const auto domain=targets_.get_allocator().domain;
    auto object=catalogue_object(target.object,domain,work);
    work.copy_key(target.revision);
    auto revision=warm::Uuid(target.revision,domain);
    warm::ToolRef pin{std::move(object),std::move(revision)};
    targets_.push_back(std::move(pin));
  }
  return static_cast<std::uint32_t>(index);
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
  for (std::size_t i=0;i<projection.occurrence_count();++i) {
    const auto &o=detail::ProjectionAccess::row(projection,i);
    work.charge(1);
    if (!alternatives && !o.active) continue;
    const auto owner=o.byte ? detail::ProjectionAccess::byte_owner_index(projection,i,&work) : i;
    need(owner<projection.occurrence_count(),
         "membership: inconsistent byte ownership");
    const auto &live = get(detail::ProjectionAccess::definition(projection,i,&work));
    need(live.mass && *live.mass > 0, "membership: unknown live mass");
    for (const auto &group : follow(detail::ProjectionAccess::definition(projection,owner,&work))) {
      work.charge(1);
      work.copy_key(o.source);
      need(out.contributions_.size() < std::size_t(INT32_MAX),
           "membership: native contribution count overflow");
      // Independent owners keep every allocation unwind-safe on GCC 11.
      warm::OccurrencePath path(o.source.path.begin(),o.source.path.end(),
          detail::SharedReservationAllocator<std::uint32_t>(domain));
      warm::OccurrenceId occurrence{o.source.entry,std::move(path)};
      const auto &target=tools.at(group)->ref;
      const auto target_index=out.retain_target(target,work);
      if (out.contributions_.size() == out.contributions_.capacity()) {
        work.elements(out.contributions_.size(),sizeof(Row));
        // Conservatively admit nested payload relocation even if a move is used.
        for (const auto &retained : out.contributions_) {
          work.copy_key(retained.occurrence);
        }
      }
      Row contribution{std::move(occurrence),
          GroupKind::Membership,target_index,*live.mass};
      out.contributions_.push_back(std::move(contribution));
    }
  }
  return out;
}

MembershipContribution MembershipFields::contribution(std::size_t index) const {
  need(index<contributions_.size(),"membership: contribution index out of range");
  const auto &row=contributions_[index];
  const auto &pin=target(row);
  // Named owners keep every earlier allocation unwind-safe on GCC 11.
  auto study=warm::Uuid(study_,{});
  warm::OccurrencePath path(row.occurrence.path.begin(),row.occurrence.path.end());
  auto scope=warm::Uuid(pin.object.scope,{});
  codec::Address address(pin.object.address.begin(),pin.object.address.end());
  auto revision=warm::Uuid(pin.revision,{});
  OccurrenceKey occurrence{std::move(study),{row.occurrence.entry,std::move(path)}};
  warm::ToolRef target{{std::move(scope),std::move(address)},std::move(revision)};
  return {std::move(occurrence),row.kind,std::move(target),row.count};
}

StudyInput MembershipFields::bind(const LetterProjection &projection,
                                  const std::vector<StudyNode> &nodes) const {
  need(!alternatives_, "membership: alternative catalogue is not a live participation set");
  need(study_ == projection.study(),
       "membership: fields belong to another study");
  auto input = projection.bind(nodes);
  for (const auto &edge : contributions_) {
    need(edge.kind == GroupKind::Membership, "membership: invalid field kind");
    input.memberships.push_back(
        {InputText(occurrence_text(study_,edge.occurrence)), InputText(warm::key(target(edge).object))});
  }
  return input;
}
StructuralStudyInput MembershipFields::bind_structure(
    const LetterProjection &projection,
    const std::vector<StudyNode> &nodes) const {
  need(!alternatives_, "membership: alternative catalogue is not a live participation set");
  need(study_ == projection.study(),
       "membership: fields belong to another study");
  auto input = projection.bind_structure(nodes);
  for (const auto &edge : contributions_) {
    need(edge.kind == GroupKind::Membership, "membership: invalid field kind");
    input.memberships.push_back(
        {InputText(occurrence_text(study_,edge.occurrence)), InputText(warm::key(target(edge).object))});
  }
  return input;
}
}  // namespace assembly
