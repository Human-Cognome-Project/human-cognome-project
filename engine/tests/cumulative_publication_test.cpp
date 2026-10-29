#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "allocation_observation.h"
#include "engine/device.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"

namespace {
using namespace field;
using namespace field::testing;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(!allocation_observation::calls,
          "ordinary boundary allocated C++ storage");
}
Parameters64 inert() {
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  return p;
}
BudgetDemand limits(NumericalProfile profile, bool cuda) {
  BudgetDemand l;
  l.values = {1048576, 70, 9, 280, 140, 1135, 9, 32, 0};
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, cuda, profile);
  return l;
}
FixtureSource bodies(std::initializer_list<float> positions, float radius = 0) {
  FixtureSource src;
  for (float x : positions) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = x;
    p[kMass] = 1;
    p[kRadius] = radius;
    src.particles.push_back(p);
    src.exact_masses.push_back(1);
  }
  return src;
}
double q(double value, int bits) {
  if (!value)
    return value;
  int exponent;
  std::frexp(std::fabs(static_cast<long double>(value)), &exponent);
  return std::copysign(
      double(std::ldexp(
          std::trunc(std::ldexp(std::fabs(static_cast<long double>(value)),
                                bits - exponent + 1)),
          exponent - 1 - bits)),
      value);
}
void close(double actual, long double expected, const char *message) {
  const auto bound = 64 * std::numeric_limits<double>::epsilon() *
                     std::max(std::fabs(expected), 1e-100L);
  require(std::fabs(actual - expected) <= bound, message);
}
struct Fixture {
  Harness h;
  PreparedFixtureAccess::Input in{70, 9, 32, 9};
  prepared_test::Metrics metrics;
  prepared_test::Sample before;
  explicit Fixture(engine::Runtime &r,
                   HostPath path,
                   NumericalProfile profile,
                   const FixtureSource &src)
      : h(r, PreparedCapacity(limits(profile, r.on_cuda()), profile), path),
        metrics(r, PreparedFixtureAccess::nodes(h)) {
    Parameters p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    no_new([&] { PreparedFixtureAccess::load(h, src, in, p, true); });
    refresh();
    before = metrics.sample();
  }
  void refresh() {
    no_new([&] { h.inspect_prepared(); });
  }
  void tick(const Parameters64 &p) {
    no_new([&] { h.tick64(p); });
    refresh();
    for (std::size_t i = 0; i < h.prepared_storage().live.particles; ++i) {
      const int row = in.particle_rows[i].record;
      long double speed2 = 0;
      for (int a = 0; a < 3; ++a) {
        const auto v = static_cast<long double>(h.particle64(kVelX + a, row));
        speed2 += v * v;
      }
      close(h.determined64()[row], std::sqrt(speed2),
            "final speed cache is stale");
    }
  }
  bool awake(int logical) const {
    return h.prepared_storage()
               .inspection_flags[in.particle_rows[logical].record] &
           2;
  }
  void finish(const char *name) {
    const auto after = metrics.sample();
    require(before.same_backing(after),
            "ordinary reuse changed native backing/compiler");
    std::cout << "case=" << name
              << ",body_bits=" << h.numerical_profile().body_fraction_bits
              << ",field_bits=" << h.numerical_profile().field_fraction_bits
              << ",S="
              << h.prepared_storage().capacity.limits()[Budget::ScratchBytes]
              << ",bindings=" << h.prepared_storage().binding_bytes
              << ",cpp_new=0 PASS\n";
    before.print("before");
    after.print("after");
  }
};
void quiet(Fixture &f, const Parameters64 &p) {
  auto &h = f.h;
  const auto positions = h.particles64(), speeds = h.determined64();
  const auto start = std::chrono::steady_clock::now();
  require(h.prepared_storage().inspection_counts[1] == 0,
          "quiet fixture has pending input");
  for (int i = 0; i < 32; ++i) {
    f.tick(p);
    require(!h.demand().particles_evaluated && !h.demand().motion_attempts &&
                !h.demand().particles_moved &&
                !h.demand().centroids_recomputed &&
                !h.demand().force_requests && !h.demand().contact_evaluations &&
                !h.demand().invalidation_particle_visits &&
                h.prepared_storage().inspection_counts[1] == 0,
            "quiet publication kept polling or lost pending-state accounting");
    require(positions == h.particles64() && speeds == h.determined64(),
            "quiet ticks changed physical state or recovered a discarded tail");
  }
  std::cout << "quiet_window,cycles=32,evaluations=0,attempts=0,publications=0,"
               "reductions=0,pending=0,elapsed_us="
            << std::chrono::duration<double, std::micro>(
                   std::chrono::steady_clock::now() - start)
                   .count()
            << '\n';
}

