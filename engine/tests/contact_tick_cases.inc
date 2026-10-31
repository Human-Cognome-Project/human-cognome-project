// Contact-safe candidate execution uses production numerical builders. Fixture
// starts below remain private test inputs, never an admission/placement API.
BudgetDemand tick_limits() {
  auto l = limits();
  l[Budget::TickWork] = 1000000000;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
                                l, false, NumericalProfile::trial()) +
                            256 * 1024 * 1024;
  return l;
}
void contact_tick_smoke(engine::Runtime &r, HostPath path) {
  const auto l = tick_limits();
  Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false, false,
            false, 1024, {}, &l);
  f.install();
  Parameters64 p;
  p.gravity = p.dt = p.contact_stiffness = p.reach_strength = 0;
  f.control->set(2, f.in.particle_rows[0].record);
  r.synchronize();
  const auto query =
      f.h.query_prepared_geometry(f.h.prepared_storage().generation);
  require(query.status == GeometryStatus::Ok, "initial tick geometry");
  const auto initial = state_hash(f);
  TickResult rejected;
  no_new([&] { rejected = f.h.tick_prepared(query.generation, p, 0); });
  require(rejected.status == TickStatus::CapacityExhausted &&
              rejected.failure.budget == Budget::TickWork &&
              state_hash(f) == initial && f.h.ready(),
          "tick-work rejection changed committed state/report");
  f.h.inspect_prepared_geometry(query);
  bool refused = false;
  try {
    f.h.tick64(p);
  } catch (const std::logic_error &) {
    refused = true;
  }
  require(refused && state_hash(f) == initial,
          "direct tick bypass was accepted");
  TickResult result;
  no_new([&] { result = f.h.tick_prepared(query.generation, p); });
  std::cout << "contact_tick_smoke,status=" << int(result.status)
            << ",tick_work=" << result.required_work
            << ",geometry_work=" << result.geometry_work
            << ",contacts=" << result.candidate.contacts << '\n';
  require(result.status == TickStatus::Ok && result.completed_ticks == 1 &&
              !result.coverage_changed && result.generation == query.generation,
          "zero-dt candidate tick failed");
  f.h.inspect_prepared();
  require(f.h.particle64(kPosX, f.in.particle_rows[0].record) == 0 &&
              f.h.demand().particles_evaluated == 1,
          "zero-dt candidate tick changed physics/native selection");
  std::cout << "contact_tick_smoke,work_rejection=unchanged,zero_dt=1,bypass="
               "refused,cpp_new_observed=0 PASS\n";
}

