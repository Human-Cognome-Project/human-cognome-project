#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <thread>

#include "cold_reader.h"
#include "pg.h"
#include "shared_manager.h"
#include "toolbox.h"

namespace {
using namespace warm;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok)
    throw Error(label);
  ++checks;
  std::cout << "PASS: " << label << '\n';
}
template <class E = Error, class F> void rejects(F run, const char *label) {
  try {
    run();
  } catch (const E &) {
    check(true, label);
    return;
  }
  throw Error(label);
}
std::string env(const char *name) {
  const auto *v = std::getenv(name);
  if (!v || !*v)
    throw Error("missing explicit fixture connection");
  return v;
}
ObjectKey object(const std::string &text) {
  return {{}, *codec::decode_token_id(text)};
}
Bundle seed(const std::string &conn) {
  Bundle b;
  b.owner = new_uuid();
  b.creator = new_uuid();
  b.ref = {new_uuid(), new_uuid()};
  const auto epoch = new_uuid();
  Administrator admin(conn);
  admin.configure(
      {{object("00"),
        BindingKind::Shared,
        epoch,
        new_uuid(),
        "retained-core",
        "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121",
        {b.creator}},
       {object("02"),
        BindingKind::Shared,
        epoch,
        new_uuid(),
        "retained-linguistics",
        "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296",
        {b.creator}}});
  ColdReader core(env("HCP_CORE_TEST_CONNINFO")),
      ling(env("HCP_LING_TEST_CONNINFO"));
  check(core.read_only() && ling.read_only(),
        "both retained sources enforced read-only");
  std::map<ObjectKey, Tool> tools;
  std::function<ToolRef(ObjectKey)> follow = [&](ObjectKey id) -> ToolRef {
    if (tools.count(id))
      return tools.at(id).ref;
    const auto row = address_text(id.address).substr(0, 2) == "00"
                         ? core.read(id.address)
                         : ling.read(id.address);
    if (!row)
      throw Error("required retained endpoint absent");
    Tool t{{id, new_uuid()},
           epoch,
           "structural-compatibility-fixture",
           row->notation,
           row->mass,
           {},
           {},
           b.creator};
    tools.emplace(id, t);
    for (const auto &p : row->parents) {
      const auto ref = follow({{}, p.address});
      t.parents.push_back(
          {ref, p.ordinal, p.stored_mass, *tools.at(ref.object).mass});
    }
    for (const auto &g : row->member_of)
      t.member_of.push_back(follow({{}, g}));
    tools.at(id) = t;
    return t.ref;
  };
  const auto letter = follow(object("02.00.00.00.03"));
  const auto byte = tools.at(letter.object).parents.at(0).target;
  for (const auto &[unused, t] : tools)
    b.tools.push_back(t);
  b.geometry = {{letter.object, {}, {{{}, 0.5}}},
                {byte.object, {}, {{{}, 0.125}}}};
  b.nodes = {{-1, ScaleRole::Root}, {0, ScaleRole::PrimaryStep}};
  const auto last = std::numeric_limits<std::uint64_t>::max();
  const auto tiny = std::numeric_limits<double>::denorm_min();
  b.occurrences = {{{0, {}}, letter.object, 1, {-2, tiny, 0}, {tiny, 0, 0}},
                   {{0, {0, 0}}, byte.object, 0, {-3, 2, 0}, {0.5, 0, 0}},
                   {{last, {}}, letter.object, 1, {2, 3, 0}, {-0.25, 0, 0}},
                   {{last, {0, 0}}, byte.object, 0, {3, 4, 0}, {-0.5, 0, 0}}};
  b.selections = {{0, Exposure::Aggregate, {{0, {0, 0}}}},
                  {last, Exposure::Parts, {{last, {0, 0}}}}};
  b.contacts = {{{0, {}}, {last, {0, 0}}}};
  return b;
}
} // namespace

