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
void admission_preflight(engine::Runtime &r, HostPath path) {
  const auto limits = placement_limits();
  Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false,
            true, false, 1024, {1}, &limits);
  f.install();
  PreparedFixtureAccess::inject_value64(f.h, kVelX,
                                        f.in.particle_rows[0].record, 0.25);
  f.control->set(2, f.in.particle_rows[0].record);
  r.synchronize();
  RefreshRequest request(f.h);
  std::vector<UpdateCommand> commands{placement_definition(901)};
  UpdateCommand edge;
  edge.operation = UpdateOp::InsertParticipation;
  edge.first = UpdateRef::pending(0);
  edge.second = UpdateRef::existing(request.nodes[0].field);
  edge.share = 1;
  edge.exact_count = 1;
  edge.ordinal = 12345;
  edge.position = f.h.prepared_storage().live.edges;
  commands.push_back(edge);
  request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
  const std::vector<PlacementIntrinsic> inputs{placement_intrinsic()};
  UpdateCommand contact;
  contact.operation = UpdateOp::InsertContact;
  contact.first = UpdateRef::pending(0);
  contact.second = UpdateRef::existing(f.in.particle_rows[0]);
  contact.ordinal = 12346;
  commands.push_back(contact);
  std::swap(contact.first, contact.second);
  contact.position = 1;
  contact.ordinal = 12347;
  commands.push_back(contact);
  const auto query = place(f, commands, inputs);
  require(query.status == PlacementStatus::Ok, "preflight placement baseline");
  const auto certificate = f.h.inspect_prepared_placement(query).certificates[0];
  const TickSnapshot before(f);
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  const auto invoke = [&](std::uint64_t work = UINT64_MAX) {
    const auto spec = request.spec();
    UpdateResult result;
    no_new([&] {
      result = PlacementAccess::preflight(f.h, commands.data(), commands.size(),
                                          inputs.data(), inputs.size(), spec,
                                          work);
    });
    return result;
  };
  for (int repeat = 0; repeat < 3; ++repeat) {
    const auto result = invoke();
    require(result.status == UpdateStatus::Ok &&
                result.generation == f.h.prepared_storage().generation,
            "generated source/hierarchy preflight failed");
  }
  const auto derived = PlacementAccess::derived(f.h);
  const auto counts = PlacementAccess::contact_counts(f.h);
  const auto contacts = PlacementAccess::contact_inspection(f.h);
  require(counts[0] == 2 && counts[2] == 2 && counts[3] == 1 &&
              counts[4] == 0 && contacts.source_to_pair[0] == 0 &&
              contacts.source_to_pair[1] == 0 &&
              contacts.pair_flags[0] == ExplicitContact &&
              contacts.pairs[0] != f.in.particle_rows[0].record &&
              contacts.pairs[f.h.contact_capacity()] == f.in.particle_rows[0].record,
          "prospective contacts lost generated endpoints, source orientation, "
          "deduplication or tangency");
  GeometryResult contact_work;
  no_new([&] { contact_work = PlacementAccess::contacts(f.h, 2, 2, 0); });
  require(contact_work.status == GeometryStatus::CapacityExhausted &&
              contact_work.failure.budget == Budget::GeometryWork &&
              before == TickSnapshot(f),
          "contact preparation work did not reject unchanged");
  no_new([&] { contact_work = PlacementAccess::contacts(f.h, 2, 2); });
  require(contact_work.status == GeometryStatus::Ok,
          "private contact retry failed");
  const auto required_work = contact_work.required_work;
  no_new([&] { contact_work = PlacementAccess::contacts(
                  f.h, 2, 2, required_work - 1); });
  require(contact_work.status == GeometryStatus::CapacityExhausted &&
              contact_work.failure.budget == Budget::GeometryWork &&
              contact_work.failure.required == required_work &&
              before == TickSnapshot(f),
          "union ordering work was not reserved before finalization");
  const auto stride = f.h.field_capacity();
  const auto field = request.nodes[0].field.record;
  require(derived[kCenM * stride + field] == 2,
          "prospective hierarchy did not consume generated mass");
  for (int axis = 0; axis < 3; ++axis)
    require(derived[(kCenX + axis) * stride + field] ==
                certificate.position[axis] / 2,
            "prospective hierarchy lost native generated position");
  require(backing.same_backing(metrics.sample()) && before == TickSnapshot(f),
          "preflight claimed records, published state or changed backing");
  const auto exhausted = invoke(0);
  require(exhausted.status == UpdateStatus::CapacityExhausted &&
              exhausted.failure.budget == Budget::CascadeWork,
          "prospective hierarchy work was not bounded");
  request.cohorts[0].pop_back();
  require(invoke().status == UpdateStatus::Unsupported,
          "incomplete prospective hierarchy lost its existing typed rejection");
  request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
  PreparedFixtureAccess::inject_failure(f.h, 1);
  require(invoke().status == UpdateStatus::InvalidInput,
          "late preclaim rejection was not retained");
  PreparedFixtureAccess::inject_failure(f.h, 0);
  require(invoke().status == UpdateStatus::Ok && f.h.ready(),
          "healthy preflight retry failed");
  const auto spec = request.spec();
  require(f.h.refresh_live_hierarchy(f.h.prepared_storage().generation,
                                     commands.data(), commands.size(), spec)
              .status == UpdateStatus::InvalidNumerics,
          "public fixture refresh consumed private generated starts");
  require(before == TickSnapshot(f) &&
              f.h.inspect_prepared_placement(query).certificates[0].position ==
                  certificate.position,
          "rejected preflight changed committed state or public observation");
  std::vector<int> destinations(f.h.particle_capacity() + f.h.field_capacity());
  for (int row = 0; row < f.h.particle_capacity(); ++row) destinations[row] = row;
  const int old_row = f.in.particle_rows[0].record, new_row = contacts.pairs[0];
  std::swap(destinations[old_row], destinations[new_row]);
  no_new([&] { PlacementAccess::remap_contacts(f.h, destinations); });
  const auto remapped = PlacementAccess::contact_inspection(f.h);
  const int p = f.h.particle_capacity();
  require(remapped.pairs[0] == old_row && remapped.pairs[f.h.contact_capacity()] == new_row &&
              remapped.source_to_pair[0] == 0 && remapped.source_to_pair[1] == 0 &&
              remapped.offsets[p + old_row] - remapped.offsets[old_row] == 1 &&
              remapped.offsets[3 * p + new_row] - remapped.offsets[2 * p + new_row] == 1 &&
              before == TickSnapshot(f),
          "nonidentity claim mapping lost source orientation, duplicate map or adjacency");
  std::cout << "admission_preflight,native_starts_consumed=1,prospective_tree=1,"
               "no_claims=1,no_publication=1,rejections=3,healthy_retry=1 PASS\n";
}
void admission_contact_capacity(engine::Runtime &r, HostPath path) {
  for (bool references : {false, true}) {
  auto limits = placement_limits();
  limits[Budget::Contacts] = references ? 4 : 2;
  if (references) limits[Budget::References] = 40;
  Fixture f(r, path, {{}}, {{1, 1, 1}}, {1, 1, 1}, {1, 1, 1},
            {0, 0, 0}, 52, 52, false, false, true, false, 1024,
            {1, 1, 1}, &limits);
  PlacementAccess::configure(f.h, f.in.particle_rows,
                             {{{0, 0, 0}, 1}, {{0, 0, 0}, 1}, {{0, 0, 0}, 1}},
                             f.in.field_rows[0], {});
  f.install();
  Parameters64 tick_parameters;
  tick_parameters.dt = tick_parameters.gravity = tick_parameters.contact_stiffness =
      tick_parameters.reach_strength = 0;
  if (references)
    require(f.h.tick_prepared(f.h.prepared_storage().generation, tick_parameters).status ==
                TickStatus::Ok, "could not install old contact union for reference peak");
  f.control->set(0);  // All overlapping survivors sleep during discovery.
  RefreshRequest request(f.h);
  std::vector<UpdateCommand> commands{placement_definition(901)};
  UpdateCommand edge;
  edge.operation = UpdateOp::InsertParticipation;
  edge.first = UpdateRef::pending(0);
  edge.second = UpdateRef::existing(f.in.field_rows[0]);
  edge.share = 1;
  edge.exact_count = 1;
  edge.ordinal = 999;
  edge.position = 3;
  commands.push_back(edge);
  request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
  const auto input = placement_intrinsic();
  const TickSnapshot before(f);
  const auto invoke = [&](bool commit = false) {
    const auto spec = request.spec();
    UpdateResult result;
    no_new([&] {
      result = PlacementAccess::preflight(f.h, commands.data(), commands.size(),
                                          &input, 1, spec, UINT64_MAX, commit);
    });
    return result;
  };
  const auto overflow = invoke();
  require(overflow.status == UpdateStatus::CapacityExhausted &&
              overflow.failure.budget ==
                  (references ? Budget::References : Budget::Contacts) &&
              overflow.contact_pairs == 3 &&
              before == TickSnapshot(f) && f.h.ready(),
          "sleeping source-empty prospective contact overflow mutated state");
  std::cout << "admission_contact_rejection,budget="
            << budget_name(overflow.failure.budget) << ",required="
            << overflow.failure.required << ",capacity="
            << overflow.failure.capacity << " PASS\n";
  UpdateCommand remove;
  remove.operation = UpdateOp::RemoveParticipation;
  remove.row = 0;
  remove.ordinal = f.h.prepared_storage().edges[0].ordinal;
  remove.generation = f.h.prepared_storage().generation;
  commands.push_back(remove);
  UpdateCommand retire;
  retire.operation = UpdateOp::RetireParticle;
  retire.first = UpdateRef::existing(f.in.particle_rows[0]);
  commands.push_back(retire);
  request.cohorts[0].erase(request.cohorts[0].begin());
  // RemoveParticipation compacts the prospective ledger before insertion.
  commands[1].position = 2;
  require(invoke().status == UpdateStatus::Ok && before == TickSnapshot(f),
          "closed retirement did not remove prospective contact occupancy");
  const auto counts = PlacementAccess::contact_counts(f.h);
  require(counts[0] == 3 && counts[2] == 0 && counts[3] == 1 && counts[4] == 1,
          "source-empty derived discovery missed sleepers or kept retirement");
  const auto generation = f.h.prepared_storage().generation;
  const auto ticks = f.h.prepared_storage().completed_ticks;
  require(invoke(true).status == UpdateStatus::Ok &&
              f.h.prepared_storage().generation == generation + 1 &&
              f.h.prepared_storage().completed_ticks == ticks &&
              f.h.prepared_storage().particle_headers[retire.first.handle.record].record < 0,
          "combined retirement admission failed or reused the outgoing claim");
  const auto tick = f.h.tick_prepared(f.h.prepared_storage().generation, tick_parameters);
  require(tick.status == TickStatus::Ok && tick.current.contacts == 1 &&
              !tick.coverage_changed,
          "retirement contact union was not coherently installed");
  std::cout << "admission_contacts,source_empty=1,sleepers=3,overflow=3,"
               "retirement_retry=1,combined_commit=1,following_tick=1 PASS\n";
  }
}

