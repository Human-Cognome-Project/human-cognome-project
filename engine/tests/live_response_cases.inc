// Test-owned snapshots intentionally copy borrowed reports across operations.
LiveResponseInspection response_snapshot(Fixture &f) {
  const auto before = state_hash(f);
  const LiveResponseInspection *view = nullptr;
  no_new([&] { view = &f.h.inspect_live_responses(UINT64_MAX); });
  require(view->status == CascadeStatus::Ok, "response snapshot unavailable");
  auto result = *view;
  require(state_hash(f) == before, "response observation changed owner state");
  return result;
}
void response_identity_equal(const LiveResponseInspection &a,
                             const LiveResponseInspection &b) {
  require(a.responses == b.responses && a.capacity == b.capacity,
          "response shape changed unexpectedly");
  for (std::size_t row = 0; row < a.responses; ++row) {
    for (int col = 0; col < 4; ++col)
      require(a.identities[col * a.capacity + row] == b.identities[col * b.capacity + row],
              "response identity changed unexpectedly");
    for (int col = 0; col < 2; ++col)
      require(a.fractions[col * a.capacity + row] == b.fractions[col * b.capacity + row],
              "response exact ratio changed unexpectedly");
  }
}
void response_validity(engine::Runtime &r, HostPath path) {
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  f.install();
  const auto clean = response_snapshot(f);
  const int body = f.in.particle_rows[0].record;
  int controls = 0;
  const auto check = [&](auto affected, bool invalid) {
    const auto observed = response_snapshot(f);
    response_identity_equal(clean, observed);
    require(observed.generation == clean.generation &&
                observed.completed_ticks == clean.completed_ticks &&
                observed.frame_anchor == clean.frame_anchor && f.h.ready(),
            "diagnostic validity changed lifecycle state");
    for (std::size_t row = 0; row < observed.responses; ++row)
      require(observed.values[9 * observed.capacity + row] ==
                  (invalid && affected(observed, row) ? 0 : 1),
              "response extraction validity mismatch");
    ++controls;
  };
  const auto recipient = [&](const auto &v, std::size_t row) {
    return v.identities[row] == body;
  };
  for (int col : {kPosX, kPosY, kPosZ, kMass}) {
    const double original = f.h.particle64(col, body);
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::denorm_min()}) {
      PreparedFixtureAccess::inject_value64(f.h, col, body, bad);
      check(recipient, true);
    }
    PreparedFixtureAccess::inject_value64(f.h, col, body, original);
    check(recipient, false);
  }
  // Finite extraction is not positive-mass or downstream formula validity.
  const double mass = f.h.particle64(kMass, body);
  for (double finite : {0., -1.}) {
    PreparedFixtureAccess::inject_value64(f.h, kMass, body, finite);
    check(recipient, false);
  }
  PreparedFixtureAccess::inject_value64(f.h, kMass, body, mass);
  const auto tree = f.h.inspect_live_hierarchy();
  auto *packets = LiveHierarchyAccess::input_array(f.h, 0);
  const auto original = tree.slots;
  const auto H = tree.selectors.size();
  const int node = tree.order[0];
  const auto source = [&](const auto &v, std::size_t row) {
    return v.identities[v.capacity + row] == node ||
           v.identities[2 * v.capacity + row] == node;
  };
  for (int col = 0; col < 4; ++col) {
    for (bool selected_slot : {false, true}) {
      auto changed = original;
      const int slot = tree.selectors[node] ^ (selected_slot ? 0 : 1);
      changed[(slot * 4 + col) * H + node] = std::numeric_limits<double>::quiet_NaN();
      engine::upload(*packets, changed.data(), changed.size() * sizeof(double));
      r.synchronize();
      check(source, selected_slot);
    }
  }
  engine::upload(*packets, original.data(), original.size() * sizeof(double));
  r.synchronize();
  // A valid exact coincidence must remain extractable, not be silently omitted.
  PreparedFixtureAccess::inject_value64(f.h, kPosX, body, selected(tree, node)[0]);
  check(recipient, false);
  const auto coincident = response_snapshot(f);
  bool found = false;
  for (std::size_t row = 0; row < coincident.responses; ++row)
    if (recipient(coincident, row) && coincident.identities[3 * coincident.capacity + row] == 0) {
      require(coincident.values[row] == 0, "coincident local displacement not zero");
      found = true;
    }
  require(found, "coincident source was omitted");
  std::cout << "response_validity,controls=" << controls
            << ",selected_packet_only=1,coincidence_retained=1 PASS\n";
}

