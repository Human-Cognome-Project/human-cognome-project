#include "snapshot.h"

#include <unistd.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <thread>

#include "membership_fixture.h"

namespace {
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  if (!ok) throw std::runtime_error(label);
}
template<class F> void rejects(F fn, const char *label) {
  try { fn(); } catch (const std::exception &) { check(true, label); return; }
  check(false, label);
}
}
int main() {
  try {
    using namespace assembly;
    using viewport::Json;
    membership_fixture::Fixture f;
    auto a = f.request(f.english, 0, warm::Exposure::Aggregate);
    auto b = f.request(f.english, 1, warm::Exposure::Parts);
    const double offset = 134217728;
    a.aggregate = {{offset / 64, 0, 0}, {1.0 / 64, 0, 0}};
    b.bytes[0].initial = {{offset + 8, 0, 0}, {3, 0, 0}};
    auto logical = ActiveAssembly::assemble(normalize_letters(f.base, {a, b}), warm::new_uuid(), false)
      .focus({{GroupKind::Membership, warm::key(f.en)}});
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 1;
    ActiveSession session(settings);
    field::Parameters parameters;
    parameters.gravity = 0;
    parameters.contact_stiffness = 0;
    parameters.reach_strength = 0;
    const auto h = session.submit(logical, parameters);
    auto ctx = viewport::context(logical, h, 0, "Capture \"é\"\n\\");
    auto report = session.report(h, logical.owner());
    auto first = viewport::capture(session, logical.owner(), ctx);
    check(first["capture"]["phase"] == "published_seeded", "tick-zero phase");
    check(first["frame"]["camera_target"][0] == 4, "tick-zero centroid from current groups");
    check(first["frame"]["reference"][0] == offset, "double reference kept separate");
    check(first["particles"].size() == 2 && std::size_t(report.prepared.capacity) > 2,
          "hidden alternatives and capacity slots absent");
    check(first["particles"][0]["id"] != first["particles"][1]["id"], "distinct occurrences");
    for (const auto &v : first["particles"]) {
      const auto slot = v["slot"].get<int>();
      for (int axis = 0; axis < 3; ++axis)
        check(v["position"][axis] == report.particles[(field::kPosX + axis) * report.prepared.capacity + slot],
              "current scalar coordinate fidelity");
      check(v["radius"] == report.particles[field::kRadius * report.prepared.capacity + slot], "actual radius fidelity");
      check(v["mass"] == 2 && v["exact_mass"] == "2", "mass forms");
    }
    check(first["particles"][0]["level"] != first["particles"][1]["level"], "explicit conceptual levels");
    check(Json::parse(first.dump())["capture"]["label"] == ctx.label, "Unicode/control/quote escaping");
    rejects([&] { viewport::capture(session, warm::new_uuid(), ctx); }, "owner guard");
    auto bad = ctx;
    bad.handle.generation = warm::new_uuid();
    rejects([&] { viewport::snapshot(report, bad); }, "generation mismatch");
    rejects([&] { viewport::capture(session, logical.owner(), bad); }, "stale native handle");
    bad = ctx;
    bad.occurrences.clear();
    rejects([&] { viewport::snapshot(report, bad); }, "missing exposure");
    session.tick(h, parameters);
    ctx.tick = 1;  // Only after synchronous successful tick.
    report = session.report(h, logical.owner());
    const auto moved = viewport::snapshot(report, ctx);
    check(moved["frame"]["camera_target"][0] == 6, "moving post-tick focal centroid");
    check(report.demand.focal_origin[0] == 4 && moved["frame"]["camera_target"][0] != report.demand.focal_origin[0],
          "last integration origin distinct from captured focus");
    check(moved["particles"][0]["position"] != first["particles"][0]["position"], "current rather than publication positions");
    auto modified = report;
    modified.particles[0] = std::nextafter(1.0f, 2.0f);
    modified.frame_anchor[1] = std::nextafter(1.0, 2.0);
    modified.prepared.particles[0].exact_mass = (std::uint64_t(1) << 63) + 3;
    const auto roundtrip = Json::parse(viewport::snapshot(modified, ctx).dump());
    check(roundtrip["frame"]["reference"][1].get<double>() == modified.frame_anchor[1], "f64 round trip");
    check(roundtrip["particles"][0]["exact_mass"] == "9223372036854775811", "large mass string");
    const int slot = modified.prepared.particles[0].occurrence.slot;
    auto wide_report = report;
    wide_report.numerical_profile = field::NumericalProfile::trial(48, 47);
    wide_report.particles64.assign(report.particles.begin(),
                                   report.particles.end());
    wide_report.groups64.assign(report.groups.begin(), report.groups.end());
    wide_report.determined64.assign(report.determined.begin(),
                                    report.determined.end());
    wide_report.particles.clear();
    wide_report.groups.clear();
    wide_report.determined.clear();
    const double precise = 1 + std::ldexp(1., -40);
    wide_report.particles64[slot] = precise;
    const auto wide_json =
        Json::parse(viewport::snapshot(wide_report, ctx).dump());
    check(wide_json["particles"][0]["position"][0].get<double>() == precise &&
              wide_json["numerics"]["storage_bits"] == 64 &&
              wide_json["numerics"]["body_fraction_bits"] == 48,
          "f64 report publication metadata and value round trip");
    wide_report.particles.push_back(0);
    rejects([&] { viewport::snapshot(wide_report, ctx); },
            "mixed float/double report rejected");
    modified.particles[field::kPosX * modified.prepared.capacity + slot] = std::nextafter(1.0f, 2.0f);
    const auto floats = Json::parse(viewport::snapshot(modified, ctx).dump());
    check(floats["particles"][0]["position"][0].get<float>() == std::nextafter(1.0f, 2.0f), "f32 round trip");
    modified.prepared.scales.push_back({{nullptr, 1, ScaleRole::SecondaryCompression}, 64});
    modified.prepared.scales.push_back({{nullptr, 2, ScaleRole::NamingEquivalent}, 64});
    modified.prepared.particles[0].occurrence.study_node = 3;
    check(viewport::snapshot(modified, ctx)["particles"][0]["level"] == 1, "compression/naming inherit conceptual level");
    modified = report;
    modified.particles[field::kRadius * modified.prepared.capacity + slot] = -1;
    rejects([&] { viewport::snapshot(modified, ctx); }, "negative radius");
    modified.particles[field::kRadius * modified.prepared.capacity + slot] = 0;
    check(viewport::snapshot(modified, ctx)["particles"][0]["radius"] == 0, "zero radius preserved");
    modified.particles[slot] = std::numeric_limits<float>::infinity();
    rejects([&] { viewport::snapshot(modified, ctx); }, "nonfinite coordinate");
    modified = report;
    modified.particles.pop_back();
    rejects([&] { viewport::snapshot(modified, ctx); }, "bad array shape");
    modified = report;
    modified.prepared.particles[1].occurrence.slot = slot;
    rejects([&] { viewport::snapshot(modified, ctx); }, "duplicate slots");
    modified = report;
    modified.prepared.focal_groups.clear();
    const auto legacy = viewport::snapshot(modified, ctx);
    check(legacy["frame"]["camera_target"][0] == -offset && legacy["frame"]["target_kind"] == "legacy_origin", "explicit legacy origin");
    modified.prepared.particles.clear();
    ctx.occurrences.clear();
    check(viewport::snapshot(modified, ctx)["particles"].empty(), "empty exposed study");
    const auto revised = logical.revise({});
    const auto refreshed = session.refresh(h, revised, parameters);
    rejects([&] { viewport::capture(session, logical.owner(), viewport::context(logical, h, 1, "old")); }, "refreshed native generation rejects old capture");
    const auto fresh = viewport::capture(session, logical.owner(), viewport::context(revised, refreshed, 0, "new"));
    check(fresh["capture"]["tick"] == "0" && fresh["frame"]["camera_target"][0] == 4, "refreshed generation gets seeded focus and new capture counter");
    // Atomic latest-file writer: exact final content, no temporary left, never a partial read.
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / ("hcp-viewport-atomic-" + std::to_string(getpid()));
    struct Cleanup {  // Removes the directory even when a check throws.
      fs::path path;
      ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); }
    } cleanup{dir};
    fs::remove_all(dir);
    fs::create_directories(dir);
    const auto latest = (dir / "latest.json").string();
    auto entries = [&] { std::vector<std::string> n; for (const auto &e : fs::directory_iterator(dir)) n.push_back(e.path().filename()); return n; };
    auto slurp = [](const std::string &p) { std::ifstream in(p, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); };
    auto make_doc = [](int generation) {
      Json v{{"generation", generation}, {"cells", Json::array()}};
      for (int i = 0; i < 4000; ++i) v["cells"].push_back({{"g", generation}, {"i", i}, {"text", "é\"\n\\"}});
      return v;
    };
    viewport::write_atomic(latest, make_doc(1));
    check(entries() == std::vector<std::string>({"latest.json"}), "first atomic write leaves only the target");
    check(slurp(latest) == make_doc(1).dump(2) + "\n", "exact atomic content");
    viewport::write_atomic(latest, make_doc(2));
    check(entries() == std::vector<std::string>({"latest.json"}) && Json::parse(slurp(latest)) == make_doc(2), "replacement leaves only the final content");
    std::atomic<bool> done{false};
    std::atomic<int> reads{0}, torn{0};
    const Json doc_a = make_doc(3), doc_b = make_doc(4), doc_c = make_doc(2);
    std::thread reader([&] {
      while (!done) {
        const auto text = slurp(latest);
        if (text.empty()) { ++torn; continue; }
        const auto v = Json::parse(text, nullptr, false);
        if (v.is_discarded() || (v != doc_a && v != doc_b && v != doc_c)) ++torn;
        ++reads;
      }
    });
    struct Join {  // A throwing write must not leave a joinable thread behind.
      std::atomic<bool> &stop;
      std::thread &thread;
      ~Join() { stop = true; if (thread.joinable()) thread.join(); }
    } join{done, reader};
    for (int i = 0; i < 150; ++i) viewport::write_atomic(latest, i % 2 ? doc_a : doc_b);
    done = true;
    reader.join();
    check(reads > 0 && torn == 0, "concurrent reader never observes partial content");
    check(entries() == std::vector<std::string>({"latest.json"}), "no temporary after repeated writes");
    fs::remove(latest);
    fs::create_directory(latest);  // A directory target makes the rename fail.
    rejects([&] { viewport::write_atomic(latest, doc_a); }, "failed rename reported");
    check(entries() == std::vector<std::string>({"latest.json"}), "failed write removes its temporary");
    rejects([&] { viewport::write_atomic((dir / "missing" / "latest.json").string(), doc_a); }, "missing directory reported");
    fs::remove_all(dir);
    std::cout << checks << " viewport snapshot checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "viewport snapshot test: " << e.what() << '\n';
    return 1;
  }
}