void admission_bootstrap(engine::Runtime &r, HostPath path) {
  for (int bits : {52, 8}) {
  auto l = placement_limits(bits == 52 ? 70 : 128);
  l[Budget::Batch] = 2;
  l[Budget::BootstrapCommands] = 16;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      l, false, NumericalProfile::trial(bits, 52)) + 256 * 1024 * 1024;
  Harness h(r, PreparedCapacity(l, NumericalProfile::trial(bits, 52)), path);
  std::vector<UpdateCommand> commands;
  for (int i = 0; i < 3; ++i)
    commands.push_back(placement_definition(900 + i));
  for (int i = 0; i < 3; ++i) {
    UpdateCommand field;
    field.operation = UpdateOp::DefineField;
    field.kind = 1;
    field.address = i;
    commands.push_back(field);
  }
  for (int i = 0; i < 6; ++i) {
    UpdateCommand edge;
    edge.operation = UpdateOp::InsertParticipation;
    edge.first = UpdateRef::pending(i % 3);
    edge.second = UpdateRef::pending(i >= 3 ? 5 : i == 2 ? 3 : 4);
    edge.position = i;
    edge.ordinal = 100 + i;
    edge.share = 1;
    edge.exact_count = 1;
    commands.push_back(edge);
  }
  std::vector<PlacementIntrinsic> inputs{
      placement_intrinsic(1, 20), placement_intrinsic(2, 30),
      placement_intrinsic(0, 10)};
  inputs[0].mass = 3;
  if (bits == 52)
    inputs[0].radius = 0;
  inputs[1].mass = 2;
  std::vector<LiveRefreshNode> nodes(3);
  for (int i = 0; i < 2; ++i) {
    nodes[i].field_command = 3 + i;
    nodes[i].comparison = 1 + i;
    nodes[i].cohort_begin = i;
    nodes[i].cohort_count = i == 0 ? 1 : 2;
  }
  nodes[2].field_command = 5;
  nodes[2].comparison = 3;
  nodes[2].level = 1;
  nodes[2].child_count = 2;
  nodes[2].cohort_begin = 3;
  nodes[2].cohort_count = 3;
  const std::array<int, 2> children{0, 1};
  std::vector<LiveRefreshContribution> members{
      {UpdateRef::pending(2), {}, 8}, {UpdateRef::pending(0), {}, 6},
      {UpdateRef::pending(1), {}, 7}, {UpdateRef::pending(0), {}, 9},
      {UpdateRef::pending(1), {}, 10}, {UpdateRef::pending(2), {}, 11}};
  LiveRefreshSpec spec{nodes.data(), nodes.size(), children.data(), children.size(),
                       members.data(), members.size()};
  const std::vector<UpdateRef> focus{UpdateRef::pending(3), UpdateRef::pending(4)};
  const auto generation = h.prepared_storage().generation;
  const auto unchanged = [&] {
    require(h.ready() && h.present_particles() == 0 &&
                h.prepared_storage().generation == generation &&
                !h.live_hierarchy_installed() &&
                h.prepared_storage().live.references() == 0,
            "rejected bootstrap changed the empty owner");
  };
  require(PlacementAccess::bootstrap(h, commands, inputs, spec, {}).status ==
              UpdateStatus::InvalidInput, "bootstrap accepted implicit focus");
  unchanged();
  auto oversized = commands;
  const std::vector<UpdateRef> excess_focus(l[Budget::Focus] + 1, UpdateRef::pending(3));
  const auto excess = PlacementAccess::bootstrap(h, commands, inputs, spec, excess_focus);
  require(excess.status == UpdateStatus::CapacityExhausted &&
              excess.failure.budget == Budget::Focus, "bootstrap focal capacity status");
  unchanged();
  const auto no_work = PlacementAccess::bootstrap(h, commands, inputs, spec, focus, true, 0);
  require(no_work.status == UpdateStatus::CapacityExhausted &&
              no_work.failure.budget == Budget::PlacementWork, "bootstrap work reservation");
  unchanged();
  oversized.resize(17);
  const auto over = PlacementAccess::bootstrap(h, oversized, inputs, spec, focus);
  require(over.status == UpdateStatus::CapacityExhausted &&
              over.failure.budget == Budget::BootstrapCommands,
          "bootstrap did not enforce its separate command reservation");
  unchanged();
  auto empty_field = commands;
  auto extra = commands[3];
  extra.address = 3;
  empty_field.push_back(extra);
  require(PlacementAccess::bootstrap(h, empty_field, inputs, spec,
              {UpdateRef::pending(12)}).status == UpdateStatus::InvalidInput,
          "bootstrap accepted a selected field without participants");
  unchanged();
  for (const auto &invalid : std::vector<std::vector<UpdateRef>>{
           {UpdateRef::pending(3), UpdateRef::pending(3)}, {UpdateRef::pending(0)},
           {UpdateRef::pending(99)}}) {
    require(PlacementAccess::bootstrap(h, commands, inputs, spec, invalid).status ==
                UpdateStatus::InvalidInput, "bootstrap accepted invalid focus");
    unchanged();
  }
  auto bad = spec;
  bad.node_count = 1;
  const auto bad_result = PlacementAccess::bootstrap(h, commands, inputs, bad, focus);
  std::cout << "bootstrap_incomplete,status=" << int(bad_result.status)
            << ",budget=" << budget_name(bad_result.failure.budget)
            << ",required=" << bad_result.failure.required << std::endl;
  require(bad_result.status ==
              UpdateStatus::InvalidInput, "bootstrap accepted incomplete hierarchy");
  unchanged();
  const std::vector<UpdateRef> solo{UpdateRef::pending(3)};
  require(PlacementAccess::bootstrap(h, commands, inputs, spec, solo, false).status ==
              UpdateStatus::Ok, "single-field bootstrap preflight failed");
  const auto solo_output = PlacementAccess::bootstrap_output(h);
  require(solo_output[4 * l[Budget::BootstrapCommands] + 2] == (bits == 52 ? 1 : 2),
          "unselected occurrence moved the partial bootstrap target");
  unchanged();
  if (bits == 52) {
    const std::vector<UpdateRef> reversed{focus[1], focus[0]};
    require(PlacementAccess::bootstrap(h, commands, inputs, spec, reversed, false).status ==
                UpdateStatus::Ok, "reordered initial selection rejected");
    const auto starts = PlacementAccess::generated(h);
    for (int k = 0; k < 3; ++k)
      require(starts[(kPosX + k) * l[Budget::BootstrapCommands]] == 0,
              "first selected field did not determine the canonical anchor");
    unchanged();
  }
  UpdateResult preflight;
  no_new([&] { preflight = PlacementAccess::bootstrap(h, commands, inputs, spec, focus, false); });
  std::cout << "bootstrap_preflight,status=" << int(preflight.status)
            << ",budget=" << budget_name(preflight.failure.budget) << std::endl;
  require(preflight.status == UpdateStatus::Ok, "bootstrap preflight failed");
  unchanged();
  PreparedFixtureAccess::inject_failure(h, 1);
  require(PlacementAccess::bootstrap(h, commands, inputs, spec, focus).status ==
              UpdateStatus::InvalidInput, "bootstrap preclaim failure hook");
  unchanged();
  PreparedFixtureAccess::inject_failure(h, 0);
  // Independent test-only certificate replay with the D1 changing target.
  const auto output = PlacementAccess::bootstrap_output(h);
  const int stride = l[Budget::BootstrapCommands];
  std::vector<PlacementSphere> occupancy;
  std::array<double, 3> target{}, weighted{};
  double mass = 0;
  for (int rank = 0; rank < 3; ++rank) {
    const int command = std::array<int, 3>{2, 0, 1}[rank];
    PlacementCertificate certificate;
    certificate.intrinsic = *std::find_if(inputs.begin(), inputs.end(),
        [&](const auto &x) { return int(x.command) == command; });
    for (int k = 0; k < 3; ++k)
      certificate.position[k] = output[k * stride + rank];
    certificate.lower_distance = output[3 * stride + rank];
    certificate.upper_distance = output[4 * stride + rank];
    certificate.tolerance = output[5 * stride + rank];
    certificate.coordinate_bound = output[6 * stride + rank];
    placement_reference::certificate(certificate, occupancy, target, bits);
    if (!rank)
      require(certificate.position == std::array<double, 3>{},
              "first selected occurrence did not establish local zero");
    occupancy.push_back({certificate.position, certificate.intrinsic.radius});
    mass += certificate.intrinsic.mass;
    for (int k = 0; k < 3; ++k) {
      weighted[k] += certificate.intrinsic.mass * certificate.position[k];
      target[k] = weighted[k] / mass;
    }
  }
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(h));
  const auto backing = metrics.sample();
  UpdateResult committed;
  no_new([&] { committed = PlacementAccess::bootstrap(h, commands, inputs, spec, focus); });
  require(committed.status == UpdateStatus::Ok && committed.generation == generation + 1 &&
              h.live_hierarchy_installed() && h.present_particles() == 3 &&
              h.prepared_storage().live.focus == 2 &&
              h.prepared_storage().completed_ticks == 0 && backing.same_backing(metrics.sample()),
          "bootstrap failed one-generation publication or allocated backing");
  h.inspect_prepared();
  const auto &s = h.prepared_storage();
  for (const auto &body : s.particle_headers)
    if (body.record >= 0)
      for (int k = 0; k < kParticleFieldCount; ++k)
        if (k != kMass && k != kRadius && (k < kPosX || k > kPosZ))
          require(h.particle64(k, body.record) == 0,
                  "bootstrap body inherited nonzero motion or force");
  for (int i = 0; i < 2; ++i)
    require(s.focal[i].address == i && s.focal[i].context != 0,
            "bootstrap focus did not resolve native field claims");
  for (int k = 0; k < 3; ++k)
    require(std::abs(s.inspection_focus64[k] - target[k]) < 1e-12,
            "published bootstrap focus differs from complete selected centroid");
  Parameters64 parameters;
  parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
  const auto tick = h.tick_prepared(s.generation, parameters);
  require(tick.status == TickStatus::Ok && !tick.coverage_changed,
          "bootstrap hierarchy/contact publication failed following tick");
  require(PlacementAccess::bootstrap(h, commands, inputs, spec, focus).status ==
              UpdateStatus::InvalidInput, "bootstrap replaced a populated owner");
  if (bits == 52)
    for (int failure : {2, 6}) {
      Harness failed(r, PreparedCapacity(l, NumericalProfile::trial(bits, 52)), path);
      const auto before = failed.prepared_storage().generation;
      PreparedFixtureAccess::inject_failure(failed, failure);
      bool threw = false;
      try { PlacementAccess::bootstrap(failed, commands, inputs, spec, focus); }
      catch (const std::runtime_error &) { threw = true; }
      require(threw && !failed.ready() &&
                  failed.prepared_storage().generation == before &&
                  PlacementAccess::bootstrap(failed, commands, inputs, spec, focus).status ==
                      UpdateStatus::Unready,
              "failed bootstrap advertised readiness or a completed generation");
    }
  std::cout << "admission_bootstrap,explicit_focus=2,anchor_command=2,commands=12,batch=2,"
               "partial_centroid=certified,preclaim_rejection=unchanged,publication=one,"
               "backing=stable,following_tick=pass PASS\n";
  }
}