std::vector<unsigned char> tick_array_bytes(taichi::lang::Ndarray &a) {
  std::vector<unsigned char> out(a.get_nelement() * a.get_element_size());
  engine::readback(a, out.data(), out.size());
  return out;
}
struct TickSnapshot {
  std::uint64_t state;
  std::vector<unsigned char> controls;
  std::vector<std::vector<unsigned char>> installed, query;
  explicit TickSnapshot(Fixture &f)
      : state(state_hash(f)), controls(TickExecutionAccess::controls(f.h)) {
    for (int id : {2, 3, 6, 8, 10, 11})
      installed.push_back(
          tick_array_bytes(TickExecutionAccess::topology(f.h, id)));
    for (auto *a : TickExecutionAccess::source_indexes(f.h))
      query.push_back(tick_array_bytes(*a));
    for (auto *a : TickExecutionAccess::public_geometry(f.h))
      query.push_back(tick_array_bytes(*a));
  }
  bool operator==(const TickSnapshot &other) const {
    return state == other.state && controls == other.controls &&
           installed == other.installed && query == other.query;
  }
};
void same_tick_execution(Fixture &a, Fixture &b) {
  a.h.inspect_prepared();
  b.h.inspect_prepared();
  require(a.h.particles64() == b.h.particles64() &&
              a.h.groups64() == b.h.groups64() &&
              a.h.determined64() == b.h.determined64() &&
              a.h.universal64() == b.h.universal64(),
          "retry differs from independent no-attempt numerical execution");
  const auto &sa = a.h.prepared_storage(), &sb = b.h.prepared_storage();
  require(sa.motion64() == sb.motion64() &&
              sa.inspection_state == sb.inspection_state &&
              sa.inspection_flags == sb.inspection_flags &&
              sa.inspection_focus64 == sb.inspection_focus64 &&
              sa.generation == sb.generation &&
              sa.completed_ticks == sb.completed_ticks,
          "retry differs from independent native/control execution");
  const auto ai = TickExecutionAccess::source_indexes(a.h),
             bi = TickExecutionAccess::source_indexes(b.h);
  for (std::size_t i = 0; i < ai.size(); ++i)
    require(tick_array_bytes(*ai[i]) == tick_array_bytes(*bi[i]),
            "retry differs from independent source/counter execution");
  const auto &ha = a.h.inspect_live_hierarchy(),
             &hb = b.h.inspect_live_hierarchy();
  require(ha.slots == hb.slots && ha.previous == hb.previous &&
              ha.selectors == hb.selectors && ha.visits == hb.visits &&
              ha.counters == hb.counters,
          "retry differs from independent packet/history execution");
  for (int id : {2, 3, 6, 8, 10, 11})
    require(tick_array_bytes(TickExecutionAccess::topology(a.h, id)) ==
                tick_array_bytes(TickExecutionAccess::topology(b.h, id)),
            "retry leaked scratch into committed contact/CSR rows");
  require(
      a.h.demand().particles_evaluated == b.h.demand().particles_evaluated &&
          a.h.demand().force_requests == b.h.demand().force_requests &&
          a.h.demand().contact_evaluations ==
              b.h.demand().contact_evaluations &&
          a.h.demand().focal_origin == b.h.demand().focal_origin,
      "retry differs from independent successful report");
}
void contact_tick_overflow(engine::Runtime &r,
                           HostPath path,
                           bool blocks,
                           bool reverse = false) {
  auto l = tick_limits();
  l[Budget::Contacts] = 1;
  const int n = blocks ? 70 : 3, peer = blocks ? 32 : 1,
            mover = reverse  ? 0
                    : blocks ? 64
                             : 2,
            first = reverse ? 64 : 0;
  std::vector<std::uint64_t> ones(n, 1);
  std::vector<double> positions(n), radii(n, .125), masses(n, 1);
  for (int i = 0; i < n; ++i)
    positions[i] = i * 100;
  positions[first] = 0;
  positions[peer] = 5;
  positions[mover] = 10;
  radii[first] = radii[peer] = radii[mover] = 2;
  Fixture a(r, path, {{}}, {ones}, ones, masses, positions, 52, 52, false,
            false, false, false, 1024, radii, &l);
  Fixture b(r, path, {{}}, {ones}, ones, masses, positions, 52, 52, false,
            false, false, false, 1024, radii, &l);
  a.install();
  b.install();
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  for (auto *f : {&a, &b}) {
    const auto initial =
        f->h.tick_prepared(f->h.prepared_storage().generation, p);
    require(initial.status == TickStatus::Ok && !initial.candidate.contacts,
            "moving overflow initial coverage");
    f->control->set(0);
    f->control->set(2, f->in.particle_rows[mover].record);
    f->h.inspect_prepared();
    f->h.particle64(kVelX, f->in.particle_rows[mover].record) = -8;
    PreparedFixtureAccess::upload_body_fixture64(f->h);
  }
  const auto query =
      a.h.query_prepared_geometry(a.h.prepared_storage().generation);
  require(query.status == GeometryStatus::Ok && !query.contacts,
          "moving query precondition");
  a.h.inspect_prepared_geometry(query);
  const TickSnapshot before(a);
  TickResult failed;
  const auto failure_times = a.h.timings();
  no_new([&] { failed = a.h.tick_prepared(query.generation, p); });
  require(std::memcmp(&failure_times, &a.h.timings(), sizeof(PhaseTimes)) == 0,
          "failed moving tick changed published timings");
  const TickSnapshot observed(a);
  std::cerr << "contact_rejection_snapshot,body_count=" << n
            << ",status=" << int(failed.status)
            << ",failure=" << budget_name(failed.failure.budget)
            << ",required=" << failed.failure.required
            << ",capacity=" << failed.failure.capacity
            << ",current=" << failed.current.contacts
            << ",candidate=" << failed.candidate.contacts
            << ",geometry_status=" << int(failed.candidate.status)
            << ",snapshot=" << (before == observed)
            << ",state=" << (before.state == observed.state)
            << ",controls=" << (before.controls == observed.controls)
            << ",topology=" << (before.installed == observed.installed)
            << ",query=" << (before.query == observed.query) << '\n';
  require(failed.status == TickStatus::CapacityExhausted &&
              failed.failure.budget == Budget::Contacts &&
              failed.failure.required == 2 && failed.failure.capacity == 1 &&
              failed.current.contacts == 0 && failed.candidate.contacts == 2 &&
              before == TickSnapshot(a) && a.h.ready(),
          "moving contact overflow changed committed state/report/query");
  a.h.inspect_prepared_geometry(query);
  p.dt = .25;
  TickResult work;
  const auto work_times = a.h.timings();
  const auto geometry_need = 2 * std::uint64_t(n) * (n - 1) / 2 + 1;
  no_new([&] {
    work =
        a.h.tick_prepared(query.generation, p, UINT64_MAX, geometry_need - 1);
  });
  require(std::memcmp(&work_times, &a.h.timings(), sizeof(PhaseTimes)) == 0,
          "failed late-work tick changed published timings");
  require(work.status == TickStatus::CapacityExhausted &&
              work.failure.budget == Budget::GeometryWork &&
              work.failure.required == geometry_need &&
              work.candidate.contacts == 1 && before == TickSnapshot(a),
          "late discovery/order work rejection changed committed state");
  TickResult accepted, independent;
  no_new([&] { accepted = a.h.tick_prepared(query.generation, p); });
  no_new([&] {
    independent = b.h.tick_prepared(b.h.prepared_storage().generation, p);
  });
  require(accepted.status == TickStatus::Ok &&
              independent.status == TickStatus::Ok &&
              accepted.candidate.contacts == 1 && accepted.coverage_changed &&
              accepted.generation == query.generation + 1,
          "valid moving retry failed");
  same_tick_execution(a, b);
  bool stale = false;
  try {
    a.h.inspect_prepared_geometry(query);
  } catch (const std::logic_error &) {
    stale = true;
  }
  require(stale, "successful moving publication retained old query token");
  // Both contact CSR directions must feed the production force pass. A
  // zero-duration visit observes force while preserving incoming velocity.
  a.h.inspect_prepared();
  p.dt = 0;
  p.contact_stiffness = 1;
  const std::vector<std::array<int, 2>> pair{
      {{std::min(a.in.particle_rows[peer].record,
                 a.in.particle_rows[mover].record),
        std::max(a.in.particle_rows[peer].record,
                 a.in.particle_rows[mover].record)}}};
  const auto force_old = a.h.particles64();
  const auto expected_force = physical_reference(a, p, true, &pair);
  TickResult forced;
  no_new([&] {
    forced = a.h.tick_prepared(a.h.prepared_storage().generation, p);
  });
  require(forced.status == TickStatus::Ok && !forced.coverage_changed &&
              a.h.demand().contact_evaluations == 2,
          "installed contact union did not feed both endpoint force paths");
  a.h.inspect_prepared();
  verify_physical(a, expected_force, force_old, p);
  a.h.particle64(kVelX, a.in.particle_rows[mover].record) = 8;
  PreparedFixtureAccess::upload_body_fixture64(a.h);
  p.dt = .5;
  p.contact_stiffness = 0;
  TickResult exit;
  no_new(
      [&] { exit = a.h.tick_prepared(a.h.prepared_storage().generation, p); });
  require(exit.status == TickStatus::Ok && exit.coverage_changed &&
              exit.current.contacts == 1 && !exit.candidate.contacts,
          "derived contact exit was not published coherently");
  std::cout << "contact_tick_overflow,body_count=" << n
            << ",peer_record=" << a.in.particle_rows[peer].record
            << ",mover_record=" << a.in.particle_rows[mover].record
            << ",contact_rejection=unchanged,late_work_rejection=unchanged,"
               "independent_retry=exact,cpp_new_observed=0 PASS\n";
}

