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
  auto l = placement_limits();
  l[Budget::Batch] = 4;
  l[Budget::BootstrapCommands] = 16;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      l, false, NumericalProfile::trial()) + 256 * 1024 * 1024;
  Harness h(r, PreparedCapacity(l, NumericalProfile::trial()), path);
  std::vector<UpdateCommand> commands;
  for (int i = 0; i < 3; ++i)
    commands.push_back(placement_definition(900 + i));
  for (int i = 0; i < 2; ++i) {
    UpdateCommand field;
    field.operation = UpdateOp::DefineField;
    field.kind = 1;
    field.address = i;
    commands.push_back(field);
  }
  for (int i = 0; i < 3; ++i) {
    UpdateCommand edge;
    edge.operation = UpdateOp::InsertParticipation;
    edge.first = UpdateRef::pending(i);
    edge.second = UpdateRef::pending(i == 2 ? 3 : 4);
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
  inputs[1].mass = 2;
  std::vector<LiveRefreshNode> nodes(2);
  for (int i = 0; i < 2; ++i) {
    nodes[i].field_command = 3 + i;
    nodes[i].comparison = 1 + i;
    nodes[i].cohort_begin = i;
    nodes[i].cohort_count = i == 0 ? 1 : 2;
  }
  std::vector<LiveRefreshContribution> members{
      {UpdateRef::pending(2), {}, 7}, {UpdateRef::pending(0), {}, 5},
      {UpdateRef::pending(1), {}, 6}};
  LiveRefreshSpec spec{nodes.data(), nodes.size(), nullptr, 0,
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
  UpdateResult preflight;
  no_new([&] { preflight = PlacementAccess::bootstrap(h, commands, inputs, spec, focus, false); });
  std::cout << "bootstrap_preflight,status=" << int(preflight.status)
            << ",budget=" << budget_name(preflight.failure.budget) << std::endl;
  require(preflight.status == UpdateStatus::Ok, "bootstrap preflight failed");
  unchanged();
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
    placement_reference::certificate(certificate, occupancy, target, 52);
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
  std::cout << "admission_bootstrap,explicit_focus=2,anchor_command=2,commands=8,batch=4,"
               "partial_centroid=certified,preclaim_rejection=unchanged,publication=one,"
               "backing=stable,following_tick=pass PASS\n";
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
