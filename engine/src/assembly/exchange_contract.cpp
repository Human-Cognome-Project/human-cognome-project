#include "assembly/exchange_contract.h"

#include <algorithm>
#include <limits>

namespace assembly {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw warm::Error(message);
}
OccurrenceKey copy_occurrence(const OccurrenceKey &key,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  return {warm::Uuid(key.study,domain),{key.source.entry,{key.source.path.begin(),key.source.path.end(),
      detail::SharedReservationAllocator<std::uint32_t>(domain)}}};
}
warm::ToolRef copy_reference(const warm::ToolRef &ref,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  return {{warm::Uuid(ref.object.scope,domain),{ref.object.address.begin(),ref.object.address.end(),
      detail::SharedReservationAllocator<codec::AddressElement>(domain)}},warm::Uuid(ref.revision,domain)};
}
template<class A>
detail::SharedReservedVector<warm::Parent> copy_parents(const std::vector<warm::Parent,A> &parents,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  detail::SharedReservedVector<warm::Parent> result{detail::SharedReservationAllocator<warm::Parent>(domain)};
  result.reserve(parents.size());
  for (const auto &parent : parents)
    result.push_back({copy_reference(parent.target,domain),parent.ordinal,parent.stored_mass,parent.contribution});
  return result;
}
auto copy_sources(const detail::SharedReservedVector<ExchangeSource> &sources,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  detail::SharedReservedVector<ExchangeSource> result{detail::SharedReservationAllocator<ExchangeSource>(domain)};
  result.reserve(sources.size());
  for (const auto &source : sources)
    result.push_back({copy_occurrence(source.occurrence,domain),copy_reference(source.definition,domain),source.mass,source.node,
                      copy_parents(source.parents,domain)});
  return result;
}
auto copy_leaves(const detail::SharedReservedVector<ExchangeLeaf> &leaves,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  detail::SharedReservedVector<ExchangeLeaf> result{detail::SharedReservationAllocator<ExchangeLeaf>(domain)};
  result.reserve(leaves.size());
  for (const auto &leaf : leaves)
    result.push_back({leaf.character,leaf.byte,leaf.nibble,copy_reference(leaf.definition,domain),leaf.count});
  return result;
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
    const std::shared_ptr<detail::ReservationDomain> &domain)
    : owner_(copy_occurrence(owner,domain)),
      recipe_{warm::Uuid(recipe.selection,domain),warm::Uuid(recipe.revision,domain)},
      outgoing_(detail::SharedReservationAllocator<ExchangeSource>(domain)),
      incoming_(detail::SharedReservationAllocator<ExchangeSource>(domain)),
      leaves_(detail::SharedReservationAllocator<ExchangeLeaf>(domain)) {}

LetterExchangeContract::LetterExchangeContract(const LetterExchangeContract &other,
    const std::shared_ptr<detail::ReservationDomain> &domain)
    : owner_(copy_occurrence(other.owner_,domain)), recipe_{warm::Uuid(other.recipe_.selection,domain),warm::Uuid(other.recipe_.revision,domain)}, from_(other.from_), to_(other.to_),
      mass_(other.mass_), match_work_(other.match_work_),
      outgoing_(copy_sources(other.outgoing_,domain)),
      incoming_(copy_sources(other.incoming_,domain)),
      leaves_(copy_leaves(other.leaves_,domain)) {}

LetterExchangeContract LetterExchangeContract::collect(const LetterProjection &projection,
    const OccurrenceKey &owner, warm::Exposure target,
    const std::shared_ptr<detail::ReservationDomain> &domain) {
  const auto &recipe = projection.recipe();
  need(recipe.recipe_version == 2, "exchange declaration requires structural recipe v2");
  need(owner.study == projection.study() && owner.source.path.empty(),
       "exchange owner is not an aggregate in this study");
  const auto selection = std::find_if(recipe.selections.begin(), recipe.selections.end(),
      [&](const auto &s) { return s.entry == owner.source.entry; });
  need(selection != recipe.selections.end(), "exchange occurrence is not declared");
  need((target == warm::Exposure::Aggregate || target == warm::Exposure::Parts) &&
       target != projection.exposure(owner.source.entry), "exchange requires the other declared alternative");
  LetterExchangeContract result(owner,recipe.ref,domain);
  result.from_ = projection.exposure(owner.source.entry);
  result.to_ = target;
  const auto source = [&](const warm::OccurrenceId &id) {
    const auto occurrence = std::find_if(projection.occurrences().begin(), projection.occurrences().end(),
        [&](const auto &o) { return o.key.source == id; });
    need(occurrence != projection.occurrences().end(), "missing exchange alternative occurrence");
    const auto tool = std::find_if(recipe.tools.begin(), recipe.tools.end(),
        [&](const auto &t) { return t.ref == occurrence->definition; });
    need(tool != recipe.tools.end() && tool->mass && *tool->mass,
         "missing exact exchange definition mass");
    return ExchangeSource{copy_occurrence(occurrence->key,domain), copy_reference(occurrence->definition,domain), *tool->mass,
                          occurrence->node, copy_parents(tool->parents,domain)};
  };
  detail::SharedReservedVector<ExchangeSource> aggregate{detail::SharedReservationAllocator<ExchangeSource>(domain)},
      parts{detail::SharedReservationAllocator<ExchangeSource>(domain)};
  aggregate.push_back(source(owner.source));
  result.mass_ = aggregate[0].mass;
  std::uint64_t mass = 0;
  for (const auto &id : selection->parts) {
    auto part = source(id);
    need(id.entry == owner.source.entry && id.path.size() == 2 && part.parents.size() == 2,
         "exchange part does not belong to the declared byte alternative");
    need(part.mass <= std::numeric_limits<std::uint64_t>::max() - mass,
         "exchange exact mass overflow");
    mass += part.mass;
    // project_letters already verified exact UTF-8 character -> byte -> nibble
    // construction against pinned revisions. Keep every ordered contribution,
    // including repeated bytes/nibbles, rather than deduplicating definitions.
    for (const auto &parent : part.parents)
      result.leaves_.push_back({id.path[0], id.path[1], parent.ordinal,
                                copy_reference(parent.target,domain), parent.contribution});
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
      const auto per_match = 8 + 4 * std::uint64_t(s.parents.size());
      need(side->size() <= (UINT64_MAX - result.match_work_) / per_match,
           "exchange comparison work overflow");
      result.match_work_ += side->size() * per_match;
    }
  return result;
}
}  // namespace assembly
