#include "assembly/exchange_contract.h"

#include <algorithm>
#include <limits>

namespace assembly {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw warm::Error(message);
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
bool bijection(const std::vector<ExchangeSource> &expected,
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
  const auto &recipe = projection.recipe();
  need(recipe.recipe_version == 2, "exchange declaration requires structural recipe v2");
  need(owner.study == projection.study() && owner.source.path.empty(),
       "exchange owner is not an aggregate in this study");
  const auto selection = std::find_if(recipe.selections.begin(), recipe.selections.end(),
      [&](const auto &s) { return s.entry == owner.source.entry; });
  need(selection != recipe.selections.end(), "exchange occurrence is not declared");
  need((target == warm::Exposure::Aggregate || target == warm::Exposure::Parts) &&
       target != projection.exposure(owner.source.entry), "exchange requires the other declared alternative");
  LetterExchangeContract result;
  result.owner_ = owner;
  result.recipe_ = recipe.ref;
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
    return ExchangeSource{occurrence->key, occurrence->definition, *tool->mass,
                          occurrence->node, tool->parents};
  };
  std::vector<ExchangeSource> aggregate{source(owner.source)}, parts;
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
                                parent.target, parent.contribution});
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
