// Runs a field scenario and records what the model does, sample by sample.
// Observation only: the tick is field::Harness::tick exactly as built. The
// probe downloads every --observe-every ticks (default: every tick) and
// hands each state to the monitor, which accumulates steps; one CSV row is
// written per --every ticks. Observing every tick is what makes touched
// counts and reversals trustworthy: coarser observation can miss motion
// that returns between observations (ticks_per_step reports the gap).
//
//   . ./build/engine-env.sh
//   ./build/field_probe --scenario clusters --particles 512 --groups 32 \
//       --ticks 2000 --every 10 --out runs/clusters.csv
//   ./build/field_probe --scenario ring --particles 64
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "engine/runtime.h"
#include "field/field.h"
#include "monitor/field_monitor.h"

namespace {

[[noreturn]] void usage(int code) {
  std::printf(
      "usage: field_probe [--scenario ring|clusters] [--particles N]\n"
      "                   [--groups N] [--max-memberships N] [--seed N]\n"
      "                   [--ticks N] [--every N] [--observe-every N]\n"
      "                   [--dt F]\n"
      "                   [--threshold F]\n"
      "                   [--threads N] [--cuda] [--out FILE]\n"
      "  ring      the field_test ring: every particle in one shared group.\n"
      "  clusters  particles scattered in a cube, each a whole member of\n"
      "            1..max-memberships groups chosen from --groups.\n"
      "  --every          ticks per CSV row (window).\n"
      "  --observe-every  ticks between observations (default 1; larger is\n"
      "                   cheaper but can miss motion that returns).\n"
      "  --threads        CPU threads per kernel. Atomic float sums are\n"
      "                   order-dependent, so only --threads 1 repeats exactly.\n"
      "  Samples go to --out as CSV (default: stdout).\n");
  std::exit(code);
}

const char *value_of(int argc, char **argv, int &i) {
  if (i + 1 >= argc) {
    std::fprintf(stderr, "%s needs a value\n", argv[i]);
    usage(2);
  }
  return argv[++i];
}

struct Options {
  std::string scenario = "clusters";
  int particles = 256;
  int groups = 16;
  int max_memberships = 3;
  std::uint32_t seed = 1;
  long ticks = 1000;
  long every = 10;
  long observe_every = 1;
  float dt = 1.0f;
  float threshold = 1e-4f;
  bool cuda = false;
  int threads = 0;  // 0: runtime default
  std::string out;
};

Options parse(int argc, char **argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i];
    if (!std::strcmp(a, "--scenario")) {
      o.scenario = value_of(argc, argv, i);
    } else if (!std::strcmp(a, "--particles")) {
      o.particles = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--groups")) {
      o.groups = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--max-memberships")) {
      o.max_memberships = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--seed")) {
      o.seed = std::uint32_t(std::strtoul(value_of(argc, argv, i), nullptr, 10));
    } else if (!std::strcmp(a, "--ticks")) {
      o.ticks = std::atol(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--every")) {
      o.every = std::atol(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--observe-every")) {
      o.observe_every = std::atol(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--dt")) {
      o.dt = float(std::atof(value_of(argc, argv, i)));
    } else if (!std::strcmp(a, "--threshold")) {
      o.threshold = float(std::atof(value_of(argc, argv, i)));
    } else if (!std::strcmp(a, "--threads")) {
      o.threads = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(a, "--cuda")) {
      o.cuda = true;
    } else if (!std::strcmp(a, "--out")) {
      o.out = value_of(argc, argv, i);
    } else if (!std::strcmp(a, "--help") || !std::strcmp(a, "-h")) {
      usage(0);
    } else {
      std::fprintf(stderr, "unknown argument %s\n", a);
      usage(2);
    }
  }
  const bool known = o.scenario == "ring" || o.scenario == "clusters";
  if (!known || o.particles < 1 || o.groups < 1 || o.max_memberships < 1 ||
      o.ticks < 0 || o.every < 1 || o.observe_every < 1) {
    usage(2);
  }
  return o;
}

// Small deterministic generator so a scenario is reproducible from its seed.
struct Lcg {
  std::uint32_t state;
  std::uint32_t next() {
    state = state * 1664525u + 1013904223u;
    return state;
  }
  float unit() { return float(next() >> 8) / float(1u << 24); }
};

void set_particle(field::Harness &h, int i, float x, float y, float z) {
  h.particle(field::kPosX, i) = x;
  h.particle(field::kPosY, i) = y;
  h.particle(field::kPosZ, i) = z;
  h.particle(field::kMass, i) = 1.0f;
}

void set_whole_edge(field::Harness &h, int e, int particle, int group) {
  h.edge_int(field::kEdgeParticle, e) = particle;
  h.edge_int(field::kEdgeGroup, e) = group;
  h.edge_float(field::kShare, e) = 1.0f;
}

double ms_since(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start)
      .count();
}

}  // namespace