// Exact binary witnesses isolate publication precision from force arithmetic.
void accepted_motion(engine::Runtime &r, HostPath path, int sign, int bits) {
  auto src = bodies({sign * 1.5f});
  Fixture f(r, path, NumericalProfile::trial(bits, 52), src);
  auto &h = f.h;
  const int row = f.in.particle_rows[0].record;
  for (double dt : {0., .5, 1.}) {
    const double old_x = sign * 1.5, incoming = sign * .03125;
    PreparedFixtureAccess::inject_value64(h, kPosX, row, old_x);
    PreparedFixtureAccess::inject_value64(h, kVelX, row, incoming);
    h.set_tick_mode(TickMode::Selective);
    auto p = inert();
    p.dt = dt;
    f.tick(p);
    const double expected_x = q(old_x + incoming * dt, bits);
    const double expected_v = dt > 0 ? (expected_x - old_x) / dt : incoming;
    require(h.particle64(kPosX, row) == expected_x &&
                h.particle64(kVelX, row) == expected_v &&
                h.determined64()[row] == std::fabs(expected_v),
            "accepted displacement, retained velocity or final speed differs");
    require(f.awake(0) == (expected_x != old_x),
            "motion precision changed publication-based activity");
    std::cout << "accepted_motion,sign=" << sign << ",bits=" << bits
              << ",dt=" << dt << ",x=" << expected_x
              << ",retained_velocity=" << expected_v << '\n';
  }
  f.finish("accepted_motion_precision_and_zero_duration");
}

void subnormal_brake(engine::Runtime &r, HostPath path, int sign) {
  auto src = bodies({sign * 1.5f, -sign * 1.5f}, 2);
  src.contacts = {{0, 1}};
  Fixture f(r, path, NumericalProfile::trial(4, 52), src);
  auto &h = f.h;
  const int row = f.in.particle_rows[0].record;
  const double incoming = sign * .525;
  PreparedFixtureAccess::inject_value64(h, kVelX, row, incoming);
  h.set_tick_mode(TickMode::Selective);
  auto p = inert();
  p.contact_stiffness = .125;
  p.dt = 1;
  const long double pre_brake = std::fabs(static_cast<long double>(incoming)) + .125L;
  const long double ratio = pre_brake / .125L;
  const long double integrated = pre_brake * std::exp(-ratio * ratio * ratio * ratio);
  require(integrated > 0 && integrated < std::numeric_limits<double>::min(),
          "normal inputs did not produce the independent subnormal witness");
  f.tick(p);
  require(h.ready() && h.particle64(kPosX, row) == sign * 1.5 &&
              h.particle64(kVelX, row) == 0 && h.determined64()[row] == 0,
          "transient subnormal braking failed accepted-motion publication");
  std::cout << "subnormal_brake,sign=" << sign
            << ",independent_long_double_intermediate=" << integrated
            << ",retained_velocity=0\n";
  f.finish("subnormal_intermediate_normal_inputs");
}