// A same-owner test consumer must not use a copied snapshot after an operation.
// This is not a production lease or a cross-owner identity check.
bool response_current(Fixture &f, const LiveResponseInspection &v) {
  return v.status == CascadeStatus::Ok &&
      v.generation == f.h.prepared_storage().generation &&
      v.completed_ticks == f.h.prepared_storage().completed_ticks &&
      v.frame_anchor == f.h.frame_anchor();
}
void response_range(engine::Runtime &r, HostPath path) {
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  f.install();
  const auto baseline = response_snapshot(f);
  const auto tree = f.h.inspect_live_hierarchy();
  auto *packets = LiveHierarchyAccess::input_array(f.h, 0);
  const auto H = tree.selectors.size();
  const int child = tree.order[0], parent = tree.order[2];
  const int body = f.in.particle_rows[0].record;
  const double original_x = f.h.particle64(kPosX, body);
  int controls = 0;
  const auto run = [&](bool inherited, double left, double right, bool valid) {
    auto values = tree.slots;
    const auto set = [&](int node, double x) {
      values[(4 * tree.selectors[node]) * H + node] = x;
    };
    if (inherited) { set(parent, left); set(child, right); }
    else set(child, left);
    engine::upload(*packets, values.data(), values.size() * sizeof(double));
    PreparedFixtureAccess::inject_value64(f.h, kPosX, body, inherited ? original_x : right);
    r.synchronize();
    const auto observed = response_snapshot(f);
    response_identity_equal(baseline, observed);
    require(response_current(f, observed), "current range snapshot rejected");
    bool found = false;
    for (std::size_t row = 0; row < observed.responses; ++row)
      if (observed.identities[row] == body &&
          observed.identities[observed.capacity + row] == child &&
          observed.identities[3 * observed.capacity + row] == int(inherited)) {
        require(observed.values[9 * observed.capacity + row] == (valid ? 1 : 0),
                "range/subtraction validity mismatch");
        if (valid)
          require(observed.values[row] == left - right, "supported subtraction changed");
        found = true;
      }
    require(found, "range control lost its response row");
    ++controls;
  };
  for (bool inherited : {false, true}) {
    const double max = std::numeric_limits<double>::max();
    const double min = std::numeric_limits<double>::min();
    run(inherited, max, -max, false);
    run(inherited, -max, max, false);
    run(inherited, min, std::nextafter(min, 1.), false);
    run(inherited, std::nextafter(min, 1.), min, false);
    run(inherited, min, min, true);
    run(inherited, min, 0., true);
    run(inherited, max, 0., true);
    run(inherited, -0., 0., true);
  }
  engine::upload(*packets, tree.slots.data(), tree.slots.size() * sizeof(double));
  PreparedFixtureAccess::inject_value64(f.h, kPosX, body, original_x);
  r.synchronize();
  const auto restored = response_snapshot(f);
  response_identity_equal(baseline, restored);
  require(restored.values == baseline.values, "range fixture failed to restore source state");
  std::cout << "response_range,controls=" << controls << ",local_and_inherited=1 PASS\n";
}
// Same-owner test consumer only, not a production lease/freshness API.
bool intent_current(Fixture &f, const LiveIntentInspection &v, double gravity) {
  return f.h.ready() && v.status == CascadeStatus::Ok && v.gravity == gravity &&
      v.generation == f.h.prepared_storage().generation &&
      v.completed_ticks == f.h.prepared_storage().completed_ticks &&
      v.frame_anchor == f.h.frame_anchor();
}
LiveIntentInspection intent_snapshot(Fixture &f, double gravity, std::uint64_t work = UINT64_MAX) {
  const auto before = state_hash(f);
  const LiveIntentInspection *view = nullptr;
  no_new([&] { view = &f.h.inspect_live_intent(gravity, work); });
  const auto v = *view;
  require(v.status == CascadeStatus::Ok && v.work_used == v.work_required &&
              v.work_required == 2 + 2*v.responses + 14*v.capacity &&
              v.readback_bytes == 13*v.capacity*sizeof(double) &&
              v.generation == f.h.prepared_storage().generation &&
              v.completed_ticks == f.h.prepared_storage().completed_ticks &&
              v.frame_anchor == f.h.frame_anchor() &&
              (v.gravity == gravity || (std::isnan(v.gravity) && std::isnan(gravity))),
          "intent metadata/accounting mismatch");
  require(state_hash(f) == before, "intent inspection changed model state");
  for (double x : v.values) require(std::isfinite(x), "nonfinite candidate escaped masking");
  return v;
}
// Independent host arithmetic over extracted current sources; handles valid
// coincidence/absence as well as supported rows, but is not an invalid-range oracle.
LiveIntentInspection intent_reference(Fixture &f, double gravity = .5) {
  const auto source = response_snapshot(f);
  const auto v = intent_snapshot(f, gravity);
  for (std::size_t body = 0; body < v.capacity; ++body) {
    long double sum = 0;
    int coincident = 0;
    std::array<long double, 3> force{}, position{};
    for (std::size_t row = 0; row < source.responses; ++row) {
      if (source.identities[row] != int(body)) continue;
      const auto R = source.capacity;
      require(source.values[9*R+row] == 1, "reference received invalid extraction");
      for (int k = 0; k < 3; ++k) position[k] = source.values[(3+k)*R+row];
      if (!source.values[8*R+row] || gravity == 0) continue;
      long double d2 = 0;
      for (int k = 0; k < 3; ++k) {
        const long double d = source.values[k*R+row]; d2 += d*d;
      }
      if (d2 == 0) { ++coincident; continue; }
      const long double coupling = gravity * static_cast<long double>(source.values[6*R+row]) *
          source.fractions[row] / source.fractions[R+row] * source.values[7*R+row];
      const auto alpha = coupling / (d2 * std::sqrt(d2));
      sum += alpha;
      for (int k = 0; k < 3; ++k) force[k] += alpha * source.values[k*R+row];
    }
    const int status = coincident ? 2 : (sum > 0 ? 1 : 0);
    near(v.values[body], status, "intent status/reference mismatch");
    near(v.values[v.capacity+body], 1, "supported-domain field unavailable");
    near(v.values[2*v.capacity+body], double(sum), "coefficient sum mismatch");
    near(v.values[3*v.capacity+body], coincident, "coincidence count mismatch");
    for (int k = 0; k < 3; ++k) {
      near(v.values[(4+k)*v.capacity+body], double(force[k]), "candidate force mismatch");
      near(v.values[(7+k)*v.capacity+body], status == 1 ? double(force[k]/sum) : 0,
           "relative candidate mismatch");
      near(v.values[(10+k)*v.capacity+body], status == 1 ? double(position[k]+force[k]/sum) : 0,
           "operative candidate mismatch");
    }
  }
  return v;
}
// Observe the unchanged production law. Candidate motion is not a convergence
// gate; endpoint references use whole-body inertia and the existing brake/Q.
void intent_trajectories(engine::Runtime &r, HostPath path) {
  constexpr int ticks = 64;
  const auto precision = std::cout.precision();
  std::cout << std::setprecision(17);
  for (int bits : {8, 24, 52}) for (double dt : {.125, .5}) {
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {2, 2, 1}, {1, 2, 3}, {1, 3, 7}, bits, bits);
    f.install();
    f.h.inspect_prepared();
    for (int i = 0; i < 3; ++i) {
      const auto b = f.in.particle_rows[i].record;
      f.h.particle64(kVelX, b) = i == 2 ? -.02 : .02;
      f.h.particle64(kVelY, b) = i == 1 ? -.03 : .03;
    }
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      f.control->set(3);
    });
    Parameters64 p;
    p.dt = dt; p.gravity = .1;
    p.contact_stiffness = p.reach_strength = 0;
    std::size_t moving_candidates = 0, moved_bodies = 0, unchanged_active = 0;
    auto candidate = intent_reference(f, p.gravity);
    for (int tick = 0; tick < ticks; ++tick) {
      f.h.inspect_prepared();
      const auto old = f.h.particles64();
      const auto expected = physical_reference(f, p, tick == 0);
      no_new([&] { f.h.tick64(p); f.h.inspect_prepared(); });
      verify_physical(f, expected, old, p);
      require(!intent_current(f, candidate, p.gravity), "tick retained stale candidate");
      const auto next = intent_reference(f, p.gravity);
      require(next.completed_ticks == candidate.completed_ticks + 1 &&
                  next.frame_anchor == candidate.frame_anchor,
              "trajectory tick/frame mismatch");
      const auto P = candidate.capacity;
      for (int i = 0; i < 3; ++i) {
        const int b = f.in.particle_rows[i].record;
        const bool active = f.reference_active[b];
        Vector force{}, x{}, carried{}, origin{};
        for (int k = 0; k < 3; ++k) {
          force[k] = f.h.particle64(kForceX+k, b);
          x[k] = old[(kPosX+k)*P+b];
          carried[k] = old[(kVelX+k)*P+b];
          origin[k] = f.h.demand().focal_origin[k];
        }
        const auto step = velocity_reference(force, x, carried, origin,
            old[kMass*P+b], f.h.universal64(), p);
        bool moved = false, intent_moved = false;
        const bool comparable = candidate.values[b] == 1 && next.values[b] == 1;
        for (int k = 0; k < 3; ++k) {
          const auto accepted = f.h.particle64(kPosX+k, b);
          const auto pre_brake = active ? x[k] +
              (carried[k] + force[k]*dt/old[kMass*P+b])*dt : x[k];
          const auto pre_q = active ? x[k] + step.value[k]*dt : x[k];
          moved |= accepted != double(x[k]);
          intent_moved |= comparable &&
              next.values[(10+k)*P+b] != candidate.values[(10+k)*P+b];
          std::cout << "intent_trace,bits=" << bits << ",dt=" << dt
                    << ",tick=" << tick+1 << ",body=" << b << ",axis=" << k
                    << ",active=" << active << ",status_before=" << candidate.values[b]
                    << ",status_after=" << next.values[b] << ",comparable=" << comparable
                    << ",x=" << double(x[k]) << ",carried=" << double(carried[k])
                    << ",force=" << double(force[k]) << ",pre_brake_reference=" << double(pre_brake)
                    << ",pre_q_reference=" << double(pre_q) << ",accepted=" << accepted
                    << ",displacement=" << accepted-double(x[k])
                    << ",retained=" << f.h.particle64(kVelX+k, b)
                    << ",publication_delta=" << double(accepted-pre_q)
                    << ",intent_before=" << candidate.values[(10+k)*P+b]
                    << ",intent_after=" << next.values[(10+k)*P+b] << '\n';
        }
        moved_bodies += moved;
        moving_candidates += intent_moved;
        unchanged_active += active && !moved;
      }
      candidate = next;
    }
    // Coverage witnesses, not requirements to converge or to keep moving forever.
    require(moved_bodies && moving_candidates, "moving-intent trajectory not exercised");
    std::cout << "intent_trajectory_summary,bits=" << bits << ",dt=" << dt
              << ",ticks=" << ticks << ",moving_candidates=" << moving_candidates
              << ",moved_bodies=" << moved_bodies << ",unchanged_active=" << unchanged_active
              << ",contacts=0,radial_damping=0 PASS\n";
  }
  std::cout.precision(precision);
}

