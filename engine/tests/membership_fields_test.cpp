#include <algorithm>
#include <iostream>
#include <set>

#include "harness.h"
#include "assembly/preparation_work.h"
#include "membership_fixture.h"

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

  // Unique scopes keep a long valid chain independent of fixture address ranges.
  letter_fixture::Fixture deep;
  std::vector<ToolRef> chain;
  constexpr std::size_t depth = 192;
  for (std::size_t i=0; i<depth; ++i) {
    auto ref = deep.add(object("02.00.00",new_uuid()),"chain",0);
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
}  // namespace

int main() {
  try {
    paths_and_contexts();
    mass_and_repetition();
    graph_failures();
    retained_alternatives();
    bounded_traversal();
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " membership value checks\n";
  return harness::report("membership_fields_test");
}
