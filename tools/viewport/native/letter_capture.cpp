#include "capture_options.h"
#include "signal_guard.h"
#include "snapshot.h"
#include "prepared_letter_profile.h"
#include "live_study.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>

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
static int run_capture(const viewport::Options &options, const viewport::LiveStudyInput *live = nullptr) {
  try {
    using namespace warm;
    using namespace assembly;
    // From here (argument parsing is over) a first SIGINT/SIGTERM is only noted, with no masking,
    // so a hung start-up or stalled acquire still ends on a second signal. Setup and the current
    // tick finish, then the final capture, release and exit 0.
    if (!live) viewport::install_stop_handlers();
    if (!live) std::filesystem::create_directories(options.output);
    engine::InstanceSettings settings;
    settings.cuda = options.cuda;
    settings.read_environment = false;
    settings.cpu_threads = options.cpu_threads;
    // Started before any database work. The engine's own error (for example CUDA requested
    // on a CPU-only Taichi build) carries no deployment details and is the actionable
    // failure: no CPU fallback.
    std::unique_ptr<ActiveSession> started;
    if (!live) {
      try {
        started = std::make_unique<ActiveSession>(settings);
      } catch (const std::exception &e) {
        std::cerr << "viewport letter capture failed: engine startup: " << e.what() << '\n';
        return 1;
      }
      // A CUDA-enabled build can start a CPU runtime when a device is missing.
      // Verify the actual backend before opening the retained sources.
      if (started->on_cuda() != settings.cuda) {
        std::cerr << "viewport letter capture failed: requested backend " << (settings.cuda ? "cuda" : "cpu")
                  << " but the engine runs on " << (started->on_cuda() ? "cuda" : "cpu")
                  << (settings.cuda ? " (no CUDA driver or device available); no fallback is permitted\n" : "\n");
        return 1;
      }
    }
    if (options.prepared_english_pair)
      for (const auto *name : {"HCP_WARM_TEST_CONNINFO", "HCP_CORE_TEST_CONNINFO", "HCP_LING_TEST_CONNINFO"}) {
        const auto *value = std::getenv(name);
        if (!value || !*value) {
          std::cerr << "prepared capture acquisition unavailable: missing " << name << '\n';
          return 1;
        }
      }
    const auto actor = new_uuid(), owner = new_uuid(), instance = new_uuid(), epoch = new_uuid();
    const auto conn = env("HCP_WARM_TEST_CONNINFO");
    Toolbox store(conn, {actor, instance, WriterRole::Manager});
    if (options.prepared_english_pair && store.schema_version() != 2) {
      std::cerr << "prepared capture acquisition unavailable: owned warm schema v2 required; migration is explicit\n";
      return 1;
    }
    ColdReader core(env("HCP_CORE_TEST_CONNINFO")), ling(env("HCP_LING_TEST_CONNINFO"));
    if (!core.read_only() || !ling.read_only()) throw Error("read-only sources required");
    Administrator admin(conn);
    admin.configure({{object("00"), BindingKind::Shared, epoch, new_uuid(), "retained-core",
                       "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121", {actor}},
                     {object("02"), BindingKind::Shared, epoch, new_uuid(), "retained-linguistics",
                       "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296", {actor}}}, live ? Administrator::Setup::Fresh : Administrator::Setup::Configure);
    SharedManager manager(store, {object("00"), object("02")}, [&](const ObjectKey &o) {
      return address_text(o.address).substr(0, 2) == "00" ? core.read(o.address) : ling.read(o.address);
    });
    std::map<ObjectKey, Tool> tools;
    std::function<ToolRef(ObjectKey)> follow = [&](ObjectKey key) -> ToolRef {
      if (tools.count(key)) return tools.at(key).ref;
      const auto record = manager.read(key);
      if (!record) throw Error("required retained letter endpoint absent");
      Tool t{{key, new_uuid()}, epoch, options.retained_memberships ? "viewport-prepared-retained-english-pair" :
              options.prepared_english_pair ? "viewport-prepared-english-pair" : "viewport-letter-capture-v1", std::optional<RecipeText>(record->notation),
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
    const auto french = options.prepared_english_pair ? english : follow(object("02.01.00.00.03"));
    const auto dutch = options.prepared_english_pair ? english : follow(object("02.00.20.00.1W"));
    Bundle definitions;
    definitions.ref = {new_uuid(), new_uuid()};
    definitions.owner = instance;
    definitions.creator = actor;
    for (const auto &[key, t] : tools) {
      definitions.tools.push_back(t);
      if (t.mass && *t.mass >= 2)
        definitions.geometry.push_back({key, {}, {{{}, !options.prepared_english_pair && key == dutch.object ? 0.25 : 0.125}}});
    }
    const auto recipe = [&] {
      if (options.retained_memberships) return viewport::prepared_retained_english_pair(definitions,english);
      if (options.prepared_english_pair) return viewport::prepared_english_pair(definitions,english);
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
      return normalize_letters(definitions, requests);
    }();
    const auto ticket = manager.publish(recipe);
    if (live) {
      const auto &ref = ticket.ref();
      viewport::write_live_event(*live,{{"command","recipe_available"},
          {"selection",ref.selection.str()},{"revision",ref.revision.str()}});
      std::string line;
      bool selected = false;
      while (live->commands.read_line(line)) {
        std::istringstream words(line);
        std::string command, selection, revision, trailing;
        if (!(words >> command) || command[0] == '#') continue;
        if (command == "quit" && !(words >> trailing)) return 0;
        const char *status = "invalid_input";
        if (command == "open" && (words >> selection >> revision) && !(words >> trailing)) {
          try {
            const BundleRef requested{Uuid(selection),Uuid(revision)};
            selected = requested.selection == ref.selection && requested.revision == ref.revision;
            status = selected ? "ok" : "unavailable";
          } catch (const warm::Error &) {}
        }
        viewport::write_live_event(*live,{{"command","open"},{"status",status}});
        if (selected) break;
      }
      if (!selected || live->commands.stopped()) return 0;
    }
    const auto consumer = new_uuid();
    // acquire commits the persistent lease before it returns. The handlers are already installed,
    // so a first signal anywhere in that window only sets the flag; the guard is built as soon as
    // acquire returns, setup finishes, the tick-0 capture is the final capture and the pin is
    // released.
    auto [pinned, pin] = viewport::acquire_guarded(
      [&] { return manager.acquire(ticket, consumer); }, [&] { manager.release(ticket, consumer); });
    const auto &pinned_bundle = pinned.bundle;
    auto logical_pin = std::make_shared<const ActiveAssembly>([&] {
      auto input = ActiveAssembly::assemble(pinned_bundle, owner, false);
      if (options.prepared_english_pair) return input;
      return input.focus({{GroupKind::SiblingDefinition, assembly::InputText(key(english.object))}});
    }());
    const auto &logical = *logical_pin;
    if (live) {
      int code = 0;
      std::exception_ptr failure;
      try {
        if (!live->commands.stopped()) {
          auto request = viewport::editable_retained_english_pair_submission(logical_pin,english,live->path);
          code = viewport::run_live_study(*live,std::move(request),
              "Retained English pair",{{"kind","retained_cold_to_warm_to_active"},
              {"profile","prepared-retained-english-pair"},
              {"selection",ticket.ref().selection.str()},{"revision",ticket.ref().revision.str()}},
              1.5,2);
        }
      } catch (...) {
        // The runner's native session has unwound before the checked release.
        failure = std::current_exception();
      }
      pin.release();
      if (failure) std::rethrow_exception(failure);
      viewport::write_live_event(*live,{{"command","recipe_release"},
          {"acquisitions",manager.activity().acquisitions},{"releases",manager.activity().releases}});
      return code;
    }
    ActiveSession &session = *started;
    field::Parameters parameters;
    parameters.dt = float(options.dt);
    field::Parameters64 wide_parameters;
    wide_parameters.dt = options.dt;
    const auto handle = [&] {
      if (!options.prepared_english_pair) return session.submit(logical, parameters);
      auto request = options.retained_memberships
        ? viewport::prepared_retained_english_pair_submission(logical_pin,english)
        : viewport::prepared_english_pair_submission(logical_pin,english);
      auto result = session.submit_prepared(std::move(request),owner);
      if (!result.handle) {
        std::cerr << "prepared capture admission refused: plan=" << int(result.preparation_status)
                  << " publication=" << int(result.publication.status)
                  << " budget=" << field::budget_name(result.failure.budget) << '\n';
        throw Error("prepared admission refused");
      }
      return *result.handle;
    }();
    viewport::Json evidence{{"backend", session.on_cuda() ? "native_cuda" : "native_cpu"}, {"requested_ticks", options.ticks},
      {"dt", options.dt}, {"cpu_threads", settings.cpu_threads.value_or(0)},
      {"every", options.every ? viewport::Json(options.every) : viewport::Json()}, {"pace_ms", options.pace_ms},
      {"source_calls", manager.activity().source_calls}};
    const auto profile_name = options.retained_memberships ? "prepared-retained-english-pair" : "prepared-english-pair";
    if (options.prepared_english_pair) evidence["study_profile"] = profile_name;
    std::uint64_t completed = 0, count = 0;
    viewport::Json first, last;  // Evidence keeps only the first and last captures: bounded for long runs.
    std::optional<Clock::time_point> previous;
    // Returns true when this capture is the last: ticks done, or a stop signal seen (also while pacing).
    const auto capture = [&](bool final) {
      if (previous && options.pace_ms)
        for (const auto due = *previous + std::chrono::milliseconds(options.pace_ms);
             !viewport::stop_requested() && Clock::now() < due;)
          std::this_thread::sleep_for(std::min<Clock::duration>(due - Clock::now(), std::chrono::milliseconds(20)));
      final = final || viewport::stop_requested();
      auto ctx = viewport::context(logical, handle, completed, options.prepared_english_pair
        ? "Retained English pair: explicit prepared profile" : "Retained English/French/Dutch letter study");
      ctx.source = {{"kind", "retained_cold_to_warm_to_active"},
        {"core_export_sha256", "f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121"},
        {"linguistics_export_sha256", "9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296"},
        {"geometry", "supplied: centred sphere radius 0.25 for Dutch aggregate, 0.125 otherwise"},
        {"parameters", {{"dt", parameters.dt}, {"gravity", parameters.gravity},
          {"contact_stiffness", parameters.contact_stiffness}, {"reach_strength", parameters.reach_strength}}}};
      if (options.prepared_english_pair) {
        ctx.source["study_profile"] = profile_name;
        ctx.source["geometry"] = "supplied: centred sphere radius 0.125; root units 1";
        ctx.source["parameters"] = {{"dt",wide_parameters.dt},{"gravity",wide_parameters.gravity},
          {"contact_stiffness",wide_parameters.contact_stiffness},{"reach_strength",wide_parameters.reach_strength}};
        ctx.source["export_identity"] = "configured provenance; not a fresh source fingerprint";
        ctx.source["recipe_pin"] = {{"selection",std::string(logical.shared_recipe().ref.selection.str())},
          {"revision",std::string(logical.shared_recipe().ref.revision.str())}};
        auto pins = viewport::Json::array();
        for (const auto &t : logical.shared_recipe().tools) {
          pins.push_back({{"object",key(t.ref.object)},{"revision",std::string(t.ref.revision.str())},
            {"epoch",std::string(t.epoch.str())}});
          if (options.retained_memberships)
            pins.back()["mass"] = t.mass ? viewport::Json(std::to_string(*t.mass)) : viewport::Json();
        }
        ctx.source["tool_pins"] = std::move(pins);
      }
      const auto report_start = Clock::now();
      const auto report = viewport::observe(session, owner, handle, options.intent_work_limit
        ? std::optional<viewport::IntentRequest>{{options.prepared_english_pair ? wide_parameters.gravity : parameters.gravity,
            *options.intent_work_limit}} : std::nullopt);
      const auto report_ms = ms(report_start);
      const auto start = Clock::now();
      // One coherent value (one study, generation and tick) is written to every file of this capture.
      const auto value = viewport::snapshot(report, ctx);
      const std::filesystem::path out(options.output);
      std::filesystem::path written;
      if (completed == 0) viewport::write_atomic((written = out / "letter-study-tick0.json").string(), value);
      if (final) viewport::write_atomic((written = out / "letter-study.json").string(), value);
      // latest.json only for a watched run (--every); otherwise the files are the fixed two.
      if (options.every) viewport::write_atomic((written = out / "latest.json").string(), value);
      last = {{"tick", completed}, {"particles", value["particles"].size()},
        {"report_ms", report_ms}, {"export_ms", ms(start)}, {"bytes", std::filesystem::file_size(written)}};
      if (!count++) first = last;
      previous = Clock::now();
      return final;
    };
    bool final = capture(false);  // Tick 0 (already final if stopped before the first tick).
    while (!final) {
      if (options.prepared_english_pair) {
        const auto tick = session.tick_prepared(handle,owner,wide_parameters,1000000000,1000000);
        if (tick.status != field::TickStatus::Ok) {
          std::cerr << "prepared capture tick refused: status=" << int(tick.status)
                    << " budget=" << field::budget_name(tick.failure.budget) << '\n';
          return 1;
        }
        completed = session.completed_ticks(handle);
      } else {
        session.tick(handle, parameters);
        ++completed;
      }
      final = completed == options.ticks || viewport::stop_requested();
      if (final || viewport::captures_at(completed, options)) final = capture(final);
    }
    pin.release();
    if (options.retained_memberships) {
      evidence["warm_activity"] = {{"publications",manager.activity().publications},
        {"acquisitions",manager.activity().acquisitions},{"releases",manager.activity().releases}};
      const auto demand = session.demand(handle,owner);
      evidence["native_demand"] = {{"fields_loaded",demand.fields_loaded},{"edges_loaded",demand.edges_loaded},
        {"force_requests",demand.force_requests},{"force_evaluations",demand.force_evaluations},
        {"force_zero_response",demand.force_zero_response}};
    }
    evidence["ticks"] = completed;
    evidence["stopped_by_signal"] = completed < options.ticks;
    evidence["capture_count"] = count;
    evidence["captures"] = count > 1 ? viewport::Json::array({first, last}) : viewport::Json::array({last});
    viewport::write((std::filesystem::path(options.output) / "native-evidence.json").string(), evidence);
    std::cout << evidence.dump(2) << '\n';
    return 0;
  } catch (const viewport::LiveOutputStopped &) {
    return 0;
  } catch (const viewport::LiveOutputError &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const viewport::PreparedLetterProfileError &e) {
    // This type contains only fixed profile diagnostics, never source/SQL text.
    std::cerr << e.what() << '\n';
    return 1;
  } catch (...) {
    // Source/SQL exceptions can carry deployment details: keep public output generic. The engine
    // fork throws bare strings from some calls; catching everything here guarantees the stack
    // unwinds (destructors and guards run) instead of std::terminate.
    std::cerr << (live ? "retained live study failed; check the fresh warm v2 store and read-only sources\n"
                       : "viewport letter capture failed; check owned output path, fixture configuration and retained endpoints\n");
    return 1;
  }
}

int viewport::run_retained_live_study(const LiveStudyInput &input) {
  Options options;
  options.prepared_english_pair = true;
  options.retained_memberships = true;
  return run_capture(options,&input);
}

int viewport_letter_capture(int argc, char **argv) {
  try {
    return run_capture(viewport::parse_options(std::vector<std::string>(argv + 1, argv + argc)));
  } catch (const viewport::UsageError &e) {
    std::cerr << e.what() << '\n' << viewport::usage();
    return 2;
  }
}
