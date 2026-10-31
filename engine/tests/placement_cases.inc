// B2 exercises candidate placement only. Fixture edits and starts are private
// test setup; no operation here claims an occurrence from a placement result.
using PlacementSphere = placement_reference::Sphere;
BudgetDemand placement_limits(int capacity = 70) {
  auto l = tick_limits();
  l[Budget::Particles] = l[Budget::Fields] = capacity;
  l[Budget::PlacementWork] = 100000000;
  l[Budget::PlacementCells] = 65536;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
                                l, false, NumericalProfile::trial()) +
                            256 * 1024 * 1024;
  return l;
}
UpdateCommand placement_definition(int address = 900) {
  UpdateCommand c;
  c.operation = UpdateOp::DefineParticle;
  c.kind = 1;
  c.address = address;
  c.exact_mass = 1;
  return c;
}
PlacementIntrinsic placement_intrinsic(std::uint64_t command = 0,
                                       std::uint64_t order = 10,
                                       double radius = 1) {
  return {command, order, 1, radius, {1, 1, true}};
}
PlacementResult place(Fixture &f,
                      const std::vector<UpdateCommand> &commands,
                      const std::vector<PlacementIntrinsic> &inputs,
                      std::uint64_t work = UINT64_MAX,
                      std::uint64_t cells = UINT64_MAX) {
  PlacementResult result;
  no_new([&] {
    result = f.h.query_prepared_placement(
        f.h.prepared_storage().generation, commands.data(), commands.size(),
        inputs.data(), inputs.size(), work, cells);
  });
  return result;
}
std::vector<PlacementCertificate> check_placement(
    Fixture &f,
    const PlacementResult &result,
    const std::vector<PlacementSphere> &spheres,
    const char *name) {
  std::cout << "placement_case,name=" << name
            << ",status=" << int(result.status) << ",work=" << result.work
            << ",prefix=" << result.source_work_bound
            << ",cells=" << result.cell_peak << ",visits=" << result.cell_visits
            << ",tests=" << result.candidate_tests
            << ",moves=" << result.heap_moves
            << ",proposals=" << result.proposals
            << ",budget=" << budget_name(result.failure.budget) << std::endl;
  require(result.status == PlacementStatus::Ok,
          "placement case was not certified");
  const PlacementInspection *view = nullptr;
  no_new([&] { view = &f.h.inspect_prepared_placement(result); });
  auto occupancy = spheres;
  for (const auto &c : view->certificates) {
    placement_reference::certificate(
        c, occupancy, result.target,
        f.h.numerical_profile().body_fraction_bits);
    occupancy.push_back({c.position, c.intrinsic.radius});
    std::cout << "placement_certificate,name=" << name
              << ",rank=" << c.intrinsic.order << ",x=" << c.position[0]
              << ",y=" << c.position[1] << ",z=" << c.position[2]
              << ",lower=" << c.lower_distance << ",upper=" << c.upper_distance
              << ",tau=" << c.tolerance << '\n';
  }
  return view->certificates;
}
void placement_geometry(engine::Runtime &r, HostPath path) {
  const auto l = placement_limits();
  for (int bits : {52, 8}) {
    Fixture f(r, path, {{}}, {{1, 1, 1, 1}}, {1, 1, 1, 1}, {1, 1, 1, 1},
              {0, 0, 0, 0}, bits, 52, false, false, true, false, 1024,
              {1, 1, 1, 1}, &l);
    f.control->set(0);
    r.synchronize();  // All four are sleeping occupants.
    const std::array<double, 3> origin{};
    const auto exercise = [&](const char *name,
                              std::vector<PlacementSphere> spheres,
                              double radius = 1,
                              std::array<double, 3> target =
                                  std::array<double, 3>{}) {
      PlacementAccess::configure(f.h, f.in.particle_rows, spheres,
                                 f.in.field_rows[0], target);
      // The unclaimed payload must not become an obstacle or a domain error.
      PlacementAccess::unused_poison(f.h, f.h.particle_capacity() - 2);
      const auto query =
          f.h.query_prepared_geometry(f.h.prepared_storage().generation);
      require(query.status == GeometryStatus::Ok,
              "placement fixture geometry query");
      const TickSnapshot before(f);
      const auto timing = f.h.timings();
      const auto result = place(f, {placement_definition()},
                                {placement_intrinsic(0, 10, radius)});
      const auto values = check_placement(f, result, spheres, name);
      require(
          std::memcmp(&timing, &f.h.timings(), sizeof(timing)) == 0 &&
              before == TickSnapshot(f),
          "placement changed committed numerical/control/source/report state");
      f.h.inspect_prepared_geometry(query);
      require(result.present == 4 && result.retained == 4 &&
                  f.h.present_particles() == 4,
              "sleeping presence or unclaimed filtering changed");
      return values;
    };
    const std::vector<PlacementSphere> single{
        {origin, 1}, {{64, 0, 0}, 0}, {{0, 64, 0}, 0}, {{0, 0, 64}, 0}};
    const auto exact = exercise("single_sphere", single);
    require(exact[0].position == std::array<double, 3>{-2, 0, 0} &&
                exact[0].lower_distance == 2 && exact[0].upper_distance == 2,
            "single-sphere exact tangency/tie changed");
    exercise("free_focus",
             {{{-4, 0, 0}, 1}, {{4, 0, 0}, 1}, {{0, 4, 0}, 0}, {{0, 0, 4}, 0}});
    exercise("zero_extents",
             {{origin, 0}, {{1, 0, 0}, 0}, {{0, 1, 0}, 0}, {{0, 0, 1}, 0}}, 0);
    exercise("coincident_contained",
             {{origin, 1}, {origin, 2}, {origin, .5}, {origin, 0}}, .5);
    exercise(
        "pair_intersection",
        {{{-.5, 0, 0}, 1}, {{.5, 0, 0}, 1}, {{0, 64, 0}, 0}, {{0, 0, 64}, 0}});
    exercise("unequal_extents",
             {{origin, 2}, {{-1, 0, 0}, .25}, {{0, 64, 0}, 0}, {{0, 0, 64}, 0}},
             .5);
    auto translated = single;
    const std::array<double, 3> target{0x1p27, 0x1p27, 0x1p27};
    for (auto &s : translated)
      for (int k = 0; k < 3; ++k)
        s.centre[k] += target[k];
    exercise("translated_binade", translated, 1, target);
    // Isolated triple-boundary minima exercise actual interval subdivision.
    // At full precision this is intentionally a bounded rejection profile.
    if (bits == 8) {
      exercise("tetrahedral_triple_minimum",
               {{{1, 1, 1}, 1.5},
                {{1, -1, -1}, 1.5},
                {{-1, 1, -1}, 1.5},
                {{-1, -1, 1}, 1.5}},
               .5);
      exercise("planar_triple_minimum",
               {{{1, 0, 0}, 1.5},
                {{-.5, .875, 0}, 1.5},
                {{-.5, -.875, 0}, 1.5},
                {{0, 0, 64}, 0}},
               .5);
    }
    // Canonical order, not caller serialization, controls prospective
    // occupancy.
    PlacementAccess::configure(f.h, f.in.particle_rows, single,
                               f.in.field_rows[0], origin);
    const TickSnapshot before(f);
    const auto first =
        place(f, {placement_definition(900), placement_definition(901)},
              {placement_intrinsic(0, 10), placement_intrinsic(1, 20)});
    const auto a = check_placement(f, first, single, "batch_forward");
    const auto second =
        place(f, {placement_definition(901), placement_definition(900)},
              {placement_intrinsic(0, 20), placement_intrinsic(1, 10)});
    const auto b = check_placement(f, second, single, "batch_permuted");
    require(a.size() == 2 && b.size() == 2 && a[0].position == b[0].position &&
                a[1].position == b[1].position && before == TickSnapshot(f),
            "batch serialized order changed placement or claimed records");
    require(a[0].position != a[1].position,
            "earlier prospective placement was not an obstacle");
  }
  std::cout
      << "placement_geometry,profiles=52_8,independent_boundary_reference=1,"
         "exact_integer_separation=1,sleepers=4,cpp_new_observed=0 PASS\n";
}
void placement_boundaries(engine::Runtime &r, HostPath path) {
  const auto l = placement_limits();
  Fixture f(r, path, {{}}, {{1, 1, 1, 1}}, {1, 1, 1, 1}, {1, 1, 1, 1},
            {0, 0, 0, 0}, 8, 52, false, false, true, false, 1024,
            {1.5, 1.5, 1.5, 1.5}, &l);
  const std::vector<PlacementSphere> tetra{{{1, 1, 1}, 1.5},
                                           {{1, -1, -1}, 1.5},
                                           {{-1, 1, -1}, 1.5},
                                           {{-1, -1, 1}, 1.5}};
  PlacementAccess::configure(f.h, f.in.particle_rows, tetra, f.in.field_rows[0],
                             {});
  f.install();
  const auto published =
      place(f, {placement_definition()}, {placement_intrinsic(0, 10, .5)});
  const auto expected =
      check_placement(f, published, tetra, "bounded_reference");
  const auto geometry =
      f.h.query_prepared_geometry(f.h.prepared_storage().generation);
  const TickSnapshot before(f);
  const auto reject = [&](const std::vector<UpdateCommand> &commands,
                          const std::vector<PlacementIntrinsic> &intrinsics,
                          PlacementStatus status,
                          std::uint64_t work = UINT64_MAX,
                          std::uint64_t cells = UINT64_MAX,
                          Budget budget = Budget::Count) {
    const auto result = place(f, commands, intrinsics, work, cells);
    std::cout << "placement_rejection,status=" << int(result.status)
              << ",budget=" << budget_name(result.failure.budget)
              << ",work=" << result.work
              << ",needed=" << result.failure.required
              << ",capacity=" << result.failure.capacity << '\n';
    require(result.status == status && f.h.ready() && before == TickSnapshot(f),
            "placement rejection mutated committed execution");
    if (budget != Budget::Count)
      require(result.failure.budget == budget,
              "wrong placement exhaustion budget");
    require(
        f.h.inspect_prepared_placement(published).certificates[0].position ==
            expected[0].position,
        "failed attempt overwrote successful placement observation");
    f.h.inspect_prepared_geometry(geometry);
    return result;
  };
  const std::vector<UpdateCommand> commands{placement_definition()};
  const std::vector<PlacementIntrinsic> inputs{placement_intrinsic(0, 10, .5)};
  reject(commands, inputs, PlacementStatus::CapacityExhausted, 0, UINT64_MAX,
         Budget::PlacementWork);
  const auto cells =
      reject(commands, inputs, PlacementStatus::CapacityExhausted, UINT64_MAX,
             0, Budget::PlacementCells);
  require(cells.proposals > 0 && cells.failure.required == 1,
          "cell rejection did not follow feasible upper-bound search");
  reject(commands, inputs, PlacementStatus::CapacityExhausted, UINT64_MAX, 1,
         Budget::PlacementCells);
  reject(commands, inputs, PlacementStatus::CapacityExhausted,
         published.source_work_bound + 10, UINT64_MAX, Budget::PlacementWork);
  auto bad = inputs;
  bad[0].geometry.complete = false;
  reject(commands, bad, PlacementStatus::InvalidInput);
  bad = inputs;
  bad[0].radius = -1;
  reject(commands, bad, PlacementStatus::InvalidInput);
  bad = inputs;
  bad[0].mass = 0;
  reject(commands, bad, PlacementStatus::InvalidInput);
  bad = inputs;
  bad[0].radius = 0x1p-1022;
  reject(commands, bad, PlacementStatus::InvalidInput);
  reject({placement_definition(), placement_definition(901)},
         {placement_intrinsic(0, 10), placement_intrinsic(1, 10)},
         PlacementStatus::InvalidInput);
  std::vector<UpdateCommand> too_many;
  std::vector<PlacementIntrinsic> too_many_inputs;
  for (int i = 0; i < 67; ++i) {
    too_many.push_back(placement_definition(900 + i));
    too_many_inputs.push_back(placement_intrinsic(i, i));
  }
  reject(too_many, too_many_inputs, PlacementStatus::CapacityExhausted,
         UINT64_MAX, UINT64_MAX, Budget::Particles);
  auto raw = commands;
  UpdateCommand retire;
  retire.operation = UpdateOp::RetireParticle;
  retire.first = UpdateRef::existing(f.in.particle_rows[0]);
  raw.push_back(retire);
  reject(raw, inputs,
         PlacementStatus::InvalidInput);  // Incident source rows still exist.
  raw = commands;
  UpdateCommand removal;
  removal.operation = UpdateOp::RemoveParticipation;
  removal.generation = f.h.prepared_storage().generation - 1;
  removal.row = 0;
  removal.ordinal = f.h.prepared_storage().edges[0].ordinal;
  raw.push_back(removal);
  reject(raw, inputs, PlacementStatus::Stale);
  // Valid retry is identical to the independently enumerated reference and to
  // the successful execution before any rejected attempt.
  const auto retried = place(f, commands, inputs);
  const auto actual =
      check_placement(f, retried, tetra, "after_failed_attempts");
  require(actual[0].position == expected[0].position &&
              actual[0].lower_distance == expected[0].lower_distance &&
              actual[0].upper_distance == expected[0].upper_distance &&
              actual[0].tolerance == expected[0].tolerance &&
              before == TickSnapshot(f),
          "failed attempt leaked into successful retry");
  // Only a checked, closed retirement removes an obstacle. The old native
  // claim remains present throughout this candidate-only source projection.
  auto closed = commands;
  removal.generation = f.h.prepared_storage().generation;
  closed.push_back(removal);
  closed.push_back(retire);
  const auto without_first = place(f, closed, inputs);
  std::vector<PlacementSphere> retained(tetra.begin() + 1, tetra.end());
  check_placement(f, without_first, retained, "closed_retirement");
  require(without_first.present == 4 && without_first.retained == 3 &&
              before == TickSnapshot(f),
          "source projection did not exclude exactly the closed outgoing "
          "occurrence");
  // Fixed backing and compilation are measured after preparation, including
  // the first query above and repeated successful/rejected queries.
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto baseline = metrics.sample();
  for (int round = 0; round < 32; ++round) {
    const auto q = place(f, commands, inputs);
    require(q.status == PlacementStatus::Ok, "placement reuse");
  }
  require(baseline.same_backing(metrics.sample()),
          "placement reuse allocated backing or compiled kernels");
  require(before == TickSnapshot(f), "placement reuse changed model state");
  PreparedFixtureAccess::inject_value64(f.h, kPosX,
                                        f.in.particle_rows[0].record, 0x1p-300);
  const TickSnapshot unsupported(f);
  const auto domain = place(f, commands, inputs);
  require(
      domain.status == PlacementStatus::InvalidNumerics &&
          unsupported == TickSnapshot(f) && f.h.ready(),
      "unsupported current-coordinate proof domain was not rejected unchanged");
  PreparedFixtureAccess::inject_value64(
      f.h, kPosX, f.in.particle_rows[0].record, tetra[0].centre[0]);
  const auto saved_round = std::fegetround();
  require(std::fesetround(FE_DOWNWARD) == 0, "rounding-mode test setup");
  PlacementResult rounding;
  try {
    rounding = place(f, commands, inputs);
  } catch (...) {
    std::fesetround(saved_round);
    throw;
  }
  std::fesetround(saved_round);
  require(rounding.status == PlacementStatus::Unsupported &&
              before == TickSnapshot(f),
          "non-nearest arithmetic was not refused unchanged");
  std::cout
      << "placement_boundaries,rejected_cases=14,successful_retry=identical,"
         "rounds=32,backing=stable,compiler=stable,cpp_new_observed=0 PASS\n";
}
void placement_capacity(engine::Runtime &r, HostPath path) {
  auto l = placement_limits();
  int rejected = 0;
  for (int test = 0; test < 5; ++test) {
    auto bad = l;
    NumericalProfile profile = NumericalProfile::trial();
    if (test == 0)
      profile = {};
    if (test == 1)
      bad[Budget::PlacementCells] = 0;
    if (test == 2)
      bad[Budget::PlacementWork] = 0;
    if (test == 3)
      bad[Budget::PlacementCells] = (1ULL << 20) + 1;
    if (test == 4)
      bad[Budget::PlacementWork] = UINT64_MAX;
    try {
      PreparedCapacity cap(bad, profile);
    } catch (const std::invalid_argument &) {
      ++rejected;
    }
  }
  require(rejected == 5,
          "unsupported placement setup capacity/profile was accepted");
  auto tiny = l;
  tiny[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
                                   tiny, false, NumericalProfile::trial()) -
                               1;
  bool exhausted = false;
  try {
    Harness h(r, PreparedCapacity(tiny, NumericalProfile::trial()), path);
  } catch (const CapacityError &e) {
    exhausted = e.failure.budget == Budget::ScratchBytes;
  }
  require(exhausted,
          "placement preparation ignored static scratch reservation");
  auto only = l;
  only[Budget::TickWork] = 0;
  only[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      only, false, NumericalProfile::trial());
  bool measured = false;
  try {
    Harness h(r, PreparedCapacity(only, NumericalProfile::trial()), path);
    for (const auto &entry : h.prepared_storage().manifest)
      if (std::string(entry.name) == "placement-native-metadata-measured") {
        require(entry.bytes == 0,
                "extra native placement metadata escaped scratch accounting");
        measured = true;
      }
  } catch (const CapacityError &e) {
    require(e.failure.budget == Budget::ScratchBytes,
            "wrong measured preparation budget");
    measured = true;
  }
  require(measured, "placement native scratch measurement missing");
  {
    Harness h(r, PreparedCapacity(l, NumericalProfile::trial()), path);
    const auto command = placement_definition();
    const auto intrinsic = placement_intrinsic();
    PlacementResult q;
    no_new(
        [&] { q = h.query_prepared_placement(0, &command, 1, &intrinsic, 1); });
    require(
        q.status == PlacementStatus::Ok && !q.present && !h.present_particles(),
        "empty-source candidate placement failed or claimed a body");
    const auto &c = h.inspect_prepared_placement(q).certificates[0];
    require(c.position == std::array<double, 3>{} && c.lower_distance == 0 &&
                c.upper_distance == 0 && c.tolerance == 0,
            "empty-source zero candidate was not exact");
  }
  auto refs = l;
  refs[Budget::References] = 9;
  Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false, true,
            false, 1024, {1}, &refs);
  f.install();
  const TickSnapshot before(f);
  const auto q = place(f, {placement_definition()}, {placement_intrinsic()});
  require(q.status == PlacementStatus::CapacityExhausted &&
              q.failure.budget == Budget::References &&
              q.failure.required == 10 && before == TickSnapshot(f),
          "placement omitted installed hierarchy references from peak demand");
  std::cout << "placement_capacity,setup_rejections=5,static_scratch_rejection="
               "1,native_scratch=measured,empty_candidate=exact,reference_peak="
               "10,reservation=9 PASS\n";
}
void placement_ownership(engine::Runtime &r, HostPath path) {
  const auto l = placement_limits(4096);
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0, 0}, {0, 1, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 1}, {0, 0, 0}, 52, 52, false, false, true, false,
            1024, {1, 1, 1}, &l, true);
  f.install();
  require(
      f.in.particle_rows[2].record == 4095 && f.in.field_rows[2].record == 4095,
      "large placement fixture missed the last physical/field record");
  const std::vector<PlacementSphere> spheres{
      {{.25, 0, 0}, 1}, {{0, 0, 0}, 1}, {{0, 0, 0}, 1}};
  PreparedFixtureAccess::inject_value64(f.h, kPosX,
                                        f.in.particle_rows[0].record, .25);
  PlacementAccess::unused_poison(f.h, 4094);
  // Create dirty packet lag and pending activity with the existing native
  // handoff. Placement may observe geometry, never consume or refresh it.
  f.control->set(0);
  f.control->set(1, f.in.field_rows[0].record);
  LiveHierarchyAccess::poll(f.h, 0, 0);
  const auto &dirty = f.h.inspect_live_hierarchy();
  require(dirty.selectors[0] == 1 && dirty.slots[0] != dirty.slots[4 * 70],
          "placement fixture lacks unequal dirty packets");
  f.h.inspect_prepared();
  require((f.h.prepared_storage().inspection_flags[f.h.particle_capacity() +
                                                   f.in.field_rows[2].record] &
           2) != 0,
          "placement fixture lacks a pending parent notification");
  const auto geometry =
      f.h.query_prepared_geometry(f.h.prepared_storage().generation);
  const TickSnapshot before(f);
  const auto timings = f.h.timings();
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto baseline = metrics.sample();
  const auto q = place(f, {placement_definition()}, {placement_intrinsic()});
  check_placement(f, q, spheres, "large_sparse_first_query");
  require(q.present == 3 && q.retained == 3 &&
              baseline.same_backing(metrics.sample()) &&
              std::memcmp(&timings, &f.h.timings(), sizeof(timings)) == 0 &&
              before == TickSnapshot(f),
          "first placement changed dirty native state or allocated/compiled "
          "backing");
  f.h.inspect_prepared_geometry(geometry);
  // The poison belongs only to the placement occupancy test. Clear that
  // deliberately invalid unused payload before exercising the separate B1
  // numerical publication policy.
  PreparedFixtureAccess::inject_value64(f.h, kPosX, 4094, 0);
  PreparedFixtureAccess::inject_value64(f.h, kRadius, 4094, 0);
  Parameters64 p;
  p.dt = p.gravity = p.contact_stiffness = p.reach_strength = 0;
  const auto tick = f.h.tick_prepared(f.h.prepared_storage().generation, p);
  std::cout << "placement_following_tick,status=" << int(tick.status)
            << ",budget=" << budget_name(tick.failure.budget) << '\n';
  require(tick.status == TickStatus::Ok,
          "zero-dt tick following placement failed");
  bool stale = false;
  try {
    f.h.inspect_prepared_placement(q);
  } catch (const std::logic_error &) {
    stale = true;
  }
  require(stale && f.h.ready(),
          "completed tick did not invalidate placement observation");
  auto forged = q;
  forged.generation = f.h.prepared_storage().generation;
  forged.completed_ticks = f.h.prepared_storage().completed_ticks;
  stale = false;
  try {
    f.h.inspect_prepared_placement(forged);
  } catch (const std::logic_error &) {
    stale = true;
  }
  require(stale, "caller-edited stamps revived a stale placement observation");
  std::cout << "placement_ownership,P=4096,G=4096,present=3,last_body=4095,"
               "last_field=4095,first_query_backing=stable,dirty_history="
               "preserved,pending=preserved,stale_tick=refused PASS\n";
}
void placement_failures(engine::Runtime &r, HostPath path) {
  const auto l = placement_limits();
  for (int point : {300, 301}) {
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false, true,
              false, 1024, {1}, &l);
    f.h.inspect_prepared();
    const auto old = f.h.particles64();
    PreparedFixtureAccess::inject_failure(f.h, point);
    bool failed = false;
    try {
      place(f, {placement_definition()}, {placement_intrinsic()});
    } catch (const std::runtime_error &) {
      failed = true;
    }
    require(failed && !f.h.ready() &&
                LiveHierarchyAccess::failed_particles(f.h) == old,
            "unexpected placement failure remained healthy or changed model "
            "values");
    const auto q = f.h.query_prepared_placement(
        f.h.prepared_storage().generation, nullptr, 0, nullptr, 0);
    require(q.status == PlacementStatus::Unready,
            "failed placement owner resumed");
  }
  std::cout << "placement_failures,candidate_and_result_publication=unready,"
               "model_forensics=unchanged,resume=refused PASS\n";
}