void combined_step(engine::Runtime &r,
                   HostPath path,
                   int sign,
                   int bits,
                   bool cancel,
                   bool reverse) {
  auto src =
      bodies({sign * 1.5f, sign * 2.5f, cancel ? sign * 2.5f : -sign * 1.5f});
  src.particles[0][kRadius] = 2;
  src.particles[2][kRadius] = cancel ? 0 : 2;
  src.kinds = {1};
  src.edges = {{0, 0, 1, 0, 1}, {1, 0, 1, 1, 1}};
  src.contacts = {reverse ? std::array<int, 2>{2, 0}
                          : std::array<int, 2>{0, 2}};
  Fixture f(r, path, NumericalProfile::trial(bits, 52), src);
  auto &h = f.h;
  const int target = f.in.particle_rows[0].record;
  const int other = f.in.particle_rows[2].record;
  auto p = inert();
  p.gravity = 1. / 64;
  p.contact_stiffness = 1. / 8;
  const long double field = sign / 8.L, contact = cancel ? -field : field,
                    total = field + contact;
  const long double velocity = total * std::exp(total ? -1.L : 0.L);
  const double expected = q(double(sign * 1.5L + velocity), bits);
  if (bits == 4 && !cancel) {
    require(
        q(double(sign * 1.5L + field * std::exp(-1.L)), bits) == sign * 1.5 &&
            q(double(sign * 1.5L + contact * std::exp(-1.L)), bits) ==
                sign * 1.5,
        "individual force witness is not subthreshold");
    require(expected == sign * 1.5625,
            "independent cumulative/cancellation reference has wrong cell");
  }
  f.tick(p);
  close(h.particle64(kForceX, target), total,
        "fields and contact were not combined once");
  close(h.particle64(kVelX, target), expected - sign * 1.5,
        "retained cumulative displacement reference differs");
  close(h.particle64(kPosX, target), expected,
        "cumulative body publication differs");
  require(h.demand().particles_evaluated == 3 &&
              h.demand().contact_evaluations == 2,
          "body integrated more than once or omitted a contact index side");
  if (cancel) {
    require(h.particle64(kPosX, target) == sign * 1.5 &&
                h.determined64()[target] == 0 &&
                h.prepared_storage().motion64()[target] == 0,
            "cancelling inputs independently generated movement");
  } else if (bits == 4) {
    require(
        h.particle64(kPosX, other) == -sign * 1.5 && f.awake(2),
        "later contact propagation lost the quiet recipient's pending work");
    const double next_contact =
        -sign * p.contact_stiffness * (4 - std::fabs(expected + sign * 1.5));
    f.tick(p);
    close(
        h.particle64(kForceX, other), next_contact,
        "pending contact recipient did not evaluate its newly published input");
  }
  std::cout << "combined_step,sign=" << sign << ",cancel=" << cancel
            << ",reverse_contact=" << reverse
            << ",individual_force=" << double(field)
            << ",combined_force=" << double(total) << ",expected_x=" << expected
            << ",expected_v=" << double(velocity) << '\n';
  f.finish("combined_step");
}

void velocity_and_contact_wake(engine::Runtime &r, HostPath path) {
  auto src = bodies({1.5f, -1.5f, 16.f}, 2);
  Fixture f(r, path, NumericalProfile::trial(4, 4), src);
  auto &h = f.h;
  auto p = inert();
  p.contact_stiffness = .01;
  f.tick(p);
  quiet(f, p);
  UpdateCommand add;
  add.operation = UpdateOp::InsertContact;
  add.first = UpdateRef::existing(f.in.particle_rows[1]);
  add.second = UpdateRef::existing(f.in.particle_rows[0]);
  UpdateResult result;
  no_new([&] {
    result = h.update_prepared(h.prepared_storage().generation, &add, 1);
  });
  f.refresh();
  require(result.status == UpdateStatus::Ok && f.awake(0) && f.awake(1) &&
              !f.awake(2),
          "contact insertion did not wake exactly its sleeping endpoints");
  f.tick(p);
  require(h.demand().particles_evaluated == 2 &&
              h.demand().contact_evaluations == 2 &&
              !h.demand().particles_moved && !f.awake(0) && !f.awake(1),
          "velocity-only contact work retained movement activity");
  close(h.particle64(kVelX, f.in.particle_rows[0].record), 0,
        "quiet publication retained unapplied motion");
  close(h.particle64(kVelX, f.in.particle_rows[1].record), 0,
        "reverse contact index side retained unapplied motion");
  quiet(f, p);
  h.set_tick_mode(TickMode::Selective);  // Explicit control-input invalidation.
  f.tick(p);
  require(h.demand().particles_evaluated == 3 && !h.demand().particles_moved,
          "explicit invalidation was blocked by quiet publications");
  quiet(f, p);
  p.contact_stiffness = .5;
  f.tick(p);
  require(
      h.demand().particles_evaluated == 3 && h.demand().particles_moved == 2,
      "parameter change failed to wake zero retained motion and cross body Q");
  f.finish("velocity_contact_and_parameter_wakes");
  // Public invalidate() withdraws publication readiness; it is not a wake API.
  h.invalidate();
  bool rejected = false;
  try {
    h.tick64(p);
  } catch (const std::logic_error &) {
    rejected = true;
  }
  require(rejected && !h.ready(),
          "invalidated publication was allowed to tick");
}

