#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>

#include "assembly/membership_fields.h"
#include "assembly/exchange_contract.h"
#include "assembly/preparation_work.h"
#include "letter_projection_fixture.h"
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "exchange_parent_observation.h"

namespace assembly::testing {
struct ExchangeContractAccess {
  static auto collect(const LetterProjection &p, const OccurrenceKey &owner, warm::Exposure target,
      const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
    return LetterExchangeContract::collect(p,owner,target,domain,&work);
  }
  static auto copy(const LetterExchangeContract &c,
      const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
    return LetterExchangeContract(c,domain,&work);
  }
  static auto reverse(const LetterExchangeContract &c,
      const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
    // The same copy and nonallocating swaps used by ExchangeOwner adoption.
    auto result = LetterExchangeContract(c,domain,&work);
    std::swap(result.from_,result.to_);
    result.outgoing_.swap(result.incoming_);
    return result;
  }
  static auto retained_reverse(const LetterExchangeContract &c,
      const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
    // Historical owner reverse-retention path, kept as a copy/lifetime control.
    work.charge(sizeof(LetterExchangeContract));
    auto result = reverse(c,domain,work);
    work.charge(sizeof(LetterExchangeContract));
    return std::allocate_shared<const LetterExchangeContract>(
        detail::SharedReservationAllocator<LetterExchangeContract>(domain),std::move(result));
  }
};
}
namespace {
using namespace letter_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok)
    throw warm::Error(label);
  ++checks;
}
template <class F>
void rejects(F run, const char *label) {
  try {
    run();
  } catch (const warm::Error &) {
    ++checks;
    return;
  }
  throw warm::Error(label);
}
void bounded_declaration(const LetterProjection &p, const OccurrenceKey &owner,
                         warm::Exposure target) {
  using namespace assembly::detail;
  using Access = assembly::testing::ExchangeContractAccess;
  const auto expected = declare_letter_exchange(p,owner,target);
  const auto exact = [&](const LetterExchangeContract &actual) {
    if (!(actual.owner() == expected.owner()) || actual.recipe().selection != expected.recipe().selection ||
        actual.recipe().revision != expected.recipe().revision || actual.from() != expected.from() ||
        actual.to() != expected.to() || actual.mass() != expected.mass() ||
        actual.match_work() != expected.match_work()) return false;
    const auto rows = [](const auto &a, const auto &b) {
      if (a.size() != b.size()) return false;
      for (std::size_t i=0; i<a.size(); ++i) {
        if (!(a[i].occurrence == b[i].occurrence) || !(a[i].definition == b[i].definition) ||
            a[i].mass != b[i].mass || a[i].node != b[i].node || a[i].parents.size() != b[i].parents.size()) return false;
        for (std::size_t j=0; j<a[i].parents.size(); ++j) {
          const auto &x=a[i].parents[j], &y=b[i].parents[j];
          if (!(x.target == y.target) || x.ordinal != y.ordinal ||
              x.contribution != y.contribution || x.stored_mass != y.stored_mass) return false;
        }
      }
      return true;
    };
    // Complete canonical paths/parents also retain every ordered nibble field.
    return rows(actual.outgoing(),expected.outgoing()) && rows(actual.incoming(),expected.incoming());
  };
  for (const bool copying : {false,true}) {
    const auto run = [&](const auto &domain, auto &work) {
      return copying ? Access::copy(expected,domain,work) : Access::collect(p,owner,target,domain,work);
    };
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    PreparationWork full(UINT64_MAX);
    { check(exact(run(domain,full)), "budgeted declaration preserves every ordered pinned source and leaf"); }
    check(!domain->result().live_bytes, "declaration output teardown releases its domain");
    const auto required=full.result().charged;
    std::uint64_t limit=0, boundaries=0;
    while (limit < required) {
      auto denied=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
      PreparationWork work(limit);
      bool stopped=false;
      try { run(denied,work); }
      catch (const PreparationStop &stop) {
        stopped=stop.result.status == PreparationStatus::WorkExhausted &&
            stop.result.charged <= limit && stop.result.next_required > limit && stop.result.next_required <= required;
        limit=stop.result.next_required;
      }
      check(stopped && !denied->result().live_bytes,
            "every declaration work boundary refuses and reclaims all partial storage");
      ++boundaries;
    }
    for (const bool bytes : {true,false}) {
      std::uint64_t allowance=0;
      const auto ceiling=bytes ? domain->result().peak_bytes : domain->result().charged_work;
      while (allowance < ceiling) {
        auto denied=std::make_shared<ReservationDomain>(ReservationBudget{
            bytes ? allowance : UINT64_MAX, bytes ? UINT64_MAX : allowance});
        PreparationWork work(required);
        bool stopped=false;
        try { run(denied,work); } catch (const ReservationStop &) { stopped=true; }
        const auto result=denied->result();
        const auto next=bytes ? result.required_bytes : result.required_work;
        check(stopped && !result.live_bytes && next > allowance && next <= ceiling,
              "every declaration allocation boundary reclaims partial identities and rows");
        allowance=next;
      }
      auto exact_domain=std::make_shared<ReservationDomain>(ReservationBudget{
          bytes ? ceiling : UINT64_MAX, bytes ? UINT64_MAX : ceiling});
      PreparationWork work(required);
      { check(exact(run(exact_domain,work)), "exact declaration allocation allowance preserves ordered values"); }
      check(!exact_domain->result().live_bytes, "exact declaration allocation retry releases storage");
    }
    PreparationWork short_work(required-1);
    bool stopped=false;
    try { run(domain,short_work); } catch (const PreparationStop &) { stopped=true; }
    check(stopped && !domain->result().live_bytes, "one-short declaration work refuses and tears down");
    PreparationWork exact_work(required);
    { check(exact(run(domain,exact_work)) && exact_work.result().charged == required,
            "exact declaration work retry retains independent values and order"); }
    check(!domain->result().live_bytes, "declaration retry releases all storage");
    std::cout << "declaration_setup,entry=" << owner.source.entry << ",copy=" << copying
              << ",work=" << required << ",boundaries=" << boundaries << " PASS\n";
  }
}
// Test-only borrowed traversal of the retained canonical byte alternative.
// No native handle supplies these semantic pins or source ordinals.
template<class Visit> bool parent_leaves(const LetterExchangeContract &contract, Visit visit) {
  const auto &parts = contract.from() == warm::Exposure::Parts ? contract.outgoing() : contract.incoming();
  if (parts.empty()) return false;
  for (const auto &source : parts) {
    if (source.occurrence.source.path.size() != 2 || source.parents.size() != 2) return false;
    const auto &path = source.occurrence.source.path;
    for (const auto &parent : source.parents)
      visit(path[0],path[1],parent.ordinal,parent.target,parent.contribution);
  }
  return true;
}
const allocation_lifetime::Entry *declaration_request(const void *pointer) {
  namespace A = allocation_lifetime;
  if (!pointer) return nullptr;
  for (std::size_t j=0, i=A::bucket(const_cast<void *>(pointer)); j<65536; ++j, i=(i+1)%65536) {
    const auto &entry = A::table[i];
    if (!entry.used) return nullptr;
    if (entry.pointer == pointer) return &entry;
  }
  return nullptr;
}
// Independent expected values only, outside the measured producer window.
struct ExpectedLeaf {
  std::uint32_t character{}, byte{}, nibble{};
  warm::ToolRef definition;
  std::uint64_t count{};
};
auto leaf_projection(Fixture &fixture) {
  fixture.base.focal_fields = {{warm::FieldKind::SiblingDefinition,fixture.repeated}};
  const auto recipe = normalize_structural_letters(fixture.base,
      {{UINT64_MAX,fixture.ref(fixture.repeated),warm::Exposure::Aggregate},
       {7,fixture.ref(fixture.dutch),warm::Exposure::Parts}});
  return std::make_unique<LetterProjection>(project_letters(recipe));
}
std::size_t leaf_inventory_cost(bool mixed, int mode) {
  using namespace assembly::detail;
  using Access = assembly::testing::ExchangeContractAccess;
  auto fixture = std::make_unique<Fixture>();
  const auto max_entry = std::numeric_limits<std::uint64_t>::max();
  auto projection = leaf_projection(*fixture);
  const OccurrenceKey owner{projection->study(),{mixed ? 7 : max_entry,{}}};
  const auto target = mixed ? warm::Exposure::Aggregate : warm::Exposure::Parts;
  // Independent expected leaves come from the validated immutable recipe,
  // independent of the declaration's retained source/parent descriptors.
  std::vector<ExpectedLeaf> expected;
  const auto &recipe = projection->recipe();
  const auto &selection = *std::find_if(recipe.selections.begin(),recipe.selections.end(),
      [&](const auto &s) { return s.entry == owner.source.entry; });
  for (const auto &id : selection.parts) {
    const auto &occurrence = *std::find_if(projection->occurrences().begin(),projection->occurrences().end(),
        [&](const auto &o) { return o.key.source == id; });
    const auto &tool = *std::find_if(recipe.tools.begin(),recipe.tools.end(),
        [&](const auto &t) { return t.ref == occurrence.definition; });
    for (const auto &parent : tool.parents)
      expected.push_back({id.path[0],id.path[1],parent.ordinal,parent.target,parent.contribution});
  }
  auto forward = std::make_unique<LetterExchangeContract>(declare_letter_exchange(*projection,owner,target));
  const auto outgoing = mode == 2 ? forward->incoming() : forward->outgoing();
  const auto incoming = mode == 2 ? forward->outgoing() : forward->incoming();
  auto stale = outgoing;
  stale.front().parents.front().target.revision = warm::new_uuid();
  const auto recipe_pin = forward->recipe();
  const auto from = mode == 2 ? forward->to() : forward->from();
  const auto to = mode == 2 ? forward->from() : forward->to();
  const auto mass = forward->mass(), match_work = forward->match_work();
  auto domain = mode == 0 ? std::shared_ptr<ReservationDomain>{} :
      std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  PreparationWork work(UINT64_MAX);
  std::optional<LetterExchangeContract> contract;
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  if (mode == 0) contract.emplace(declare_letter_exchange(*projection,owner,target));
  else if (mode == 1) contract.emplace(Access::collect(*projection,owner,target,domain,work));
  else contract.emplace(Access::reverse(*forward,domain,work));
  allocation_observation::enabled = false;
  const auto calls = allocation_observation::calls.load(), requested = allocation_observation::bytes.load();
  const auto retained = allocation_lifetime::counts;
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  const auto parent_cost=exchange_parent_observation::inspect(*projection,*contract,domain.get(),
      [&](const void *pointer,std::size_t bytes) {
        const auto *request=declaration_request(pointer);
        check(request && request->bytes==bytes,"parent duplicate is an actual independent producer request");
      });
  const auto lookup_calls=allocation_observation::calls.load();
  allocation_observation::enabled = false;
  check(!lookup_calls && parent_cost.bytes()<=retained.live,
        "full pinned recipe lookup allocates nothing and parent requests fit actual producer");
  {
    namespace P=exchange_parent_observation;
    const auto &original=contract->outgoing().front();
    PreparationWork lookup_work(UINT64_MAX);
    const auto *tool=P::lookup(*projection,*contract,original,lookup_work);
    check(tool && P::equal(original,*tool),"recipe parent lookup checks exact source association");
    for (int defect=0;defect<6;++defect) {
      auto changed=original;
      if (defect==0) changed.occurrence.study=warm::new_uuid();
      if (defect==1) ++changed.occurrence.source.entry;
      if (defect==2) changed.occurrence.source.path.push_back(99);
      if (defect==3) changed.definition.revision=warm::new_uuid();
      if (defect==4) ++changed.mass;
      if (defect==5) ++changed.node;
      check(!P::lookup(*projection,*contract,changed,lookup_work),
            "borrowed parent lookup refuses foreign stale path mass or node association");
    }
    for (int defect=0;defect<5;++defect) {
      auto changed=original;
      if (defect==0) changed.parents.front().target.revision=warm::new_uuid();
      if (defect==1) ++changed.parents.front().ordinal;
      if (defect==2) changed.parents.front().stored_mass=999;
      if (defect==3) ++changed.parents.front().contribution;
      if (defect==4) changed.parents.push_back(changed.parents.front());
      check(P::lookup(*projection,*contract,changed,lookup_work)==tool && !P::equal(changed,*tool),
            "matching source pins alone do not excuse changed parent provenance or order");
    }
    PreparationWork zero(0);
    bool denied=false;
    try { P::lookup(*projection,*contract,original,zero); }
    catch (const PreparationStop &) { denied=true; }
    check(denied,"inspection lookup work is explicit rather than an unlimited operation allowance");
    PreparationWork measured(UINT64_MAX);
    check(P::lookup(*projection,*contract,original,measured)==tool,"fresh lookup retains full source identity");
    PreparationWork short_work(measured.result().charged-1), exact_work(measured.result().charged);
    denied=false;
    try { P::lookup(*projection,*contract,original,short_work); }
    catch (const PreparationStop &) { denied=true; }
    check(denied && P::lookup(*projection,*contract,original,exact_work)==tool,
          "one-short lookup refuses and exact retry retains existing recipe parents");
  }
  const auto &parts = contract->from() == warm::Exposure::Parts ? contract->outgoing() : contract->incoming();
  const auto part_rows = parts.size(), part_capacity = parts.capacity();
  const auto *backing = parts.data();
  std::size_t parent_bytes = 0, parent_blocks = 0, rows = 0;
  bool exact = true, attributed = declaration_request(backing);
  const auto association = parent_leaves(*contract,[&](auto character,auto byte,auto nibble,const auto &pin,auto count) {
    const auto index = rows++;
    if (index >= expected.size()) { exact = false; return; }
    const auto &input = expected[index];
    exact &= character == input.character && byte == input.byte && nibble == input.nibble && pin == input.definition && count == input.count;
    const void *parent_buffers[]{pin.object.scope.str().data(),pin.object.address.data(),pin.revision.str().data()};
    for (const auto *buffer : parent_buffers) {
      const auto *request = declaration_request(buffer);
      attributed &= request != nullptr;
      if (request) { parent_bytes += request->bytes; ++parent_blocks; }
    }
    attributed &= pin.object.scope.str().get_allocator().domain == domain &&
        pin.object.address.get_allocator().domain == domain && pin.revision.str().get_allocator().domain == domain;
  });
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  std::size_t visited = 0;
  const auto borrow = parent_leaves(*contract,[&](auto character,auto byte,auto nibble,const auto &pin,auto count) {
    if (visited >= expected.size()) { exact = false; return; }
    const auto &leaf = expected[visited++];
    exact &= character == leaf.character && byte == leaf.byte && nibble == leaf.nibble && pin == leaf.definition && count == leaf.count;
  });
  const auto borrow_calls = allocation_observation::calls.load(), borrow_bytes = allocation_observation::bytes.load();
  allocation_observation::enabled = false;
  forward.reset(); projection.reset(); fixture.reset();
  bool detached = parts.data() == backing && contract->owner() == owner && contract->recipe().selection == recipe_pin.selection &&
      contract->recipe().revision == recipe_pin.revision && contract->from() == from && contract->to() == to && contract->mass() == mass &&
      contract->match_work() == match_work && contract->matches(outgoing.data(),outgoing.size(),incoming.data(),incoming.size(),match_work);
  std::size_t index = 0;
  const auto detached_visit = parent_leaves(*contract,[&](auto character,auto byte,auto nibble,const auto &pin,auto count) {
    if (index >= expected.size()) { detached = false; return; }
    const auto &leaf = expected[index++];
    detached &= character == leaf.character && byte == leaf.byte && nibble == leaf.nibble && pin == leaf.definition && count == leaf.count;
  });
  detached &= detached_visit;
  const bool stale_refused = !contract->matches(stale.data(),stale.size(),incoming.data(),incoming.size(),match_work);
  const auto domain_live = domain ? domain->result().live_bytes : 0;
  contract.reset();
  const auto released = allocation_lifetime::end();
  check(exact && association && borrow && index == rows && visited == rows && detached,
        "actual source parents preserve every ordered complete nibble pin/count after all sources die");
  check(rows == (mixed ? 10U : 8U) && attributed && parent_blocks == 3*rows && parent_bytes,
        "actual factory retains complete parent pin backing for repeated and multibyte paths");
  check(!domain || domain_live == retained.live,
        "selected domain equals the complete actual retained declaration requests");
  check(!borrow_calls && !borrow_bytes,"existing source parents supply every ordered leaf without allocation");
  check(stale_refused,"full parent target revision remains required with borrowed nibble provenance");
  check(retained.acquired == calls && !released.live && !released.blocks && released.acquired == released.released && (!domain || !domain->result().live_bytes),
        "actual factory scratch and detached declaration release every tracked request");
  std::cout << "exchange_parent_producer,mixed=" << mixed << ",mode=" << mode
            << ",sources=" << parent_cost.sources << ",parents=" << parent_cost.parents
            << ",parent_vector_requested=" << parent_cost.vector_bytes
            << ",parent_pin_requested=" << parent_cost.nested_bytes
            << ",parent_requests=" << parent_cost.requests << ",parent_requested=" << parent_cost.bytes()
            << ",repeated_sources=" << parent_cost.repeated_sources << ",lookup_work=" << parent_cost.lookup_work
            << ",lookup_allocations=" << lookup_calls << ",runtime_saving=0,detached=1 PASS\n";
  std::cout << "exchange_leaf_cost,mixed=" << mixed << ",mode=" << mode << ",rows=" << rows
            << ",part_rows=" << part_rows << ",part_capacity=" << part_capacity
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",retained_requested=" << retained.live << ",peak_requested=" << retained.peak
            << ",parent_pin_blocks=" << parent_blocks << ",parent_pin_requested=" << parent_bytes
            << ",contract_object_bytes=" << sizeof(LetterExchangeContract) << ",domain_live=" << domain_live
            << ",borrow_calls=" << borrow_calls << ",after_destroy_requested=" << released.live
            << ",preparation_work=" << work.result().charged << '\n';
  return calls;
}
void declaration_allocation_failures(bool mixed, int mode, std::size_t requests) {
  using namespace assembly::detail;
  using Access = assembly::testing::ExchangeContractAccess;
  Fixture fixture;
  const auto projection = leaf_projection(fixture);
  const OccurrenceKey owner{projection->study(),{mixed ? 7 : UINT64_MAX,{}}};
  const auto target = mixed ? warm::Exposure::Aggregate : warm::Exposure::Parts;
  const auto forward = declare_letter_exchange(*projection,owner,target);
  for (std::size_t fail=1; fail<=requests; ++fail) {
    auto domain = mode == 0 ? std::shared_ptr<ReservationDomain>{} :
        std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    PreparationWork work(UINT64_MAX);
    std::optional<LetterExchangeContract> contract;
    bool refused = false;
    allocation_lifetime::begin(fail);
    try {
      if (mode == 0) contract.emplace(declare_letter_exchange(*projection,owner,target));
      else if (mode == 1) contract.emplace(Access::collect(*projection,owner,target,domain,work));
      else contract.emplace(Access::reverse(forward,domain,work));
    } catch (const std::bad_alloc &) { refused = true; }
    contract.reset();
    const auto released = allocation_lifetime::end();
    check(refused && released.attempts == fail && !released.live && !released.blocks &&
              released.acquired == released.released && (!domain || !domain->result().live_bytes),
          "every actual declaration C++ allocation refusal releases scratch and output");
  }
}
template<class A, class B> bool same_source_rows(const A &a, const B &b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i=0; i<a.size(); ++i) {
    if (!(a[i].occurrence == b[i].occurrence) || !(a[i].definition == b[i].definition) ||
        a[i].mass != b[i].mass || a[i].node != b[i].node || a[i].parents.size() != b[i].parents.size()) return false;
    for (std::size_t j=0; j<a[i].parents.size(); ++j) {
      const auto &x=a[i].parents[j], &y=b[i].parents[j];
      if (!(x.target == y.target) || x.ordinal != y.ordinal ||
          x.stored_mass != y.stored_mass || x.contribution != y.contribution) return false;
    }
  }
  return true;
}
void reverse_payload_cost(int sample, bool bounded) {
  using namespace assembly::detail;
  using Access = assembly::testing::ExchangeContractAccess;
  auto fixture = std::make_unique<Fixture>();
  auto projection = [&] {
    if (sample != 2) return leaf_projection(*fixture);
    fixture->base.focal_fields = {{warm::FieldKind::SiblingDefinition,fixture->english}};
    return std::make_unique<LetterProjection>(project_letters(normalize_structural_letters(fixture->base,
        {{13,fixture->ref(fixture->english),warm::Exposure::Aggregate}})));
  }();
  const OccurrenceKey owner{projection->study(),{sample == 0 ? UINT64_MAX : sample == 1 ? 7 : 13,{}}};
  const auto target = sample == 1 ? warm::Exposure::Aggregate : warm::Exposure::Parts;
  auto forward = std::make_shared<const LetterExchangeContract>(declare_letter_exchange(*projection,owner,target));
  std::weak_ptr<const LetterExchangeContract> source_watch = forward;
  const auto expected_outgoing = forward->incoming(), expected_incoming = forward->outgoing();
  auto stale = expected_outgoing, foreign = expected_outgoing;
  stale.front().parents.front().target.revision = warm::new_uuid();
  foreign.front().occurrence.study = warm::new_uuid();
  const auto recipe_pin = forward->recipe();
  const auto from = forward->to(), to = forward->from();
  const auto mass = forward->mass(), matching_work = forward->match_work();
  auto domain = bounded ? std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}) :
      std::shared_ptr<ReservationDomain>{};
  std::weak_ptr<ReservationDomain> domain_watch = domain;
  PreparationWork work(UINT64_MAX);
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  auto reverse = Access::retained_reverse(*forward,domain,work);
  allocation_observation::enabled = false;
  const auto calls = allocation_observation::calls.load(), requested = allocation_observation::bytes.load();
  const auto retained = allocation_lifetime::counts;
  const auto address = reinterpret_cast<std::uintptr_t>(reverse.get());
  std::size_t object_request = 0;
  for (const auto &entry : allocation_lifetime::table) {
    const auto base = reinterpret_cast<std::uintptr_t>(entry.pointer);
    if (entry.pointer && address >= base && address-base < entry.bytes) object_request = entry.bytes;
  }
  std::size_t payload_bytes = 0, payload_blocks = 0;
  bool attributed = true, distinct = true;
  const auto buffer = [&](const void *value, std::size_t bytes, const void *source) {
    if (!bytes) return;
    const auto *request = declaration_request(value);
    attributed &= request && request->bytes == bytes;
    distinct &= value != source;
    if (request) { payload_bytes += request->bytes; ++payload_blocks; }
  };
  const auto table = [&](const auto &value, const auto &source) {
    using T = typename std::decay_t<decltype(value)>::value_type;
    attributed &= value.get_allocator().domain == domain;
    buffer(value.data(),value.capacity()*sizeof(T),source.data());
  };
  const auto uuid = [&](const auto &value, const auto &source) {
    attributed &= value.str().get_allocator().domain == domain;
    buffer(value.str().data(),warm::recipe_text_backing(value.str()),source.str().data());
  };
  const auto reference = [&](const auto &value, const auto &source) {
    uuid(value.object.scope,source.object.scope);
    table(value.object.address,source.object.address);
    uuid(value.revision,source.revision);
  };
  const auto sources = [&](const auto &value, const auto &source) {
    table(value,source);
    if (value.size() != source.size()) { attributed = false; return; }
    for (std::size_t i=0; i<value.size(); ++i) {
      uuid(value[i].occurrence.study,source[i].occurrence.study);
      table(value[i].occurrence.source.path,source[i].occurrence.source.path);
      reference(value[i].definition,source[i].definition);
      table(value[i].parents,source[i].parents);
      if (value[i].parents.size() != source[i].parents.size()) { attributed = false; return; }
      for (std::size_t j=0; j<value[i].parents.size(); ++j)
        reference(value[i].parents[j].target,source[i].parents[j].target);
    }
  };
  uuid(reverse->owner().study,forward->owner().study);
  table(reverse->owner().source.path,forward->owner().source.path);
  uuid(reverse->recipe().selection,forward->recipe().selection);
  uuid(reverse->recipe().revision,forward->recipe().revision);
  sources(reverse->outgoing(),forward->incoming());
  sources(reverse->incoming(),forward->outgoing());
  const auto domain_live = domain ? domain->result().live_bytes : 0;
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  auto alias = forward; // Prototype: existing immutable ownership, explicit reversed roles.
  const bool roles = reverse->from() == alias->to() && reverse->to() == alias->from() &&
      same_source_rows(reverse->outgoing(),alias->incoming()) && same_source_rows(reverse->incoming(),alias->outgoing());
  const bool matching = alias->matches(reverse->incoming().data(),reverse->incoming().size(),
      reverse->outgoing().data(),reverse->outgoing().size(),matching_work);
  const bool wrong_role = !alias->matches(reverse->outgoing().data(),reverse->outgoing().size(),
      reverse->incoming().data(),reverse->incoming().size(),matching_work);
  const bool short_work = !alias->matches(reverse->incoming().data(),reverse->incoming().size(),
      reverse->outgoing().data(),reverse->outgoing().size(),matching_work-1);
  const bool stale_refused = !alias->matches(expected_incoming.data(),expected_incoming.size(),stale.data(),stale.size(),matching_work);
  const bool foreign_refused = !alias->matches(expected_incoming.data(),expected_incoming.size(),foreign.data(),foreign.size(),matching_work);
  const auto alias_calls = allocation_observation::calls.load(), alias_bytes = allocation_observation::bytes.load();
  allocation_observation::enabled = false;
  forward.reset(); projection.reset(); fixture.reset();
  bool detached = !source_watch.expired() && roles && same_source_rows(reverse->outgoing(),alias->incoming()) &&
      same_source_rows(reverse->incoming(),alias->outgoing());
  alias.reset();
  detached &= source_watch.expired() && reverse->owner() == owner && reverse->recipe().selection == recipe_pin.selection &&
      reverse->recipe().revision == recipe_pin.revision && reverse->from() == from && reverse->to() == to && reverse->mass() == mass &&
      reverse->matches(expected_outgoing.data(),expected_outgoing.size(),expected_incoming.data(),expected_incoming.size(),matching_work);
  source_watch.reset();
  std::weak_ptr<const LetterExchangeContract> weak = reverse;
  reverse.reset();
  const auto weak_requested = allocation_lifetime::counts.live;
  const bool weak_lifetime = weak.expired() && allocation_lifetime::counts.blocks == 1 && weak_requested == object_request &&
      (!domain || domain->result().live_bytes == object_request);
  domain.reset();
  const bool domain_held = !bounded || !domain_watch.expired();
  weak.reset();
  const bool domain_released = !bounded || domain_watch.expired();
  const auto released = allocation_lifetime::end();
  check(attributed && distinct && payload_blocks+1 == retained.blocks && payload_bytes+object_request == retained.live &&
            object_request >= sizeof(LetterExchangeContract) && (!bounded || domain_live == retained.live),
        "actual retained reverse snapshot owns separately attributed copies of every immutable source buffer");
  check(roles && matching && wrong_role && short_work && stale_refused && foreign_refused,
        "explicit reversed roles preserve full source matching, revisions, owner identity and work refusal");
  check(sample != 2 || (expected_outgoing.size() == expected_incoming.size() && wrong_role),
        "equal one-byte row counts cannot determine semantic exchange direction");
  check(!alias_calls && !alias_bytes && detached,
        "a const payload alias serves both explicit roles without allocations after projection/source teardown");
  check(weak_lifetime && domain_held && domain_released,
        "weak references retain the shared object request/domain after data destruction until final weak release");
  check(retained.acquired == calls && !released.live && !released.blocks && released.acquired == released.released,
        "retained reverse factory and weak teardown release every observed C++ request");
  std::cout << "reverse_payload_cost,sample=" << sample << ",bounded=" << bounded
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",retained_requested=" << retained.live << ",peak_requested=" << retained.peak
            << ",payload_blocks=" << payload_blocks << ",payload_requested=" << payload_bytes
            << ",object_request=" << object_request << ",contract_object_bytes=" << sizeof(LetterExchangeContract)
            << ",weak_requested=" << weak_requested << ",domain_live=" << domain_live
            << ",alias_calls=" << alias_calls << ",after_destroy_requested=" << released.live
            << ",preparation_work=" << work.result().charged << '\n';
}
}  // namespace

