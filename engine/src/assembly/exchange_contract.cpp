#include "assembly/exchange_contract.h"
#include "assembly/preparation_work.h"

#include <algorithm>
#include <limits>

namespace assembly {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw warm::Error(message);
}
OccurrenceKey copy_occurrence(const OccurrenceKey &key,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter = nullptr) {
  if (meter) meter->copy_key(key);
  // Named owners keep earlier allocations alive for exception cleanup on GCC 11.
  auto study = warm::Uuid(key.study,domain);
  detail::SharedReservedVector<std::uint32_t> path(key.source.path.begin(),key.source.path.end(),
      detail::SharedReservationAllocator<std::uint32_t>(domain));
  return {std::move(study),{key.source.entry,std::move(path)}};
}
warm::ToolRef copy_reference(const warm::ToolRef &ref,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter = nullptr) {
  if (meter) meter->copy_key(ref);
  auto scope = warm::Uuid(ref.object.scope,domain);
  codec::Address address(ref.object.address.begin(),ref.object.address.end(),
      detail::SharedReservationAllocator<codec::AddressElement>(domain));
  auto revision = warm::Uuid(ref.revision,domain);
  return {{std::move(scope),std::move(address)},std::move(revision)};
}
template<class A>
detail::SharedReservedVector<warm::Parent> copy_parents(const std::vector<warm::Parent,A> &parents,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter = nullptr) {
  detail::SharedReservedVector<warm::Parent> result{detail::SharedReservationAllocator<warm::Parent>(domain)};
  if (meter) { meter->charge(1); meter->elements(parents.size(),sizeof(warm::Parent)); }
  result.reserve(parents.size());
  for (const auto &parent : parents)
    result.push_back({copy_reference(parent.target,domain,meter),parent.ordinal,parent.stored_mass,parent.contribution});
  return result;
}
template<class Parents> auto copy_sources(const detail::SharedReservedVector<ExchangeSource> &sources,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter, Parents values) {
  detail::SharedReservedVector<ExchangeSource> result{detail::SharedReservationAllocator<ExchangeSource>(domain)};
  if (meter) { meter->charge(1); meter->elements(sources.size(),sizeof(ExchangeSource)); }
  result.reserve(sources.size());
  for (const auto &source : sources) {
    auto occurrence = copy_occurrence(source.occurrence,domain,meter);
    auto definition = copy_reference(source.definition,domain,meter);
    auto parents = copy_parents(values(source),domain,meter);
    result.push_back({std::move(occurrence),std::move(definition),source.mass,source.node,
                      std::move(parents)});
  }
  return result;
}
warm::Uuid copy_uuid(const warm::Uuid &id,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter) {
  if (meter) meter->copy_key(id);
  return warm::Uuid(id,domain);
}
warm::BundleRef copy_recipe(const warm::BundleRef &recipe,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter) {
  auto selection = copy_uuid(recipe.selection,domain,meter);
  auto revision = copy_uuid(recipe.revision,domain,meter);
  return {std::move(selection),std::move(revision)};
}
void relocation_work(const ExchangeSource &source, detail::PreparationWork &work) {
  work.copy_key(source.occurrence); work.copy_key(source.definition);
  work.elements(source.parents.size(),sizeof(warm::Parent));
  for (const auto &parent : source.parents) work.copy_key(parent.target);
}
template<class T>
void append_work(const detail::SharedReservedVector<T> &rows, detail::PreparationWork &work) {
  work.charge(sizeof(T));
  if (rows.size() == rows.capacity()) {
    work.elements(rows.size(),sizeof(T));
    for (const auto &row : rows) relocation_work(row,work);
  }
}
template<class A,class B> bool same(const ExchangeSource &a, const ExchangeSource &b,
                                   const A &ap, const B &bp) {
  if (!(a.occurrence == b.occurrence) || !(a.definition == b.definition) ||
      a.mass != b.mass || a.node != b.node || ap.size() != bp.size())
    return false;
  for (std::size_t i = 0; i < ap.size(); ++i) {
    const auto &x = ap[i], &y = bp[i];
    if (!(x.target == y.target) || x.ordinal != y.ordinal ||
        x.stored_mass != y.stored_mass || x.contribution != y.contribution) return false;
  }
  return true;
}
template<class A,class Compare> bool bijection(const std::vector<ExchangeSource,A> &expected,
               const ExchangeSource *actual, std::size_t count,Compare compare) {
  if (expected.empty() || !actual || count != expected.size()) return false;
  for (const auto &source : expected) {
    std::size_t matches = 0;
    for (std::size_t i = 0; i < count; ++i) matches += compare(source, actual[i]);
    if (matches != 1) return false;
  }
  return true;
}
}  // namespace