int main(int argc, char **argv) {
  try {
    const auto conn = env("HCP_WARM_TEST_CONNINFO");
    detail::Pg sql(conn);
    if (argc == 2 && std::string(argv[1]) == "seed") {
      auto b = seed(conn);
      Toolbox store(conn, {b.creator, b.owner, WriterRole::Manager});
      check(store.schema_version() == 1,
            "original schema discovered without migration");
      store.publish(store.prepare(b), {});
      store.acquire(b.ref.selection, new_uuid());
      auto v2 = structural_from_v1(
          b, {new_uuid(), new_uuid()},
          {{FieldKind::SiblingDefinition, b.occurrences[0].definition}});
      rejects([&] { store.prepare(v2); },
              "v2 refused on original schema without publication");
      std::cout << checks << " pre-migration checks passed\n";
      return 0;
    }
    auto row = sql.query("SELECT selection,revision,owner,creator FROM "
                         "warm_toolbox.bundle_revision "
                         "WHERE recipe_version=1 ORDER BY selection LIMIT 1");
    check(PQntuples(row.get()) == 1, "retained original fixture present");
    const BundleRef ref{Uuid(detail::cell(row, 0, 0)),
                        Uuid(detail::cell(row, 0, 1))};
    const Context context{Uuid(detail::cell(row, 0, 3)),
                          Uuid(detail::cell(row, 0, 2)), WriterRole::Manager};
    Toolbox store(conn, context);
    check(store.schema_version() == 2, "structural schema discovered");
    auto lease = sql.query("SELECT consumer FROM warm_toolbox.bundle_lease "
                           "WHERE selection=$1 AND bundle_revision=$2",
                           detail::params(ref));
    const Uuid old_consumer(detail::cell(lease, 0, 0));
    const auto legacy = store.read_pinned(ref, old_consumer).bundle;
    check(legacy.recipe_version == 1 && legacy.occurrences.size() == 4 &&
              legacy.occurrences[0].position.x == -2 &&
              legacy.occurrences[0].position.y ==
                  std::numeric_limits<double>::denorm_min() &&
              legacy.occurrences[0].velocity.x ==
                  std::numeric_limits<double>::denorm_min(),
          "v1 pin and smallest finite starts survive migration exactly");
    const auto letter = std::find_if(
        legacy.tools.begin(), legacy.tools.end(), [&](const auto &t) {
          return t.ref.object == legacy.occurrences[0].definition;
        });
    const auto focus = std::vector<FocalField>{
        {FieldKind::SiblingDefinition, legacy.occurrences[0].definition},
        {FieldKind::ParentValue, letter->parents[0].target.object}};
    auto v2 = structural_from_v1(legacy, {ref.selection, new_uuid()}, focus);
    const auto published = store.publish(store.prepare(v2), ref.revision);
    const auto consumer = new_uuid();
    Toolbox fresh(conn, context);
    const auto acquired = fresh.acquire(published.selection, consumer).bundle;
    check(acquired.recipe_version == 2 && acquired.occurrences.empty() &&
              acquired.structural_occurrences.size() ==
                  v2.structural_occurrences.size() &&
              acquired.focal_fields.size() == 2 &&
              acquired.focal_fields[0].kind == focus[0].kind &&
              acquired.focal_fields[0].object == focus[0].object,
          "fresh v2 read contains structure and explicit focus only");
    bool exact = acquired.nodes.size() == v2.nodes.size() &&
                 acquired.selections.size() == 2 &&
                 acquired.contacts.size() == 1;
    for (std::size_t i = 0; i < acquired.structural_occurrences.size(); ++i) {
      const auto &a = acquired.structural_occurrences[i],
                 &b = v2.structural_occurrences[i];
      exact &= a.id == b.id && a.definition == b.definition && a.node == b.node;
    }
    for (const auto &t : acquired.tools) {
      auto old = std::find_if(legacy.tools.begin(), legacy.tools.end(),
                              [&](const auto &v) { return t.ref == v.ref; });
      exact &= old != legacy.tools.end() && tool_body(t) == tool_body(*old);
    }
    for (std::size_t i = 0; i < acquired.focal_fields.size(); ++i)
      exact &= acquired.focal_fields[i].kind == focus[i].kind &&
               acquired.focal_fields[i].object == focus[i].object;
    for (std::size_t i = 0; i < acquired.nodes.size(); ++i)
      exact &= acquired.nodes[i].parent == v2.nodes[i].parent &&
               acquired.nodes[i].role == v2.nodes[i].role;
    for (std::size_t i = 0; i < acquired.selections.size(); ++i)
      exact &= acquired.selections[i].entry == v2.selections[i].entry &&
               acquired.selections[i].exposure == v2.selections[i].exposure &&
               acquired.selections[i].parts == v2.selections[i].parts;
    exact &= acquired.contacts[0].a == v2.contacts[0].a &&
             acquired.contacts[0].b == v2.contacts[0].b;
    const auto same_point = [](Point a, Point b) {
      return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    exact &= acquired.geometry.size() == v2.geometry.size();
    for (const auto &g : acquired.geometry) {
      const auto old =
          std::find_if(v2.geometry.begin(), v2.geometry.end(),
                       [&](const auto &v) { return v.object == g.object; });
      exact &= old != v2.geometry.end() &&
               same_point(g.centroid, old->centroid) &&
               g.spheres.size() == old->spheres.size();
      if (old != v2.geometry.end() && g.spheres.size() == old->spheres.size())
        for (std::size_t i = 0; i < g.spheres.size(); ++i)
          exact &= same_point(g.spheres[i].centre, old->spheres[i].centre) &&
                   g.spheres[i].radius == old->spheres[i].radius;
    }
    check(exact, "full uint64 paths, revisions, repeated uses and construction "
                 "round-trip");
    auto no_starts =
        sql.query("SELECT count(*) FROM warm_toolbox.bundle_occurrence WHERE "
                  "selection=$1 AND bundle_revision=$2",
                  detail::params(published));
    check(detail::cell(no_starts, 0, 0) == "0",
          "v2 writes no fake numerical rows into legacy storage");
    check(
        store.read_pinned(ref, old_consumer).bundle.occurrences[0].position.x ==
            -2,
        "v1 owned pin retained after head moves to v2");
    auto invalid = v2;
    invalid.ref = {new_uuid(), new_uuid()};
    invalid.occurrences = legacy.occurrences;
    rejects([&] { store.prepare(invalid); }, "mixed v1/v2 rejected");
    SharedManager manager(store, {object("00"), object("02")},
                          [](const ObjectKey &) -> std::optional<ColdRecord> {
                            throw Error("source lookup must not occur");
                          });
    const ObjectKey private_id{new_uuid(),
                               *codec::decode_token_id("y0.00.00.00.00")};
    invalid = v2;
    invalid.structural_occurrences[0].definition = private_id;
    rejects<Unauthorized>([&] { manager.publish(invalid, {}); },
                          "private structural occurrence refused before "
                          "manager publication/lookup");
    invalid = v2;
    invalid.focal_fields[0].object = private_id;
    rejects<Unauthorized>(
        [&] { manager.publish(invalid, {}); },
        "private focal field refused before manager publication/lookup");
    check(manager.activity().publications == 0 &&
              manager.activity().source_calls == 0,
          "private v2 requests perform no manager publication or source calls");
    invalid = v2;
    invalid.ref = {new_uuid(), new_uuid()};
    std::map<ObjectKey, ToolRef> changed;
    for (auto &t : invalid.tools) {
      t.ref.revision = new_uuid();
      t.recipe = "failed-structural-publication";
      changed.emplace(t.ref.object, t.ref);
    }
    for (auto &t : invalid.tools) {
      for (auto &p : t.parents)
        p.target = changed.at(p.target.object);
      for (auto &g : t.member_of)
        g = changed.at(g.object);
    }
    const auto counts = [&] {
      auto r =
          sql.query("SELECT (SELECT count(*) FROM warm_toolbox.tool_revision),"
                    "(SELECT count(*) FROM warm_toolbox.tool_head),"
                    "(SELECT count(*) FROM warm_toolbox.bundle_head),"
                    "(SELECT count(*) FROM warm_toolbox.bundle_revision),"
                    "(SELECT count(*) FROM warm_toolbox.bundle_occurrence_v2),"
                    "(SELECT count(*) FROM warm_toolbox.bundle_focus_v2)");
      std::string result;
      for (int i = 0; i < 6; ++i)
        result += detail::cell(r, 0, i) + "/";
      return result;
    };
    const auto before_failure = counts();
    sql.command("ALTER TABLE warm_toolbox.bundle_contact_v2 ADD CONSTRAINT "
                "injected_failure "
                "CHECK(selection <> '" +
                invalid.ref.selection.str() + "'::uuid)");
    rejects([&] { store.publish(store.prepare(invalid), {}); },
            "late v2 contact failure rolls back");
    check(counts() == before_failure, "late v2 failure rolls back new "
                                      "definitions, heads and structural rows");
    auto absent = sql.query(
        "SELECT count(*) FROM warm_toolbox.bundle_revision WHERE selection=$1",
        {invalid.ref.selection.str()});
    check(detail::cell(absent, 0, 0) == "0",
          "failed v2 publication exposes no header or partial head");
    auto next = v2;
    next.ref.revision = new_uuid();
    auto competing = next;
    competing.ref.revision = new_uuid();
    auto a = store.prepare(next), b = store.prepare(competing);
    std::atomic<int> winners{0}, conflicts{0};
    std::exception_ptr failures[2];
    auto publish = [&](Prepared p, int index) {
      try {
        Toolbox writer(conn, context);
        writer.publish(std::move(p), published.revision);
        ++winners;
      } catch (const Conflict &) {
        ++conflicts;
      } catch (...) {
        failures[index] = std::current_exception();
      }
    };
    std::thread t1(publish, std::move(a), 0), t2(publish, std::move(b), 1);
    t1.join();
    t2.join();
    for (const auto &failure : failures)
      if (failure)
        std::rethrow_exception(failure);
    check(winners == 1 && conflicts == 1,
          "competing v2 publications have one coherent head winner");
    check(fresh.read_pinned(published, consumer)
                  .bundle.structural_occurrences.size() == 4,
          "superseded v2 pin remains readable");
    fresh.release(published, consumer);
    // Retain the original v1 pin for independent historical-reader inspection.
    std::cout << checks << " structural PostgreSQL checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
