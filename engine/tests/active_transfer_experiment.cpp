#include <sys/resource.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

#include "active_workbench_checks.h"

namespace {
using namespace active_checks;
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}
struct Case {
  const char *name;
  int P, G, E;
  bool skew;
};
void run(engine::Runtime &runtime, const Case &c, field::HostPath path) {
  rusage before{};
  getrusage(RUSAGE_SELF, &before);
  field::Harness f(runtime, c.P, c.G, c.E, 0, path);
  const auto pack_start = Clock::now();
  for (int p = 0; p < c.P; ++p) {
    f.particle(field::kMass, p) = 2;
    f.particle(field::kRadius, p) = 0.125f;
    f.particle(field::kPosX, p) = float(p * 4);
  }
  for (int e = 0; e < c.E; ++e) {
    int particle, group;
    if (c.P == 16 || c.skew) {
      particle = e % 16;
      group = e / 16;
    } else {
      const int degree = c.E / c.P;
      particle = e / degree;
      group = (particle + e % degree) % c.G;
    }
    f.edge_int(field::kEdgeParticle, e) = particle;
    f.edge_int(field::kEdgeGroup, e) = group;
    f.edge_float(field::kShare, e) = 1;
  }
  const auto pack_ms = elapsed(pack_start);
  field::Parameters parameters;
  parameters.gravity = 0.000125f;
  parameters.dt = 0;
  f.upload();
  check(!f.ready(), "uploaded payload is not ready before seeding");
  check(f.uploaded_payload_matches(),
        "all five packed f32/i32 spans match exact transfer bit patterns");
  f.seed_centroids(parameters);
  f.download();
  std::vector<double> mass(c.G), weighted(c.G), force(c.P);
  for (int e = 0; e < c.E; ++e) {
    const int p = f.edge_int(field::kEdgeParticle, e),
              g = f.edge_int(field::kEdgeGroup, e);
    mass[g] += 2;
    weighted[g] += double(p * 4) * 2;
  }
  double max_centroid_error = 0;
  bool exact_mass = true;
  for (int g = 0; g < c.G; ++g) {
    const double x = mass[g] ? weighted[g] / mass[g] : 0;
    max_centroid_error =
        std::max(max_centroid_error, std::abs(f.group(field::kCenX, g) - x));
    exact_mass &= f.group(field::kCenM, g) == mass[g];
  }
  check(exact_mass, "synthetic field mass exact");
  check(max_centroid_error <= 1e-3,
        "density fixture centroid error within recorded 1e-3 budget");
  for (int e = 0; e < c.E; ++e) {
    const int p = f.edge_int(field::kEdgeParticle, e),
              g = f.edge_int(field::kEdgeGroup, e);
    const double dx = weighted[g] / mass[g] - p * 4;
    if (dx && mass[g] != 2)
      force[p] +=
          parameters.gravity * 2 * mass[g] * dx / std::pow(std::abs(dx), 3);
  }
  const auto first_start = Clock::now();
  f.tick(parameters);
  const auto first_ms = elapsed(first_start);
  f.download();
  bool coherent = true;
  double max_force_error = 0;
  for (int p = 0; p < c.P; ++p) {
    const double actual = f.particle(field::kForceX, p);
    const auto error = std::abs(actual - force[p]);
    max_force_error = std::max(max_force_error, error);
    coherent &=
        std::isfinite(actual) && error <= 0.005 + 0.001 * std::abs(force[p]);
  }
  check(
      coherent,
      "density fixture first-force error within abs 0.005 + rel 0.001 budget");
  const auto steady_start = Clock::now();
  for (int step = 0; step < 3; ++step) f.tick(parameters);
  const auto steady_ms = elapsed(steady_start) / 3;
  const auto t = f.timings();
  rusage after{};
  getrusage(RUSAGE_SELF, &after);
  std::cout << "MEASURE case=" << c.name << " path="
            << (path == field::HostPath::BulkCopy ? "copy" : "cpu-direct")
            << " P=" << c.P << " N=" << c.P << " G=" << c.G << " E=" << c.E
            << " K=0"
            << " pack_ms=" << pack_ms << " allocation_ms=" << t.allocation_ms
            << " compile_or_cache_ms=" << t.compilation_ms
            << " upload_ms=" << t.upload_ms << " seed_ms=" << t.seed_ms
            << " first_ms=" << first_ms << " steady_ms=" << steady_ms
            << " report_ms=" << t.report_ms
            << " payload_upload_bytes=" << t.upload_bytes
            << " payload_download_bytes=" << t.download_bytes
            << " minor_faults=" << after.ru_minflt - before.ru_minflt
            << " major_faults=" << after.ru_majflt - before.ru_majflt
            << " max_centroid_error=" << max_centroid_error
            << " max_force_error=" << max_force_error << '\n';
}
}  // namespace
int main(int argc, char **argv) {
  try {
    const bool cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    engine::InstanceSettings settings;
    settings.cuda = cuda;
    settings.read_environment = false;
    settings.cpu_threads = 2;
    engine::Runtime runtime(settings);
    const Case cases[] = {{"few-dense", 16, 4096, 65536, false},
                          {"many-low", 4096, 32, 8192, false},
                          {"matched-uniform", 4096, 4096, 65536, false},
                          {"matched-skew", 4096, 4096, 65536, true}};
    for (const auto &c : cases) {
      run(runtime, c, field::HostPath::BulkCopy);
      if (!cuda) run(runtime, c, field::HostPath::CpuDirect);
    }
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  return harness::report("active_transfer_experiment");
}