int main() {
  try {
    for (int sample=0; sample<3; ++sample)
      for (bool bounded : {false,true}) reverse_payload_cost(sample,bounded);
    for (bool mixed : {false,true})
      for (int mode=0; mode<3; ++mode)
        declaration_allocation_failures(mixed,mode,leaf_inventory_cost(mixed,mode));
    Fixture f;
    auto base = f.base;
    base.focal_fields = {
        {warm::FieldKind::ParentValue, f.ref(f.english).object}};
    // Select a real positive parent field, not an arbitrary definition name.
    base.focal_fields[0].object =
        f.base.tools.at(f.tools.at(f.english)).parents[0].target.object;
    const auto max_entry = std::numeric_limits<std::uint64_t>::max();
    auto b = normalize_structural_letters(
        base, {{max_entry, f.ref(f.english), warm::Exposure::Aggregate},
               {3, f.ref(f.english), warm::Exposure::Parts},
               {7, f.ref(f.dutch), warm::Exposure::Aggregate},
               {8, f.ref(f.repeated), warm::Exposure::Parts}});
    check(b.recipe_version == 2 && b.occurrences.empty() &&
              b.structural_occurrences.size() == 15,
          "coordinate-free aggregate and all exact byte alternatives");
    // Published identities must outlive the source of borrowed expansion refs.
    auto p = [&] {
      auto source = b;
      std::reverse(source.structural_occurrences.begin(), source.structural_occurrences.end());
      return project_letters(source);
    }();
    for (const auto &published : p.occurrences()) {
      const auto selection = std::find_if(b.selections.begin(), b.selections.end(),
          [&](const auto &s) { return s.entry == published.key.source.entry; });
      check(selection != b.selections.end(), "published occurrence has a selection");
      const bool aggregate = published.key.source.path.empty();
      check(published.active == (aggregate ==
                (selection->exposure == warm::Exposure::Aggregate)),
            "source-death publication retains exact aggregate/parts activity");
      const auto &definition = b.tools.at(f.tools.at(published.definition.object));
      const bool complete = aggregate && definition.parents.size() == 1;
      check(bool(published.complete_character) == complete &&
                (!complete || *published.complete_character == definition.parents.front().target),
            "complete character retains pinned reference even for inactive aggregates");
    }
    std::size_t parent_index = 0;
    for (const auto &published : p.occurrences()) {
      if (!published.active) continue;
      const auto &definition = b.tools.at(f.tools.at(published.definition.object));
      for (const auto &expected : definition.parents) {
        check(parent_index < p.parents().size(), "all active parent rows are published");
        const auto &parent = p.parents()[parent_index++];
        check(parent.occurrence == published.key &&
                  parent.field.kind == GroupKind::ParentValue &&
                  parent.field.object == expected.target.object &&
                  parent.ordinal == expected.ordinal && parent.count == expected.contribution,
              "parent publication retains full occurrence/field/ordinal/count after source death");
      }
    }
    check(parent_index == p.parents().size(), "no inactive or duplicate parent rows published");
    std::vector<warm::OccurrenceId> expected_ids;
    for (const auto &row : b.structural_occurrences) expected_ids.push_back(row.id);
    std::sort(expected_ids.begin(), expected_ids.end());
    std::vector<warm::OccurrenceId> published_ids;
    for (const auto &row : p.occurrences()) published_ids.push_back(row.key.source);
    check(published_ids == expected_ids,
          "published occurrence order is by value after shuffled source death");
    for (const auto &published : p.occurrences()) {
      const auto source = std::find_if(b.structural_occurrences.begin(), b.structural_occurrences.end(),
          [&](const auto &row) { return row.id == published.key.source; });
      check(source != b.structural_occurrences.end() &&
                published.definition == f.ref(source->definition) && published.node == source->node,
            "published occurrence retains exact source definition and node after source death");
    }
    check(std::all_of(p.occurrences().begin(), p.occurrences().end(),
                      [](const auto &o) { return o.slot == -1; }),
          "structural projection assigns no numerical slots");
    check(p.parents().size() == 14,
          "ordered parent contribution rows preserved");
    std::vector<StudyNode> nodes;
    for (const auto &n : b.nodes)
      nodes.push_back({reinterpret_cast<const taichi::lang::SNode *>(1),
                       n.parent, static_cast<assembly::ScaleRole>(n.role)});
    auto input = membership_fields(p).bind_structure(p, nodes);
    std::set<std::string> expected_definitions;
    for (const auto &row : b.structural_occurrences)
      expected_definitions.insert(warm::key(row.definition));
    std::vector<std::string> published_definitions;
    for (const auto &definition : input.definitions)
      published_definitions.emplace_back(definition.id);
    check(published_definitions == std::vector<std::string>(
              expected_definitions.begin(), expected_definitions.end()),
          "definitions survive source death with exact value deduplication and encoded order");
    check(input.occurrences.size() == 15 && input.selections.size() == 4 &&
              input.focal_fields.size() == 1 && input.nodes.size() == 2,
          "structural handoff retains definitions, selection, focus and roles");
    rejects([&] { p.bind(nodes); }, "v2 cannot enter legacy numerical binding");
    auto aliased = b;
    aliased.nodes.push_back({0, warm::ScaleRole::SecondaryCompression});
    aliased.nodes.push_back({2, warm::ScaleRole::NamingEquivalent});
    for (auto &o : aliased.structural_occurrences)
      if (!o.id.path.empty())
        o.node = 3;
    warm::validate(aliased);
    auto alias_nodes = nodes;
    alias_nodes.push_back(
        {nodes[0].layout, 0, assembly::ScaleRole::SecondaryCompression});
    alias_nodes.push_back(
        {nodes[0].layout, 2, assembly::ScaleRole::NamingEquivalent});
    check(
        project_letters(aliased).bind_structure(alias_nodes).nodes.size() == 4,
        "secondary compression and naming preserve conceptual depth at 1:1");
    auto malformed = b;
    malformed.occurrences.push_back({{99, {}}, f.english, 0, {}, {}});
    rejects([&] { warm::validate(malformed); },
            "legacy starts in v2 must reject");
    malformed = b;
    malformed.structural_occurrences.pop_back();
    rejects([&] { warm::validate(malformed); },
            "missing inactive alternative must reject");
    malformed = b;
    malformed.focal_fields.clear();
    rejects([&] { warm::validate(malformed); },
            "missing explicit focus must reject");
    malformed = b;
    malformed.focal_fields.push_back(malformed.focal_fields[0]);
    rejects([&] { warm::validate(malformed); }, "duplicate focus must reject");
    malformed = b;
    malformed.focal_fields = {{warm::FieldKind::SiblingDefinition, f.accented}};
    rejects([&] { warm::validate(malformed); }, "unused focus must reject");
    malformed = b;
    malformed.focal_fields = {{static_cast<warm::FieldKind>(99), f.english}};
    rejects([&] { warm::validate(malformed); },
            "unknown focus kind must reject");
    malformed = b;
    malformed.structural_occurrences[0].node = 99;
    rejects([&] { warm::validate(malformed); },
            "unsupported role binding must reject");
    auto legacy = normalize_letters(
        f.base, {f.request(f.english, 4, warm::Exposure::Aggregate)});
    const auto original = legacy.occurrences[0].position;
    auto adapted = warm::structural_from_v1(
        legacy, {warm::new_uuid(), warm::new_uuid()},
        {{warm::FieldKind::SiblingDefinition, f.english}});
    check(adapted.occurrences.empty() &&
              adapted.structural_occurrences.size() == 2 &&
              legacy.recipe_version == 1 &&
              legacy.occurrences[0].position.x == original.x,
          "explicit legacy adapter leaves original starts unchanged");
    rejects(
        [&] {
          warm::structural_from_v1(legacy, legacy.ref,
              {adapted.focal_fields.begin(),adapted.focal_fields.end()});
        },
        "adapter must not overwrite an immutable publication identity");
    auto old = project_letters(legacy);
    rejects([&] { old.bind_structure({nodes[0]}); },
            "legacy requires explicit structural adapter");
    auto f2 = f;
    // Group contributions remain distinct per live occurrence, including bytes
    // inheriting their owning letter's groups.
    const auto group = f2.add(object("02.00.00.00.00"), "English", 0);
    f2.base.tools.at(f2.tools.at(f2.english)).member_of = {group};
    f2.base.focal_fields = {{warm::FieldKind::Membership, group.object}};
    auto grouped = normalize_structural_letters(
        f2.base, {{0, f2.ref(f2.english), warm::Exposure::Aggregate},
                  {1, f2.ref(f2.english), warm::Exposure::Parts}});
    auto gp = project_letters(grouped);
    check(membership_fields(gp).contributions().size() == 2,
          "membership retains distinct uses and byte owner inheritance");
    check(
        membership_fields(gp).bind_structure(gp, nodes).memberships.size() == 2,
        "structural membership handoff complete");
    // Declarations are pinned to one study/occurrence, not just a definition.
    for (const auto entry : {max_entry, std::uint64_t(3), std::uint64_t(7), std::uint64_t(8)}) {
      const bool aggregate = entry == max_entry || entry == 7;
      const auto target = aggregate ? warm::Exposure::Parts : warm::Exposure::Aggregate;
      const OccurrenceKey owner{p.study(), {entry, {}}};
      bounded_declaration(p,owner,target);
      const auto contract = declare_letter_exchange(p, owner, target);
      auto outgoing = contract.outgoing(), incoming = contract.incoming();
      const auto matches = [&] {
        return contract.matches(outgoing.data(), outgoing.size(), incoming.data(), incoming.size(),
                                contract.match_work());
      };
      check(contract.owner() == owner && contract.to() == target &&
                contract.recipe().revision == b.ref.revision && matches(),
            "exact aggregate/byte exchange declaration");
      check(!contract.matches(outgoing.data(), outgoing.size(), incoming.data(), incoming.size(),
                               contract.match_work() - 1), "comparison budget rejects before matching");
      std::uint64_t out_mass = 0, in_mass = 0, leaf_mass = 0;
      for (const auto &s : outgoing) out_mass += s.mass;
      for (const auto &s : incoming) in_mass += s.mass;
      std::size_t leaf_count = 0;
      bool repeated_paths = true;
      const auto walked = parent_leaves(contract,[&](auto character,auto byte,auto,const auto &,auto count) {
        leaf_mass += count;
        if (entry == 8 && (leaf_count == 4 || leaf_count == 6))
          repeated_paths &= character == 2 && byte == (leaf_count == 4 ? 0U : 1U);
        ++leaf_count;
      });
      check(walked && out_mass == contract.mass() && in_mass == out_mass && leaf_mass == out_mass,
            "both alternatives retain exact ordered leaf mass");
      if (entry == 8)
        check(outgoing.size() == 4 && outgoing[0].definition == outgoing[1].definition &&
                  !(outgoing[0].occurrence == outgoing[1].occurrence) && leaf_count == 8 && repeated_paths,
              "repeated bytes retain distinct occurrence paths and contributions");
      std::reverse(outgoing.begin(), outgoing.end());
      std::reverse(incoming.begin(), incoming.end());
      check(matches(), "adapter permutation preserves exchange identity");
      const auto original = outgoing;
      outgoing[0].occurrence.study = warm::new_uuid();
      check(!matches(), "foreign study rejected");
      outgoing = original;
      outgoing[0].occurrence.source.entry = entry == max_entry ? 3 : max_entry;
      check(!matches(), "another copy of the same definition is not outgoing authority");
      outgoing = original;
      outgoing[0].definition.revision = warm::new_uuid();
      check(!matches(), "stale definition revision rejected");
      outgoing = original;
      ++outgoing[0].mass;
      check(!matches(), "changed exact body mass rejected");
      outgoing = original;
      ++outgoing[0].node;
      check(!matches(), "changed scale binding rejected");
      outgoing = original;
      ++outgoing[0].parents[0].contribution;
      check(!matches(), "changed exact source contribution rejected");
      outgoing = original;
      ++outgoing[0].parents[0].ordinal;
      check(!matches(), "changed parent order rejected");
      outgoing = original;
      outgoing[0].parents[0].stored_mass = 999;
      check(!matches(), "changed source mass provenance rejected");
      outgoing = original;
      check(!contract.matches(nullptr, outgoing.size(), incoming.data(), incoming.size(), contract.match_work()) &&
                !contract.matches(outgoing.data(), 0, incoming.data(), incoming.size(), contract.match_work()),
            "missing outgoing binding rejected");
      incoming[0].definition.revision = warm::new_uuid();
      check(!matches(), "undeclared replacement rejected");
      incoming = contract.incoming();
      auto &parts = outgoing.size() > 1 ? outgoing : incoming;
      if (parts.size() > 1) {
        parts[1] = parts[0];
        check(!matches(), "duplicate part cannot substitute for a missing part");
      }
      rejects([&] { declare_letter_exchange(p, owner, contract.from()); }, "same alternative rejected");
      rejects([&] { declare_letter_exchange(p, owner, static_cast<warm::Exposure>(99)); },
              "unknown alternative rejected");
    }
    rejects([&] { declare_letter_exchange(p, {warm::new_uuid(), {3, {}}}, warm::Exposure::Aggregate); },
            "foreign declaration owner rejected");
    rejects([&] { declare_letter_exchange(p, {p.study(), {3, {0, 0}}}, warm::Exposure::Aggregate); },
            "byte path is not an aggregate exchange owner");
    rejects([&] { declare_letter_exchange(p, {p.study(), {99, {}}}, warm::Exposure::Parts); },
            "undeclared occurrence rejected");
    rejects([&] { declare_letter_exchange(old, {old.study(), {4, {}}}, warm::Exposure::Parts); },
            "legacy numerical recipe cannot authorize structural exchange");
    const auto expanded_recipe = normalize_structural_letters(base,
        {{11, f.ref(f.accented), warm::Exposure::Parts},
         {12, f.ref(f.english), warm::Exposure::Aggregate}});
    const auto unicode = project_letters(expanded_recipe);
    const auto unicode_exchange = declare_letter_exchange(unicode, {unicode.study(), {11, {}}},
                                                         warm::Exposure::Aggregate);
    std::size_t unicode_leaves = 0;
    const auto unicode_walked = parent_leaves(unicode_exchange,[&](auto,auto,auto,const auto &,auto) { ++unicode_leaves; });
    check(unicode_walked && unicode_exchange.outgoing().size() == 2 && unicode_leaves == unicode_exchange.mass(),
          "multibyte UTF-8 alternative retains every ordered nibble contribution");
    check(p.parents().size() == 14 && p.recipe().selections[0].exposure == b.selections[0].exposure &&
              p.recipe().ref.revision == b.ref.revision,
          "declaration and rejection leave projection unchanged");
    std::cout << "PASS: " << checks << " structural recipe checks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