void exchange_state(engine::Runtime &r, HostPath path) {
  for (int bits : {52, 8}) {
    const auto l = placement_limits();
    Fixture f(r, path, {{}}, {{1, 1, 1}}, {1, 1, 1}, {1, 3, 2}, {-4, 8, 40},
              bits, 52, false, false, true, false, 1024, {}, &l, true);
    f.install();
    const auto a = f.in.particle_rows[0], b = f.in.particle_rows[1];
    PreparedFixtureAccess::inject_value64(f.h, kVelX, a.record, 2);
    PreparedFixtureAccess::inject_value64(f.h, kVelX, b.record, -2);
    PreparedFixtureAccess::inject_value64(f.h, kPosY, a.record, 4);
    PreparedFixtureAccess::inject_value64(f.h, kPosY, b.record, -4);
    PreparedFixtureAccess::inject_value64(f.h, kVelZ, a.record, 8);
    // Current resident values, including a sleeper, are the authority.
    const auto geometry = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
    const auto query = place(f, {placement_definition()}, {placement_intrinsic()});
    const TickSnapshot before(f);
    const auto history = PlacementAccess::rebase_history(f.h);
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    std::vector<engine::RecordHandle> selected{b, a};
    UpdateResult result;
    no_new([&] { result = PlacementAccess::exchange_state(f.h, selected); });
    require(result.status == UpdateStatus::Ok && before == TickSnapshot(f) &&
                history == PlacementAccess::rebase_history(f.h) &&
                backing.same_backing(metrics.sample()), "exchange state changed live execution");
    const auto state = PlacementAccess::exchange_output(f.h);
    require(state == std::array<double, 8>{5, -2, 0, -1, 0, 2, 4, 0},
            "outgoing state did not use resident mass-weighted centre/velocity");
    std::reverse(selected.begin(), selected.end());
    require(PlacementAccess::exchange_state(f.h, selected).status == UpdateStatus::Ok &&
                state == PlacementAccess::exchange_output(f.h), "caller order changed exchange reduction");
    require(PlacementAccess::exchange_state(f.h, {a}).status == UpdateStatus::Ok &&
                PlacementAccess::exchange_output(f.h) ==
                    std::array<double, 8>{-4, 4, 0, 2, 0, 8, 1, 0},
            "aggregate outgoing state differs from its current body");
    auto stale = a;
    ++stale.incarnation;
    require(PlacementAccess::exchange_state(f.h, {stale}).status == UpdateStatus::Stale &&
                PlacementAccess::exchange_state(f.h, selected, UINT64_MAX,
                    f.h.prepared_storage().generation + 1).status == UpdateStatus::Stale &&
                PlacementAccess::exchange_state(f.h, {}).status == UpdateStatus::InvalidInput &&
                PlacementAccess::exchange_state(f.h, {a, a}).status == UpdateStatus::InvalidInput &&
                PlacementAccess::exchange_state(f.h, selected, 0).status == UpdateStatus::CapacityExhausted,
            "invalid exchange selection or work accepted");
    for (auto poison : {std::numeric_limits<double>::quiet_NaN(), 0x1p-300, 0x1p129}) {
      PreparedFixtureAccess::inject_value64(f.h, kVelX, a.record, poison);
      const TickSnapshot invalid(f);
      require(PlacementAccess::exchange_state(f.h, selected).status == UpdateStatus::InvalidNumerics &&
                  invalid == TickSnapshot(f) && f.h.ready(), "invalid exchange velocity published");
    }
    PreparedFixtureAccess::inject_value64(f.h, kVelX, a.record, 2);
    require(before == TickSnapshot(f) && PlacementAccess::exchange_state(f.h, selected).status == UpdateStatus::Ok,
            "exchange retry did not retain original state");
    f.h.inspect_prepared_geometry(geometry);
    f.h.inspect_prepared_placement(query);
    std::cout << "exchange_state,bits=" << bits
              << ",mass_weighted=1,current_velocity=1,order_independent=1,rejection=unchanged,backing=stable PASS\n";
  }
}

