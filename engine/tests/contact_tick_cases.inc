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
