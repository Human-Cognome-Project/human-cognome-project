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
auto copy_sources(const detail::SharedReservedVector<ExchangeSource> &sources,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter = nullptr) {
  detail::SharedReservedVector<ExchangeSource> result{detail::SharedReservationAllocator<ExchangeSource>(domain)};
  if (meter) { meter->charge(1); meter->elements(sources.size(),sizeof(ExchangeSource)); }
  result.reserve(sources.size());
  for (const auto &source : sources) {
    auto occurrence = copy_occurrence(source.occurrence,domain,meter);
    auto definition = copy_reference(source.definition,domain,meter);
    auto parents = copy_parents(source.parents,domain,meter);
    result.push_back({std::move(occurrence),std::move(definition),source.mass,source.node,
                      std::move(parents)});
  }
  return result;
}
auto copy_leaves(const detail::SharedReservedVector<ExchangeLeaf> &leaves,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter = nullptr) {
  detail::SharedReservedVector<ExchangeLeaf> result{detail::SharedReservationAllocator<ExchangeLeaf>(domain)};
  if (meter) { meter->charge(1); meter->elements(leaves.size(),sizeof(ExchangeLeaf)); }
  result.reserve(leaves.size());
  for (const auto &leaf : leaves)
    result.push_back({leaf.character,leaf.byte,leaf.nibble,copy_reference(leaf.definition,domain,meter),leaf.count});
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
void relocation_work(const ExchangeLeaf &leaf, detail::PreparationWork &work) {
  work.copy_key(leaf.definition);
}
template<class T>
void append_work(const detail::SharedReservedVector<T> &rows, detail::PreparationWork &work) {
  work.charge(sizeof(T));
  if (rows.size() == rows.capacity()) {
    work.elements(rows.size(),sizeof(T));
    for (const auto &row : rows) relocation_work(row,work);
  }
}
bool same(const ExchangeSource &a, const ExchangeSource &b) {
  if (!(a.occurrence == b.occurrence) || !(a.definition == b.definition) ||
      a.mass != b.mass || a.node != b.node || a.parents.size() != b.parents.size())
    return false;
  for (std::size_t i = 0; i < a.parents.size(); ++i) {
    const auto &x = a.parents[i], &y = b.parents[i];
    if (!(x.target == y.target) || x.ordinal != y.ordinal ||
        x.stored_mass != y.stored_mass || x.contribution != y.contribution) return false;
  }
  return true;
}
template<class A> bool bijection(const std::vector<ExchangeSource,A> &expected,
               const ExchangeSource *actual, std::size_t count) {
  if (expected.empty() || !actual || count != expected.size()) return false;
  for (const auto &source : expected) {
    std::size_t matches = 0;
    for (std::size_t i = 0; i < count; ++i) matches += same(source, actual[i]);
    if (matches != 1) return false;
  }
  return true;
}
}  // namespace

bool LetterExchangeContract::matches(const ExchangeSource *outgoing, std::size_t out,
                                    const ExchangeSource *incoming, std::size_t in,
                                    std::uint64_t work) const {
  return work >= match_work_ && bijection(outgoing_, outgoing, out) &&
         bijection(incoming_, incoming, in);
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
      incoming_(detail::SharedReservationAllocator<ExchangeSource>(domain)),
      leaves_(detail::SharedReservationAllocator<ExchangeLeaf>(domain)) {}

LetterExchangeContract::LetterExchangeContract(const LetterExchangeContract &other,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter)
    : owner_(copy_occurrence(other.owner_,domain,meter)), recipe_(copy_recipe(other.recipe_,domain,meter)), from_(other.from_), to_(other.to_),
      mass_(other.mass_), match_work_(other.match_work_),
      outgoing_(copy_sources(other.outgoing_,domain,meter)),
      incoming_(copy_sources(other.incoming_,domain,meter)),
      leaves_(copy_leaves(other.leaves_,domain,meter)) {}

LetterExchangeContract LetterExchangeContract::collect(const LetterProjection &projection,
    const OccurrenceKey &owner, warm::Exposure target,
    const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork *meter) {
  detail::PreparationWork standalone(UINT64_MAX);
  auto &work = meter ? *meter : standalone;
  work.charge(1);
  const auto &recipe = projection.recipe();
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
  result.from_ = exposure;
  result.to_ = target;
  const auto source = [&](const warm::OccurrenceId &id) {
    work.charge(sizeof(ExchangeSource));
    const auto occurrence = std::find_if(projection.occurrences().begin(), projection.occurrences().end(),
        [&](const auto &o) { return work.equal(o.key.source,id); });
    need(occurrence != projection.occurrences().end(), "missing exchange alternative occurrence");
    const auto tool = std::find_if(recipe.tools.begin(), recipe.tools.end(),
        [&](const auto &t) { return work.equal(t.ref,occurrence->definition); });
    need(tool != recipe.tools.end() && tool->mass && *tool->mass,
         "missing exact exchange definition mass");
    auto key = copy_occurrence(occurrence->key,domain,&work);
    auto definition = copy_reference(occurrence->definition,domain,&work);
    auto parents = copy_parents(tool->parents,domain,&work);
    return ExchangeSource{std::move(key),std::move(definition),*tool->mass,
                          occurrence->node,std::move(parents)};
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
    need(id.entry == owner.source.entry && id.path.size() == 2 && part.parents.size() == 2,
         "exchange part does not belong to the declared byte alternative");
    need(part.mass <= std::numeric_limits<std::uint64_t>::max() - mass,
         "exchange exact mass overflow");
    mass += part.mass;
    // project_letters already verified exact UTF-8 character -> byte -> nibble
    // construction against pinned revisions. Keep every ordered contribution,
    // including repeated bytes/nibbles, rather than deduplicating definitions.
    for (const auto &parent : part.parents) {
      append_work(result.leaves_,work);
      result.leaves_.push_back({id.path[0], id.path[1], parent.ordinal,
                                copy_reference(parent.target,domain,&work), parent.contribution});
    }
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
      const auto per_match = 8 + 4 * std::uint64_t(s.parents.size());
      need(side->size() <= (UINT64_MAX - result.match_work_) / per_match,
           "exchange comparison work overflow");
      result.match_work_ += side->size() * per_match;
    }
  return result;
}
}  // namespace assembly