void coordinate_rebase(engine::Runtime &r, HostPath path) {
  for (int capacity : {70, 128}) {
    const auto l = placement_limits(capacity);
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0}, {0, 1}, {0, 0}},
              {1, 1}, {1, 1}, {-4, 4}, 52, 52, false, true, true,
              false, 1024, {}, &l, true);
    f.install();
    Parameters64 stationary;
    stationary.dt = stationary.gravity = stationary.contact_stiffness = stationary.reach_strength = 0;
    require(f.h.tick_prepared(f.h.prepared_storage().generation, stationary).status == TickStatus::Ok,
            "rebase fixture contact installation");
    f.control->set(0);
    PreparedFixtureAccess::inject_value64(f.h, kPosX, f.in.particle_rows[1].record, 6);
    PreparedFixtureAccess::inject_value64(f.h, kVelX, f.in.particle_rows[0].record, .5);
    f.control->set(1, f.in.field_rows[1].record);
    LiveHierarchyAccess::poll(f.h, 0, f.in.field_rows[1].record);
    const auto geometry = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
    const TickSnapshot before(f);
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    const auto anchor = f.h.frame_anchor();
    const FrameAnchor delta{134217728., -16., 8.};
    PlacementAccess::disabled_rebase(f.h, delta);
    require(PlacementAccess::rebase(f.h, {}, UINT64_MAX, generation + 1).status == UpdateStatus::Stale &&
                PlacementAccess::rebase(f.h, {}).status == UpdateStatus::NoChange,
            "rebase stale/no-op disposition");
    const auto work = PlacementAccess::rebase(f.h, delta, 0);
    require(work.status == UpdateStatus::CapacityExhausted && work.failure.budget == Budget::CascadeWork,
            "rebase work reservation ignored");
    for (double bad : {std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN(), 0x1p-100, 0x1p-1000, 0x1p100})
      require(PlacementAccess::rebase(f.h, {bad, 0, 0}).status == UpdateStatus::InvalidNumerics,
              "inexact or nonfinite rebase accepted");
    PreparedFixtureAccess::inject_failure(f.h, 410);
    require(PlacementAccess::rebase(f.h, delta).status == UpdateStatus::InvalidInput,
            "rebase prepublication failure hook");
    PreparedFixtureAccess::inject_failure(f.h, 0);
    const auto &tree = f.h.inspect_live_hierarchy();
    for (int input : {16, 0, 13}) {
      auto *array = LiveHierarchyAccess::input_array(f.h, input);
      std::vector<double> original(array->get_nelement()), observed(original.size());
      engine::readback(*array, original.data(), original.size() * sizeof(double));
      auto inexact = original;
      const auto at = input == 16 ? 3 * f.h.field_capacity() + f.in.field_rows[0].record :
          (input == 0 ? 4 * tree.selectors.size() : 0) + tree.order[0];
      for (double tiny : {0x1p-100, 0x1p-1000}) {
        inexact[at] = tiny;
        engine::upload(*array, inexact.data(), inexact.size() * sizeof(double));
        require(PlacementAccess::rebase(f.h, delta).status == UpdateStatus::InvalidNumerics,
                "rebase ignored inexact or FTZ-range history");
        engine::readback(*array, observed.data(), observed.size() * sizeof(double));
        require(observed == inexact && f.h.ready(), "rebase rejection changed retained history");
      }
      engine::upload(*array, original.data(), original.size() * sizeof(double));
    }
    require(before == TickSnapshot(f) && anchor == f.h.frame_anchor() && f.h.ready(),
            "rebase rejection or disabled publication changed live state");
    f.h.inspect_prepared_geometry(geometry);
    const auto bodies = f.h.particles64(), groups = f.h.groups64();
    const auto history = PlacementAccess::rebase_history(f.h);
    const auto hierarchy = f.h.inspect_live_hierarchy();
    const auto focus = f.h.prepared_storage().inspection_focus64;
    const auto motion = f.h.prepared_storage().motion64();
    const auto flags = f.h.prepared_storage().inspection_flags;
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    UpdateResult result;
    no_new([&] { result = PlacementAccess::rebase(f.h, delta); });
    require(result.status == UpdateStatus::Ok && result.generation == generation + 1 &&
                ticks == f.h.prepared_storage().completed_ticks && backing.same_backing(metrics.sample()),
            "rebase failed atomic generation/backing checks");
    f.h.inspect_prepared();
    const auto &s = f.h.prepared_storage();
    const auto moved_history = PlacementAccess::rebase_history(f.h);
    for (int axis = 0; axis < 3; ++axis) {
      require(f.h.frame_anchor()[axis] == anchor[axis] + delta[axis] &&
                  s.inspection_focus64[axis] == focus[axis] - delta[axis] &&
                  s.inspection_focus64[8 + axis] == focus[8 + axis] - delta[axis],
              "rebase frame/focal controls disagree");
    }
    for (int row = 0; row < f.h.particle_capacity(); ++row)
      for (int col = 0; col < kParticleFieldCount; ++col) {
        const auto at = col * f.h.particle_capacity() + row;
        const auto offset = col < 3 && s.particle_headers[row].record >= 0 ? delta[col] : 0;
        require(f.h.particles64()[at] == bodies[at] - offset, "rebase body payload/presence changed");
      }
    for (int row = 0; row < f.h.field_capacity(); ++row) {
      const bool present = s.field_headers[row].record >= 0;
      for (int col = 0; col < kGroupFieldCount; ++col) {
        const auto at = col * f.h.field_capacity() + row;
        require(f.h.groups64()[at] == groups[at] - (present && col < 3 ? delta[col] : 0),
                "rebase field payload changed");
      }
      for (int col = 0; col < 6; ++col) {
        const auto at = col * f.h.field_capacity() + row;
        require(moved_history[at] == history[at] - (present && col >= 3 ? delta[col - 3] : 0),
                "rebase previous centre or displacement changed incorrectly");
      }
    }
    auto moved = f.h.inspect_live_hierarchy();
    const auto stride = moved.selectors.size();
    for (std::size_t n = 0; n < hierarchy.node_count; ++n) {
      const int slot = hierarchy.order[n];
      for (int col = 0; col < 8; ++col)
        require(moved.slots[col * stride + slot] == hierarchy.slots[col * stride + slot] -
                    (col % 4 < 3 ? delta[col % 4] : 0), "rebase lost a hierarchy packet/history slot");
      for (int col = 0; col < 4; ++col)
        require(moved.previous[col * stride + slot] == hierarchy.previous[col * stride + slot] -
                    (col < 3 ? delta[col] : 0), "rebase reader history translation");
    }
    require(moved.selectors == hierarchy.selectors && moved.counters == hierarchy.counters &&
                moved.visits == hierarchy.visits &&
                std::equal(motion.begin(), motion.begin() + 3 * (f.h.particle_capacity() + f.h.field_capacity()),
                           s.motion64().begin()) && s.inspection_flags == flags,
            "rebase consumed pending work or changed activity/motion");
    const TickSnapshot after(f);
    require(after.installed == before.installed && after.query == before.query,
            "rebase rebuilt source/contact indexes");
    bool stale = false;
    try { f.h.inspect_prepared_geometry(geometry); } catch (const std::logic_error &) { stale = true; }
    require(stale, "rebase retained stale observation");
    require(PlacementAccess::rebase(f.h, {-delta[0], -delta[1], -delta[2]}).status == UpdateStatus::Ok,
            "inverse exact rebase rejected");
    f.h.inspect_prepared();
    const auto restored = f.h.inspect_live_hierarchy();
    require(f.h.frame_anchor() == anchor && f.h.particles64() == bodies && f.h.groups64() == groups &&
                restored.slots == hierarchy.slots && restored.previous == hierarchy.previous &&
                PlacementAccess::rebase_history(f.h) == history,
            "exact rebase round trip lost physical/history state");
    Fixture reference(r, path, {{}, {}, {0, 1}}, {{1, 0}, {0, 1}, {0, 0}},
                      {1, 1}, {1, 1}, {-4, 4}, 52, 52, false, true, true,
                      false, 1024, {}, &l, true);
    reference.install();
    require(reference.h.tick_prepared(reference.h.prepared_storage().generation, stationary).status == TickStatus::Ok,
            "rebase continuity reference tick");
    reference.control->set(0);
    PreparedFixtureAccess::inject_value64(reference.h, kPosX, reference.in.particle_rows[1].record, 6);
    PreparedFixtureAccess::inject_value64(reference.h, kVelX, reference.in.particle_rows[0].record, .5);
    reference.control->set(1, reference.in.field_rows[1].record);
    LiveHierarchyAccess::poll(reference.h, 0, reference.in.field_rows[1].record);
    f.control->set(2, f.in.particle_rows[0].record);
    reference.control->set(2, reference.in.particle_rows[0].record);
    auto moving = stationary;
    moving.dt = .25;
    require(PlacementAccess::rebase(f.h, delta).status == UpdateStatus::Ok &&
                f.h.tick_prepared(f.h.prepared_storage().generation, moving).status == TickStatus::Ok &&
                reference.h.tick_prepared(reference.h.prepared_storage().generation, moving).status == TickStatus::Ok &&
                PlacementAccess::rebase(f.h, {-delta[0], -delta[1], -delta[2]}).status == UpdateStatus::Ok,
            "rebase translated-frame tick failed");
    f.h.inspect_prepared();
    reference.h.inspect_prepared();
    const auto continued = f.h.inspect_live_hierarchy(), expected = reference.h.inspect_live_hierarchy();
    require(f.h.particle64(kPosX, f.in.particle_rows[0].record) !=
                bodies[kPosX * f.h.particle_capacity() + f.in.particle_rows[0].record],
            "rebase continuation witness did not move a body");
    require(f.h.particles64() == reference.h.particles64() && f.h.groups64() == reference.h.groups64() &&
                f.h.determined64() == reference.h.determined64() &&
                f.h.prepared_storage().inspection_flags == reference.h.prepared_storage().inspection_flags &&
                continued.slots == expected.slots && continued.previous == expected.previous &&
                continued.selectors == expected.selectors &&
                PlacementAccess::rebase_history(f.h) == PlacementAccess::rebase_history(reference.h),
            "translated-frame tick differs from unrebased continuation");
    std::cout << "coordinate_rebase,capacity=" << capacity << ",translation=2^27,exact=1,history=both_slots,"
                 "rejection=unchanged,round_trip=exact,following_tick=moving,backing=stable PASS\n";
  }
  for (int failure : {411, 412}) {
    const auto l = placement_limits();
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false, true,
              false, 1024, {}, &l);
    f.install();
    const auto generation = f.h.prepared_storage().generation;
    PreparedFixtureAccess::inject_failure(f.h, failure);
    bool failed = false;
    try { PlacementAccess::rebase(f.h, {16, 0, 0}); } catch (const std::runtime_error &) { failed = true; }
    require(failed && !f.h.ready() && f.h.prepared_storage().generation == generation &&
                PlacementAccess::rebase(f.h, {}).status == UpdateStatus::Unready,
            "rebase postpublication failure advertised resumable execution");
  }
  for (int reduced : {0, 1, 2, 3}) {
    const auto l = placement_limits();
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, reduced == 3 ? 0 : reduced == 0 ? 8 : 52,
              reduced == 3 ? 0 : reduced == 1 ? 8 : 52, false, false, true, false, 1024, {}, &l, true);
    if (reduced == 2) f.nodes[0].retained_fraction_bits = 8;
    f.install();
    if (reduced != 0) {
      const auto &tree = f.h.inspect_live_hierarchy();
      for (int input : {16, 0, 13}) {
        auto *array = LiveHierarchyAccess::input_array(f.h, input);
        std::vector<double> original(array->get_nelement()), observed(original.size());
        engine::readback(*array, original.data(), original.size() * sizeof(double));
        auto prior = original;
        const auto at = input == 16 ? 3 * f.h.field_capacity() + f.in.field_rows[0].record :
            (input == 0 ? 4 * tree.selectors.size() : 0) + tree.order[0];
        prior[at] = 0x1p-10;  // Exactly retained even with zero fraction bits.
        engine::upload(*array, prior.data(), prior.size() * sizeof(double));
        const auto observation = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
        const TickSnapshot before(f);
        require(PlacementAccess::rebase(f.h, {16, 0, 0}).status == UpdateStatus::InvalidNumerics &&
                    before == TickSnapshot(f) && f.h.ready(),
                "rebase ignored reduced precision in an old history slot");
        engine::readback(*array, observed.data(), observed.size() * sizeof(double));
        require(observed == prior, "reduced-history rejection changed previous coordinates");
        f.h.inspect_prepared_geometry(observation);
        engine::upload(*array, original.data(), original.size() * sizeof(double));
      }
    }
    const auto generation = f.h.prepared_storage().generation;
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    UpdateResult moved;
    no_new([&] { moved = PlacementAccess::rebase(f.h, {16, 0, 0}); });
    f.h.inspect_prepared();
    require(moved.status == UpdateStatus::Ok && moved.generation == generation + 1 &&
                f.h.particle64(kPosX, f.in.particle_rows[0].record) == -16 &&
                f.h.group64(kCenX, f.in.field_rows[0].record) == -16 &&
                f.h.frame_anchor()[0] == 16 && backing.same_backing(metrics.sample()),
            "exact reduced-profile rebase rejected or rounded");
    const auto geometry = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
    const TickSnapshot before(f);
    const auto history = PlacementAccess::rebase_history(f.h);
    // -16 - 2^-10 is exact f64, but has nonzero discarded bits in an 8-bit
    // (or zero-bit) destination. This distinguishes the precision gate from TwoDiff.
    require(PlacementAccess::rebase(f.h, {0x1p-10, 0, 0}).status == UpdateStatus::InvalidNumerics &&
                before == TickSnapshot(f) && history == PlacementAccess::rebase_history(f.h) &&
                f.h.frame_anchor()[0] == 16 && f.h.ready(),
            "rebase silently rounded a reduced-precision destination");
    f.h.inspect_prepared_geometry(geometry);
    require(PlacementAccess::rebase(f.h, {-16, 0, 0}).status == UpdateStatus::Ok,
            "reduced-profile inverse rejected");
    f.h.inspect_prepared();
    require(f.h.frame_anchor() == FrameAnchor{} &&
                f.h.particle64(kPosX, f.in.particle_rows[0].record) == 0 &&
                f.h.group64(kCenX, f.in.field_rows[0].record) == 0,
            "reduced-profile inverse lost coordinates");
    Parameters64 step;
    step.dt = .125;
    step.gravity = step.contact_stiffness = step.reach_strength = 0;
    PreparedFixtureAccess::inject_value64(f.h, kVelX, f.in.particle_rows[0].record, 1);
    f.control->set(2, f.in.particle_rows[0].record);
    require(f.h.tick_prepared(f.h.prepared_storage().generation, step).status == TickStatus::Ok,
            "reduced rebase could not continue ticking");
    std::cout << "coordinate_rebase_reduced,profile=" << reduced
              << ",exact_shift=16,discarded_bit_rejection=unchanged,inverse=exact,following_tick=pass PASS\n";
  }
}