void intent_observation(engine::Runtime &r, HostPath path) {
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5}, 52, 52, false, true);
  bool rejected = false;
  try { f.h.inspect_live_intent(1, UINT64_MAX); }
  catch (const std::logic_error &) { rejected = true; }
  require(rejected && f.h.ready(), "preinstall intent query changed readiness");
  f.install();
  const auto hash = state_hash(f);
  const auto &short_result = f.h.inspect_live_intent(1, 0);
  const auto need = short_result.work_required;
  require(short_result.status == CascadeStatus::CapacityExhausted &&
              !short_result.work_used && !short_result.readback_bytes,
          "short intent budget performed work");
  no_new([&] {
    const auto &v = f.h.inspect_live_intent(1, need-1);
    require(v.status == CascadeStatus::CapacityExhausted &&
                v.failure.required == need && !v.work_used && !v.readback_bytes,
            "one-short intent budget accepted");
  });
  require(state_hash(f) == hash, "short intent query changed model state");
  intent_snapshot(f, .5, need);
  intent_reference(f, .5); intent_reference(f, 0);
  Parameters64 p; p.dt = .01; p.gravity = .1; p.contact_stiffness = .3;
  f.h.tick64(p);
  intent_reference(f, .5);
  const int body = f.in.particle_rows[0].record;
  for (double invalid : {-1., std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::quiet_NaN()}) {
    const auto v = intent_snapshot(f, invalid);
    const auto &headers = f.h.prepared_storage().particle_headers;
    for (std::size_t row = 0; row < v.capacity; ++row)
      require(v.values[row] == (headers[row].record >= 0 ? 3 : 0),
              "invalid gravity/live-row candidate mismatch");
  }
  const auto tree = f.h.inspect_live_hierarchy();
  PreparedFixtureAccess::inject_value64(f.h, kPosX, body, selected(tree, tree.order[0])[0]);
  auto coincidence = intent_snapshot(f, .5);
  require(coincidence.values[body] == 2 && coincidence.values[coincidence.capacity+body] == 1 &&
              coincidence.values[3*coincidence.capacity+body] == 1,
          "coincidence status or gated-force availability lost");
  for (int col = 7; col < 13; ++col)
    require(coincidence.values[col*coincidence.capacity+body] == 0, "unresolved candidate not masked");
  PreparedFixtureAccess::inject_value64(f.h, kMass, body, -1);
  const auto unsupported = intent_snapshot(f, .5);
  require(unsupported.values[body] == 3 && unsupported.values[unsupported.capacity+body] == 0,
          "unsupported input did not override coincidence");
  std::cout << "live_intent,work=" << need << ",readback=" << unsupported.readback_bytes
            << ",reference=long_double,post_tick=1,precedence=1 PASS\n";
}
