#include <cstdlib>
#include <functional>
#include <map>

#include "active_workbench_checks.h"
#include "shared_manager.h"

namespace {
using namespace warm;
using namespace active_checks;
std::string env(const char *name) {
  const auto *v = std::getenv(name);
  if (!v || !*v)
    throw Error(std::string("missing fixture connection: ") + name);
  return v;
}
void compare(const Bundle &a, const Bundle &b) {
  check(a.ref.selection == b.ref.selection &&
            a.ref.revision == b.ref.revision && a.owner == b.owner &&
            a.creator == b.creator && a.recipe_version == b.recipe_version,
        "fresh-client recipe identity and ownership exact");
  check(a.tools.size() == b.tools.size() &&
            a.occurrences.size() == b.occurrences.size() &&
            a.nodes.size() == b.nodes.size() &&
            a.geometry.size() == b.geometry.size() &&
            a.selections.size() == b.selections.size() &&
            a.contacts.size() == b.contacts.size(),
        "fresh-client complete recipe counts exact");
  bool exact = true;
  for (const auto &t : b.tools) {
    const auto found =
        std::find_if(a.tools.begin(), a.tools.end(),
                     [&](const auto &x) { return x.ref == t.ref; });
    exact &= found != a.tools.end() && tool_body(*found) == tool_body(t) &&
             found->author == t.author;
  }
  check(exact,
        "all tool revisions, repeated ordered parents, memberships and NULLs "
        "exact");
  for (std::size_t i = 0; i < b.occurrences.size(); ++i) {
    const auto &x = a.occurrences[i], &y = b.occurrences[i];
    exact &= x.id == y.id && x.definition == y.definition && x.node == y.node &&
             x.position.x == y.position.x && x.position.y == y.position.y &&
             x.position.z == y.position.z && x.velocity.x == y.velocity.x &&
             x.velocity.y == y.velocity.y && x.velocity.z == y.velocity.z;
  }
  for (std::size_t i = 0; i < b.geometry.size(); ++i) {
    const auto &x = a.geometry[i], &y = b.geometry[i];
    exact &= x.object == y.object && x.centroid.x == y.centroid.x &&
             x.centroid.y == y.centroid.y && x.centroid.z == y.centroid.z &&
             x.spheres.size() == y.spheres.size();
    for (std::size_t j = 0; j < y.spheres.size(); ++j)
      exact &= x.spheres[j].radius == y.spheres[j].radius &&
               x.spheres[j].centre.x == y.spheres[j].centre.x &&
               x.spheres[j].centre.y == y.spheres[j].centre.y &&
               x.spheres[j].centre.z == y.spheres[j].centre.z;
  }
  for (std::size_t i = 0; i < b.nodes.size(); ++i)
    exact &= a.nodes[i].parent == b.nodes[i].parent &&
             a.nodes[i].role == b.nodes[i].role;
  for (std::size_t i = 0; i < b.selections.size(); ++i)
    exact &= a.selections[i].entry == b.selections[i].entry &&
             a.selections[i].exposure == b.selections[i].exposure &&
             a.selections[i].parts == b.selections[i].parts;
  for (std::size_t i = 0; i < b.contacts.size(); ++i)
    exact &= a.contacts[i].a == b.contacts[i].a &&
             a.contacts[i].b == b.contacts[i].b;
  check(exact,
        "full occurrence paths, geometry, explicit units, exposure and initial "
        "values exact");
}
}  // namespace
int main(int argc, char **argv) {
  try {
    const bool cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    const auto conn = env("HCP_WARM_TEST_CONNINFO");
    const auto instance = new_uuid(), actor = new_uuid(), epoch = new_uuid();
    const Context context{actor, instance, WriterRole::Manager};
    std::vector<Binding> directory{
        {object("00"),
         BindingKind::Shared,
         epoch,
         new_uuid(),
         "retained-core",
         "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121",
         {actor}},
        {object("02"),
         BindingKind::Shared,
         epoch,
         new_uuid(),
         "retained-linguistics",
         "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296",
         {actor}}};
    Administrator admin(conn);
    admin.configure(directory);
    ColdReader core(env("HCP_CORE_TEST_CONNINFO")),
        ling(env("HCP_LING_TEST_CONNINFO"));
    check(core.read_only() && ling.read_only(),
          "retained cold sources server-enforced read-only");
    std::size_t calls = 0;
    const auto source = [&](const ObjectKey &o) {
      ++calls;
      return address_text(o.address).substr(0, 2) == "00"
                 ? core.read(o.address)
                 : ling.read(o.address);
    };
    std::optional<SharedTicket> ticket;
    Bundle recipe;
    const auto consumer1 = new_uuid(), consumer2 = new_uuid();
    {
      Toolbox store(conn, context);
      SharedManager manager(store, {object("00"), object("02")}, source);
      rejects(
          [&] { manager.read(object("y0.00.00.00.00", instance)); },
          "private scope rejected before public source resolution/open/query");
      check(calls == 0 && manager.activity().source_calls == 0,
            "rejected private request makes zero source calls");
      rejects([&] { manager.read(object("z0.00.00.00.00")); },
              "undelegated temporary root rejected before lookup");
      rejects(
          [&] { SharedManager bad(store, {object("y0", instance)}, source); },
          "private source delegation rejected");
      std::map<ObjectKey, Tool> tools;
      std::function<ToolRef(ObjectKey)> follow = [&](ObjectKey id) -> ToolRef {
        if (tools.count(id)) return tools.at(id).ref;
        const auto r = manager.read(id);
        if (!r) throw Error("required shared fixture endpoint missing");
        Tool t{{id, new_uuid()},
               epoch,
               "stage-d-shared-fixture-v1",
               r->notation,
               r->mass,
               {},
               {},
               actor};
        tools.emplace(id, t);
        for (const auto &p : r->parents) {
          const auto ref = follow({{}, p.address});
          if (!tools.at(ref.object).mass)
            throw Error("unknown source contribution");
          t.parents.push_back(
              {ref, p.ordinal, p.stored_mass, *tools.at(ref.object).mass});
        }
        if (address_text(id.address).substr(0, 2) == "02")
          for (const auto &g : r->member_of)
            t.member_of.push_back(follow({{}, g}));
        tools.at(id) = t;
        return t.ref;
      };
      const auto english = follow(object("02.00.00.00.03"));
      // Retain the multi-scalar Dutch closure as repeated-construction
      // evidence.
      follow(object("02.00.20.00.1W"));
      Bundle definitions;
      definitions.ref = {new_uuid(), new_uuid()};
      definitions.owner = instance;
      definitions.creator = actor;
      for (const auto &[id, t] : tools) definitions.tools.push_back(t);
      definitions.geometry = {
          {english.object, {}, {{{}, 0.5}}},
          {tools.at(english.object).parents.at(0).target.object,
           {},
           {{{}, 0.125}}}};
      std::sort(
          definitions.geometry.begin(), definitions.geometry.end(),
          [](const auto &a, const auto &b) { return a.object < b.object; });
      const LetterRequest first{0,
                                english,
                                warm::Exposure::Aggregate,
                                {{-4, 0, 0}, {0.25, 0, 0}},
                                {{{0, 0}, {{-4, 0, 0}, {}}}}};
      const LetterRequest second{1,
                                 english,
                                 warm::Exposure::Parts,
                                 {{4, 0, 0}, {}},
                                 {{{0, 0}, {{4, 0, 0}, {-0.5, 0, 0}}}}};
      definitions.contacts = {{{0, {}}, {1, {0, 0}}}};
      recipe = normalize_letters(definitions, {first, second});
      auto private_request = recipe;
      private_request.geometry.at(0).object.scope = instance;
      rejects([&] { manager.publish(private_request); },
              "private recipe rejected before manager storage preparation");
      check(manager.activity().publications == 0,
            "rejected private publication makes zero storage calls");
      ticket = manager.publish(recipe);
      compare(manager.acquire(*ticket, consumer1).bundle, recipe);
    }
    // A genuinely fresh connection/client recovers the complete shared recipe.
    Toolbox fresh(conn, context);
    SharedManager manager(fresh, {object("00"), object("02")}, source);
    const auto recovered = manager.acquire(*ticket, consumer2);
    compare(recovered.bundle, recipe);
    const auto source_calls = calls;
    const auto activity = manager.activity();
    active_checks::run(recovered.bundle, cuda);
    check(calls == source_calls &&
              manager.activity().publications == activity.publications &&
              manager.activity().acquisitions == activity.acquisitions &&
              manager.activity().pinned_reads == activity.pinned_reads,
          "local assembly, execution, refresh, reporting and restart make zero "
          "manager/source calls");
    compare(manager.pinned(*ticket, consumer2).bundle, recipe);
    auto replacement = recipe;
    replacement.ref.revision = new_uuid();
    const auto new_ticket = manager.publish(replacement, recipe.ref.revision);
    const auto rejected_consumer = new_uuid(), latest_consumer = new_uuid();
    rejects(
        [&] { manager.acquire(*ticket, rejected_consumer); },
        "old publication ticket cannot silently acquire a different revision");
    rejects([&] { manager.pinned(new_ticket, rejected_consumer); },
            "unexpected head acquisition leaves no consumer lease");
    compare(manager.acquire(new_ticket, latest_consumer).bundle, replacement);
    rejects([&] { manager.acquire(*ticket, latest_consumer); },
            "old ticket rejects without disturbing an already-owned current pin");
    compare(manager.pinned(new_ticket, latest_consumer).bundle, replacement);
    manager.release(new_ticket, latest_consumer);
    const auto next_epoch = new_uuid();
    for (auto &binding : directory) {
      binding.epoch = next_epoch;
      binding.policy = new_uuid();
    }
    admin.configure(directory);
    rejects([&] { manager.acquire(*ticket, new_uuid()); },
            "changed source epoch blocks new acquisitions");
    compare(manager.pinned(*ticket, consumer2).bundle, recipe);
    rejects([&] { manager.pinned(*ticket, new_uuid()); },
            "another consumer cannot read someone else's old pin");
    manager.release(*ticket, consumer1);
    manager.release(*ticket, consumer2);
    std::cout << "SOURCE addressed_reads=" << calls << " private_reads=0\n";
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << active_checks::checks << " warm-to-active integration checks\n";
  return harness::report("warm_active_integration");
}