void live_focus(engine::Runtime &r, HostPath path) {
  for (bool radial : {false, true}) {
    const auto l = placement_limits(radial ? 128 : 70);
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0, 0}, {0, 1, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 2, 1}, {-4, 0, 8}, 52, 52, false, false, true,
              false, 1024, {}, &l, true);
    f.install();
    f.control->set(0);
    PreparedFixtureAccess::inject_value64(f.h, kPosX, f.in.particle_rows[1].record, .25);
    f.control->set(1, f.in.field_rows[1].record);
    LiveHierarchyAccess::poll(f.h, 0, f.in.field_rows[1].record);
    PlacementAccess::radial(f.h, radial);
    const auto geometry = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
    const TickSnapshot before(f);
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    const std::vector<engine::RecordHandle> selected{f.in.field_rows[0]};
    auto stale = selected;
    ++stale[0].incarnation;
    require(PlacementAccess::focus(f.h, stale).status == UpdateStatus::Stale &&
                PlacementAccess::focus(f.h, selected, UINT64_MAX, generation + 1).status == UpdateStatus::Stale &&
                PlacementAccess::focus(f.h, {}).status == UpdateStatus::InvalidInput &&
                PlacementAccess::focus(f.h, {selected[0], selected[0]}).status == UpdateStatus::InvalidInput,
            "invalid focus request accepted");
    const auto work = PlacementAccess::focus(f.h, selected, 0);
    const auto capacity = PlacementAccess::focus(f.h,
        std::vector<engine::RecordHandle>(l[Budget::Focus] + 1, selected[0]));
    require(work.status == UpdateStatus::CapacityExhausted && work.failure.budget == Budget::CascadeWork &&
                capacity.status == UpdateStatus::CapacityExhausted && capacity.failure.budget == Budget::Focus,
            "focus reservation rejection missing");
    PreparedFixtureAccess::inject_failure(f.h, 400);
    require(PlacementAccess::focus(f.h, selected).status == UpdateStatus::InvalidInput,
            "focus prepublication failure hook");
    PreparedFixtureAccess::inject_failure(f.h, 0);
    PlacementAccess::disabled_focus_publication(f.h);
    require(before == TickSnapshot(f) && f.h.ready(), "rejected focus changed execution");
    for (double mass : {0., -1., std::numeric_limits<double>::quiet_NaN()}) {
      PlacementAccess::focal_mass(f.h, selected[0].record, mass);
      const TickSnapshot poisoned(f);
      require(PlacementAccess::focus(f.h, selected).status == UpdateStatus::InvalidNumerics &&
                  poisoned == TickSnapshot(f) && f.h.ready(),
              "nonpositive/nonfinite selected mass was published");
    }
    PlacementAccess::focal_mass(f.h, selected[0].record, 1);
    require(before == TickSnapshot(f), "focus negative fixture did not restore its source");
    f.h.inspect_prepared_geometry(geometry);
    f.h.inspect_prepared();
    const auto particles = f.h.particles64(), groups = f.h.groups64(),
               determined = f.h.determined64();
    const auto universal = f.h.universal64();
    const auto motion = f.h.prepared_storage().motion64();
    const auto states = f.h.prepared_storage().inspection_state;
    const auto flags = f.h.prepared_storage().inspection_flags;
    const auto hierarchy = f.h.inspect_live_hierarchy();
    require(hierarchy.selectors[1] != 0,
            "focus fixture failed to establish a dirty child publication");
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    UpdateResult result;
    no_new([&] { result = PlacementAccess::focus(f.h, selected); });
    require(result.status == UpdateStatus::Ok && result.generation == generation + 1 &&
                f.h.prepared_storage().completed_ticks == ticks && backing.same_backing(metrics.sample()),
            "focus publication changed ticks or prepared backing");
    f.h.inspect_prepared();
    const auto &s = f.h.prepared_storage();
    const auto after_hierarchy = f.h.inspect_live_hierarchy();
    require(f.h.particles64() == particles && f.h.groups64() == groups &&
                f.h.determined64() == determined && f.h.universal64() == universal &&
                s.motion64() == motion && s.inspection_state == states &&
                hierarchy.slots == after_hierarchy.slots &&
                hierarchy.previous == after_hierarchy.previous &&
                hierarchy.selectors == after_hierarchy.selectors &&
                hierarchy.visits == after_hierarchy.visits &&
                hierarchy.counters == after_hierarchy.counters,
            "focus change repaired or reset live numerical/history state");
    require(s.live.focus == 1 && s.focal[0].record == selected[0].record &&
                s.inspection_focus64[0] == -4 && s.inspection_focus64[1] == 0 &&
                s.inspection_focus64[2] == 0, "focus origin/selection not published");
    const auto focal_candidate = PlacementAccess::derived(f.h);
    for (int k = 0; k < 11; ++k)
      require(s.inspection_focus64[k] == focal_candidate[14 * f.h.field_capacity() + 1 + k],
              "focal publication omitted part of the candidate control tuple");
    for (int row = 0; row < f.h.particle_capacity(); ++row)
      require(s.inspection_flags[row] ==
                  (radial && s.particle_headers[row].record >= 0 ? 3 : flags[row]),
              "focus wake ignored radial dependency or activated an absent body");
    for (int row = 0; row < f.h.field_capacity(); ++row)
      require(s.inspection_flags[f.h.particle_capacity() + row] ==
                  flags[f.h.particle_capacity() + row], "focus consumed a pending field notification");
    const auto next_placement = place(f, {placement_definition()},
                                        {placement_intrinsic(0, 10, 0)});
    require(next_placement.status == PlacementStatus::Ok &&
                next_placement.target == std::array<double, 3>{-4, 0, 0},
            "subsequent placement did not use the newly selected focus");
    const TickSnapshot after(f);
    require(after.installed == before.installed && after.query == before.query,
            "focus republished source/contact indexes");
    require(PlacementAccess::focus(f.h, selected).status == UpdateStatus::NoChange &&
                after == TickSnapshot(f), "identical focus selection was not an unchanged no-op");
    bool obsolete = false;
    try { f.h.inspect_prepared_geometry(geometry); }
    catch (const std::logic_error &) { obsolete = true; }
    require(obsolete, "focus publication retained a stale geometry observation");
    const std::vector<engine::RecordHandle> combined{f.in.field_rows[1], f.in.field_rows[0]};
    require(PlacementAccess::focus(f.h, combined).status == UpdateStatus::Ok,
            "ordered combined focus rejected");
    f.h.inspect_prepared();
    const double expected = (3 * f.h.group64(kCenX, combined[0].record) - 4) / 4;
    require(std::abs(f.h.prepared_storage().inspection_focus64[0] - expected) < 1e-12,
            "combined focus differs from independent weighted centroid");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = 0;
    parameters.reach_strength = radial;
    const auto tick = f.h.tick_prepared(f.h.prepared_storage().generation, parameters);
    require(tick.status == TickStatus::Ok && !tick.coverage_changed,
            "following tick rejected retagged hierarchy or changed contact coverage");
    if (!radial) {
      auto limited = l;
      const auto peak = PlacementAccess::focal_reference_peak(f.h, 1);
      // f now selects two fields; the fresh fixture below selects one.
      limited[Budget::References] = peak - 2;
      Fixture pressure(r, path, {{}, {}, {0, 1}}, {{1, 0, 0}, {0, 1, 1}, {0, 0, 0}},
                       {1, 1, 1}, {1, 2, 1}, {-4, 0, 8}, 52, 52, false, false, true,
                       false, 1024, {}, &limited);
      pressure.install();
      const TickSnapshot retained(pressure);
      const auto rejected = PlacementAccess::focus(pressure.h, {pressure.in.field_rows[0]});
      require(rejected.status == UpdateStatus::CapacityExhausted &&
                  rejected.failure.budget == Budget::References &&
                  rejected.failure.required == limited[Budget::References] + 1 &&
                  retained == TickSnapshot(pressure), "focus old-plus-new reference peak not enforced");
    }
    std::cout << "live_focus,radial=" << radial << ",history=preserved,source_contacts=preserved,"
                 "ordered_centroid=pass,publication=one,no_tick=1,backing=stable,retry=pass PASS\n";
  }
  // The explicit ledger and installed physical union have separate endpoints,
  // even when they describe the same pair. Exercise both retained allocations.
  std::uint64_t contact_peak = 0;
  for (int boundary : {0, -1, 1}) {
    auto l = placement_limits();
    if (boundary) l[Budget::References] = contact_peak - (boundary < 0);
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0}, {0, 1}, {0, 0}},
              {1, 1}, {1, 1}, {-4, 4}, 52, 52, false, true, true,
              false, 1024, {}, &l);
    // Establish the retained union before hierarchy installation adds its own
    // references; a later tick's multi-union peak is intentionally not this test.
    Parameters64 stationary;
    stationary.dt = stationary.gravity = stationary.contact_stiffness =
        stationary.reach_strength = 0;
    require(f.h.tick_prepared(f.h.prepared_storage().generation, stationary).status == TickStatus::Ok,
            "focus contact fixture could not establish installed union");
    for (auto &ref : f.refs)
      ref.source.generation = f.h.prepared_storage().generation;
    f.install();
    require(f.h.prepared_storage().live.contacts == 1 &&
                PlacementAccess::installed_contacts(f.h) == 1,
            "focus reference fixture lacks separately retained contact endpoints");
    if (!boundary) {
      contact_peak = PlacementAccess::focal_reference_peak(f.h, 1);
      continue;
    }
    f.control->set(0);
    PreparedFixtureAccess::inject_value64(f.h, kPosX, f.in.particle_rows[1].record, 5);
    f.control->set(1, f.in.field_rows[1].record);
    LiveHierarchyAccess::poll(f.h, 0, f.in.field_rows[1].record);
    const auto geometry = f.h.query_prepared_geometry(f.h.prepared_storage().generation);
    require(geometry.status == GeometryStatus::Ok, "focus contact witness observation");
    const TickSnapshot before(f);
    const auto generation = f.h.prepared_storage().generation;
    const auto result = PlacementAccess::focus(f.h, {f.in.field_rows[0]});
    if (boundary < 0) {
      require(result.status == UpdateStatus::CapacityExhausted &&
                  result.failure.budget == Budget::References &&
                  result.failure.required == contact_peak &&
                  result.failure.capacity == contact_peak - 1 &&
                  before == TickSnapshot(f) && f.h.ready(),
              "focus omitted separately retained source/union contact references");
      f.h.inspect_prepared_geometry(geometry);
    } else {
      require(result.status == UpdateStatus::Ok && result.generation == generation + 1,
              "focus rejected exact source/union reference limit");
    }
    std::cout << "live_focus_contact_references,source=1,installed=1,required="
              << contact_peak << ",capacity=" << l[Budget::References]
              << ",rejected=" << (boundary < 0) << " PASS\n";
  }
  for (int failure : {401, 402}) {
    const auto l = placement_limits();
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0}, {0, 1}, {0, 0}},
              {1, 1}, {1, 1}, {-4, 4}, 52, 52, false, false, true,
              false, 1024, {}, &l);
    f.install();
    const auto generation = f.h.prepared_storage().generation;
    PreparedFixtureAccess::inject_failure(f.h, failure);
    bool failed = false;
    try { PlacementAccess::focus(f.h, {f.in.field_rows[0]}); }
    catch (const std::runtime_error &) { failed = true; }
    require(failed && !f.h.ready() && f.h.prepared_storage().generation == generation &&
                PlacementAccess::focus(f.h, {f.in.field_rows[0]}).status == UpdateStatus::Unready,
            "post-publication focal failure resumed or advertised a generation");
  }
}

