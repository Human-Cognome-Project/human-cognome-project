#include <algorithm>
#include <iostream>
#include <set>
#include <optional>

#include "harness.h"
#include "assembly/preparation_work.h"
#include "membership_fixture.h"
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"

namespace assembly::testing {
struct MembershipFieldsAccess {
  static auto catalogue(const LetterProjection &p) { return MembershipFields::collect(p,true); }
  static auto bounded(const LetterProjection &p, bool alternatives,
      const std::shared_ptr<detail::ReservationDomain> &domain, detail::PreparationWork &work) {
    return MembershipFields::collect(p,alternatives,domain,&work);
  }
};
}

namespace {
using namespace membership_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  CHECK_TRUE(ok, label);
}
template <class F>
void rejects(F run, const char *label) {
  bool refused = false;
  try {
    run();
  } catch (const warm::Error &) {
    refused = true;
  }
  check(refused, label);
}
void paths_and_contexts() {
  membership_fixture::Fixture f;
  const auto b = normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.english, 1, warm::Exposure::Aggregate)});
  const auto p = project_letters(b);
  const auto fields = membership_fields(p);
  const auto mass = totals(fields);
  check(fields.study() == p.study() && fields.contributions().size() == 6,
        "two uses of A each reach three membership fields");
  check(mass.at(f.en) == 4 && mass.at(f.en_alphabet) == 4 &&
            mass.at(f.family) == 4,
        "direct/alphabet-mediated English and ancestor paths deduplicate per "
        "occurrence");
  std::set<std::pair<std::string, std::string>> unique;
  bool identities = true, naming = true;
  for (const auto &edge : fields.contributions()) {
    unique.emplace(occurrence_text(edge.occurrence),
                   warm::key(edge.field.object));
    identities &= edge.field.kind == assembly::GroupKind::Membership &&
                  edge.target == f.ref(edge.field.object) &&
                  edge.occurrence.study == p.study() && edge.count == 2;
    naming &= !b.tools.at(f.tools.at(edge.target.object)).mass;
  }
  check(unique.size() == 6 && identities,
        "field kinds, full occurrence identities, pinned groups and exact "
        "counts retained");
  check(naming && p.occurrences().size() == 4 && p.parents().size() == 2,
        "NULL naming anchors add metadata/fields without particles or parent "
        "changes");
  bool unchanged = true;
  for (const auto &t : f.base.tools)
    unchanged &= warm::tool_body(t) ==
                 warm::tool_body(p.recipe().tools.at(f.tools.at(t.ref.object)));
  check(
      unchanged,
      "membership traversal preserves every source definition and membership");
  check(
      !mass.count(f.core_class),
      "core byte classification is not the owning language's membership graph");

  const auto mixed = project_letters(normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.english, 1, warm::Exposure::Parts),
               f.request(f.french, 2, warm::Exposure::Parts)}));
  const auto contextual = membership_fields(mixed);
  check(
      contextual.contributions().size() == 8,
      "mixed exposure emits memberships only for the three active occurrences");
  const auto active_byte =
      [&](std::uint64_t entry) -> const ProjectedOccurrence & {
    return *std::find_if(
        mixed.occurrences().begin(), mixed.occurrences().end(),
        [&](const auto &o) { return o.active && o.key.source.entry == entry; });
  };
  check(active_byte(1).definition == active_byte(2).definition,
        "English/French byte occurrences use exactly the same pinned byte "
        "definition");
  bool isolated = true, owners = true;
  for (const auto &edge : contextual.contributions()) {
    const bool french = edge.occurrence.source.entry == 2;
    const bool french_group =
        edge.field.object == f.fr || edge.field.object == f.fr_alphabet;
    isolated &= french == french_group && !(edge.field.object == f.core_class);
    if (!edge.occurrence.source.path.empty())
      owners &=
          active_byte(edge.occurrence.source.entry).byte->owner ==
          OccurrenceKey{mixed.study(), {edge.occurrence.source.entry, {}}};
  }
  check(isolated && owners,
        "shared byte definitions retain independent owner-context memberships "
        "without language leakage");
  const auto mixed_mass = totals(contextual);
  check(mixed_mass.at(f.en) == 4 && mixed_mass.at(f.en_alphabet) == 4 &&
            mixed_mass.at(f.fr) == 2,
        "active aggregate and byte uses contribute their own whole mass once "
        "per field");
  for (const auto &o : mixed.occurrences()) {
    if (o.active) continue;
    check(
        std::none_of(
            contextual.contributions().begin(),
            contextual.contributions().end(),
            [&](const auto &edge) { return edge.occurrence == o.key; }),
        "inactive aggregate/byte alternative has no membership participation");
  }
}
void mass_and_repetition() {
  membership_fixture::Fixture f;
  std::map<ObjectKey, std::uint64_t> aggregate_mass;
  for (const auto exposure :
       {warm::Exposure::Aggregate, warm::Exposure::Parts}) {
    const auto p = project_letters(normalize_letters(
        f.base,
        {f.request(f.repeated, 0, exposure), f.request(f.dutch, 1, exposure),
         f.request(f.accented, 2, exposure)}));
    const auto fields = membership_fields(p);
    const auto mass = totals(fields);
    check(
        mass.at(f.nl) == 22 && mass.at(f.nl_alphabet) == 22 && mass.size() == 2,
        "aa-combining-accent, Dutch and multibyte fixtures contribute exact "
        "mass 8+10+4");
    if (exposure == warm::Exposure::Aggregate) {
      aggregate_mass = mass;
      int repeated = 0;
      for (const auto &edge : p.parents())
        if (edge.occurrence.source.entry == 0 && edge.count == 2 &&
            edge.ordinal < 2)
          ++repeated;
      check(
          repeated == 2,
          "membership preparation leaves repeated source a ordinals separate");
    } else {
      check(mass == aggregate_mass && fields.contributions().size() == 22,
            "byte exposure conserves membership mass with one edge per target "
            "and byte position");
      check(std::all_of(fields.contributions().begin(),
                        fields.contributions().end(),
                        [](const auto &edge) {
                          return edge.count == 2 &&
                                 edge.occurrence.source.path.size() == 2;
                        }),
            "each exposed byte uses its mass and its distinct character/byte "
            "path");
      check(p.parents().size() == 22,
            "twenty-two ordered nibble contributions survive contextual "
            "membership preparation");
    }
  }
  letter_fixture::Fixture bare;
  const auto p = project_letters(normalize_letters(
      bare.base, {bare.request(bare.english, 0, warm::Exposure::Aggregate)}));
  check(membership_fields(p).contributions().empty(),
        "absent source ancestry stays absent; no inferred group paths");
}
void graph_failures() {
  membership_fixture::Fixture f;
  const auto b = normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate)});
  auto prepare = [](const Bundle &recipe) {
    return membership_fields(project_letters(recipe));
  };
  auto bad = b;
  f.tool(bad, f.en).member_of.push_back({object("02.00.00.00.zz"), new_uuid()});
  rejects([&] { prepare(bad); },
          "missing pinned membership endpoint rejected before handoff");
  bad = b;
  f.tool(bad, f.en_alphabet).member_of[0].revision = new_uuid();
  rejects([&] { prepare(bad); }, "inconsistent membership revision rejected");
  bad = b;
  f.tool(bad, f.en).member_of = {f.ref(f.en_alphabet)};
  rejects([&] { prepare(bad); }, "indirect upward membership cycle rejected");
  bad = b;
  f.tool(bad, f.en).member_of = {f.ref(f.en)};
  rejects([&] { prepare(bad); }, "self membership cycle rejected");
  bad = b;
  f.tool(bad, f.english).member_of.push_back(f.ref(f.en));
  rejects([&] { prepare(bad); },
          "duplicate direct source row rejected; multiple valid paths remain "
          "supported");
}

