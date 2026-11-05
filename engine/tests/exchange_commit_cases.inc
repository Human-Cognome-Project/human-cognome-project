void exchange_commit(engine::Runtime &r, HostPath path) {
  using Bind = assembly::testing::ExchangeBindingAccess;
  using Access = assembly::testing::ExchangeCoordinatorAccess;
  // Both directions, peak exhaustion, and failures across the mutation boundary.
  for (int scenario : {0, 1, -1, 2, 3, 6}) {
    const bool compression = scenario == 1;
    const int bits = compression ? 8 : 52;
    letter_fixture::Fixture letters;
    const auto group = letters.add(letter_fixture::object("02.00.00.00.00"), "group", 0);
    letters.base.tools.at(letters.tools.at(letters.english)).member_of = {group};
    letters.base.focal_fields = {{warm::FieldKind::Membership, group.object}};
    const auto recipe = assembly::normalize_structural_letters(letters.base,
        {{0, letters.ref(letters.english), warm::Exposure::Aggregate},
         {1, letters.ref(letters.english), warm::Exposure::Aggregate},
         {2, letters.ref(letters.english), warm::Exposure::Parts}});
    const auto projection = assembly::project_letters(recipe);
    auto l = placement_limits();
    if (scenario == -1) l[Budget::Particles] = 3;
    Fixture f(r, path, {{}, {}, {0,1}, {}, {3}, {2,4}},
        {{0,0,1}, {0,0,1}, {0,0,2}, {2,2,0}, {2,2,0}, {2,2,2}},
        {2,2,2}, {2,2,2}, {0,20,40}, bits,52,false,true,true,false,1024,{},&l,true);
    f.install();
    f.control->set(1, f.in.field_rows[0].record);
    LiveHierarchyAccess::poll(f.h, 0, f.in.field_rows[0].record);
    f.control->set(4);
    const auto byte = letters.base.tools.at(letters.tools.at(letters.english)).parents[0].target;
    const auto &byte_tool = letters.base.tools.at(letters.tools.at(byte.object));
    std::vector<assembly::detail::OccurrenceBinding> rows{
        {{projection.study(), {2,{0,0}}}, f.in.particle_rows[2]},
        {{projection.study(), {0,{}}}, f.in.particle_rows[0]},
        {{projection.study(), {1,{}}}, f.in.particle_rows[1]}};
    std::vector<assembly::detail::ExchangeFieldBinding> fields{
        {{assembly::GroupKind::SiblingDefinition, letters.english}, f.in.field_rows[3]},
        {{assembly::GroupKind::ParentValue, byte.object}, f.in.field_rows[4]},
        {{assembly::GroupKind::SiblingDefinition, byte.object}, f.in.field_rows[2]},
        {{assembly::GroupKind::ParentValue, byte_tool.parents[0].target.object}, f.in.field_rows[0]},
        {{assembly::GroupKind::ParentValue, byte_tool.parents[1].target.object}, f.in.field_rows[1]},
        {{assembly::GroupKind::Membership, group.object}, f.in.field_rows[5]}};
    const auto declaration = assembly::declare_letter_exchange(projection,
        {projection.study(), {compression ? 2u : 0u,{}}},
        compression ? warm::Exposure::Aggregate : warm::Exposure::Parts);
    std::vector<assembly::detail::ExchangeIncoming> incoming;
    for (const auto &source : declaration.incoming())
      incoming.push_back({source.occurrence, source.definition, source.node, 1,
                          int(900 + incoming.size()), 0.5, {1,1,true}});
    const auto prepare = [&](const auto &inputs) {
      return Access::prepare(Bind::prepare(f.h, projection, declaration, rows, fields), inputs);
    };
    if (scenario == 0) {
      auto wrong = incoming;
      wrong[0].occurrence.source.entry = 1;
      bool rejected = false;
      try { prepare(wrong); } catch (const warm::Error &) { rejected = true; }
      require(rejected, "foreign incoming alternative accepted");
      wrong = incoming; wrong[0].definition = letters.ref(letters.french);
      rejected = false;
      try { prepare(wrong); } catch (const warm::Error &) { rejected = true; }
      require(rejected, "different incoming definition accepted");
    }
    auto coordinator = prepare(incoming);
    const auto &commands = Access::commands(*coordinator);
    RefreshRequest request(f.h);
    for (std::size_t node = 0; node < request.nodes.size(); ++node) {
      auto &cohort = request.cohorts[node];
      cohort.erase(std::remove_if(cohort.begin(), cohort.end(), [&](const auto &ref) {
        return std::any_of(commands.begin(), commands.end(), [&](const auto &c) {
          return c.operation == UpdateOp::RemoveParticipation && c.row == ref.source.row;
        });
      }), cohort.end());
      for (std::size_t c = 0; c < commands.size(); ++c)
        if (commands[c].operation == UpdateOp::InsertParticipation &&
            commands[c].second.handle.record == request.nodes[node].field.record)
          cohort.push_back({commands[c].first, {}, int(c)});
    }
    Access::seal(*coordinator, request.spec());
    const auto outgoing = f.in.particle_rows[compression ? 2 : 0];
    const std::array<double,3> velocity{0.125,-0.375,0.625};
    for (int k = 0; k < 3; ++k)
      PreparedFixtureAccess::inject_value64(f.h, kVelX + k, outgoing.record, velocity[k]);
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    const auto before = TickSnapshot(f);
    const auto history = PlacementAccess::rebase_history(f.h);
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    const auto run = [&](std::uint64_t work = UINT64_MAX, std::uint64_t hierarchy = UINT64_MAX) {
      UpdateResult result;
      no_new([&] { result = Access::execute(*coordinator, generation, work, UINT64_MAX, hierarchy); });
      return result;
    };
    if (scenario < 2) {
      require(Access::execute(*coordinator, generation + 1).status == UpdateStatus::Stale,
              "exchange accepted stale generation");
      require(run(Access::work(*coordinator) - 1).status == UpdateStatus::CapacityExhausted,
              "exchange ignored combined work reservation");
      if (scenario != -1) {
        require(run(UINT64_MAX, 1).status == UpdateStatus::CapacityExhausted,
                "exchange ignored prospective hierarchy work");
        PreparedFixtureAccess::inject_failure(f.h, 1);
        require(run().status == UpdateStatus::InvalidInput, "exchange ignored preclaim rejection");
        PreparedFixtureAccess::inject_failure(f.h, 0);
      }
      require(before == TickSnapshot(f) && history == PlacementAccess::rebase_history(f.h) &&
                  f.h.ready() && Access::generation(*coordinator) == generation,
              "ordinary exchange rejection changed state/history/mappings");
    }
    if (scenario > 1) {
      PreparedFixtureAccess::inject_failure(f.h, scenario);
      bool failed = false;
      try { run(); } catch (const std::runtime_error &) { failed = true; }
      require(failed && !f.h.ready() && Access::generation(*coordinator) == generation &&
                  Access::execute(*coordinator, generation).status == UpdateStatus::Unready,
              "late exchange failure left healthy execution or published owner mapping");
      continue;
    }
    const auto result = run();
    if (scenario == -1) {
      require(result.status == UpdateStatus::CapacityExhausted &&
                  result.failure.budget == Budget::Particles && result.failure.required == 4 &&
                  before == TickSnapshot(f) && f.h.ready(), "retirement funded replacement claim");
      continue;
    }
    require(result.status == UpdateStatus::Ok && result.generation == generation + 1 &&
                f.h.prepared_storage().completed_ticks == ticks &&
                Access::generation(*coordinator) == result.generation,
            "exchange did not publish one coherent unticked generation");
    const auto &s = f.h.prepared_storage();
    require(s.particle_headers[outgoing.record].record < 0 && s.live.particles == 3,
            "outgoing body not retired with replacement");
    f.h.inspect_prepared();
    for (const auto &mapping : Access::mappings(*coordinator)) {
      const bool replacement = mapping.occurrence == declaration.incoming()[0].occurrence;
      require(mapping.handle.context && mapping.handle.incarnation &&
                  mapping.handle.address == s.particle_headers[mapping.handle.record].address,
              "mapping does not resolve actual native claim");
      if (replacement) {
        require(mapping.handle.record != outgoing.record &&
                    s.source_masses[mapping.handle.record] == 2, "replacement reused outgoing claim");
        for (int k = 0; k < 3; ++k)
          require(f.h.particle64(kVelX + k, mapping.handle.record) == velocity[k],
                  "exchange commit lost transferred motion");
      } else {
        const auto old = std::find_if(rows.begin(), rows.end(), [&](const auto &v) {
          return v.occurrence == mapping.occurrence;
        });
        require(old != rows.end() && old->handle.record == mapping.handle.record &&
                    old->handle.incarnation == mapping.handle.incarnation,
                "unrelated occurrence mapping changed");
      }
    }
    require(Access::execute(*coordinator, result.generation).status == UpdateStatus::Stale,
            "consumed exchange transaction replayed");
    require(backing.same_backing(metrics.sample()), "exchange grew native backing");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation, parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed && tick.completed_ticks == ticks + 1,
            "following tick rejected exchange contact/hierarchy generation");
  }
  std::cout << "exchange_commit,compression=1,expansion=1,source_generated=1,mappings=atomic,"
               "generation_once=1,unticked=1,peak_claims=1,rejection_retry=1,late_unready=1,"
               "following_tick=1,cpp_new=0 PASS\n";
}