bool LetterExchangeContract::matches(const ExchangeSource *outgoing, std::size_t out,
                                    const ExchangeSource *incoming, std::size_t in,
                                    std::uint64_t work) const {
  // Raw caller descriptors own their parent facts. A private index has no
  // authority outside its pinned declaration; never resolve it by coincidence.
  const auto compare=[&](const auto &a,const auto &b) {
    return b.recipe_tool_==SIZE_MAX && same(a,b,parents(a),b.parents);
  };
  return work >= match_work_ && bijection(outgoing_, outgoing, out,compare) &&
         bijection(incoming_, incoming, in,compare);
}

bool LetterExchangeContract::matches(const LetterExchangeContract &other,bool reverse,
                                    std::uint64_t work) const {
  const auto compare=[&](const auto &a,const auto &b) {
    return same(a,b,parents(a),other.parents(b));
  };
  const auto &out=reverse ? other.incoming_ : other.outgoing_;
  const auto &in=reverse ? other.outgoing_ : other.incoming_;
  return work>=match_work_ && bijection(outgoing_,out.data(),out.size(),compare) &&
         bijection(incoming_,in.data(),in.size(),compare);
}

const detail::SharedReservedVector<warm::Parent> &LetterExchangeContract::parents(
    const ExchangeSource &source,detail::PreparationWork *meter) const {
  if (!recipe_pin_) {
    need(source.recipe_tool_==SIZE_MAX,"unowned exchange recipe index");
    return source.parents;
  }
  if (meter) meter->charge(1);
  need(source.recipe_tool_<recipe_pin_->tools.size() && source.parents.empty(),
       "invalid pinned exchange recipe index");
  const auto &tool=recipe_pin_->tools[source.recipe_tool_];
  need(meter ? meter->equal(tool.ref,source.definition) : tool.ref==source.definition,
       "pinned exchange definition mismatch");
  return tool.parents;
}

LetterExchangeContract declare_letter_exchange(const LetterProjection &projection,
    const OccurrenceKey &owner, warm::Exposure target) {
  return LetterExchangeContract::collect(projection,owner,target,{});
}

LetterExchangeContract::LetterExchangeContract(const OccurrenceKey &owner, const warm::BundleRef &recipe,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter)
    : owner_(copy_occurrence(owner,domain,meter)),
      recipe_(copy_recipe(recipe,domain,meter)),
      outgoing_(detail::SharedReservationAllocator<ExchangeSource>(domain)),
      incoming_(detail::SharedReservationAllocator<ExchangeSource>(domain)) {}

LetterExchangeContract::LetterExchangeContract(const LetterExchangeContract &other,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter)
    : owner_(copy_occurrence(other.owner_,domain,meter)), recipe_(copy_recipe(other.recipe_,domain,meter)), from_(other.from_), to_(other.to_),
      mass_(other.mass_), match_work_(other.match_work_),
      outgoing_(copy_sources(other.outgoing_,domain,meter,[&](const auto &s)->const auto & {return other.parents(s,meter);})),
      incoming_(copy_sources(other.incoming_,domain,meter,[&](const auto &s)->const auto & {return other.parents(s,meter);})) {}

LetterExchangeContract::LetterExchangeContract(const LetterExchangeContract &other)
    : LetterExchangeContract(other,{}) {}

LetterExchangeContract &LetterExchangeContract::operator=(const LetterExchangeContract &other) {
  if (this!=&other) {
    auto detached=LetterExchangeContract(other,outgoing_.get_allocator().domain);
    *this=std::move(detached);
  }
  return *this;
}

LetterExchangeContract &LetterExchangeContract::operator=(LetterExchangeContract &&other) noexcept {
  if (this!=&other) {
    // As for LetterProjection, destroy displaced buffers with their allocators
    // intact, then transfer the value by noexcept construction. Equal-domain
    // libstdc++ string assignment can otherwise strand reservation accounting.
    this->~LetterExchangeContract();
    ::new (static_cast<void *>(this)) LetterExchangeContract(std::move(other));
  }
  return *this;
}