std::vector<std::array<int, 2>> tick_contact_reference(Fixture &f) {
  std::vector<std::array<int, 2>> out;
  std::set<std::pair<int, int>> keys;
  const auto add = [&](int a, int b) {
    if (keys.insert(std::minmax(a, b)).second)
      out.push_back({a, b});
  };
  const auto &s = f.h.prepared_storage();
  for (std::size_t j = 0; j < s.live.contacts; ++j)
    add(s.contacts[j].first.record, s.contacts[j].second.record);
  std::vector<std::pair<int, int>> derived;
  for (std::size_t j = 0; j < f.denominators.size(); ++j)
    for (std::size_t k = j + 1; k < f.denominators.size(); ++k) {
      int a = f.in.particle_rows[j].record, b = f.in.particle_rows[k].record;
      long double d2 = 0;
      for (int axis = 0; axis < 3; ++axis) {
        const long double d =
            static_cast<long double>(f.h.particle64(kPosX + axis, a)) -
            f.h.particle64(kPosX + axis, b);
        d2 += d * d;
      }
      const long double extent =
          static_cast<long double>(f.h.particle64(kRadius, a)) +
          f.h.particle64(kRadius, b);
      if (extent > 0 && d2 < extent * extent)
        derived.push_back(std::minmax(a, b));
    }
  std::sort(derived.begin(), derived.end());
  for (auto pair : derived)
    add(pair.first, pair.second);
  return out;
}
void contact_tick_dirty(engine::Runtime &r, HostPath path) {
  const auto l = tick_limits();
  std::vector<std::uint64_t> ones(70, 1);
  std::vector<double> positions(70), mass(70, 1), radii(70, .125);
  for (int i = 0; i < 70; ++i)
    positions[i] = 4 * i;
  std::vector<std::vector<std::uint64_t>> count(4,
                                                std::vector<std::uint64_t>(70));
  for (int i = 0; i < 70; ++i)
    count[i ? 1 : 0][i] = 1;
  Fixture f(r, path, {{}, {}, {0, 1}, {2}}, count, ones, mass, positions, 8, 6,
            false, false, false, false, 1024, radii, &l);
  f.install();
  f.h.inspect_prepared();
  const auto &v = f.h.inspect_live_hierarchy();
  auto slots = v.slots, previous = v.previous;
  auto selectors = v.selectors;
  selectors[1] = 1;
  for (int k = 0; k < 4; ++k) {
    slots[(4 + k) * 70 + 1] = slots[k * 70 + 1];
    slots[k * 70 + 1] = k == 3 ? slots[(4 + k) * 70 + 1] : -16 * (k + 1);
    previous[k * 70 + 1] = k == 3 ? slots[(4 + k) * 70 + 1] : -32 * (k + 1);
  }
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 0), slots.data(),
                 slots.size() * 8);
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 1), selectors.data(),
                 selectors.size() * 4);
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 13), previous.data(),
                 previous.size() * 8);
  f.h.particle64(kPosX, f.in.particle_rows[69].record) += 256;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  f.control->set(0);
  f.control->set(1, f.nodes[1].field.record);
  r.synchronize();
  const TickSnapshot old(f);
  Parameters64 p;
  p.gravity = .125;
  p.dt = p.contact_stiffness = p.reach_strength = 0;
  TickResult rejected;
  no_new([&] {
    rejected = f.h.tick_prepared(f.h.prepared_storage().generation, p,
                                 UINT64_MAX, 4829);
  });
  require(rejected.status == TickStatus::CapacityExhausted &&
              rejected.failure.budget == Budget::GeometryWork &&
              old == TickSnapshot(f),
          "dirty late rejection lost selected packet/history/pending input");
  f.h.inspect_prepared();
  const auto body_old = f.h.particles64();
  const auto contacts = tick_contact_reference(f);
  const auto expected = physical_reference(f, p, true, &contacts);
  auto before = f.h.inspect_live_hierarchy();
  TickResult ok;
  no_new([&] { ok = f.h.tick_prepared(f.h.prepared_storage().generation, p); });
  require(ok.status == TickStatus::Ok,
          "dirty candidate tick rejected valid retry");
  f.h.inspect_prepared();
  verify_physical(f, expected, body_old, p);
  poll_lifetime(f, before);
  std::cout << "contact_tick_dirty,selector=1,unequal_packets=1,distinct_"
               "history=1,pending_lag=1,late_rejection=unchanged,force="
               "independent_reference PASS\n";
}
void contact_tick_reuse(engine::Runtime &r, HostPath path) {
  const auto l = tick_limits();
  const auto data = letter_data(r);
  std::vector<std::vector<std::uint64_t>> count(3,
                                                std::vector<std::uint64_t>(3));
  count[0][0] = data.denominators[0];
  count[1][1] = data.denominators[1];
  count[1][2] = data.denominators[2];
  Fixture f(r, path, {{}, {}, {0, 1}}, count, data.denominators, data.masses,
            data.positions, 8, 6, false, true, true, true, 1024, data.radii,
            &l);
  f.install();
  f.h.set_tick_mode(TickMode::AllLoaded);
  auto nodes = PreparedFixtureAccess::nodes(f.h);
  for (auto set : {TickExecutionAccess::selection(f.h).particles(),
                   TickExecutionAccess::selection(f.h).centroids()})
    for (auto *node : {set.cells->parent->parent, set.cells->parent, set.cells})
      if (std::find(nodes.begin(), nodes.end(), node) == nodes.end())
        nodes.push_back(node);
  prepared_test::Metrics metrics(r, nodes);
  const auto resource_before = metrics.sample();
  for (int round = 0; round < 32; ++round) {
    f.h.inspect_prepared();
    const auto union_before = tick_contact_reference(f);
    const auto &s = f.h.prepared_storage();
    const auto discovery = s.live.particles * (s.live.particles - 1) / 2 +
                           s.live.contacts * s.live.contacts;
    const auto limit =
        2 * discovery + union_before.size() * union_before.size() - 1;
    Parameters64 p;
    p.dt = round % 2 ? .25 : .5;
    const TickSnapshot baseline(f);
    TickResult rejected;
    no_new([&] {
      rejected = f.h.tick_prepared(s.generation, p, UINT64_MAX, limit);
    });
    require(rejected.status == TickStatus::CapacityExhausted &&
                rejected.failure.budget == Budget::GeometryWork &&
                baseline == TickSnapshot(f),
            "prepared letter reuse rejection leaked candidate state");
    f.h.inspect_prepared();
    const auto body_old = f.h.particles64();
    const auto expected = physical_reference(f, p, true, &union_before);
    auto before = f.h.inspect_live_hierarchy();
    TickResult accepted;
    no_new([&] { accepted = f.h.tick_prepared(s.generation, p); });
    require(accepted.status == TickStatus::Ok,
            "prepared letter tick reuse failed");
    f.h.inspect_prepared();
    verify_physical(f, expected, body_old, p);
    poll_lifetime(f, before);
    const auto expected_union = tick_contact_reference(f);
    require(accepted.candidate.contacts == expected_union.size(),
            "candidate coverage differs from independent pair reference");
  }
  const auto resource_after = metrics.sample();
  resource_before.print("contact_tick_reuse_before");
  resource_after.print("contact_tick_reuse_after");
  require(resource_before.same_backing(resource_after),
          "prepared tick reuse grew native backing/layout/compiler resources");
  std::cout << "contact_tick_reuse,rounds=32,late_rejections=32,letters="
               "retained,body_bits=8,field_bits=6,permuted_native_order=1,"
               "force_motion=independent_reference,cpp_new_observed=0 PASS\n";
}

