#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <thread>

#include "cold_reader.h"
#include "pg.h"
#include "toolbox.h"

namespace {
using namespace warm;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok) throw Error(std::string("FAIL: ") + label);
  ++checks;
  std::cout << "PASS: " << label << '\n';
}
template <class Exception = Error, class Function>
void rejects(Function run, const char *label) {
  try {
    run();
  } catch (const Exception &) {
    check(true, label);
    return;
  }
  throw Error(std::string("accepted invalid operation: ") + label);
}
std::string required(const char *name) {
  auto value = std::getenv(name);
  if (!value || !*value) throw Error(std::string("missing ") + name);
  return value;
}
ObjectKey object(const std::string &text, Uuid scope = {}) {
  auto address = codec::decode_token_id(text);
  if (!address) throw Error("test address");
  return {scope, *address};
}
const Uuid instance = new_uuid(), actor = new_uuid(), actor2 = new_uuid(),
           epoch = new_uuid();
const Context manager{actor, instance, WriterRole::Manager};
const Context second{actor2, instance, WriterRole::Manager};
const std::string core_hash =
    "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121";
const std::string ling_hash =
    "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296";

// Addressed closure ingestion for TEST fixtures, not letter projection. Cold
// classification memberships are deliberately outside this bounded recipe.
std::vector<Tool> source_tools(ColdReader &core, ColdReader &ling) {
  std::map<ObjectKey, Tool> tools;
  std::function<ToolRef(ObjectKey)> follow = [&](ObjectKey id) -> ToolRef {
    if (tools.count(id)) return tools.at(id).ref;
    auto &reader = address_text(id.address).substr(0, 2) == "02" ? ling : core;
    auto r = reader.read(id.address);
    if (!r) throw Error("retained fixture endpoint missing");
    Tool t{{id, new_uuid()}, epoch, "seed-v1", r->notation,
           r->mass,          {},    {},        actor};
    tools.emplace(id, t);
    for (const auto &parent : r->parents) {
      auto ref = follow({{}, parent.address});
      const auto mass = tools.at(ref.object).mass;
      if (!mass) throw Error("unknown source parent mass");
      t.parents.push_back({ref, parent.ordinal, parent.stored_mass, *mass});
    }
    if (address_text(id.address).substr(0, 2) == "02")
      for (const auto &group : r->member_of)
        t.member_of.push_back(follow({{}, group}));
    tools.at(id) = t;
    return t.ref;
  };
  follow(object("02.00.00.00.03"));  // English A
  follow(object("02.01.00.00.03"));  // French A, distinct linguistic identity
  follow(object("02.00.20.00.1W"));  // Actual Dutch multi-scalar íj́
  std::vector<Tool> out;
  for (const auto &[unused, t] : tools) out.push_back(t);
  return out;
}
Bundle fixture(const std::vector<Tool> &tools) {
  Bundle b;
  b.ref = {new_uuid(), new_uuid()};
  b.owner = instance;
  b.creator = actor;
  b.tools = tools;
  const auto letter = object("02.00.00.00.03");
  const auto &t = *std::find_if(tools.begin(), tools.end(), [&](const auto &v) {
    return v.ref.object == letter;
  });
  const auto byte = t.parents.at(0).target.object;
  // Supplied normalized mixed-exposure fixture. Stage A does not generate it.
  b.nodes = {{-1, ScaleRole::Root}, {0, ScaleRole::PrimaryStep}};
  b.geometry = {{letter, {}, {{{}, 0.5}, {{0.25, 0, 0}, 0.25}}},
                {byte, {}, {{{}, 0.125}}}};
  b.occurrences = {{{0, {}}, letter, 1, {-2, 1, 0}, {0.25, 0, 0}},
                   {{0, {0, 0}}, byte, 0, {-3, 2, 0}, {0.5, 0, 0}},
                   {{1, {}}, letter, 1, {2, 3, 0}, {-0.25, 0, 0}},
                   {{1, {0, 0}}, byte, 0, {3, 4, 0}, {-0.5, 0, 0}}};
  b.selections = {{0, Exposure::Aggregate, {{0, {0, 0}}}},
                  {1, Exposure::Parts, {{1, {0, 0}}}}};
  b.contacts = {{{0, {}}, {1, {0, 0}}}};
  return b;
}
Bundle fresh(Bundle b) {
  b.ref = {new_uuid(), new_uuid()};
  return b;
}
Tool &find(Bundle &b, const ObjectKey &o) {
  return *std::find_if(b.tools.begin(), b.tools.end(),
                       [&](const auto &t) { return t.ref.object == o; });
}
void retag(Bundle &b, const std::string &recipe, Uuid input_epoch = epoch) {
  std::map<ObjectKey, ToolRef> refs;
  for (auto &t : b.tools) {
    t.ref.revision = new_uuid();
    t.recipe = recipe;
    t.epoch = input_epoch;
    refs.emplace(t.ref.object, t.ref);
  }
  for (auto &t : b.tools) {
    for (auto &p : t.parents) p.target = refs.at(p.target.object);
    for (auto &g : t.member_of) g = refs.at(g.object);
  }
}
void compare_recipe(const Bundle &actual, const Bundle &expected) {
  check(actual.ref.selection == expected.ref.selection &&
            actual.ref.revision == expected.ref.revision &&
            actual.owner == instance && actual.creator == actor &&
            actual.recipe_version == expected.recipe_version,
        "bundle identity and ownership round-trip");
  check(actual.tools.size() == expected.tools.size() &&
            actual.geometry.size() == expected.geometry.size() &&
            actual.occurrences.size() == 4 && actual.selections.size() == 2 &&
            actual.contacts.size() == 1,
        "complete repeated-use recipe counts");
  for (const auto &t : expected.tools) {
    const auto &a = *std::find_if(
        actual.tools.begin(), actual.tools.end(),
        [&](const auto &v) { return v.ref.object == t.ref.object; });
    if (tool_body(a) != tool_body(t)) throw Error("tool round-trip differs");
  }
  check(true,
        "all exact tool attributes, revisions, ordinals and NULL cells "
        "round-trip");
  for (const auto &o : expected.occurrences) {
    const auto &a =
        *std::find_if(actual.occurrences.begin(), actual.occurrences.end(),
                      [&](const auto &v) { return v.id == o.id; });
    if (!(a.definition == o.definition) || a.node != o.node ||
        a.position.x != o.position.x || a.position.y != o.position.y ||
        a.position.z != o.position.z || a.velocity.x != o.velocity.x ||
        a.velocity.y != o.velocity.y || a.velocity.z != o.velocity.z)
      throw Error("initial occurrence round-trip differs");
  }
  check(true,
        "all repeated occurrence paths, bindings and initial "
        "position/velocity values round-trip");
  for (const auto &s : expected.selections) {
    const auto &a =
        *std::find_if(actual.selections.begin(), actual.selections.end(),
                      [&](const auto &v) { return v.entry == s.entry; });
    if (a.exposure != s.exposure || a.parts != s.parts)
      throw Error("selection round-trip differs");
  }
  check(actual.contacts[0].a == expected.contacts[0].a &&
            actual.contacts[0].b == expected.contacts[0].b &&
            actual.nodes.size() == 2 && actual.nodes[0].parent == -1 &&
            actual.nodes[0].role == ScaleRole::Root &&
            actual.nodes[1].parent == 0 &&
            actual.nodes[1].role == ScaleRole::PrimaryStep,
        "exposure, contact and normalized node records round-trip");
  for (const auto &g : expected.geometry) {
    const auto &a =
        *std::find_if(actual.geometry.begin(), actual.geometry.end(),
                      [&](const auto &v) { return v.object == g.object; });
    if (a.centroid.x != g.centroid.x || a.centroid.y != g.centroid.y ||
        a.centroid.z != g.centroid.z || a.spheres.size() != g.spheres.size())
      throw Error("geometry differs");
    for (std::size_t i = 0; i < g.spheres.size(); ++i)
      if (a.spheres[i].radius != g.spheres[i].radius ||
          a.spheres[i].centre.x != g.spheres[i].centre.x ||
          a.spheres[i].centre.y != g.spheres[i].centre.y ||
          a.spheres[i].centre.z != g.spheres[i].centre.z)
        throw Error("sphere differs");
  }
  check(true, "all supplied centroid/component geometry round-trips");
}
}  // namespace