void admission_commit(engine::Runtime &r, HostPath path) {
  for (int capacity : {70, 128}) {
  const auto limits = placement_limits(capacity);
  Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false,
            true, false, 1024, {1}, &limits);
  f.install();
  Parameters64 parameters;
  parameters.dt = parameters.gravity = parameters.contact_stiffness =
      parameters.reach_strength = 0;
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  for (int round = 0; round < 2; ++round) {
    require(f.h.tick_prepared(f.h.prepared_storage().generation, parameters).status ==
                TickStatus::Ok, "pre-admission tick failed");
    f.h.inspect_prepared();
    const auto survivor = f.in.particle_rows[0];
    std::array<double, kParticleFieldCount> old{};
    for (int k = 0; k < kParticleFieldCount; ++k)
      old[k] = f.h.particle64(k, survivor.record);
    RefreshRequest request(f.h);
    std::vector<UpdateCommand> commands{placement_definition(901 + round)};
    UpdateCommand edge;
    edge.operation = UpdateOp::InsertParticipation;
    edge.first = UpdateRef::pending(0);
    edge.second = UpdateRef::existing(request.nodes[0].field);
    edge.share = 1;
    edge.exact_count = 1;
    edge.ordinal = 15000 + round;
    edge.position = f.h.prepared_storage().live.edges;
    commands.push_back(edge);
    request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
    UpdateCommand contact;
    contact.operation = UpdateOp::InsertContact;
    contact.first = UpdateRef::pending(0);
    contact.second = UpdateRef::existing(survivor);
    contact.position = f.h.prepared_storage().live.contacts;
    contact.ordinal = 16000 + round;
    commands.push_back(contact);
    const auto input = placement_intrinsic();
    const auto placement = place(f, commands, {input});
    require(placement.status == PlacementStatus::Ok, "commit placement baseline");
    const auto expected = f.h.inspect_prepared_placement(placement).certificates[0];
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    const auto spec = request.spec();
    const TickSnapshot before(f);
    PreparedFixtureAccess::inject_failure(f.h, 1);
    auto rejected = PlacementAccess::preflight(f.h, commands.data(), commands.size(),
                                               &input, 1, spec, UINT64_MAX, true);
    require(rejected.status == UpdateStatus::InvalidInput && before == TickSnapshot(f),
            "ordinary admission preclaim rejection changed live state");
    PreparedFixtureAccess::inject_failure(f.h, 0);
    UpdateResult result;
    no_new([&] { result = PlacementAccess::preflight(
        f.h, commands.data(), commands.size(), &input, 1, spec, UINT64_MAX, true); });
    require(result.status == UpdateStatus::Ok && result.generation == generation + 1 &&
                f.h.prepared_storage().completed_ticks == ticks &&
                f.h.present_particles() == std::size_t(round + 2),
            "ordinary admission did not publish exactly one unticked generation");
    f.h.inspect_prepared();
    engine::RecordHandle added;
    for (const auto &handle : f.h.prepared_storage().particle_headers)
      if (handle.address == 901 + round) added = handle;
    require(added.record >= 0, "admission claim missing");
    for (int k = 0; k < kParticleFieldCount; ++k) {
      require(f.h.particle64(k, survivor.record) == old[k],
              "admission changed survivor numerical state");
      const double value = k >= kPosX && k <= kPosZ ? expected.position[k - kPosX]
                           : k == kMass || k == kRadius ? 1 : 0;
      require(f.h.particle64(k, added.record) == value,
              "admission lost generated initialization");
    }
    const auto &source = f.h.prepared_storage().contacts[round];
    require(source.first.record == added.record && source.second.record == survivor.record,
            "source did not resolve actual native claims");
    const auto geometry = PlacementAccess::contact_inspection(f.h);
    require(geometry.pairs[round] == added.record &&
                geometry.pairs[f.h.contact_capacity() + round] == survivor.record,
            "contact union retained provisional endpoints");
    std::vector<int> installed(2 * f.h.contact_capacity());
    engine::readback(TickExecutionAccess::topology(f.h, 2), installed.data(),
                     installed.size() * sizeof(int));
    require(installed == geometry.pairs, "contact union was not installed");
    bool stale = false;
    try { f.h.inspect_prepared_placement(placement); }
    catch (const std::logic_error &) { stale = true; }
    require(stale, "prior placement observation survived admission generation");
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation, parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed &&
                tick.generation == result.generation && tick.completed_ticks == ticks + 1,
            "post-admission tick did not consume installed contact generation");
  }
  require(backing.same_backing(metrics.sample()), "admission grew prepared backing");
  std::cout << "admission_commit,rounds=2,survivors=exact,claims=resolved,"
               "generation_once=1,no_tick_advance=1,following_tick=1,cpp_new=0 PASS\n";
  }
}