void contact_tick_source_union(engine::Runtime &r, HostPath path) {
  const auto l = tick_limits();
  Fixture f(r, path, {{}}, {{1, 1, 1}}, {1, 1, 1}, {1, 1, 1}, {0, 1, 4}, 52, 52,
            false, false, false, false, 1024, {.75, .75, .75}, &l);
  f.install();
  RefreshRequest request(f.h);
  std::vector<UpdateCommand> contacts(3);
  const std::array<std::array<int, 2>, 3> logical{
      {{{2, 0}}, {{0, 2}}, {{1, 2}}}};
  for (int row = 0; row < 3; ++row) {
    auto &c = contacts[row];
    c.operation = UpdateOp::InsertContact;
    c.position = row;
    c.ordinal = row + 100;
    c.first = UpdateRef::existing(f.in.particle_rows[logical[row][0]]);
    c.second = UpdateRef::existing(f.in.particle_rows[logical[row][1]]);
  }
  refresh_ok(refresh_apply(f, request, contacts));
  PreparedFixtureAccess::read_contacts(f.h);
  const auto source = f.h.contact_int_data;
  f.h.inspect_prepared();
  const auto old = f.h.particles64();
  const auto pairs = tick_contact_reference(f);
  require(pairs.size() == 3, "independent source-union fixture");
  Parameters64 p;
  p.gravity = p.dt = p.reach_strength = 0;
  const auto expected = physical_reference(f, p, true, &pairs);
  TickResult result;
  no_new([&] {
    result = f.h.tick_prepared(f.h.prepared_storage().generation, p);
  });
  require(result.status == TickStatus::Ok &&
              result.current.source_contacts == 3 &&
              result.candidate.contacts == 3 && result.geometry_work == 42,
          "source duplicates were not deduplicated in the physical union");
  f.h.inspect_prepared();
  verify_physical(f, expected, old, p);
  const auto query = f.h.query_prepared_geometry(result.generation);
  const auto &v = f.h.inspect_prepared_geometry(query);
  const int C = f.h.contact_capacity();
  for (int row = 0; row < 3; ++row) {
    require(v.pairs[row] == pairs[row][0] && v.pairs[C + row] == pairs[row][1],
            "explicit-first source orientation/canonical derived order");
    require(v.pair_flags[row] == (row < 2 ? ExplicitContact : DerivedContact),
            "source/derived contact provenance flags");
  }
  require(v.source_to_pair[0] == 0 && v.source_to_pair[1] == 0 &&
              v.source_to_pair[2] == 1,
          "duplicate source rows lost individual mappings");
  PreparedFixtureAccess::read_contacts(f.h);
  require(source == f.h.contact_int_data &&
              f.h.prepared_storage().live.contacts == 3,
          "tick overwrote the explicit source ledger");
  f.h.particle64(kVelX, f.in.particle_rows[1].record) = 2;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  p.dt = 1;
  p.contact_stiffness = 0;
  no_new([&] {
    result = f.h.tick_prepared(f.h.prepared_storage().generation, p);
  });
  require(result.status == TickStatus::Ok && result.current.contacts == 3 &&
              result.candidate.contacts == 2 && result.coverage_changed &&
              f.h.prepared_storage().live.contacts == 3,
          "derived exit removed an explicit request");
  RefreshRequest remove(f.h);
  UpdateCommand command;
  command.operation = UpdateOp::RemoveContact;
  command.row = 1;
  command.generation = f.h.prepared_storage().generation;
  refresh_ok(refresh_apply(f, remove, {command}));
  p.dt = 0;
  no_new([&] {
    result = f.h.tick_prepared(f.h.prepared_storage().generation, p);
  });
  require(result.status == TickStatus::Ok &&
              result.current.source_contacts == 2 &&
              result.candidate.contacts == 2 && result.coverage_changed,
          "source-map change did not publish with refreshed hierarchy stamps");
  std::cout << "contact_tick_source_union,source_rows=3,unique_union=3,"
               "orientation=retained,duplicate_maps=complete,derived_exit=1,"
               "refresh_after_tick=1 PASS\n";
}
void contact_tick_failures(engine::Runtime &r, HostPath path) {
  for (int point : {200, 201}) {
    const auto l = tick_limits();
    Fixture f(r, path, {{}}, {{1, 1}}, {1, 1}, {1, 1}, {0, 4}, 52, 52, false,
              false, false, false, 1024, {.125, .125}, &l);
    f.install();
    f.h.inspect_prepared();
    const auto body = f.in.particle_rows[1].record;
    f.h.particle64(kVelX, body) = 1;
    PreparedFixtureAccess::upload_body_fixture64(f.h);
    f.h.inspect_prepared();
    const auto old = f.h.particles64();
    Parameters64 p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    PreparedFixtureAccess::inject_failure(f.h, point);
    bool failed = false;
    try {
      f.h.tick_prepared(f.h.prepared_storage().generation, p);
    } catch (const std::runtime_error &) {
      failed = true;
    }
    require(
        failed && !f.h.ready() &&
            f.h.tick_prepared(f.h.prepared_storage().generation, p).status ==
                TickStatus::Unready,
        "backend/publication failure was advertised as healthy rejection");
    const auto observed = LiveHierarchyAccess::failed_particles(f.h);
    require(
        point == 200 ? observed == old : observed != old,
        "candidate/publication injection did not reach its intended boundary");
  }
  {
    const auto l = tick_limits();
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false, false,
              false, 1024, {}, &l);
    f.install();
    const TickSnapshot before(f);
    Parameters64 p;
    p.dt = std::numeric_limits<double>::quiet_NaN();
    require(f.h.tick_prepared(f.h.prepared_storage().generation, p).status ==
                    TickStatus::InvalidInput &&
                before == TickSnapshot(f),
            "invalid tick parameter mutated state");
    p.dt = 0;
    require(
        f.h.tick_prepared(f.h.prepared_storage().generation + 1, p).status ==
                TickStatus::Stale &&
            before == TickSnapshot(f),
        "stale tick request mutated state");
  }
  std::cout
      << "contact_tick_failures,candidate=unready,publication=unready,no_"
         "rollback_claim=1,invalid_input=unchanged,stale=unchanged PASS\n";
}
void contact_tick_capacity(engine::Runtime &r, HostPath path) {
  for (bool large_fields : {false, true}) {
    auto l = tick_limits();
    l[large_fields ? Budget::Fields : Budget::Particles] = 4096;
    l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
                                  l, false, NumericalProfile::trial()) +
                              256 * 1024 * 1024;
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 1, 1}, {0, 4, 8}, 52, 52, false, false, false,
              false, 1024, {}, &l, large_fields);
    if (large_fields)
      require(f.in.field_rows[2].record == 4095,
              "large-field fixture did not populate its last record");
    f.install();
    f.h.inspect_prepared();
    f.h.particle64(kVelX, f.in.particle_rows[2].record) = 1;
    PreparedFixtureAccess::upload_body_fixture64(f.h);
    auto nodes = PreparedFixtureAccess::nodes(f.h);
    for (auto set : {TickExecutionAccess::selection(f.h).particles(),
                     TickExecutionAccess::selection(f.h).centroids()})
      for (auto *node :
           {set.cells->parent->parent, set.cells->parent, set.cells})
        if (std::find(nodes.begin(), nodes.end(), node) == nodes.end())
          nodes.push_back(node);
    prepared_test::Metrics metrics(r, nodes);
    const auto before = metrics.sample();
    Parameters64 p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    p.dt = .25;
    for (int round = 0; round < 32; ++round) {
      no_new([&] {
        f.control->set(2, f.in.particle_rows[2].record);
        f.control->set(3);
        const auto result =
            f.h.tick_prepared(f.h.prepared_storage().generation, p);
        require(result.status == TickStatus::Ok &&
                    result.current.present == 3 &&
                    result.current.compaction_slots == l[Budget::Particles],
                "asymmetric candidate capacity tick");
      });
      f.h.inspect_prepared();
      const auto &packets = f.h.inspect_live_hierarchy();
      const auto root = selected(packets, 2);
      const auto root_field = f.nodes[2].field.record;
      for (int k = 0; k < 4; ++k)
        require(f.h.group64(kCenX + k, root_field) == root[k],
                "last-field numerical publication is incoherent");
      require(
          f.h.prepared_storage().inspection_state[2 * f.h.particle_capacity() +
                                                  root_field] == 1 &&
              f.h.determined64()[f.in.particle_rows[2].record] == 1,
          "last-record flags/speed publication missing");
    }
    const auto after = metrics.sample();
    require(before.same_backing(after),
            "asymmetric candidate ticks grew native backing");
    std::cout << "contact_tick_capacity,particles=" << l[Budget::Particles]
              << ",fields=" << l[Budget::Fields]
              << ",present_bodies=3,present_fields=3,rounds=32,last_body="
              << f.in.particle_rows[2].record
              << ",last_field=" << f.in.field_rows[2].record
              << ",cpp_new_observed=0 PASS\n";
  }
}