void radial_cache(engine::Runtime &r, HostPath path) {
  auto src = bodies({1.5f});
  for (int axis = 0; axis < 3; ++axis) {
    src.particles[0][kPosX + axis] = 1.5f;
    src.particles[0][kVelX + axis] = .01f;
  }
  Fixture f(r, path, NumericalProfile::trial(4, 4), src);
  auto &h = f.h;
  const int row = f.in.particle_rows[0].record;
  auto p = inert();
  p.reach_strength = 1. / 3;
  const long double v = static_cast<long double>(.01f) * std::exp(-1.25L);
  f.tick(p);
  for (int a = 0; a < 3; ++a) {
    require(h.particle64(kPosX + a, row) == 1.5,
            "radial witness moved unexpectedly");
    close(h.particle64(kVelX + a, row), 0,
          "quiet radial publication retained unapplied motion");
  }
  close(h.determined64()[row], 0,
        "cached speed differs from retained motion");
  require(!f.awake(0), "radial velocity-only change retained activity");
  quiet(f, p);
  std::cout << "radial_cache,expected_component=" << double(v)
            << ",expected_speed=" << double(std::sqrt(3.L) * v) << '\n';
  f.finish("post_radial_three_axis_cache");
}

void centroid_witness(engine::Runtime &r, HostPath path, int sign, int kind) {
  const double step = 1. / 16;
  auto src = bodies(
      kind == 2 ? std::initializer_list<float>{sign * 1.125f, sign * 1.125f}
                : std::initializer_list<float>{sign * 1.f, sign * 1.25f});
  src.kinds = {1};
  if (kind == 2) {
    src.exact_masses = {4, 4};
    src.edges = {{0, 0, .25f, 0, 1}, {1, 0, .25f, 1, 1}, {1, 0, .5f, 2, 2}};
    src.particles[0][kVelX] = sign * 3 * step;
    src.particles[1][kVelX] = sign * step;
  } else {
    src.edges = {{0, 0, 1, 0, 1}, {1, 0, 1, 1, 1}};
    const double velocity = kind == 3 ? step / 4 : 1.5 * step;
    src.particles[0][kVelX] = sign * velocity;
    src.particles[1][kVelX] = (kind == 1 ? -sign : sign) * velocity;
    if (kind == 4) {
      std::array<float, kParticleFieldCount> sleeper{};
      sleeper[kPosX] = sign * 1.125f;
      sleeper[kMass] =
          2;  // Physical mass is independent of structural denominator.
      src.particles.push_back(sleeper);
      src.exact_masses.push_back(4);
      src.edges.push_back({2, 0, .5f, 2, 2});
    }
  }
  Fixture f(r, path, NumericalProfile::trial(52, 4), src);
  auto &h = f.h;
  const int group = f.in.field_rows[0].record;
  const double before = h.group64(kCenX, group);
  f.tick(inert());
  long double mass = 0, weighted = 0;
  for (const auto &e : src.edges) {
    const long double m =
        static_cast<long double>(src.particles[e.particle][kMass]) *
        e.exact_count / src.exact_masses[e.particle];
    mass += m;
    weighted += m * h.particle64(kPosX, f.in.particle_rows[e.particle].record);
  }
  const double expected = q(double(weighted / mass), 4);
  const auto &s = h.prepared_storage();
  require(
      h.group64(kCenX, group) == expected &&
          h.group64(kCenM, group) == double(mass) &&
          h.demand().centroids_recomputed == 1 &&
          h.demand().centroid_reduction_edges == src.edges.size(),
      "combined centroid differs from independent exact weighted reference");
  require((kind == 1 || kind == 3) ? expected == before : expected != before,
          "centroid witness missed its declared publication/cancellation cell");
  require(s.inspection_state[2 * h.particle_capacity() + h.field_capacity() +
                             group] == int(expected != before),
          "centroid Changed is not cumulative publication");
  require(s.motion64()[3 * h.particle_capacity() + 3 * h.field_capacity() +
                       group] == before,
          "centroid transient reduction overwrote published history");
  for (int a = 0; a < 3; ++a)
    require(s.motion64()[3 * h.particle_capacity() + a * h.field_capacity() +
                         group] == 0,
            "centroid retained discarded positional detail");
  if (kind == 4)
    require(h.particle64(kPosX, f.in.particle_rows[2].record) == sign * 1.125 &&
                !f.awake(2) && h.universal64() == 4,
            "sleeping member was omitted, moved, or counted as summary mass");
  if (kind == 3) {
    for (int i = 0; i < 2; ++i)
      PreparedFixtureAccess::inject_value64(h, kVelX,
                                            f.in.particle_rows[i].record, 0);
    // Explicit all-loaded reference forces repeated fresh reductions of the
    // same fine accepted positions. No field remainder may accumulate.
    h.set_tick_mode(TickMode::AllLoaded);
    for (int round = 0; round < 32; ++round) {
      f.tick(inert());
      require(h.group64(kCenX, group) == before &&
                  h.demand().centroids_recomputed == 1 &&
                  !h.demand().motion_attempts,
              "repeated fresh reductions recovered a discarded field tail");
    }
    h.set_tick_mode(TickMode::Selective);
    f.tick(inert());
    quiet(f, inert());
  }
  std::cout << "centroid_witness,sign=" << sign << ",kind=" << kind
            << ",old=" << before << ",raw=" << double(weighted / mass)
            << ",published=" << expected << ",mass=" << double(mass) << '\n';
  f.finish("centroid_witness");
}

