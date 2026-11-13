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