void contact_tick_modes(engine::Runtime &r, HostPath path) {
  auto l = tick_limits();
  l[Budget::LiveResponses] = 0;
  auto ordinary = l;
  ordinary[Budget::TickWork] = 0;
  Fixture a(r, path, {{}}, {{1, 2, 3}}, {1, 2, 3}, {1, 2, 3}, {0, 2, 5}, 8, 6,
            false, false, false, false, 1024, {0, 0, 0}, &l);
  Fixture b(r, path, {{}}, {{1, 2, 3}}, {1, 2, 3}, {1, 2, 3}, {0, 2, 5}, 8, 6,
            false, false, false, false, 1024, {0, 0, 0}, &ordinary);
  require(!a.h.live_hierarchy_installed() && !b.h.live_hierarchy_installed(),
          "single-level mode fixture");
  a.h.set_tick_mode(TickMode::AllLoaded);
  b.h.set_tick_mode(TickMode::AllLoaded);
  const auto unsupported_before = state_hash(b);
  require(b.h.tick_prepared(b.h.prepared_storage().generation, {}).status ==
                  TickStatus::Unsupported &&
              unsupported_before == state_hash(b),
          "disabled candidate family changed ordinary execution");
  Parameters64 p;
  p.gravity = .125;
  p.dt = .25;
  p.reach_strength = .01;
  for (int step = 0; step < 32; ++step) {
    p.dt = step % 2 ? 0 : .25;
    TickResult result;
    no_new([&] {
      result = a.h.tick_prepared(a.h.prepared_storage().generation, p);
      b.h.tick64(p);
    });
    require(result.status == TickStatus::Ok && !result.coverage_changed,
            "single-level candidate tick failed");
    a.h.inspect_prepared();
    b.h.inspect_prepared();
    require(a.h.particles64() == b.h.particles64() &&
                a.h.groups64() == b.h.groups64() &&
                a.h.determined64() == b.h.determined64() &&
                a.h.universal64() == b.h.universal64() &&
                a.h.prepared_storage().motion64() ==
                    b.h.prepared_storage().motion64() &&
                a.h.prepared_storage().inspection_flags ==
                    b.h.prepared_storage().inspection_flags &&
                a.h.prepared_storage().inspection_state ==
                    b.h.prepared_storage().inspection_state,
            "single-level shared-builder candidate/ordinary parity");
  }
  auto empty = tick_limits();
  for (auto kind :
       {Budget::Particles, Budget::Fields, Budget::Participation,
        Budget::Contacts, Budget::Focus, Budget::CascadeNodes,
        Budget::CascadeLinks, Budget::CascadeReferences, Budget::CascadeLevels,
        Budget::CascadeWork, Budget::CascadeCoverage, Budget::LiveResponses})
    empty[kind] = 0;
  Harness h(r, PreparedCapacity(empty, NumericalProfile::trial()), path);
  p.gravity = p.dt = p.contact_stiffness = p.reach_strength = 0;
  TickResult zero;
  no_new([&] { zero = h.tick_prepared(0, p, UINT64_MAX, 0); });
  require(zero.status == TickStatus::Ok && zero.completed_ticks == 1 &&
              zero.generation == 0 && !zero.current.present &&
              !zero.candidate.contacts && !h.present_particles(),
          "empty candidate execution created a physical sentinel");
  int invalid = 0;
  try {
    PreparedCapacity bad(l);
  } catch (const std::invalid_argument &) {
    ++invalid;
  }
  auto missing = l;
  missing[Budget::GeometryWork] = 0;
  try {
    PreparedCapacity bad(missing, NumericalProfile::trial());
  } catch (const std::invalid_argument &) {
    ++invalid;
  }
  require(invalid == 2, "candidate capability prerequisites were accepted");
  auto scratch = empty;
  scratch[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      scratch, false, NumericalProfile::trial());
  bool scratch_failed = false;
  try {
    Harness insufficient(
        r, PreparedCapacity(scratch, NumericalProfile::trial()), path);
  } catch (const CapacityError &e) {
    scratch_failed = e.failure.budget == Budget::ScratchBytes;
  }
  require(scratch_failed,
          "static-only scratch reservation omitted measured native metadata");
  std::cout
      << "contact_tick_modes,nonhierarchy_rounds=32,ordinary_parity=bit_exact,"
         "positive_and_zero_dt=1,disabled=unchanged,empty=1,setup_rejections=2,"
         "scratch_native_rejection=1,cpp_new_observed=0 PASS\n";
}
void contact_tick_references(engine::Runtime &r, HostPath path) {
  auto l = tick_limits();
  l[Budget::References] = 21;
  Fixture f(r, path, {{}}, {{1, 1, 1}}, {1, 1, 1}, {1, 1, 1}, {0, 5, 10}, 52,
            52, false, false, false, false, 1024, {2, 2, 2}, &l);
  f.install();
  f.h.inspect_prepared();
  f.h.particle64(kVelX, f.in.particle_rows[2].record) = -8;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  const TickSnapshot before(f);
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  p.dt = .25;
  TickResult failed;
  no_new([&] {
    failed = f.h.tick_prepared(f.h.prepared_storage().generation, p);
  });
  require(
      failed.status == TickStatus::CapacityExhausted &&
          failed.failure.budget == Budget::References &&
          failed.failure.required == 22 && failed.failure.capacity == 21 &&
          failed.candidate.contacts == 1 && before == TickSnapshot(f),
      "candidate reference-peak failure did not preserve committed execution");
  p.dt = 0;
  TickResult retry;
  no_new(
      [&] { retry = f.h.tick_prepared(f.h.prepared_storage().generation, p); });
  require(retry.status == TickStatus::Ok && !retry.candidate.contacts,
          "valid reference retry failed");
  std::cout
      << "contact_tick_references,source_refs=10,hierarchy_refs=10,candidate_"
         "peak=22,reservation=21,rejection=unchanged,retry=valid PASS\n";
}