LetterExchangeContract LetterExchangeContract::collect(const LetterProjection &projection,
    const OccurrenceKey &owner, warm::Exposure target,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter,
    std::shared_ptr<const warm::Bundle> pin) {
  detail::PreparationWork standalone(UINT64_MAX);
  auto &work = meter ? *meter : standalone;
  work.charge(1);
  const auto &recipe = projection.recipe();
  work.charge(1);
  need(!pin || pin.get()==&recipe,"exchange recipe pin is not the exact projection recipe");
  need(recipe.recipe_version == 2, "exchange declaration requires structural recipe v2");
  need(work.equal(owner.study,projection.study()) && owner.source.path.empty(),
       "exchange owner is not an aggregate in this study");
  const auto selection = std::find_if(recipe.selections.begin(), recipe.selections.end(),
      [&](const auto &s) { work.charge(1); return s.entry == owner.source.entry; });
  need(selection != recipe.selections.end(), "exchange occurrence is not declared");
  work.elements(recipe.selections.size(),1); // Conservative prepayment for exposure lookup.
  const auto exposure = projection.exposure(owner.source.entry);
  need((target == warm::Exposure::Aggregate || target == warm::Exposure::Parts) &&
       target != exposure, "exchange requires the other declared alternative");
  work.charge(sizeof(LetterExchangeContract));
  LetterExchangeContract result(owner,recipe.ref,domain,&work);
  if (pin) { work.charge(1); result.recipe_pin_=std::move(pin); }
  result.from_ = exposure;
  result.to_ = target;
  const auto source = [&](const warm::OccurrenceId &id) {
    work.charge(sizeof(ExchangeSource));
    const auto occurrence = std::find_if(projection.occurrences().begin(), projection.occurrences().end(),
        [&](const auto &o) { return work.equal(o.key.source,id); });
    need(occurrence != projection.occurrences().end(), "missing exchange alternative occurrence");
    need(work.equal(occurrence->key.study,owner.study) && occurrence->key.source.entry==owner.source.entry,
         "foreign exchange alternative occurrence");
    const auto tool = std::find_if(recipe.tools.begin(), recipe.tools.end(),
        [&](const auto &t) { return work.equal(t.ref,occurrence->definition); });
    need(tool != recipe.tools.end() && tool->mass && *tool->mass,
         "missing exact exchange definition mass");
    auto key = copy_occurrence(occurrence->key,domain,&work);
    auto definition = copy_reference(occurrence->definition,domain,&work);
    auto parents = result.recipe_pin_
        ? detail::SharedReservedVector<warm::Parent>{detail::SharedReservationAllocator<warm::Parent>(domain)}
        : copy_parents(tool->parents,domain,&work);
    ExchangeSource value{std::move(key),std::move(definition),*tool->mass,
                         occurrence->node,std::move(parents)};
    if (result.recipe_pin_) { work.charge(1); value.recipe_tool_=std::size_t(tool-recipe.tools.begin()); }
    return value;
  };
  detail::SharedReservedVector<ExchangeSource> aggregate{detail::SharedReservationAllocator<ExchangeSource>(domain)},
      parts{detail::SharedReservationAllocator<ExchangeSource>(domain)};
  append_work(aggregate,work);
  aggregate.push_back(source(owner.source));
  result.mass_ = aggregate[0].mass;
  std::uint64_t mass = 0;
  for (const auto &id : selection->parts) {
    work.charge(1);
    auto part = source(id);
    need(id.entry == owner.source.entry && id.path.size() == 2 && result.parents(part,&work).size() == 2,
         "exchange part does not belong to the declared byte alternative");
    need(part.mass <= std::numeric_limits<std::uint64_t>::max() - mass,
         "exchange exact mass overflow");
    mass += part.mass;
    // project_letters already verified exact UTF-8 character -> byte -> nibble
    // construction against pinned revisions. The path and retained parents
    // preserve every ordered/repeated nibble contribution without a second table.
    append_work(parts,work);
    parts.push_back(std::move(part));
  }
  need(!parts.empty() && mass == result.mass_, "exchange alternatives differ in exact mass");
  if (target == warm::Exposure::Parts) {
    result.outgoing_ = std::move(aggregate);
    result.incoming_ = std::move(parts);
  } else {
    result.outgoing_ = std::move(parts);
    result.incoming_ = std::move(aggregate);
  }
  // Conservative structural comparison units, reserved before inspecting caller
  // arrays. Includes repeated matching keys with different parent descriptors.
  for (const auto *side : {&result.outgoing_, &result.incoming_})
    for (const auto &s : *side) {
      work.charge(1);
      const auto per_match = 8 + 4 * std::uint64_t(result.parents(s,&work).size());
      need(side->size() <= (UINT64_MAX - result.match_work_) / per_match,
           "exchange comparison work overflow");
      result.match_work_ += side->size() * per_match;
    }
  return result;
}
}  // namespace assembly