int main() {
  using namespace warm;
  try {
    const auto connection = required("HCP_WARM_TEST_CONNINFO");
    const auto core_connection = required("HCP_CORE_TEST_CONNINFO"),
               ling_connection = required("HCP_LING_TEST_CONNINFO");
    detail::Pg sql(connection);
    auto empty = sql.query("SELECT count(*) FROM warm_toolbox.bundle_revision");
    check(detail::cell(empty, 0, 0) == "0",
          "explicitly selected new warm fixture is empty (no reset)");
    ColdReader core(core_connection), ling(ling_connection);
    check(core.read_only() && ling.read_only(),
          "both cold sessions enforce read-only mode");
    {
      detail::Pg readonly(
          core_connection + " options='-c default_transaction_read_only=off'",
          true);
      rejects(
          [&] {
            readonly.command(
                "UPDATE public.token SET notation=notation WHERE false");
          },
          "read-only startup overrides writable conninfo options; server "
          "rejects UPDATE");
    }
    const auto tools = source_tools(core, ling);
    auto base = fixture(tools);
    check(tools.size() > 10,
          "bounded addressed reads from actual retained core/linguistic pair");
    bool repeated = false, null_mass = false, null_parent = false;
    for (const auto &t : tools) {
      null_mass |= !t.mass;
      for (const auto &p : t.parents) null_parent |= !p.stored_mass;
      if (t.parents.size() == 2 && t.parents[0].target == t.parents[1].target)
        repeated = true;
    }
    check(repeated && null_mass && null_parent,
          "real closure includes repeated nibble contributions and NULL "
          "group/parent masses");
    Administrator admin(connection);
    std::vector<Binding> directory = {{object("00"),
                                       BindingKind::Shared,
                                       epoch,
                                       new_uuid(),
                                       "retained-core",
                                       core_hash,
                                       {actor, actor2}},
                                      {object("02"),
                                       BindingKind::Shared,
                                       epoch,
                                       new_uuid(),
                                       "retained-linguistics",
                                       ling_hash,
                                       {actor, actor2}}};
    auto incompatible = directory;
    incompatible[1].epoch = new_uuid();
    rejects([&] { admin.configure(incompatible); },
            "incompatible source-pair epochs rejected");
    admin.configure(directory);
    Toolbox writer(connection, manager);
    check(writer.resolve(object("00.00")).prefix == object("00") &&
              writer.resolve(object("02.00")).prefix == object("02"),
          "existing shared root bindings retained");
    rejects<UnboundRoot>([&] { writer.resolve(object("01")); },
                         "01 has no assigned binding");
    rejects([&] { writer.resolve(object("z*", instance)); },
            "wildcard rejected as concrete identity");
    rejects<UnboundRoot>(
        [&] { writer.resolve(object("z0.00", instance)); },
        "reservation alone does not authorize a temporary binding");
    {
      Toolbox preparing(connection, manager);
      preparing.publish(preparing.prepare(base), {});
    }
    const auto consumer = new_uuid();
    Toolbox fresh_reader(connection, {actor, instance, WriterRole::Reader});
    const auto acquired = fresh_reader.acquire(base.ref.selection, consumer);
    compare_recipe(acquired.bundle, base);
    check(!acquired.lists.children.empty() && !acquired.lists.members.empty() &&
              acquired.lists.members.size() == acquired.lists.member_of.size(),
          "stored child/member return lists and both membership directions "
          "read back exactly");
    Toolbox other_actor(connection, {actor2, instance, WriterRole::Reader});
    rejects<Unauthorized>([&] { other_actor.release(base.ref, consumer); },
                          "another actor cannot release a consumer's pin");
    rejects<Unauthorized>(
        [&] { other_actor.acquire(base.ref.selection, consumer); },
        "consumer UUID cannot adopt another actor's pin");
    Toolbox outsider(connection, {actor, new_uuid(), WriterRole::Reader});
    rejects<Unauthorized>(
        [&] { outsider.acquire(base.ref.selection, new_uuid()); },
        "cross-instance bundle reader rejected");
    rejects<Unauthorized>([&] { fresh_reader.prepare(fresh(base)); },
                          "reader cannot grant itself writer authority");
    Toolbox unauthorized(connection,
                         {new_uuid(), instance, WriterRole::Manager});
    rejects<Unauthorized>([&] { unauthorized.prepare(fresh(base)); },
                          "unconfigured manager actor rejected");

    auto changed = fresh(base);
    changed.ref.selection = base.ref.selection;
    auto next = writer.publish(writer.prepare(changed), base.ref.revision);
    check(fresh_reader.read_pinned(base.ref, consumer).bundle.ref.revision ==
              base.ref.revision,
          "old pinned revision remains available after head advance");
    check(fresh_reader.acquire(base.ref.selection, new_uuid())
                  .bundle.ref.revision == next.revision,
          "new acquire sees complete new head");
    rejects<Conflict>(
        [&] { writer.publish(writer.prepare(changed), base.ref.revision); },
        "stale expected head cannot overwrite publication");

    auto race = fresh(base);
    retag(race, "concurrent-new-tools");
    std::promise<void> gate;
    std::promise<void> ready1, ready2;
    auto start = gate.get_future().share();
    std::atomic<int> wins{0}, conflicts{0};
    std::exception_ptr failure1, failure2;
    auto compete = [&](Context context, std::exception_ptr &failure,
                       std::promise<void> &ready) {
      bool signalled = false;
      try {
        Toolbox client(connection, context);
        auto request = race;
        request.ref.revision = new_uuid();
        auto candidate = client.prepare(request);
        ready.set_value();
        signalled = true;
        start.wait();
        try {
          client.publish(candidate, {});
          ++wins;
        } catch (const Conflict &) {
          ++conflicts;
        }
      } catch (...) {
        failure = std::current_exception();
        if (!signalled) ready.set_value();
      }
    };
    std::thread a(compete, manager, std::ref(failure1), std::ref(ready1)),
        b(compete, second, std::ref(failure2), std::ref(ready2));
    // Both independent connections have prepared against the same directory
    // before either publication is allowed to start.
    ready1.get_future().wait();
    ready2.get_future().wait();
    gate.set_value();
    a.join();
    b.join();
    if (failure1) std::rethrow_exception(failure1);
    if (failure2) std::rethrow_exception(failure2);
    check(wins == 1 && conflicts == 1,
          "two native writers, same expected head: exactly one winner and one "
          "conflict");
    std::atomic<int> unrelated{0};
    failure1 = {};
    failure2 = {};
    auto independent = [&](Context c, std::exception_ptr &failure) {
      try {
        Toolbox client(connection, c);
        client.publish(client.prepare(fresh(base)), {});
        ++unrelated;
      } catch (...) {
        failure = std::current_exception();
      }
    };
    std::thread c(independent, manager, std::ref(failure1)),
        d(independent, second, std::ref(failure2));
    c.join();
    d.join();
    if (failure1) std::rethrow_exception(failure1);
    if (failure2) std::rethrow_exception(failure2);
    check(unrelated == 2, "two unrelated selections both publish correctly");

    auto failed = fresh(base);
    retag(failed, "late-failure-fixture");
    sql.command(
        "ALTER TABLE warm_toolbox.bundle_contact ADD CONSTRAINT "
        "injected_late_failure CHECK(selection <> '" +
        failed.ref.selection.str() + "'::uuid)");
    const auto counts = [&] {
      auto r = sql.query(
          "SELECT (SELECT count(*) FROM warm_toolbox.tool_revision),(SELECT "
          "count(*) FROM warm_toolbox.bundle_revision),"
          "(SELECT count(*) FROM warm_toolbox.tool_head),(SELECT count(*) FROM "
          "warm_toolbox.bundle_head)");
      std::string out;
      for (int i = 0; i < 4; ++i) out += detail::cell(r, 0, i) + "/";
      return out;
    };
    const auto before = counts();
    rejects(
        [&] { writer.publish(writer.prepare(failed), {}); },
        "late contact constraint failure rejects otherwise-valid publication");
    check(counts() == before,
          "late failure rolls back every new tool, bundle and head");
    rejects([&] { fresh_reader.acquire(failed.ref.selection, new_uuid()); },
            "failed publication exposes no partial readiness");

    const auto reject_recipe = [&](std::function<void(Bundle &)> mutate,
                                   const char *label) {
      auto invalid = fresh(base);
      mutate(invalid);
      rejects([&] { writer.prepare(invalid); }, label);
    };
    reject_recipe(
        [](auto &v) {
          v.occurrences[1].id.path = {0, 1};
        },
        "inconsistent/missing source byte path rejected");
    reject_recipe([](auto &v) { v.selections[1].entry = 0; },
                  "overlapping selections rejected");
    reject_recipe([](auto &v) { v.selections[0].parts.clear(); },
                  "missing recipe parts rejected");
    reject_recipe([](auto &v) { v.tools[0].mass = UINT64_MAX; },
                  "mass overflow rejected before SQL conversion");
    reject_recipe([](auto &v) { v.tools[0].ref.revision = Uuid(); },
                  "nil tool revision rejected");
    reject_recipe([](auto &v) { v.tools[0].notation = std::string("a\0b", 3); },
                  "embedded NUL text cannot be truncated by libpq");
    reject_recipe([](auto &v) { v.tools.push_back(v.tools[0]); },
                  "duplicate object revisions rejected");
    reject_recipe(
        [](auto &v) {
          for (auto &t : v.tools)
            if (!t.parents.empty()) {
              t.parents[0].stored_mass = 0;
              break;
            }
        },
        "stored parent-row mass conflict rejected");
    reject_recipe(
        [](auto &v) {
          v.occurrences[0].position.x = std::numeric_limits<double>::infinity();
        },
        "non-finite initial state rejected");
    reject_recipe(
        [](auto &v) { v.nodes[1].role = ScaleRole::SecondaryCompression; },
        "supplied binding inconsistent with normalized mixed exposure "
        "rejected");
    reject_recipe(
        [](auto &v) {
          v.contacts[0].b = {1, {}};
        },
        "contact with inactive alternative rejected");
    reject_recipe(
        [](auto &v) {
          for (auto &t : v.tools)
            if (!t.parents.empty()) {
              t.parents[0].target.revision = new_uuid();
              break;
            }
        },
        "inconsistent pinned dependency revision rejected");
    rejects([] { Uuid("not-a-uuid"); }, "malformed UUID rejected");
    rejects(
        [&] {
          writer.release({{}, {}}, new_uuid());
        },
        "nil bundle release identity rejected");
    auto wrong_epoch = fresh(base);
    wrong_epoch.tools[0].epoch = new_uuid();
    rejects<Stale>([&] { writer.prepare(wrong_epoch); },
                   "initial supplied input epoch checked against binding");
    auto owned_candidate = writer.prepare(fresh(base));
    Toolbox other_preparer(connection, second);
    rejects<Unauthorized>(
        [&] { other_preparer.publish(owned_candidate, {}); },
        "prepared candidate cannot transfer writer authority");
    auto letter_only = fresh(base);
    letter_only.nodes.resize(1);
    letter_only.contacts.clear();
    for (auto &s : letter_only.selections) s.exposure = Exposure::Aggregate;
    for (auto &o : letter_only.occurrences) o.node = 0;
    auto letter_ref = writer.publish(writer.prepare(letter_only), {});
    check(fresh_reader.acquire(letter_ref.selection, new_uuid())
                  .bundle.nodes.size() == 1,
          "supplied letter-only binding round-trips with inactive bytes and "
          "root units 1");
    auto bytes_only = fresh(base);
    bytes_only.contacts.clear();
    for (auto &s : bytes_only.selections) s.exposure = Exposure::Parts;
    auto byte_ref = writer.publish(writer.prepare(bytes_only), {});
    check(fresh_reader.acquire(byte_ref.selection, new_uuid())
                  .bundle.selections[0]
                  .exposure == Exposure::Parts,
          "supplied byte-only recipe round-trips");
    auto max_entry = fresh(base);
    for (auto &o : max_entry.occurrences)
      if (o.id.entry == 1) o.id.entry = UINT64_MAX;
    max_entry.selections[1].entry = UINT64_MAX;
    max_entry.selections[1].parts[0].entry = UINT64_MAX;
    max_entry.contacts[0].b.entry = UINT64_MAX;
    auto max_ref = writer.publish(writer.prepare(max_entry), {});
    check(fresh_reader.acquire(max_ref.selection, new_uuid())
                  .bundle.selections.back()
                  .entry == UINT64_MAX,
          "uint64 request-entry maximum round-trips losslessly");

    // Every finite native double is a permitted warm input, including
    // subnormals that std::stod rejects on some libcs. Exercise every persisted
    // real field.
    auto small = fresh(base);
    const auto denormal = std::numeric_limits<double>::denorm_min();
    const Point tiny{denormal, 1e-310, -1e-310};
    for (auto &o : small.occurrences) {
      o.position = tiny;
      o.velocity = {-denormal, -1e-310, std::numeric_limits<double>::min()};
    }
    for (auto &g : small.geometry) {
      g.centroid = tiny;
      for (auto &sphere : g.spheres) {
        sphere.centre = {-1e-310, denormal, 1e-310};
        sphere.radius = denormal;
      }
    }
    writer.publish(writer.prepare(small), {});
    {
      Toolbox small_reader(connection, {actor, instance, WriterRole::Reader});
      const auto small_consumer = new_uuid();
      const auto recovered =
          small_reader.acquire(small.ref.selection, small_consumer);
      compare_recipe(recovered.bundle, small);
      check(small_reader.read_pinned(small.ref, small_consumer)
                    .bundle.occurrences[0]
                    .position.x == denormal,
            "smallest finite subnormal survives fresh acquire and pinned read");
    }
    for (const auto &invalid : {"1e9999", "1e-9999", "nan", "inf", "1.0junk"})
      rejects(
          [&] { detail::real_value(invalid); },
          "numeric parser rejects nonrepresentable/non-finite/invalid text");

    auto policy_candidate = writer.prepare(fresh(base));
    directory[0].policy = new_uuid();
    admin.configure(directory);
    rejects<Conflict>([&] { writer.publish(policy_candidate, {}); },
                      "changed policy invalidates a prepared candidate");
    auto epoch_candidate = writer.prepare(fresh(base));
    const auto next_epoch = new_uuid();
    for (auto &root : directory) {
      root.epoch = next_epoch;
      root.policy = new_uuid();
    }
    admin.configure(directory);
    rejects<Conflict>([&] { writer.publish(epoch_candidate, {}); },
                      "changed epoch invalidates a prepared candidate");
    rejects<Stale>(
        [&] { fresh_reader.acquire(base.ref.selection, new_uuid()); },
        "default acquire rejects superseded source epoch");
    check(fresh_reader.read_pinned(base.ref, consumer).bundle.tools.size() ==
              tools.size(),
          "old owned pin can finish after epoch adoption");
    auto adopted = fresh(base);
    adopted.ref.selection = base.ref.selection;
    retag(adopted, "seed-v1", next_epoch);
    auto adopted_ref = writer.publish(writer.prepare(adopted), next.revision);
    check(fresh_reader.acquire(adopted_ref.selection, new_uuid())
                  .bundle.tools[0]
                  .epoch == next_epoch,
          "new validated pair epoch publishes and acquires without mixing old "
          "dependencies");
    check(!admin.affected(object("00"), epoch).empty(),
          "source epoch has addressed return paths to dependent bundles");
    fresh_reader.release(base.ref, consumer);
    fresh_reader.release(base.ref, consumer);
    rejects<Unauthorized>(
        [&] { fresh_reader.read_pinned(base.ref, consumer); },
        "owned release is idempotent and removes only that pin");

    // Restore no source state: adopt another warm directory version only.
    // Explicit y0/z0 bindings below exist solely inside this new test database.
    for (auto &root : directory) {
      root.epoch = epoch;
      root.policy = new_uuid();
    }
    admin.configure(directory);
    const auto personal = object("y0", instance),
               temporary = object("z0", instance);
    directory.push_back({personal,
                         BindingKind::Personal,
                         epoch,
                         new_uuid(),
                         "fixture-personal",
                         std::string(64, 'a'),
                         {actor}});
    directory.push_back({temporary,
                         BindingKind::Temporary,
                         epoch,
                         new_uuid(),
                         "fixture-temporary",
                         {},
                         {actor}});
    admin.configure(directory);
    auto overlap = directory;
    auto nested = directory.back();
    nested.prefix = object("z0.00", instance);
    overlap.push_back(nested);
    rejects([&] { admin.configure(overlap); },
            "overlapping concrete bindings rejected");
    auto wrong_family = directory;
    wrong_family.back().prefix = object("x0", instance);
    rejects([&] { admin.configure(wrong_family); },
            "temporary binding outside reserved family rejected");
    auto private_bundle = fresh(base);
    const auto personal_object = object("y0.00", instance),
               temporary_object = object("z0.00", instance);
    const auto letter = find(private_bundle, object("02.00.00.00.03"));
    Tool private_tool{{personal_object, new_uuid()},
                      epoch,
                      "personal-fixture",
                      {},
                      2,
                      {{letter.ref, 0, {}, 2}},
                      {},
                      actor};
    Tool temporary_tool{{temporary_object, new_uuid()},
                        epoch,
                        "temporary-fixture",
                        {},
                        2,
                        {{private_tool.ref, 0, {}, 2}},
                        {},
                        actor};
    private_bundle.tools.push_back(private_tool);
    private_bundle.tools.push_back(temporary_tool);
    auto private_ref = writer.publish(writer.prepare(private_bundle), {});
    const auto private_consumer = new_uuid();
    auto private_read =
        fresh_reader.acquire(private_ref.selection, private_consumer);
    check(find(private_read.bundle, personal_object).parents[0].target ==
                  letter.ref &&
              find(private_read.bundle, temporary_object)
                      .parents[0]
                      .target.object == personal_object,
          "positive personal/temporary integration retains source identities "
          "and local reciprocals");
    auto retry = fresh(private_bundle);
    writer.publish(writer.prepare(retry), {});
    check(true,
          "same creator and identical temporary construction retries "
          "idempotently");
    Toolbox temporary_writer(connection,
                             {actor, instance, WriterRole::Temporary});
    auto own_temp = fresh(base);
    auto local = temporary_tool;
    local.ref.object = object("z0.01", instance);
    local.ref.revision = new_uuid();
    local.parents = {{letter.ref, 0, {}, 2}};
    own_temp.tools.push_back(local);
    temporary_writer.publish(temporary_writer.prepare(own_temp), {});
    check(true,
          "temporary writer creates an authorized local tool reusing "
          "cached shared dependencies");
    auto changed_temp = fresh(private_bundle);
    find(changed_temp, temporary_object).notation = "changed";
    rejects<Conflict>([&] { writer.publish(writer.prepare(changed_temp), {}); },
                      "temporary body cannot overwrite an existing address");
    auto other_recipe = fresh(private_bundle);
    find(other_recipe, temporary_object).recipe = "another-key";
    rejects<Conflict>([&] { writer.publish(writer.prepare(other_recipe), {}); },
                      "temporary create-only claim cannot be bypassed with a "
                      "different recipe key");
    auto shared_conflict = fresh(base);
    find(shared_conflict, letter.ref.object).notation = "forged";
    rejects<Conflict>(
        [&] { writer.publish(writer.prepare(shared_conflict), {}); },
        "same shared epoch/recipe cannot overwrite cached source definition");
    auto outward = fresh(private_bundle);
    find(outward, letter.ref.object).member_of.push_back(temporary_tool.ref);
    rejects([&] { writer.prepare(outward); },
            "shared definition cannot reference private-derived tool");
    auto cross_instance = fresh(private_bundle);
    find(cross_instance, temporary_object).ref.object.scope = new_uuid();
    rejects<Unauthorized>([&] { writer.prepare(cross_instance); },
                          "cross-instance private construction rejected");
    auto other_creator_directory = directory;
    other_creator_directory.back().writers.push_back(actor2);
    other_creator_directory.back().policy = new_uuid();
    admin.configure(other_creator_directory);
    Toolbox another_creator(connection, second);
    rejects<Unauthorized>(
        [&] {
          another_creator.publish(
              another_creator.prepare(fresh(private_bundle)), {});
        },
        "another creator cannot claim a supplied temporary body, even "
        "unchanged");
    auto no_creation_rights = directory;
    no_creation_rights.back().policy = new_uuid();
    admin.configure(no_creation_rights);

    // A's temporary-writer tool is a reference, not a creation/retry request by
    // B. B needs read scope for it, not permission to claim A's address.
    auto reuse = fresh(base);
    auto reuse_ref = another_creator.publish(
        another_creator.prepare(reuse, {local.ref}), {});
    Toolbox reader_b(connection, {actor2, instance, WriterRole::Reader});
    auto reused = reader_b.acquire(reuse_ref.selection, new_uuid());
    const auto &referenced = find(reused.bundle, local.ref.object);
    check(referenced.ref == local.ref && referenced.author == actor &&
              tool_body(referenced) == tool_body(local) &&
              reused.bundle.creator == actor2,
          "B references A's temporary-writer tool unchanged without address "
          "creation rights");

    auto ambiguous = fresh(base);
    auto changed_reference = local;
    changed_reference.notation = "changed";
    ambiguous.tools.push_back(changed_reference);
    rejects([&] { another_creator.prepare(ambiguous, {local.ref}); },
            "an existing reference cannot also supply a replacement body");
    auto changed_dependency = fresh(base);
    find(changed_dependency, letter.ref.object).notation = "changed dependency";
    rejects<Conflict>(
        [&] { another_creator.prepare(changed_dependency, {local.ref}); },
        "a supplied dependency cannot replace the existing reference's pinned "
        "body");
    auto missing_ref = local.ref;
    missing_ref.revision = new_uuid();
    rejects([&] { another_creator.prepare(fresh(base), {missing_ref}); },
            "existing reference must name a committed revision");
    auto foreign_ref = local.ref;
    foreign_ref.object.scope = new_uuid();
    rejects<Unauthorized>(
        [&] { another_creator.prepare(fresh(base), {foreign_ref}); },
        "explicit existing reference cannot cross instance scope");

    auto composed = fresh(base);
    Tool composition{{object("z0.02", instance), new_uuid()},
                     epoch,
                     "cross-writer-composition",
                     {},
                     4,
                     {{local.ref, 0, {}, 2}, {temporary_tool.ref, 1, {}, 2}},
                     {},
                     actor2};
    composed.tools.push_back(composition);
    rejects<Unauthorized>(
        [&] {
          another_creator.publish(
              another_creator.prepare(composed,
                                      {local.ref, temporary_tool.ref}),
              {});
        },
        "immutable reference access does not authorize a new temporary "
        "address");
    auto allowed_composition = no_creation_rights;
    allowed_composition.back().writers.push_back(actor2);
    allowed_composition.back().policy = new_uuid();
    admin.configure(allowed_composition);
    Toolbox temporary_b(connection, {actor2, instance, WriterRole::Temporary});
    auto composed_ref = temporary_b.publish(
        temporary_b.prepare(composed, {local.ref, temporary_tool.ref}), {});
    auto composed_read = reader_b.acquire(composed_ref.selection, new_uuid());
    const auto &created = find(composed_read.bundle, composition.ref.object);
    check(
        created.author == actor2 && created.parents[0].target == local.ref &&
            created.parents[1].target == temporary_tool.ref &&
            find(composed_read.bundle, local.ref.object).author == actor &&
            find(composed_read.bundle, temporary_object).author == actor &&
            find(composed_read.bundle, personal_object).ref == private_tool.ref,
        "authorized B composes a new tool from A's tools with exact transitive "
        "private dependencies");
    auto nested_ref =
        writer.publish(writer.prepare(fresh(base), {created.ref}), {});
    auto nested_read = fresh_reader.acquire(nested_ref.selection, new_uuid());
    check(find(nested_read.bundle, created.ref.object).author == actor2 &&
              find(nested_read.bundle, local.ref.object).author == actor &&
              find(nested_read.bundle, temporary_object).author == actor,
          "referencing a composed tool preserves mixed-author temporary "
          "dependency closure");
    auto claim = sql.query(
        "SELECT creator,temporary_revision FROM warm_toolbox.object_slot "
        "WHERE scope=$1 AND address=$2",
        detail::params(local.ref.object));
    check(Uuid(detail::cell(claim, 0, 0)) == actor &&
              Uuid(detail::cell(claim, 0, 1)) == local.ref.revision,
          "cross-writer reference reuse preserves original creator and address "
          "claim");
    check(fresh_reader.read_pinned(private_ref, private_consumer)
                  .bundle.tools.size() == private_bundle.tools.size(),
          "private pinned revision remains available through policy changes");
    fresh_reader.release(private_ref, private_consumer);
    auto retained = sql.query(
        "SELECT count(*) FROM warm_toolbox.bundle_revision WHERE selection=$1",
        {base.ref.selection.str()});
    check(detail::cell(retained, 0, 0) == "3",
          "superseded revisions retained after consumer release");
    std::cout << checks << " Stage A PostgreSQL acceptance checks passed; "
              << tools.size() << " retained source definitions read.\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << " (after " << checks << " checks)\n";
    return 1;
  }
}