const std::vector<std::set<int>> &tick_writer_schema() {
  static const std::vector<std::set<int>> schema{
      {0, 1, 4},
      {2, 9},
      {1, 5},
      {0, 5, 6, 8, 12},
      {1, 3},
      {3, 7, 8},
      {0, 2, 3, 4},
      {2, 3, 5, 6},
      {6},
      {1},
      {6, 8},
      {7, 14, 15},
      {0, 1, 8, 13, 14, 15, 16, 28, 29},
      {8, 13, 14, 16, 29},
      {3, 4, 5},
      {1, 2},
      {5, 6, 7},
      {1, 2, 3, 4, 5, 6, 7},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {3, 4, 5},
      {1, 2},
      {5, 6, 7},
      {1, 2, 3, 4, 5, 6, 7},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};
  return schema;
}
std::set<int> tick_allowed_writers(
    int index,
    const taichi::lang::Kernel &kernel,
    const std::vector<tick_audit::CopyRows> &layout) {
  if (index < 24)
    return tick_writer_schema().at(index);
  if (index == 24 || index == 25) {
    std::set<int> out;
    for (std::size_t slot = 0; slot < layout.size(); ++slot)
      if (index == 25 || layout[slot].staged)
        out.insert(2 * slot + 1);
    return out;
  }
  if (index < 29)
    return {8};
  if (index == 29)
    return {2, 3, 4, 5};
  return {int(kernel.parameter_list.size()) - 1};
}
void tick_module_contract(Harness &h,
                          const engine::CompiledKernel &compiled,
                          const llvm::Module &module,
                          int index) {
  const auto &kernel = engine::testing::CompiledKernelAccess::kernel(compiled);
  const auto layout = TickExecutionAccess::copy_layout(h);
  const auto &s = TickExecutionAccess::selection(h);
  const auto allowed = tick_allowed_writers(index, kernel, layout);
  const auto report = tick_audit::writes(kernel, module, allowed,
                                         {s.particles(), s.centroids()});
  require(report.writers == allowed,
          "candidate emitted writer coverage differs from its role schema");
  if (index == 24 || index == 25)
    tick_audit::copy(kernel, module, layout, index == 25);
}
void tick_ir_controls(Fixture &f) {
  const auto kernels = TickExecutionAccess::kernels(f.h);
  int negatives = 0, positives = 0;
  for (int index = 0; index < 31; ++index) {
    const auto &compiled = *kernels[index];
    const auto &kernel =
        engine::testing::CompiledKernelAccess::kernel(compiled);
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(compiled))
             .get_internal_data()
             .compiled_data.module;
    try {
      tick_module_contract(f.h, compiled, module, index);
    } catch (const std::exception &e) {
      std::cerr << "tick_audit_baseline,index=" << index
                << ",kernel=" << kernel.get_name() << ",reason=" << e.what()
                << '\n';
      std::error_code ec;
      llvm::raw_fd_ostream output(
          "/tmp/hcp-tick-baseline-failed-" + std::to_string(index) + ".ll", ec);
      if (!ec)
        module.print(output, nullptr);
      throw;
    }
    const auto layout = TickExecutionAccess::copy_layout(f.h);
    const auto allowed = tick_allowed_writers(index, kernel, layout);
    if (index != 25)
      TickExecutionAccess::verify_writer_bindings(f.h, compiled, allowed);
    const auto expect = [&](const char *label, auto mutate) {
      auto clone = llvm::CloneModule(module);
      mutate(*clone);
      require(!llvm::verifyModule(*clone, &llvm::errs()),
              "invalid tick LLVM negative control");
      bool failed = false;
      try {
        tick_module_contract(f.h, compiled, *clone, index);
      } catch (const std::runtime_error &) {
        failed = true;
      }
      require(failed, "tick compiler audit accepted unsafe control");
      ++negatives;
      std::cout << "tick_compiler_negative,index=" << index
                << ",control=" << label << " rejected PASS\n";
    };
    int wrong = -1;
    for (std::size_t arg = 0; arg < kernel.parameter_list.size(); ++arg)
      if (kernel.parameter_list[arg].is_array && !allowed.count(arg)) {
        wrong = int(arg);
        break;
      }
    if (wrong >= 0)
      expect("readonly_target", [&](llvm::Module &clone) {
        hierarchy_audit::Analyzer a(kernel, clone, 100);
        for (auto &fn : clone)
          for (auto &bb : fn)
            for (auto &instruction : bb) {
              llvm::Value *pointer = nullptr;
              if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
                pointer = store->getPointerOperand();
              else if (auto *atomic =
                           llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction))
                pointer = atomic->getPointerOperand();
              if (!pointer || !allowed.count(a.data_arg(pointer)))
                continue;
              auto *load = llvm::dyn_cast<llvm::LoadInst>(
                  llvm::getUnderlyingObject(pointer));
              require(load, "tick wrong-target control lacks argument load");
              std::int64_t offset = 0;
              auto *buffer = llvm::GetPointerBaseWithConstantOffset(
                  load->getPointerOperand(), offset, clone.getDataLayout());
              llvm::IRBuilder<> builder(load);
              auto *address = builder.CreateGEP(
                  builder.getInt8Ty(), buffer,
                  builder.getInt64(kernel.args_type->get_element_offset(
                      {wrong,
                       taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY})));
              load->setOperand(0, address);
              return;
            }
        throw std::runtime_error("tick wrong-target control missing writer");
      });
    else
      expect("omitted_surface", [&](llvm::Module &clone) {
        hierarchy_audit::Analyzer a(kernel, clone, 100);
        std::vector<llvm::Instruction *> erase;
        for (auto &fn : clone)
          for (auto &bb : fn)
            for (auto &instruction : bb)
              if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
                if (a.data_arg(store->getPointerOperand()) == 0)
                  erase.push_back(store);
        require(!erase.empty(), "tick omission control lacks output");
        for (auto *store : erase)
          store->eraseFromParent();
      });
    if (index == 24 || index == 25) {
      expect("mixed_copy_column", [&](llvm::Module &clone) {
        hierarchy_audit::Analyzer a(kernel, clone, 100);
        for (auto &fn : clone)
          for (auto &bb : fn)
            for (auto &instruction : bb) {
              auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction);
              if (!store || a.data_arg(store->getPointerOperand()) < 0)
                continue;
              auto *load =
                  llvm::dyn_cast<llvm::LoadInst>(store->getValueOperand());
              require(load, "copy column control lacks source load");
              llvm::IRBuilder<> builder(load);
              load->setOperand(0, builder.CreateGEP(load->getType(),
                                                    load->getPointerOperand(),
                                                    builder.getInt64(1)));
              return;
            }
        throw std::runtime_error("copy column control lacks destination");
      });
      expect("missing_copy_guard", [&](llvm::Module &clone) {
        const int enabled = 2 * int(layout.size());
        for (auto &fn : clone)
          for (auto &bb : fn)
            if (auto *branch =
                    llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator()))
              if (branch->isConditional())
                for (bool truth : {true, false})
                  if (tick_audit::implies_enabled(branch->getCondition(), truth,
                                                  kernel, clone, enabled)) {
                    branch->setCondition(llvm::ConstantInt::get(
                        llvm::Type::getInt1Ty(clone.getContext()), truth));
                    return;
                  }
        throw std::runtime_error("copy enable control lacks guard");
      });
      expect("omitted_copy_surface", [&](llvm::Module &clone) {
        hierarchy_audit::Analyzer a(kernel, clone, 100);
        std::vector<llvm::Instruction *> erase;
        for (auto &fn : clone)
          for (auto &bb : fn)
            for (auto &instruction : bb)
              if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
                if (a.data_arg(store->getPointerOperand()) == 1)
                  erase.push_back(store);
        require(!erase.empty(), "copy omission control lacks output");
        for (auto *store : erase)
          store->eraseFromParent();
      });
    }
    auto safe = llvm::CloneModule(module);
    hierarchy_audit::Analyzer a(kernel, *safe, 100);
    llvm::StoreInst *store = nullptr;
    for (auto &fn : *safe)
      for (auto &bb : fn)
        for (auto &instruction : bb)
          if (auto *s = llvm::dyn_cast<llvm::StoreInst>(&instruction))
            if (allowed.count(a.data_arg(s->getPointerOperand()))) {
              store = s;
              break;
            }
    if (store) {
      store->clone()->insertAfter(store);
      require(!llvm::verifyModule(*safe, &llvm::errs()),
              "invalid safe tick LLVM control");
      tick_module_contract(f.h, compiled, *safe, index);
      ++positives;
    }
    std::cout << "tick_compiler_audit,index=" << index
              << ",owned_writes=1,copy_delivery="
              << (index == 24 || index == 25) << ",native_owned_sc=1 PASS\n";
  }
  TickExecutionAccess::verify_copy_bindings(f.h);
  audit_negative(*kernels[11], 0);
  audit_negative(*kernels[12], 1);
  const auto &selection = TickExecutionAccess::selection(f.h);
  int mode = 0;
  for (const auto *k :
       detail::testing::NativeSelectionAccess::kernels(selection)) {
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(*k))
             .get_internal_data()
             .compiled_data.module;
    const auto allowed =
        mode == 2 ? detail::testing::NativeSelectionAccess::committed(selection)
                  : std::array<detail::NativeSet, 2>{selection.particles(),
                                                     selection.centroids()};
    selection_audit::Analyzer(allowed).run(module, mode++);
  }
  std::cout
      << "contact_tick_compiler,modules=34,unsafe_controls=" << negatives
      << ",safe_duplicates=" << positives
      << ",C3_semantic_controls=preserved,copy_bindings=frozen_owned PASS\n";
}
void contact_tick_compiler(engine::Runtime &r, HostPath path) {
  const auto l = tick_limits();
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5}, 8, 6, false, false, false, false,
            1024, {}, &l);
  f.install();
  tick_ir_controls(f);
}
void collect_contact_tick_ir(engine::Runtime &r,
                             HostPath path,
                             bool no_unroll) {
  const char *directory = std::getenv("HCP_B1_TICK_IR_DIR");
  require(directory, "contact tick IR directory not supplied");
  int profiles = 0;
  for (bool narrow : {false, true}) {
    auto l = tick_limits();
    if (narrow)
      l[Budget::Contacts] = 1;
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 1, 2}, {0, 2, 5}, narrow ? 52 : 8, narrow ? 52 : 6,
              false, false, false, false, 1024, {}, &l);
    f.install();
    tick_ir_controls(f);
    const std::string stem = narrow ? "tick-b52-c1" : "tick-b8-c70";
    retain_selection_modules(TickExecutionAccess::kernels(f.h), stem,
                             directory);
    retain_selection_modules(detail::testing::NativeSelectionAccess::kernels(
                                 TickExecutionAccess::selection(f.h)),
                             stem + "-selection", directory);
    ++profiles;
  }
  std::ofstream metadata(std::string(directory) + "/configuration.txt");
  metadata << "actual_backend=x64\nworkers=" << r.cpu_threads()
           << "\nsource_path="
           << (path == HostPath::BulkCopy ? "bulk" : "direct")
           << "\nllvm_unroll="
           << (no_unroll ? "disabled-test-control" : "default")
           << "\nmath=strict\nprofiles=" << profiles << '\n';
  std::cout << "contact_tick_ir_collection,profiles=2,modules=68,actual_owner="
               "1 PASS\n";
}
