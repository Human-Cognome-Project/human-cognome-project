#include "capture_options.h"
#include "snapshot.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>

#include "shared_manager.h"

namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
std::string env(const char *name) {
  const char *v = std::getenv(name);
  if (!v || !*v) throw std::invalid_argument(std::string("missing ") + name);
  return v;
}
warm::ObjectKey object(const std::string &s) {
  const auto address = codec::decode_token_id(s);
  if (!address) throw std::invalid_argument("invalid fixture address");
  return {{}, *address};
}
}
int main(int argc, char **argv) {
  try {
    viewport::Options options;
    try {
      options = viewport::parse_options(std::vector<std::string>(argv + 1, argv + argc));
    } catch (const viewport::UsageError &e) {
      std::cerr << e.what() << '\n' << viewport::usage();
      return 2;
    }
    using namespace warm;
    using namespace assembly;
    std::filesystem::create_directories(options.output);
    engine::InstanceSettings settings;
    settings.cuda = options.cuda;
    settings.read_environment = false;
    settings.cpu_threads = options.cpu_threads;
    // Started before any database work. The engine's own error (for example CUDA requested
    // on a CPU-only Taichi build) carries no deployment details and is the actionable
    // failure: no CPU fallback.
    std::unique_ptr<ActiveSession> started;
    try {
      started = std::make_unique<ActiveSession>(settings);
    } catch (const std::exception &e) {
      std::cerr << "viewport letter capture failed: engine startup: " << e.what() << '\n';
      return 1;
    }
    ActiveSession &session = *started;
    const auto actor = new_uuid(), owner = new_uuid(), instance = new_uuid(), epoch = new_uuid();
    const auto conn = env("HCP_WARM_TEST_CONNINFO");
    Administrator admin(conn);
    admin.configure({{object("00"), BindingKind::Shared, epoch, new_uuid(), "retained-core",
                       "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121", {actor}},
                     {object("02"), BindingKind::Shared, epoch, new_uuid(), "retained-linguistics",
                       "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296", {actor}}});
    ColdReader core(env("HCP_CORE_TEST_CONNINFO")), ling(env("HCP_LING_TEST_CONNINFO"));
    if (!core.read_only() || !ling.read_only()) throw Error("read-only sources required");
    Toolbox store(conn, {actor, instance, WriterRole::Manager});
    SharedManager manager(store, {object("00"), object("02")}, [&](const ObjectKey &o) {
      return address_text(o.address).substr(0, 2) == "00" ? core.read(o.address) : ling.read(o.address);
    });
    std::map<ObjectKey, Tool> tools;
    std::function<ToolRef(ObjectKey)> follow = [&](ObjectKey key) -> ToolRef {
      if (tools.count(key)) return tools.at(key).ref;
      const auto record = manager.read(key);
      if (!record) throw Error("required retained letter endpoint absent");
      Tool t{{key, new_uuid()}, epoch, "viewport-letter-capture-v1", record->notation,
              record->mass, {}, {}, actor};
      tools.emplace(key, t);
      for (const auto &p : record->parents) {
        auto ref = follow({{}, p.address});
        if (!tools.at(ref.object).mass) throw Error("missing parent contribution");
        t.parents.push_back({ref, p.ordinal, p.stored_mass, *tools.at(ref.object).mass});
      }
      if (address_text(key.address).substr(0, 2) == "02")
        for (const auto &m : record->member_of) t.member_of.push_back(follow({{}, m}));
      tools.at(key) = t;
      return t.ref;
    };
    const auto english = follow(object("02.00.00.00.03"));
    const auto french = follow(object("02.01.00.00.03"));
    const auto dutch = follow(object("02.00.20.00.1W"));
    Bundle definitions;
    definitions.ref = {new_uuid(), new_uuid()};
    definitions.owner = instance;
    definitions.creator = actor;
    for (const auto &[key, t] : tools) {
      definitions.tools.push_back(t);
      if (t.mass && *t.mass >= 2)
        definitions.geometry.push_back({key, {}, {{{}, key == dutch.object ? 0.25 : 0.125}}});
    }
    // Test geometry and placements are supplied, not inferred from cold records.
    std::vector<LetterRequest> requests;
    for (std::uint64_t entry = 0; entry < 8; ++entry) {
      const auto ref = entry < 2 ? english : entry < 4 ? french : dutch;
      const bool parts = entry % 2 == 1;
      LetterRequest r{entry, ref, parts ? warm::Exposure::Parts : warm::Exposure::Aggregate,
        {{double(entry / 2) * 0.8 - 1.2, double(entry % 2), 0}, {}}, {}};
      // Expand only the source's ordered character/byte construction.
      const auto &letter = tools.at(ref.object);
      int index = 0;
      for (const auto &p : letter.parents) {
        const auto &character = tools.at(p.target.object);
        const auto bytes = *character.mass == 2 ? 1 : character.parents.size();
        for (std::size_t j = 0; j < bytes; ++j, ++index)
          r.bytes.push_back({{p.ordinal, std::uint32_t(j)},
             {{double(entry / 2) * 40 - 60 + index * 2.0, 10.0 + entry * 3.0, index % 2 ? 1.0 : -1.0}, {0.01, 0, 0}}});
      }
      requests.push_back(std::move(r));
    }
    const auto recipe = normalize_letters(definitions, requests);
    const auto ticket = manager.publish(recipe);
    const auto consumer = new_uuid();
    const auto pinned = manager.acquire(ticket, consumer);
    auto logical = ActiveAssembly::assemble(pinned.bundle, owner, false)
      .focus({{GroupKind::SiblingDefinition, key(english.object)}});
    field::Parameters parameters;
    parameters.dt = float(options.dt);
    const auto handle = session.submit(logical, parameters);
    viewport::Json evidence{{"backend", settings.cuda ? "native_cuda" : "native_cpu"}, {"ticks", options.ticks},
      {"dt", options.dt}, {"cpu_threads", settings.cpu_threads.value_or(0)}, {"source_calls", manager.activity().source_calls}};
    for (std::uint64_t tick = 0; tick <= options.ticks; ++tick) {
      if (tick) session.tick(handle, parameters);
      if (tick != 0 && tick != options.ticks) continue;
      auto ctx = viewport::context(logical, handle, tick, "Retained English/French/Dutch letter study");
      ctx.source = {{"kind", "retained_cold_to_warm_to_active"},
        {"core_export_sha256", "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121"},
        {"linguistics_export_sha256", "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296"},
        {"geometry", "supplied: centred sphere radius 0.25 for Dutch aggregate, 0.125 otherwise"},
        {"parameters", {{"dt", parameters.dt}, {"gravity", parameters.gravity},
          {"contact_stiffness", parameters.contact_stiffness}, {"reach_strength", parameters.reach_strength}}}};
      const auto report_start = Clock::now();
      const auto report = session.report(handle, owner);
      const auto report_ms = ms(report_start);
      const auto start = Clock::now();
      const auto value = viewport::snapshot(report, ctx);
      const auto path = std::filesystem::path(options.output) / (tick == 0 ? "letter-study-tick0.json" : "letter-study.json");
      viewport::write(path.string(), value);
      evidence["captures"].push_back({{"tick", tick}, {"particles", value["particles"].size()},
        {"report_ms", report_ms}, {"export_ms", ms(start)}, {"bytes", std::filesystem::file_size(path)}});
    }
    manager.release(ticket, consumer);
    viewport::write((std::filesystem::path(options.output) / "native-evidence.json").string(), evidence);
    std::cout << evidence.dump(2) << '\n';
  } catch (const std::exception &e) {
    // Source/SQL exceptions can carry deployment details: keep public output generic.
    std::cerr << "viewport letter capture failed; check owned output path, fixture configuration and retained endpoints\n";
    return 1;
  }
}