void count_and_focus_wakes(engine::Runtime &r,
                           HostPath path,
                           bool change_focus) {
  const std::uint64_t denominator = change_focus ? 2 : std::uint64_t{1} << 53;
  auto src = bodies({1.5f, change_focus ? 2.5f : 1.5f, 16.f});
  src.kinds = {1};
  src.exact_masses = {1, denominator, 1};
  const auto count = change_focus ? denominator : denominator - 1;
  src.edges = {{0, 0, 1, 0, 1},
               {1, 0, float(double(count) / denominator), 1, count}};
  src.focus = {0};
  Fixture f(r, path, NumericalProfile::trial(4, 4), src);
  auto &h = f.h;
  const int group = f.in.field_rows[0].record;
  auto p = inert();
  p.reach_strength = .001;
  f.tick(p);
  quiet(f, p);
  const auto bodies_before = h.particles64(), speed_before = h.determined64();
  std::uint64_t evaluated = 0, uploads = 0, readbacks = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 32; ++i) {
    const auto old_mass = h.group64(kCenM, group),
               old_x = h.group64(kCenX, group);
    UpdateCommand edit;
    edit.operation = UpdateOp::ReplaceParticipation;
    edit.row = 1;
    edit.generation = h.prepared_storage().generation;
    edit.ordinal = 1;
    edit.first = UpdateRef::existing(f.in.particle_rows[1]);
    edit.second = UpdateRef::existing(f.in.field_rows[0]);
    edit.exact_count = i % 2 ? count : count - 1;
    edit.share = float(double(*edit.exact_count) / denominator);
    UpdateResult result;
    no_new([&] { result = h.update_prepared(edit.generation, &edit, 1); });
    uploads += result.upload_bytes;
    readbacks += result.readback_bytes;
    f.refresh();
    require(result.status == UpdateStatus::Ok &&
                h.group64(kCenM, group) != old_mass &&
                (change_focus ? h.group64(kCenX, group) != old_x
                              : h.group64(kCenX, group) == old_x) &&
                f.awake(0) && f.awake(1) && f.awake(2) == change_focus &&
                bodies_before == h.particles64() &&
                speed_before == h.determined64(),
            "count/focal repair lost invalidation or reset survivor state");
    require(h.prepared_storage().motion64()[3 * h.particle_capacity() +
                                            3 * h.field_capacity() + group] ==
                old_x,
            "coherent repair erased previous publication history");
    f.tick(p);
    evaluated += h.demand().particles_evaluated;
    require(h.demand().particles_evaluated == (change_focus ? 3 : 2) &&
                !h.demand().particles_moved && !h.demand().motion_attempts &&
                !h.prepared_storage().inspection_counts[1],
            "count/focus input was not evaluated before consuming quiet work");
  }
  quiet(f, p);
  std::cout << "count_focus_reuse,focus_changed=" << change_focus
            << ",same_f32_share=" << !change_focus
            << ",cycles=32,evaluated=" << evaluated
            << ",upload_bytes=" << uploads << ",readback_bytes=" << readbacks
            << ",elapsed_us="
            << std::chrono::duration<double, std::micro>(
                   std::chrono::steady_clock::now() - start)
                   .count()
            << '\n';
  f.finish("count_and_focus_reuse");
}
}  // namespace

