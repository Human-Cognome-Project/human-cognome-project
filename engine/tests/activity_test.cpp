#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "field/field.h"
#include "harness.h"

namespace {
using namespace field;
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  harness::record(ok, label, "activity contract failed");
  if (!ok) throw std::runtime_error(label);
}
template <class F>
void rejects(F run, const char *label) {
  bool refused = false;
  try {
    run();
  } catch (const std::exception &) {
    refused = true;
  }
  check(refused, label);
}
struct Edge {
  int particle, group;
  float share{1};
};
struct Input {
  std::vector<float> x, mass, velocity;
  int groups{0};
  std::vector<Edge> edges;
  std::vector<std::pair<int, int>> contacts;
};
std::unique_ptr<Harness> make(engine::Runtime &runtime, const Input &input,
                              const Parameters &p,
                              TickMode mode = TickMode::Selective) {
  auto h = std::make_unique<Harness>(runtime, int(input.x.size()), input.groups,
                                     int(input.edges.size()),
                                     int(input.contacts.size()));
  for (std::size_t i = 0; i < input.x.size(); ++i) {
    h->particle(kPosX, int(i)) = input.x[i];
    h->particle(kMass, int(i)) = input.mass[i];
    h->particle(kVelX, int(i)) = input.velocity.empty() ? 0 : input.velocity[i];
    h->particle(kRadius, int(i)) = 0.125f;
  }
  for (std::size_t e = 0; e < input.edges.size(); ++e) {
    h->edge_int(kEdgeParticle, int(e)) = input.edges[e].particle;
    h->edge_int(kEdgeGroup, int(e)) = input.edges[e].group;
    h->edge_float(kShare, int(e)) = input.edges[e].share;
  }
  for (std::size_t k = 0; k < input.contacts.size(); ++k) {
    h->contact_int(kContactA, int(k)) = input.contacts[k].first;
    h->contact_int(kContactB, int(k)) = input.contacts[k].second;
  }
  h->set_tick_mode(mode);
  h->publish(p);
  return h;
}
void row(const char *scenario, int tick, const Harness &h) {
  const auto &d = h.demand();
  std::cout << "DEMAND," << scenario << ',' << tick << ',' << d.particles_loaded
            << ',' << d.particles_exposed << ',' << d.fields_loaded << ','
            << d.fields_exposed << ',' << d.edges_loaded << ','
            << d.edges_exposed << ',' << d.particles_evaluated << ','
            << d.particles_skipped << ',' << d.particles_moved << ','
            << d.particles_woken << ',' << d.force_requests << ','
            << d.force_evaluations << ',' << d.force_zero_response << ','
            << d.force_skipped << ',' << d.gate_checks << ',' << d.gate_rejected
            << ',' << d.geometry_rejected << ',' << d.touch_adjacency_visits
            << ',' << d.force_mask_visits << ',' << d.centroid_gate_visits
            << ',' << d.centroid_mask_visits << ',' << d.centroids_flagged
            << ',' << d.centroids_recomputed << ',' << d.centroid_touch_edges
            << ',' << d.centroid_reduction_edges << ',' << d.tick_ms << '\n';
}
void compare(Harness &a, Harness &b, float absolute = 1e-5f) {
  a.download();
  b.download();
  bool coherent = true;
  for (std::size_t i = 0; i < a.particle_data.size(); ++i) {
    const float x = a.particle_data[i], y = b.particle_data[i];
    coherent &=
        std::isfinite(x) && std::isfinite(y) && std::abs(x - y) <= absolute;
  }
  for (int g = 0; g < a.group_count(); ++g) {
    coherent &= a.group(kCenM, g) == b.group(kCenM, g);
    for (int axis = 0; axis < 3; ++axis)
      coherent &= std::abs(a.group(kCenX + axis, g) -
                           b.group(kCenX + axis, g)) <= absolute;
  }
  if (!coherent) {
    for (std::size_t i = 0; i < a.particle_data.size(); ++i)
      if (a.particle_data[i] != b.particle_data[i])
        std::cerr << "particle scalar " << i << ": " << a.particle_data[i]
                  << " vs " << b.particle_data[i] << '\n';
    for (int g = 0; g < a.group_count(); ++g)
      std::cerr << "group " << g << ": " << std::hexfloat << a.group(kCenX, g)
                << " vs " << b.group(kCenX, g) << std::defaultfloat << '\n';
  }
  check(coherent,
        "selective result agrees with all-loaded reference within stated "
        "fixture tolerance");
}
Parameters parameters(float gravity = 0.125f, float dt = 1) {
  Parameters p;
  p.gravity = gravity;
  p.dt = dt;
  p.reach_strength = 0;
  return p;
}
void settled_and_invalidation(engine::Runtime &runtime) {
  auto p = parameters();
  const Input input{{0, 0}, {1, 1}, {}, 1, {{0, 0}, {1, 0}}, {}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 3; ++tick) {
    s->tick(p);
    r->tick(p);
    compare(*s, *r);
    row("fixed-point", tick, *s);
    row("fixed-point-all-loaded", tick, *r);
    check(s->demand().particles_moved == 0 && s->demand().particles_woken == 0,
          "coincident balanced particles never pass positional wake gate");
  }
  check(s->demand().force_evaluations == 0 && s->demand().force_skipped == 2 &&
            s->demand().particles_skipped == 2 &&
            s->demand().centroids_recomputed == 0,
        "fixed point skips repeated force and centroid work");
  check(s->demand().touch_adjacency_visits == 2 &&
            s->demand().force_mask_visits == 2 &&
            s->demand().centroid_gate_visits == 2 &&
            s->demand().centroid_mask_visits == 3 && s->demand().tick_ms > 0,
        "demand reports mask/adjacency visits and time separately from avoided "
        "force work");
  p.gravity *= 2;
  s->tick(p);
  r->tick(p);
  compare(*s, *r);
  check(s->demand().particles_evaluated == 2 &&
            s->demand().force_evaluations == 2 &&
            s->demand().particles_woken == 0,
        "parameter invalidation reevaluates work without automatically waking "
        "particles");
  s->publish(p);
  check(s->ready() && s->demand().force_evaluations == 0,
        "publication reseeds and clears previous demand generation");
  s->tick(p);
  check(s->demand().force_evaluations == 2,
        "publication invalidates persistent exclusions");
  s->particle(kMass, 0) = 2;
  s->publish(p);
  s->tick(p);
  s->download();
  check(s->group(kCenM, 0) == 3 && s->demand().force_requests == 2,
        "mass change publication rebuilds cached group mass and invalidates "
        "old work");
}
void ineffective_touch(engine::Runtime &runtime) {
  const auto p = parameters(0);
  const Input input{{0, 1000000, -1000000},
                    {1073741824.0f, 1, 1},
                    {0, 1, 0},
                    1,
                    {{0, 0}, {1, 0}, {2, 0}},
                    {}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 2; ++tick) {
    s->tick(p);
    r->tick(p);
    compare(*s, *r);
    row("ineffective-touch", tick, *s);
  }
  check(s->group(kCenX, 0) != 0 && s->demand().geometry_rejected >= 1,
        "changed centroid rejects interactions whose computed relative vector "
        "is unchanged");
  check(s->particle(kPosX, 0) == 0 && s->particle(kPosX, 2) == -1000000 &&
            s->demand().particles_woken == 0 && s->demand().gate_rejected >= 1,
        "changed centroid with no resulting displacement does not wake "
        "stationary particles");
  check(s->demand().force_requests == 2 && s->demand().force_evaluations == 0 &&
            s->demand().particles_skipped == 1,
        "unchanged interaction inputs justify actual force omission, not a "
        "speed heuristic");
}
void all_fields_and_sleeping_members(engine::Runtime &runtime) {
  const auto p = parameters(1e-5f, 0.125f);
  const Input input{{0, -4, 4}, {1, 16777216.0f, 16777216.0f},    {0, 0.25f, 0},
                    2,          {{0, 0}, {1, 0}, {0, 1}, {2, 1}}, {}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  s->tick(p);
  r->tick(p);
  compare(*s, *r);
  check(s->particle(kPosX, 0) == 0 && s->demand().centroids_recomputed == 1,
        "initial combined forces balance while one supplied moving member "
        "perturbs one field");
  s->tick(p);
  r->tick(p);
  compare(*s, *r);
  row("all-connected-fields", 2, *s);
  check(s->particle(kPosX, 0) != 0 && s->demand().particles_woken == 1 &&
            s->demand().force_requests == 3 &&
            s->demand().force_evaluations == 2,
        "one changed field wakes the particle using both connected fields, "
        "including unchanged field");
  check(s->demand().centroids_flagged == 1 &&
            s->demand().centroids_recomputed == 1,
        "tiny participation-mass ratio does not propagate to the massive "
        "unchanged field");

  const auto symmetric = parameters();
  const Input balanced{{-4, 0, 4}, {1, 1, 1}, {}, 1, {{0, 0}, {1, 0}, {2, 0}},
                       {}};
  auto b = make(runtime, balanced, symmetric),
       br = make(runtime, balanced, symmetric, TickMode::AllLoaded);
  for (int tick = 1; tick <= 2; ++tick) {
    b->tick(symmetric);
    br->tick(symmetric);
    compare(*b, *br);
  }
  row("any-active-once", 2, *b);
  check(b->demand().particles_evaluated == 2 &&
            b->demand().centroids_flagged == 1 &&
            b->demand().centroids_recomputed == 1 &&
            b->demand().centroid_reduction_edges == 3 &&
            b->group(kCenX, 0) == 0 && b->group(kCenM, 0) == 3,
        "multiple qualifying interactions recompute once, including sleeping "
        "member, even if motion cancels");
}
void collective_effects(engine::Runtime &runtime) {
  constexpr float centre = 1048576;
  const auto p = parameters(0);
  const Input tiny{{100, 102}, {1e-30f, 1e-30f}, {}, 1, {{0, 0}, {1, 0}}, {}};
  auto small = make(runtime, tiny, p);
  small->download();
  check(small->group(kCenX, 0) == 101,
        "ordinary centroid preserves finite small positive mass without an "
        "epsilon denominator");
  const Input combined{{centre, centre, centre, centre}, {1, 1, 1, 1},
                       {0, 0.125f, 0.125f, 0.125f},      1,
                       {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, {}};
  auto s = make(runtime, combined, p),
       r = make(runtime, combined, p, TickMode::AllLoaded);
  s->tick(p);
  r->tick(p);
  compare(*s, *r);
  row("collective-centroid", 1, *s);
  check(s->group(kCenX, 0) == centre + 0.125f &&
            s->demand().centroids_flagged == 1,
        "collective sub-ULP member effects combine before centroid gate");
  auto delayed = combined;
  delayed.velocity = {0, 0.125f, 0, 0};
  auto d = make(runtime, delayed, p),
       dr = make(runtime, delayed, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 3; ++tick) {
    d->tick(p);
    dr->tick(p);
    compare(*d, *dr);
    row("accumulated-centroid", tick, *d);
    if (tick < 3)
      check(
          d->demand().centroids_flagged == 0,
          "sub-ULP centroid contribution retained without premature placement");
  }
  check(d->group(kCenX, 0) == centre + 0.125f &&
            d->demand().centroids_recomputed == 1,
        "retained cross-tick centroid contributions eventually trigger "
        "placement");

  auto seeded = delayed;
  seeded.x[1] += 0.25f;
  seeded.velocity = {0, 0, 0.125f, 0};
  auto seeded_s = make(runtime, seeded, p),
       seeded_r = make(runtime, seeded, p, TickMode::AllLoaded);
  seeded_s->tick(p);
  seeded_r->tick(p);
  compare(*seeded_s, *seeded_r);
  check(seeded_s->group(kCenX, 0) == centre + 0.125f &&
            seeded_s->demand().centroids_flagged == 1,
        "seeded centroid rounding residue combines with later small motion");

  const auto attract = parameters(0.001f, 0.1f);
  const Input two_fields{{centre, centre + 64, centre + 64},
                         {1, 16777216, 16777216},
                         {},
                         2,
                         {{0, 0}, {1, 0}, {0, 1}, {2, 1}},
                         {}};
  auto two = make(runtime, two_fields, attract),
       tr = make(runtime, two_fields, attract, TickMode::AllLoaded);
  auto one_field = two_fields;
  one_field.groups = 1;
  one_field.edges.resize(2);
  auto one = make(runtime, one_field, attract);
  one->tick(attract);
  one->download();
  two->tick(attract);
  tr->tick(attract);
  compare(*two, *tr);
  check(one->particle(kPosX, 0) == centre && two->particle(kPosX, 0) > centre,
        "individually insufficient field contributions combine before particle "
        "positional gate");
}
void carried_motion(engine::Runtime &runtime) {
  constexpr float centre = 1048576;
  const auto p = parameters(1, 0.1f);
  const Input input{
      {centre, centre + 4096}, {1, 16777216}, {}, 1, {{0, 0}, {1, 0}}, {}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 7; ++tick) {
    s->tick(p);
    r->tick(p);
    compare(*s, *r);
    row("carried-motion", tick, *s);
    if (tick == 1)
      check(s->particle(kPosX, 0) == centre && s->particle(kVelX, 0) > 0 &&
                s->demand().particles_moved == 0,
            "velocity changes are retained when current position does not "
            "change");
  }
  check(s->particle(kPosX, 0) > centre && s->demand().particles_woken == 1,
        "carried acceleration accumulates to a later positional wake");
  auto stopped = parameters(0, 0);
  Input moving{{0}, {1}, {1}, 0, {}, {}};
  auto m = make(runtime, moving, stopped);
  m->tick(stopped);
  m->tick(stopped);
  check(m->demand().particles_skipped == 1,
        "fixed full state can sleep with nonzero stored speed at dt zero");
  stopped.dt = 0.5f;
  m->tick(stopped);
  m->download();
  check(m->particle(kPosX, 0) == 0.5f && m->particle(kVelX, 0) == 1,
        "dt invalidation preserves and resumes previously carried motion");
}
void focal_frame(engine::Runtime &runtime) {
  auto p = parameters(0, 0.1f);
  p.reach_strength = 1;
  const Input a{{100, 101}, {1, 1}, {0, 0.1f}, 1, {{0, 0}}, {}};
  auto s = make(runtime, a, p), r = make(runtime, a, p, TickMode::AllLoaded);
  s->set_focus({0});
  r->set_focus({0});
  rejects([&] { s->set_focus({0, 0}); }, "duplicate focal fields rejected");
  rejects([&] { s->set_focus({1}); }, "unbound focal field rejected");
  auto shifted = a;
  for (auto &x : shifted.x) x += 900;
  auto t = make(runtime, shifted, p);
  t->set_focus({0});
  for (int tick = 1; tick <= 4; ++tick) {
    s->tick(p);
    r->tick(p);
    t->tick(p);
    compare(*s, *r);
    t->download();
    check(std::abs((s->particle(kPosX, 1) - 100) -
                   (t->particle(kPosX, 1) - 1000)) <= 1e-4f,
          "translated selected-focus calculation preserves relative motion "
          "within 1e-4");
  }
  row("focal-origin", 4, *s);
  check(s->demand().focal_origin[0] == 100 && s->particle(kVelX, 1) == 0.1f,
        "selected focal centroid changes actual origin-dependent radial "
        "integration");
  Input multiple{{100, 104}, {2, 1}, {}, 2, {{0, 0}, {1, 1}}, {}};
  auto many = make(runtime, multiple, p);
  many->set_focus({0, 1});
  many->tick(p);
  check(std::abs(many->demand().focal_origin[0] - 304.0f / 3) <= 1e-5f,
        "multiple focal fields form the ordinary mass-weighted centroid");
  auto unused = a;
  unused.groups = 2;
  auto empty = make(runtime, unused, p);
  empty->tick(p);
  check(empty->demand().fields_loaded == 2 &&
            empty->demand().fields_exposed == 1 &&
            empty->demand().edges_exposed == 1,
        "demand distinguishes loaded field capacity from exposed fields");
  rejects([&] { empty->set_focus({1}); },
          "mass-zero unused group cannot establish mathematical focus");
  s->set_focus({});
  s->tick(p);
  s->download();
  check(s->demand().focal_origin[0] == 0 && s->particle(kVelX, 1) == 0,
        "explicitly clearing focus restores legacy origin and invalidates "
        "radial calculation");
}
void ripple(engine::Runtime &runtime) {
  const auto p = parameters();
  const Input input{{0, 0, 0, 0, 0, 0},
                    {1, 1, 1, 1, 16777216, 1},
                    {1, 0, 0, 0, 0, 0},
                    5,
                    {{0, 0},
                     {1, 0},
                     {1, 1},
                     {2, 1},
                     {2, 2},
                     {3, 2},
                     {3, 3, 1.0f / 16777216},
                     {4, 3},
                     {4, 4},
                     {5, 4}},
                    {}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 5; ++tick) {
    s->tick(p);
    r->tick(p);
    compare(*s, *r, 1e-4f);
    row("bounded-ripple", tick, *s);
    row("bounded-ripple-all-loaded", tick, *r);
    for (int i = tick; i < 4; ++i)
      check(s->particle(kPosX, i) == 0,
            "wake ripple does not jump unpublished field dependencies");
    if (tick <= 4)
      check(s->particle(kPosX, tick - 1) != 0,
            "qualifying disturbance reaches next linked particle");
  }
  check(s->particle(kPosX, 4) == 0 && s->particle(kPosX, 5) == 0,
        "ripple stops at unaffected massive participant and leaves its other "
        "field dormant");
}
void contact(engine::Runtime &runtime) {
  const auto p = parameters(0, 0.1f);
  const Input input{{-0.1f, 0.1f}, {1, 1}, {}, 0, {}, {{0, 1}}};
  auto s = make(runtime, input, p),
       r = make(runtime, input, p, TickMode::AllLoaded);
  for (int tick = 1; tick <= 4; ++tick) {
    s->tick(p);
    r->tick(p);
    compare(*s, *r);
  }
  check(s->demand().contact_evaluations == 2 && s->particle(kPosX, 0) < -0.1f &&
            s->particle(kPosX, 1) > 0.1f,
        "body-contact dependencies preserve equal/opposite whole-particle "
        "motion");
}
}  // namespace
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 1;
    for (int i = 1; i < argc; ++i) {
      const std::string option = argv[i];
      if (option == "--cuda")
        settings.cuda = true;
      else if (option == "--parallel")
        settings.cpu_threads = 2;
      else
        throw std::invalid_argument("expected --cuda or --parallel");
    }
    engine::Runtime runtime(settings);
    std::cout << "DEMAND,scenario,tick,P,N,G,exposed_G,E,exposed_E,evaluated,"
                 "skipped,moved,woken,"
                 "force_requests,force_evaluations,zero_response,force_skipped,"
                 "gate_checks,gate_rejected,geometry_rejected,touch_visits,"
                 "force_mask_visits,centroid_gate_visits,centroid_mask_visits,"
                 "flagged,recomputed,"
                 "touched_edges,reduced_edges,tick_ms\n";
    settled_and_invalidation(runtime);
    ineffective_touch(runtime);
    all_fields_and_sleeping_members(runtime);
    collective_effects(runtime);
    carried_motion(runtime);
    focal_frame(runtime);
    ripple(runtime);
    contact(runtime);
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " activity checks\n";
  return harness::report("activity_test");
}