void admission_contact_remap(engine::Runtime &r, HostPath path) {
  const auto limits = placement_limits();
  Fixture f(r, path, {{}}, {{1, 1, 1}}, {1, 1, 1}, {1, 1, 1}, {0, 0, 0},
            52, 52, false, false, true, false, 1024, {1, 1, 1}, &limits);
  PlacementAccess::configure(f.h, f.in.particle_rows,
      {{{0, 0, 0}, 1}, {{0, 0, 0}, 1}, {{0, 0, 0}, 1}}, f.in.field_rows[0], {});
  f.install();
  RefreshRequest request(f.h);
  std::vector<UpdateCommand> commands{placement_definition(901)};
  UpdateCommand edge;
  edge.operation = UpdateOp::InsertParticipation;
  edge.first = UpdateRef::pending(0);
  edge.second = UpdateRef::existing(f.in.field_rows[0]);
  edge.share = 1;
  edge.exact_count = 1;
  edge.position = 3;
  edge.ordinal = 10001;
  commands.push_back(edge);
  request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
  const auto input = placement_intrinsic();
  const auto spec = request.spec();
  const TickSnapshot before(f);
  require(PlacementAccess::preflight(f.h, commands.data(), commands.size(),
                                      &input, 1, spec).status == UpdateStatus::Ok,
          "derived remap preparation failed");
  std::vector<int> destinations(f.h.particle_capacity() + f.h.field_capacity());
  for (int row = 0; row < f.h.particle_capacity(); ++row) destinations[row] = row;
  std::array<int, 3> rows{f.in.particle_rows[0].record,
                         f.in.particle_rows[1].record, f.in.particle_rows[2].record};
  std::sort(rows.begin(), rows.end());
  std::swap(destinations[rows[0]], destinations[rows[2]]);
  no_new([&] { PlacementAccess::remap_contacts(f.h, destinations); });
  const auto contacts = PlacementAccess::contact_inspection(f.h);
  const int c = f.h.contact_capacity();
  require(PlacementAccess::contact_counts(f.h)[3] == 3 &&
              contacts.pairs[0] == rows[0] && contacts.pairs[c] == rows[1] &&
              contacts.pairs[1] == rows[0] && contacts.pairs[c + 1] == rows[2] &&
              contacts.pairs[2] == rows[1] && contacts.pairs[c + 2] == rows[2] &&
              before == TickSnapshot(f),
          "derived contact ordering used provisional rather than resolved endpoints");
  std::cout << "admission_contact_remap,nonidentity=1,derived_order=canonical,"
               "committed_state=unchanged PASS\n";
}