int main(int argc, char **argv) {
  const Options o = parse(argc, argv);

  // Memberships first: the harness is sized by its edge count.
  std::vector<std::vector<int>> memberships(std::size_t(o.particles));
  Lcg rng{o.seed};
  int groups = 1;
  if (o.scenario == "ring") {
    for (auto &m : memberships) {
      m.push_back(0);
    }
  } else {
    groups = o.groups;
    for (auto &m : memberships) {
      const int count = 1 + int(rng.next() % std::uint32_t(o.max_memberships));
      while (int(m.size()) < std::min(count, groups)) {
        const int g = int(rng.next() % std::uint32_t(groups));
        bool seen = false;
        for (int existing : m) {
          seen = seen || existing == g;
        }
        if (!seen) {
          m.push_back(g);
        }
      }
    }
  }
  int edges = 0;
  for (const auto &m : memberships) {
    edges += int(m.size());
  }

  engine::InstanceSettings settings;
  settings.cuda = o.cuda;
  settings.snode_capacity = 1 << 20;
  settings.snode_tree_capacity = 1 << 16;
  if (o.threads > 0) {
    settings.cpu_threads = o.threads;
  }
  engine::Runtime runtime(settings);
  field::Harness h(runtime, o.particles, groups, edges);

  const float side = std::cbrt(float(o.particles));
  int e = 0;
  for (int i = 0; i < o.particles; ++i) {
    if (o.scenario == "ring") {
      const float a = float(i) * 0.61f;
      set_particle(h, i, std::cos(a) * (1.0f + 0.03f * i),
                   std::sin(a) * (1.0f + 0.03f * i), 0.02f * i);
    } else {
      set_particle(h, i, side * rng.unit(), side * rng.unit(), side * rng.unit());
    }
    for (int g : memberships[std::size_t(i)]) {
      set_whole_edge(h, e++, i, g);
    }
  }

  field::Parameters p;
  p.dt = o.dt;
  h.upload();
  h.seed_centroids(p);

  std::FILE *out = stdout;
  if (!o.out.empty()) {
    out = std::fopen(o.out.c_str(), "w");
    if (!out) {
      std::fprintf(stderr, "cannot open %s\n", o.out.c_str());
      return 1;
    }
  }
  std::fprintf(stderr, "field_probe: %s, %d particles, %d groups, %d edges\n",
               o.scenario.c_str(), o.particles, groups, edges);

  monitor::Settings ms;
  ms.motion_threshold = o.threshold;
  monitor::FieldMonitor mon(ms);
  monitor::write_csv_header(out);

  const std::size_t bytes =
      (h.particle_data.size() + h.group_data.size() + h.determined().size()) *
      sizeof(float);
  double download_ms_total = 0.0;
  auto observe = [&](long tick) {
    const auto start = std::chrono::steady_clock::now();
    h.download();
    download_ms_total += ms_since(start);
    mon.observe(monitor::view_of(h), tick);
  };

  observe(0);
  double tick_ms_total = 0.0;
  long ticks_in_window = 0;
  long observations_in_window = 0;
  for (long t = 1; t <= o.ticks; ++t) {
    const auto start = std::chrono::steady_clock::now();
    h.tick(p);
    runtime.synchronize();
    tick_ms_total += ms_since(start);
    ++ticks_in_window;
    const bool row = t % o.every == 0 || t == o.ticks;
    if (t % o.observe_every == 0 || row) {
      observe(t);
      ++observations_in_window;
    }
    if (row) {
      monitor::Sample s = mon.take_sample();
      s.tick_ms = tick_ms_total / double(ticks_in_window);
      s.download_ms = download_ms_total / double(std::max(1L, observations_in_window));
      s.download_bytes = bytes;
      monitor::write_csv_row(out, s);
      if (s.nonfinite_particles > 0 || s.nonfinite_groups > 0) {
        std::fprintf(stderr, "field_probe: non-finite state at tick %ld\n", t);
      }
      tick_ms_total = 0.0;
      download_ms_total = 0.0;
      ticks_in_window = 0;
      observations_in_window = 0;
    }
  }
  if (out != stdout) {
    std::fclose(out);
  }
  return 0;
}
