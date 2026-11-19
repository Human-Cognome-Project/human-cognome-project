#include "prepared_letter_profile.h"
#include "retained_membership_fixture.h"
#include "snapshot.h"
#include "field/cascade_source.h"
#include <iostream>
#include <map>

namespace {
void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
bool same(const assembly::FieldKey &a, const assembly::FieldKey &b) {
  return a.kind == b.kind && a.object == b.object;
}
template<class F> void rejects(F f) {
  bool refused = false;
  try { f(); } catch (const viewport::PreparedLetterProfileError &) { refused = true; }
  check(refused,"unsupported membership input accepted");
}
void run(int workers, field::HostPath path) {
  using namespace assembly;
  retained_membership_fixture::Input input;
  const auto english = input.fixture.ref(input.fixture.english);
  const auto &base = input.fixture.base;
  rejects([&] { viewport::prepared_english_pair(base,english); });
  for (int i=0; i<5; ++i) {
    auto bad = base;
    auto &a = bad.tools[input.fixture.tools.at(input.fixture.english)];
    auto &alpha = bad.tools[input.fixture.tools.at(input.groups[0])];
    if (i == 0) a.member_of.pop_back();
    if (i == 1) alpha.mass = 1;
    if (i == 2) alpha.member_of[0].revision = warm::new_uuid();
    if (i == 3) bad.tools[input.fixture.tools.at(input.groups[3])].member_of.push_back(english);
    if (i == 4) bad.tools.erase(bad.tools.begin()+input.fixture.tools.at(input.groups[2]));
    rejects([&] { viewport::prepared_retained_english_pair(bad,english); });
  }
  const auto recipe = viewport::prepared_retained_english_pair(base,english);
  check(recipe.recipe_version == 2 && recipe.selections.size() == 2 && recipe.occurrences.empty(),"unexpected recipe form");
  for (const auto &t : base.tools) {
    bool same = false;
    for (const auto &pin : recipe.tools)
      if (pin.ref == t.ref && pin.epoch == t.epoch && warm::tool_body(pin) == warm::tool_body(t)) same = true;
    check(same,"preset changed supplied definition pins or NULL masses");
  }
  const auto owner = warm::new_uuid();
  auto source = std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(recipe,owner,false));
  auto request = viewport::prepared_retained_english_pair_submission(source,english,path);
  const auto &b = request.bindings, &expected = input.bindings;
  check(b.fields.size() == 6 && b.comparisons.size() == 6 && b.bodies.size() == 2 &&
        b.focus.size() == 1 && same(b.focus[0],expected.focus[0]) &&
        b.layout.size() == 1 && !b.layout[0].node.layout && b.layout[0].units == 1,
        "preset differs from reviewed topology/focus/layout");
  for (std::size_t i=0; i<6; ++i) {
    const auto &n = b.comparisons[i], &e = expected.comparisons[i];
    check(same(b.fields[i].field,expected.fields[i].field) && same(n.field,e.field) && n.comparison == e.comparison &&
          n.level == e.level && n.retained_fraction_bits == e.retained_fraction_bits && n.children == e.children,
          "preset differs from reviewed comparison chain");
  }
  check(request.capacity.limits().values == retained_membership_fixture::capacity().limits().values,
        "preset differs from reviewed finite capacity");
  std::map<std::string,int> addresses;
  for (const auto &body : b.bodies) {
    check(body.definition == english && body.mass == 1.5 && body.geometry.complete,"preset intrinsics changed");
    addresses.emplace(occurrence_text(body.occurrence),body.address);
  }
  engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
  ActiveSession session(settings);
  const auto result = session.submit_prepared(std::move(request),owner);
  check(result.handle && session.ready(*result.handle),"retained preset failed native submission");
  const auto before = viewport::observe(session,owner,*result.handle);
  field::Parameters64 parameters; parameters.dt = 0.1;
  const auto tick = session.tick_prepared(*result.handle,owner,parameters,1000000000,1000000);
  const auto after = viewport::observe(session,owner,*result.handle);
  check(tick.status == field::TickStatus::Ok && before.tick == 0 && after.tick == 1 &&
        after.prepared_generation == tick.generation && after.comparisons.size() == 6 && after.body_records.size() == 2,
        "retained preset did not publish/tick/observe complete native state");
  for (std::size_t i=0; i<2; ++i) {
    const auto &h = after.body_records[i];
    const auto &p = after.report.prepared.particles[i].occurrence;
    check(h.context && h.incarnation && h.record == p.slot && h.address == addresses.at(std::string(p.id)) &&
          field::detail::same_handle(h,before.body_records[i]),"retained preset lost full occurrence claim");
  }
  const auto demand = session.demand(*result.handle,owner);
  check(demand.fields_loaded == 6 && demand.edges_loaded == 12 && demand.force_requests == 12 &&
        demand.force_evaluations == 2 && demand.force_zero_response == 10,"preset changed reviewed cohort behavior");
  auto context = viewport::context(*source,*result.handle,1,"Synthetic retained preset control");
  context.source = {{"kind","fixture_only"},{"study_profile","prepared-retained-english-pair"}};
  const auto snapshot = viewport::snapshot(after,context);
  check(snapshot.at("frame").at("focal_fields").size() == 1 &&
        snapshot.at("frame").at("focal_fields").at(0).at("kind") == "parent_value","preset focus changed");
  std::cout << "retained_profile,source=fixture_only,retained_tick=unqualified,claims=2,comparisons=6,"
    "completed_tick=1,participation=12,force_evaluations=2,zero_responses=10 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("workers path required");
    run(std::stoi(argv[1]),std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy);
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