void admission_commit_failures(engine::Runtime &r, HostPath path) {
  for (int failure : {2, 6}) {
    const auto limits = placement_limits();
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false,
              true, false, 1024, {1}, &limits);
    f.install();
    RefreshRequest request(f.h);
    std::vector<UpdateCommand> commands{placement_definition(901)};
    UpdateCommand edge;
    edge.operation = UpdateOp::InsertParticipation;
    edge.first = UpdateRef::pending(0);
    edge.second = UpdateRef::existing(f.in.field_rows[0]);
    edge.share = 1;
    edge.exact_count = 1;
    edge.position = 1;
    edge.ordinal = 10001;
    commands.push_back(edge);
    request.cohorts[0].push_back({UpdateRef::pending(0), {}, 1});
    UpdateCommand contact;
    contact.operation = UpdateOp::InsertContact;
    contact.first = UpdateRef::pending(0);
    contact.second = UpdateRef::existing(f.in.particle_rows[0]);
    contact.ordinal = 10002;
    commands.push_back(contact);
    const auto input = placement_intrinsic();
    const auto spec = request.spec();
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    PreparedFixtureAccess::inject_failure(f.h, failure);
    bool failed = false;
    try {
      PlacementAccess::preflight(f.h, commands.data(), commands.size(), &input,
                                  1, spec, UINT64_MAX, true);
    } catch (const std::runtime_error &) { failed = true; }
    require(failed && !f.h.ready() && f.h.prepared_storage().generation == generation &&
                f.h.prepared_storage().completed_ticks == ticks,
            "late admission failure advertised a healthy or completed generation");
    require(f.h.tick_prepared(generation, {}).status == TickStatus::Unready,
            "failed admission execution resumed");
    std::cout << "admission_commit_failure,point=" << failure << ",unready=1 PASS\n";
  }
}

void placement_generated(engine::Runtime &r, HostPath path) {
  auto limits = placement_limits();
  limits[Budget::BootstrapCommands] = limits[Budget::Batch] + 7;
  Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, 52, 52, false, false,
            true, false, 1024, {1}, &limits);
  f.install();
  const std::vector<UpdateCommand> commands{placement_definition(901),
                                            placement_definition(902)};
  // Canonical order is deliberately opposite to request/command order.
  const std::vector<PlacementIntrinsic> inputs{
      placement_intrinsic(0, 20, 0.25), placement_intrinsic(1, 10, 0.5)};
  const auto query = place(f, commands, inputs);
  require(query.status == PlacementStatus::Ok, "generated rows baseline");
  const auto expected = f.h.inspect_prepared_placement(query).certificates;
  const TickSnapshot before(f);
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  PlacementResult staged;
  for (int repeat = 0; repeat < 3; ++repeat) {
    no_new([&] {
      staged = PlacementAccess::stage(f.h, commands.data(), commands.size(),
                                      inputs.data(), inputs.size());
    });
    require(staged.status == PlacementStatus::Ok && !staged.query &&
                staged.placements == inputs.size(),
            "private placement stage published an observation");
  }
  require(backing.same_backing(metrics.sample()),
          "generated initialization allocated or compiled during reuse");
  const auto rows = PlacementAccess::generated(f.h);
  const auto stride = f.h.prepared_storage().capacity.command_capacity();
  for (const auto &certificate : expected) {
    const auto command = certificate.intrinsic.command;
    for (int field = 0; field < 12; ++field) {
      double value = 0;
      if (field >= kPosX && field <= kPosZ)
        value = certificate.position[field - kPosX];
      if (field == kMass) value = certificate.intrinsic.mass;
      if (field == kRadius) value = certificate.intrinsic.radius;
      require(rows[field * stride + command] == value,
              "native start differs from certificate or has nonzero motion");
    }
  }
  // A later shorter stage clears earlier rows, without replacing the B2 query.
  no_new([&] {
    staged = PlacementAccess::stage(f.h, commands.data(), 1, inputs.data(), 1);
  });
  require(staged.status == PlacementStatus::Ok, "shorter generated stage");
  const auto shorter = PlacementAccess::generated(f.h);
  for (int field = 0; field < 12; ++field)
    for (std::size_t row = 1; row < stride; ++row)
      require(shorter[field * stride + row] == 0, "stale generated row survived");
  auto invalid = inputs[0];
  invalid.geometry.complete = false;
  staged = PlacementAccess::stage(f.h, commands.data(), 1, &invalid, 1);
  require(staged.status == PlacementStatus::InvalidInput && f.h.ready(),
          "invalid stage was not rejected healthily");
  const auto &observed = f.h.inspect_prepared_placement(query).certificates;
  require(observed.size() == expected.size() &&
              observed[0].position == expected[0].position &&
              observed[1].position == expected[1].position &&
              before == TickSnapshot(f),
          "private staging changed committed state or public observation");
  std::cout << "placement_generated,native_rows=1,zero_motion=1,canonical_remap=1,"
               "private_observation=1,survivors_preserved=1,reuse=1 PASS\n";
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
    auto bootstrap = l;
    bootstrap[Budget::BootstrapCommands] = l[Budget::Batch] + 17;
    require(detail::PreparedStorage::required_scratch(
                bootstrap, false, NumericalProfile::trial()) >
                detail::PreparedStorage::required_scratch(
                    l, false, NumericalProfile::trial()),
            "bootstrap backing was omitted from scratch accounting");
    Harness h(r, PreparedCapacity(bootstrap, NumericalProfile::trial()), path);
    const auto command = placement_definition();
    const auto intrinsic = placement_intrinsic();
    const auto oversized = h.update_prepared(
        0, &command, l[Budget::Batch] + 1);
    require(oversized.status == UpdateStatus::CapacityExhausted &&
                oversized.failure.budget == Budget::Batch,
            "bootstrap backing widened ordinary update admission");
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
