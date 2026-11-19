#pragma once

#include <algorithm>
#include <array>

#include "assembly/exchange_contract.h"
#include "assembly/preparation_work.h"

// Inspection only: resolve existing immutable recipe parents without building a
// replacement declaration, registry or retained view. No production call site.
namespace exchange_parent_observation {
inline void need(bool ok) {
  if (!ok) throw warm::Error("exchange parent ownership inspection failed");
}
inline const warm::Tool *lookup(const assembly::LetterProjection &projection,
    const assembly::LetterExchangeContract &contract, const assembly::ExchangeSource &source,
    assembly::detail::PreparationWork &work) {
  work.charge(1);
  if (!work.equal(contract.owner().study,projection.study()) ||
      !work.equal(contract.recipe().selection,projection.recipe().ref.selection) ||
      !work.equal(contract.recipe().revision,projection.recipe().ref.revision) ||
      !work.equal(source.occurrence.study,projection.study()) ||
      source.occurrence.source.entry!=contract.owner().source.entry) return nullptr;
  const auto &occurrences=projection.occurrences();
  const auto occurrence=std::find_if(occurrences.begin(),occurrences.end(),[&](const auto &o) {
    return work.equal(o.key,source.occurrence);
  });
  if (occurrence==occurrences.end() || !work.equal(occurrence->definition,source.definition) ||
      occurrence->node!=source.node) return nullptr;
  const auto &tools=projection.recipe().tools;
  const auto tool=std::find_if(tools.begin(),tools.end(),[&](const auto &t) {
    return work.equal(t.ref,source.definition);
  });
  if (tool==tools.end() || !tool->mass || *tool->mass!=source.mass) return nullptr;
  return &*tool;
}
inline bool equal(const assembly::ExchangeSource &source,const warm::Tool &tool) {
  if (source.parents.size()!=tool.parents.size()) return false;
  for (std::size_t i=0;i<source.parents.size();++i) {
    const auto &a=source.parents[i], &b=tool.parents[i];
    if (!(a.target==b.target) || a.ordinal!=b.ordinal ||
        a.stored_mass!=b.stored_mass || a.contribution!=b.contribution) return false;
  }
  return true;
}
struct Cost {
  std::size_t sources{},parents{},vector_bytes{},nested_bytes{},requests{},repeated_sources{};
  std::uint64_t lookup_work{};
  std::size_t bytes() const { return vector_bytes+nested_bytes; }
  void add(const Cost &b) {
    sources+=b.sources; parents+=b.parents; vector_bytes+=b.vector_bytes;
    nested_bytes+=b.nested_bytes; requests+=b.requests; repeated_sources+=b.repeated_sources;
    lookup_work+=b.lookup_work;
  }
};
// Resolve independently from occurrence facts, then compare the declaration's
// actual parent authority to the exact vector in the owner's pinned recipe.
template<class Access> Cost inspect_pinned(const assembly::LetterProjection &projection,
    const assembly::LetterExchangeContract &contract) {
  Cost cost;
  assembly::detail::PreparationWork work(UINT64_MAX);
  need(Access::contract_recipe(contract).get()==&projection.recipe());
  for (const auto *side : {&contract.outgoing(), &contract.incoming()}) {
    for (std::size_t i=0;i<side->size();++i) {
      const auto &source=(*side)[i];
      const auto *tool=lookup(projection,contract,source,work);
      need(tool && source.parents.capacity()==0 &&
           &Access::contract_parents(contract,source)==&tool->parents);
      ++cost.sources;
      cost.parents+=tool->parents.size();
      for (std::size_t j=0;j<i;++j) if ((*side)[j].definition==source.definition) {
        ++cost.repeated_sources;
        break;
      }
    }
  }
  cost.lookup_work=work.result().charged;
  return cost;
}
template<class Visit> Cost inspect(const assembly::LetterProjection &projection,
    const assembly::LetterExchangeContract &contract,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost cost;
  std::array<const void *,4096> seen{};
  std::size_t used=0;
  assembly::detail::PreparationWork work(UINT64_MAX);
  const auto buffer=[&](const void *p,std::size_t n,bool nested) {
    if (!n) return;
    need(used<seen.size() && std::find(seen.begin(),seen.begin()+used,p)==seen.begin()+used);
    seen[used++]=p;
    visit(p,n); ++cost.requests;
    (nested ? cost.nested_bytes : cost.vector_bytes)+=n;
  };
  for (const auto *side : {&contract.outgoing(),&contract.incoming()}) {
    for (std::size_t i=0;i<side->size();++i) {
      const auto &source=(*side)[i];
      const auto *tool=lookup(projection,contract,source,work);
      need(tool && equal(source,*tool));
      ++cost.sources; cost.parents+=source.parents.size();
      for (std::size_t j=0;j<i;++j) if ((*side)[j].definition==source.definition) {
        ++cost.repeated_sources; break;
      }
      need(!source.parents.empty() && source.parents.data()!=tool->parents.data() &&
           source.parents.get_allocator().domain.get()==domain);
      buffer(source.parents.data(),source.parents.capacity()*sizeof(warm::Parent),false);
      for (std::size_t j=0;j<source.parents.size();++j) {
        const auto &pin=source.parents[j].target, &original=tool->parents[j].target;
        const auto text=[&](const auto &a,const auto &b) {
          need(a.get_allocator().domain.get()==domain);
          const auto bytes=warm::recipe_text_backing(a);
          need(!bytes || a.data()!=b.data());
          buffer(a.data(),bytes,true);
        };
        text(pin.object.scope.str(),original.object.scope.str());
        text(pin.revision.str(),original.revision.str());
        need(pin.object.address.get_allocator().domain.get()==domain &&
             pin.object.address.data()!=original.object.address.data());
        buffer(pin.object.address.data(),pin.object.address.capacity()*sizeof(pin.object.address[0]),true);
      }
    }
  }
  cost.lookup_work=work.result().charged;
  return cost;
}
}  // namespace exchange_parent_observation