void bounded_traversal() {
  using namespace assembly::detail;
  using Access = assembly::testing::MembershipFieldsAccess;
  membership_fixture::Fixture f;
  auto p = project_letters(normalize_letters(f.base,
      {f.request(f.english,0,warm::Exposure::Aggregate),
       f.request(f.english,1,warm::Exposure::Parts)}));
  const auto signature = [](const MembershipFields &fields) {
    std::vector<std::string> out;
    for (const auto &m : fields.contributions())
      out.push_back(occurrence_text(m.occurrence)+"|"+warm::key(m.field.object)+"|"+
          std::string(m.target.revision.str())+"|"+std::to_string(m.count));
    return out;
  };
  const auto expected = signature(Access::catalogue(p));
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  PreparationWork full(UINT64_MAX);
  {
    const auto fields = Access::bounded(p,true,domain,full);
    check(signature(fields) == expected, "metered diamond catalogue preserves exact ordered values");
  }
  check(domain->result().live_bytes == 0, "successful traversal releases temporary and output storage");
  const auto required = full.result().charged;
  std::uint64_t limit = 0, boundaries = 0;
  while (limit < required) {
    auto denied = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    PreparationWork work(limit);
    bool stopped = false;
    try { Access::bounded(p,true,denied,work); }
    catch (const PreparationStop &stop) {
      stopped = stop.result.status == PreparationStatus::WorkExhausted &&
          stop.result.charged <= limit && stop.result.next_required > limit &&
          stop.result.next_required <= required;
      limit = stop.result.next_required;
    }
    check(stopped && denied->result().live_bytes == 0,
          "every work boundary refuses before its step and tears down all storage");
    ++boundaries;
  }
  for (const bool bytes : {true,false}) {
    std::uint64_t allowance = 0;
    const auto ceiling = bytes ? domain->result().peak_bytes : domain->result().charged_work;
    while (allowance < ceiling) {
      auto denied = std::make_shared<ReservationDomain>(ReservationBudget{
          bytes ? allowance : UINT64_MAX, bytes ? UINT64_MAX : allowance});
      PreparationWork work(required);
      bool stopped = false;
      try { Access::bounded(p,true,denied,work); }
      catch (const ReservationStop &) { stopped = true; }
      const auto result = denied->result();
      const auto next = bytes ? result.required_bytes : result.required_work;
      check(stopped && !result.live_bytes && next > allowance && next <= ceiling,
            "every allocation boundary including frame growth reclaims and reports next request");
      allowance = next;
    }
    auto exact = std::make_shared<ReservationDomain>(ReservationBudget{
        bytes ? ceiling : UINT64_MAX, bytes ? UINT64_MAX : ceiling});
    PreparationWork work(required);
    { check(signature(Access::bounded(p,true,exact,work)) == expected,
            "exact allocation allowance retries with identical contributions"); }
    check(!exact->result().live_bytes, "exact allocation retry releases all storage");
  }
  PreparationWork short_work(required-1);
  bool short_stop = false;
  try { Access::bounded(p,true,domain,short_work); }
  catch (const PreparationStop &) { short_stop = true; }
  check(short_stop && !domain->result().live_bytes, "one-short work limit refuses and tears down");
  PreparationWork exact_work(required);
  check(signature(Access::bounded(p,true,domain,exact_work)) == expected &&
            exact_work.result().charged == required,
        "exact work retry preserves deterministic contributions");
  std::cout << "membership_traversal,work=" << required << ",boundaries=" << boundaries
            << ",peak=" << domain->result().peak_bytes << " PASS\n";

  // Mutate an owned test projection so collect itself, not projection validation,
  // must reject the pin on a node already reached through the direct diamond edge.
  auto stale = p;
  f.tool(const_cast<Bundle &>(stale.recipe()),f.en_alphabet).member_of[0].revision = new_uuid();
  rejects([&] { membership_fields(stale); }, "memoized membership visit still validates pinned revision");
  auto cycle = p;
  f.tool(const_cast<Bundle &>(cycle.recipe()),f.en).member_of = {f.ref(f.en)};
  rejects([&] { membership_fields(cycle); }, "iterative traversal rejects active-stack cycles");

  // A separate public fixture address range keeps every chain dependency valid.
  letter_fixture::Fixture deep;
  std::vector<ToolRef> chain;
  constexpr std::size_t depth = 192;
  for (std::size_t i=0; i<depth; ++i) {
    const auto couplet = *codec::code_to_couplet(std::uint16_t(i));
    auto ref = deep.add(object("02.10."+std::string{couplet.first,couplet.second}),"chain",0);
    deep.tool(deep.base,ref.object).mass.reset();
    chain.push_back(ref);
  }
  for (std::size_t i=1; i<depth; ++i)
    deep.tool(deep.base,chain[i-1].object).member_of = {chain[i]};
  deep.tool(deep.base,deep.english).member_of = {chain.front(),chain.back()};
  const auto deep_projection = project_letters(normalize_letters(deep.base,
      {deep.request(deep.english,0,warm::Exposure::Aggregate)}));
  auto deep_domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  PreparationWork deep_work(UINT64_MAX);
  {
    const auto fields = Access::bounded(deep_projection,false,deep_domain,deep_work);
    check(fields.contributions().size() == depth,
          "deep explicit frame traversal deduplicates repeated terminal reachability");
    std::vector<ObjectKey> expected_keys;
    for (const auto &ref : chain) expected_keys.push_back(ref.object);
    std::sort(expected_keys.begin(),expected_keys.end());
    for (std::size_t i=0; i<depth; ++i)
      check(fields.contributions()[i].field.object == expected_keys[i],
            "deep traversal retains deterministic value order");
  }
  check(!deep_domain->result().live_bytes, "deep frame stack and reached sets fully release");
}
void retained_alternatives() {
  membership_fixture::Fixture f;
  f.base.focal_fields = {{warm::FieldKind::Membership,f.en}};
  std::vector<StructuralLetterRequest> requests =
      {{0,f.ref(f.english),warm::Exposure::Aggregate},
       {1,f.ref(f.english),warm::Exposure::Parts},
       {2,f.ref(f.french),warm::Exposure::Parts},
       {3,f.ref(f.repeated),warm::Exposure::Aggregate},
       {4,f.ref(f.accented),warm::Exposure::Parts}};
  const auto recipe = normalize_structural_letters(f.base,requests);
  const auto initial = project_letters(recipe);
  const auto catalogue = assembly::testing::MembershipFieldsAccess::catalogue(initial);
  const auto *backing = catalogue.contributions().data();
  const auto count = catalogue.contributions().size();
  check(std::all_of(catalogue.contributions().begin(),catalogue.contributions().end(),
                   [&](const auto &m) { return m.occurrence.study == initial.study(); }),
        "catalogue retains its own complete study identity");
  // Fresh projections mint their own study UUIDs. Compare source paths and all
  // pinned descriptors while checking each view's study separately.
  for (unsigned mask = 0; mask < 32; ++mask) {
    for (std::size_t i = 0; i < requests.size(); ++i)
      requests[i].exposure = mask & (1u << i) ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    // Normalization also assigns the matching structural floor for each view.
    const auto current = project_letters(normalize_structural_letters(f.base,requests));
    const auto live = membership_fields(current);
    bool exact = true;
    for (const auto &m : live.contributions()) {
      exact &= m.occurrence.study == current.study();
      exact &= std::count_if(catalogue.contributions().begin(),catalogue.contributions().end(),
          [&](const auto &c) { return c.occurrence.source == m.occurrence.source &&
            c.field.kind == m.field.kind && c.field.object == m.field.object &&
            c.target == m.target && c.count == m.count; }) == 1;
    }
    std::size_t active = 0;
    for (const auto &m : catalogue.contributions())
      active += std::any_of(current.occurrences().begin(),current.occurrences().end(),
          [&](const auto &o) { return o.active && o.key.source == m.occurrence.source; });
    check(exact && active == live.contributions().size(),
          "catalogue matches fresh memberships across exposure, repeated copies, diamonds and language contexts");
  }
  check(catalogue.contributions().data() == backing && catalogue.contributions().size() == count,
        "all exposure combinations preserve the retained catalogue");
  rejects([&] { catalogue.bind(initial,{}); }, "alternative catalogue cannot bind as live legacy participation");
  rejects([&] { catalogue.bind_structure(initial,{}); }, "alternative catalogue cannot bind as live structural participation");
}
const allocation_lifetime::Entry *tracked(const void *pointer) {
  if (!pointer) return nullptr;
  auto *p = const_cast<void *>(pointer);
  for (std::size_t n=0, i=allocation_lifetime::bucket(p); n<65536; ++n, i=(i+1)%65536) {
    const auto &entry = allocation_lifetime::table[i];
    if (!entry.used) return nullptr;
    if (entry.pointer == p) return &entry;
  }
  return nullptr;
}
// Attribute the actual collector's duplicated field key. The full target pin
// already owns that object; this witness changes no production representation.
void membership_key_cost(int mode) {
  using namespace assembly::detail;
  using Access = assembly::testing::MembershipFieldsAccess;
  auto fixture = std::make_unique<membership_fixture::Fixture>();
  if (mode != 0) fixture->base.focal_fields = {{warm::FieldKind::Membership,fixture->en}};
  auto recipe = mode == 0 ? normalize_letters(fixture->base,
      {fixture->request(fixture->english,2,warm::Exposure::Aggregate),
       fixture->request(fixture->english,10,warm::Exposure::Parts),
       fixture->request(fixture->french,20,warm::Exposure::Parts)}) :
      normalize_structural_letters(fixture->base,
      {{2,fixture->ref(fixture->english),warm::Exposure::Aggregate},
       {10,fixture->ref(fixture->english),warm::Exposure::Parts},
       {20,fixture->ref(fixture->french),warm::Exposure::Parts}});
  auto projection = std::make_unique<LetterProjection>(project_letters(recipe));
  const auto recipe_version = recipe.recipe_version;
  struct Expected { OccurrenceKey occurrence; ToolRef target; std::uint64_t count; };
  std::vector<Expected> expected;
  for (const auto &o : projection->occurrences()) {
    if (mode != 2 && !o.active) continue;
    std::vector<ObjectKey> groups = o.key.source.entry == 20
        ? std::vector<ObjectKey>{fixture->fr,fixture->fr_alphabet}
        : std::vector<ObjectKey>{fixture->en,fixture->en_alphabet,fixture->family};
    std::sort(groups.begin(),groups.end());
    for (const auto &group : groups) expected.push_back({o.key,fixture->ref(group),2});
  }
  const auto study = projection->study();
  auto domain = mode == 0 ? std::shared_ptr<ReservationDomain>{} :
      std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  PreparationWork work(UINT64_MAX);
  std::optional<MembershipFields> fields;
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try {
    if (mode == 0) fields.emplace(membership_fields(*projection));
    else fields.emplace(Access::bounded(*projection,mode == 2,domain,work));
  } catch (...) {
    allocation_observation::enabled = false;
    fields.reset(); allocation_lifetime::end(); throw;
  }
  allocation_observation::enabled = false;
  const auto calls = allocation_observation::calls.load(), requested = allocation_observation::bytes.load();
  const auto retained = allocation_lifetime::counts;
  const auto count = fields->contributions().size(), capacity = fields->contributions().capacity();
  const auto *outer = tracked(fields->contributions().data());
  const auto outer_bytes = outer ? outer->bytes : 0;
  std::size_t duplicate_bytes = 0, duplicate_blocks = 0, pin_object_bytes = 0;
  bool exact = count == expected.size(), distinct = true, attributed = outer;
  for (std::size_t i=0; i<count && i<expected.size(); ++i) {
    const auto &row = fields->contributions()[i];
    const auto &pin = row.target.object;
    exact &= row.field.kind == GroupKind::Membership && row.field.object == pin &&
             row.target == expected[i].target && row.occurrence == expected[i].occurrence && row.count == expected[i].count;
    const auto *scope = tracked(row.field.object.scope.str().data());
    const auto *address = tracked(row.field.object.address.data());
    const auto *pin_scope = tracked(pin.scope.str().data());
    const auto *pin_address = tracked(pin.address.data());
    attributed &= scope && address && pin_scope && pin_address &&
        row.field.object.scope.str().get_allocator().domain == domain &&
        row.field.object.address.get_allocator().domain == domain && row.target.revision.str().get_allocator().domain == domain;
    distinct &= row.field.object.scope.str().data() != pin.scope.str().data() && row.field.object.address.data() != pin.address.data();
    if (scope && address) { duplicate_bytes += scope->bytes + address->bytes; duplicate_blocks += 2; }
    if (pin_scope && pin_address) pin_object_bytes += pin_scope->bytes + pin_address->bytes;
  }
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  for (const auto &row : fields->contributions()) exact &= row.field.object == row.target.object;
  allocation_observation::enabled = false;
  const auto borrow_calls = allocation_observation::calls.load(), borrow_bytes = allocation_observation::bytes.load();
  const auto *backing = fields->contributions().data();
  projection.reset(); fixture.reset(); // All borrowed source/graph inputs die.
  const bool detached = fields->study() == study && fields->contributions().data() == backing &&
      allocation_lifetime::counts.live == retained.live;
  for (std::size_t i=0; i<count && i<expected.size(); ++i) {
    const auto &row = fields->contributions()[i];
    exact &= row.occurrence == expected[i].occurrence && row.target == expected[i].target &&
             row.field.object == expected[i].target.object && row.count == expected[i].count;
  }
  const auto domain_live = domain ? domain->result().live_bytes : 0;
  fields.reset();
  const auto released = allocation_lifetime::end();
  check(exact && detached,"membership field and full target pin retain exact detached context/count/revision after source death");
  check(count == (mode == 2 ? 16U : 8U) && attributed && distinct && duplicate_blocks == 2*count &&
        duplicate_bytes && duplicate_bytes == pin_object_bytes,
        "actual live/catalogue producer owns distinct equal field/pin key backing for every occurrence");
  check(outer_bytes == capacity*sizeof(MembershipContribution) && (!domain || domain_live == retained.live),
        "field member extent is inside actual vector capacity and selected-domain retained storage");
  check(!borrow_calls && !borrow_bytes,"existing full-pin object comparison allocates no storage");
  check(retained.acquired == calls && !released.live && !released.blocks && released.acquired == released.released &&
        (!domain || !domain->result().live_bytes),"collector scratch and detached contributions release every tracked request");
  std::cout << "membership_key_cost,mode=" << mode << ",recipe=" << recipe_version << ",rows=" << count << ",capacity=" << capacity
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",retained_requested=" << retained.live << ",peak_requested=" << retained.peak
            << ",duplicate_blocks=" << duplicate_blocks << ",duplicate_requested=" << duplicate_bytes
            << ",pin_object_requested=" << pin_object_bytes << ",field_member_bytes=" << sizeof(FieldKey)
            << ",key_object_bytes=" << sizeof(warm::ObjectKey)
            << ",row_object_bytes=" << sizeof(MembershipContribution) << ",outer_capacity_bytes=" << outer_bytes
            << ",domain_live=" << domain_live << ",borrow_calls=" << borrow_calls
            << ",after_destroy_requested=" << released.live << '\n';
}
}  // namespace

int main() {
  try {
    paths_and_contexts();
    mass_and_repetition();
    graph_failures();
    retained_alternatives();
    bounded_traversal();
    for (int mode=0; mode<3; ++mode) membership_key_cost(mode);
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " membership value checks\n";
  return harness::report("membership_fields_test");
}