int main(int argc, char **argv) {
  try {
    bool direct = false, cuda = false, motion_only = false;
    int threads = 1;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--motion-only")
        motion_only = true;
      else if (arg == "--backend" && i + 1 < argc) {
        const std::string backend = argv[++i];
        require(backend == "cpu" || backend == "cuda", "unknown backend");
        cuda = backend == "cuda";
      }
      else if (arg == "--threads" && i + 1 < argc)
        threads = std::stoi(argv[++i]);
      else
        throw std::invalid_argument("unknown cumulative publication option");
    }
    require(threads == 1 || threads == 2,
            "expected one/two native CPU workers");
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = threads;
    require(!(cuda && direct), "CpuDirect cannot execute on CUDA");
    if (cuda)
      require(engine::cuda_available(), "CUDA requested but no visible device");
    settings.cuda = cuda;
    if (cuda) settings.device_memory_gb = 1.0;
    engine::Runtime r(settings);
    require(r.arch() == (cuda ? taichi::Arch::cuda : taichi::Arch::x64),
            "requested backend differs from actual backend");
    std::cout << std::setprecision(17)
              << "actual_backend=" << (cuda ? "cuda" : "cpu") << ",threads=" << threads
              << ",path=" << (direct ? "direct" : "bulk") << '\n';
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    for (int sign : {1, -1}) {
      for (int bits : {4, 8, 52})
        accepted_motion(r, path, sign, bits);
      subnormal_brake(r, path, sign);
    }
    if (motion_only) return 0;
    velocity_and_contact_wake(r, path);
    radial_cache(r, path);
    count_and_focus_wakes(r, path, false);
    count_and_focus_wakes(r, path, true);
    for (int sign : {1, -1}) {
      combined_step(r, path, sign, 4, false, false);
      combined_step(r, path, sign, 4, false, true);
      combined_step(r, path, sign, 4, true, false);
      combined_step(r, path, sign, 52, false, false);
      for (int kind = 0; kind < 5; ++kind)
        centroid_witness(r, path, sign, kind);
    }
    return 0;
  } catch (const std::string &e) {
    std::cerr << e << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
