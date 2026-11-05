void exchange_fields(engine::Runtime &r, HostPath path) {
  using Bind = assembly::testing::ExchangeBindingAccess;
  using Access = assembly::testing::ExchangeCoordinatorAccess;
  using assembly::GroupKind;
  for (int scenario : {0, 1, 2, 3, 4}) {
    const bool compression = scenario == 1;
    letter_fixture::Fixture letters;
    const auto group = letters.add(letter_fixture::object("02.00.00.00.00"), "group", 0);
    letters.base.tools.at(letters.tools.at(letters.accented)).member_of = {group};
    letters.base.focal_fields = {{warm::FieldKind::Membership, group.object}};
    const auto recipe = assembly::normalize_structural_letters(letters.base,
        {{0, letters.ref(letters.accented), compression ? warm::Exposure::Parts : warm::Exposure::Aggregate}});
    const auto projection = assembly::project_letters(recipe);
    const auto character = letters.base.tools.at(letters.tools.at(letters.accented)).parents[0].target;
    const auto &character_tool = letters.base.tools.at(letters.tools.at(character.object));
    std::vector<assembly::FieldKey> keys;
    for (const auto &parent : character_tool.parents) {
      const auto &byte = letters.base.tools.at(letters.tools.at(parent.target.object));
      for (const auto &nibble : byte.parents) keys.push_back({GroupKind::ParentValue, nibble.target.object});
      keys.push_back({GroupKind::SiblingDefinition, byte.ref.object});
    }
    keys.push_back({GroupKind::ParentValue, character.object});
    keys.push_back({GroupKind::SiblingDefinition, letters.accented});
    keys.push_back({GroupKind::Membership, group.object});
    auto limits = placement_limits();
    if (scenario == 2) limits[Budget::Fields] = 7; // Final seven fits; peak nine does not.
    const std::vector<int> old_ids = compression ? std::vector<int>{0,1,2,3,4,5,8}
                                                : std::vector<int>{6,7,8};
    const std::vector<int> next_ids = compression ? std::vector<int>{6,7,8}
                                                 : std::vector<int>{0,1,2,3,4,5,8};
    Fixture f(r, path,
        compression ? std::vector<std::vector<int>>{{},{},{0,1},{},{},{3,4},{2,5}}
                    : std::vector<std::vector<int>>{{},{0},{1}},
        compression ? std::vector<std::vector<std::uint64_t>>{{1,0},{1,0},{2,0},{0,1},{0,1},{0,2},{2,2}}
                    : std::vector<std::vector<std::uint64_t>>{{4},{4},{4}},
        compression ? std::vector<std::uint64_t>{2,2} : std::vector<std::uint64_t>{4},
        compression ? std::vector<double>{2,2} : std::vector<double>{4},
        compression ? std::vector<double>{-4,4} : std::vector<double>{0},
        compression ? 8 : 52,52,false,false,true,false,1024,{},&limits,true);
    f.install();
    if (scenario == 3)
      require(PlacementAccess::focus(f.h, {f.in.field_rows[0]}).status == UpdateStatus::Ok,
              "cannot prepare focal retirement witness");
    std::vector<assembly::detail::OccurrenceBinding> rows;
    for (const auto &o : projection.occurrences()) if (o.active)
      rows.push_back({o.key, f.in.particle_rows[compression ? o.key.source.path[1] : 0]});
    std::vector<assembly::detail::ExchangeFieldBinding> fields;
    for (std::size_t i = 0; i < old_ids.size(); ++i) fields.push_back({keys[old_ids[i]], f.in.field_rows[i]});
    const auto declaration = assembly::declare_letter_exchange(projection,
        {projection.study(), {0,{}}}, compression ? warm::Exposure::Aggregate : warm::Exposure::Parts);
    std::vector<assembly::detail::ExchangeIncoming> incoming;
    for (const auto &source : declaration.incoming())
      incoming.push_back({source.occurrence, source.definition, source.node, 1,
                          int(900 + incoming.size()), 0.5, {1,1,true}});
    std::vector<assembly::detail::ExchangeNewField> additions;
    for (int id : next_ids) if (id != 8) additions.push_back({keys[id], 1, 50 + id});
    const auto prepare = [&](const auto &added) {
      return Access::prepare(Bind::prepare(f.h, projection, declaration, rows, fields), incoming, added);
    };
    if (scenario == 0) {
      auto bad = additions; bad.push_back(additions[0]);
      bool rejected = false;
      try { prepare(bad); } catch (const warm::Error &) { rejected = true; }
      require(rejected, "duplicate new field accepted");
      bad = additions; bad.pop_back(); rejected = false;
      try { prepare(bad); } catch (const warm::Error &) { rejected = true; }
      require(rejected, "missing incoming field accepted");
      bad = additions; bad.push_back({{GroupKind::SiblingDefinition, letters.french},1,60}); rejected = false;
      try { prepare(bad); } catch (const warm::Error &) { rejected = true; }
      require(rejected, "extra undeclared field accepted");
    }
    auto coordinator = prepare(additions);
    const auto &commands = Access::commands(*coordinator);
    std::vector<LiveRefreshNode> nodes;
    std::vector<int> children;
    std::vector<LiveRefreshContribution> contributions;
    const auto &s = f.h.prepared_storage();
    for (int id : next_ids) {
      const auto &planned = Access::planned_fields(*coordinator);
      const auto where = std::find_if(planned.begin(), planned.end(), [&](const auto &v) {
        return v.field.kind == keys[id].kind && v.field.object == keys[id].object;
      });
      require(where != planned.end(), "missing prospective field mapping");
      const auto command = Access::field_commands(*coordinator)[where - planned.begin()];
      LiveRefreshNode node;
      node.field = where->handle;
      node.field_command = command;
      node.comparison = id == 8 ? f.nodes.back().comparison : 100 + id;
      node.child_begin = children.size();
      std::vector<int> child_ids;
      if (id == 2) child_ids = {0,1};
      if (id == 5) child_ids = {3,4};
      if (id == 7) child_ids = {6};
      if (id == 8) child_ids = compression ? std::vector<int>{7} : std::vector<int>{2,5};
      for (int child : child_ids) {
        const auto index = int(std::find(next_ids.begin(), next_ids.end(), child) - next_ids.begin());
        children.push_back(index);
        node.level = std::max(node.level, nodes[index].level + 2);
      }
      node.child_count = child_ids.size();
      node.cohort_begin = contributions.size();
      for (std::size_t row = 0; row < s.live.edges; ++row) {
        const bool removed = std::any_of(commands.begin(), commands.end(), [&](const auto &c) {
          return c.operation == UpdateOp::RemoveParticipation && c.row == row;
        });
        if (!removed && command < 0 && s.edges[row].second.record == node.field.record)
          contributions.push_back({UpdateRef::existing(s.edges[row].first),
                                    {s.generation,row,s.edges[row].ordinal},-1});
      }
      for (std::size_t i = 0; i < commands.size(); ++i) {
        const auto &c = commands[i];
        if (c.operation == UpdateOp::InsertParticipation &&
            (command < 0 ? c.second.command < 0 && c.second.handle.record == node.field.record
                         : c.second.command == command))
          contributions.push_back({c.first, {}, int(i)});
      }
      node.cohort_count = contributions.size() - node.cohort_begin;
      nodes.push_back(node);
    }
    Access::seal(*coordinator, {nodes.data(),nodes.size(),children.data(),children.size(),
                                contributions.data(),contributions.size()});
    const auto generation = s.generation, ticks = s.completed_ticks;
    const auto before = TickSnapshot(f);
    const auto history = PlacementAccess::rebase_history(f.h);
    prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
    const auto backing = metrics.sample();
    const auto run = [&] {
      UpdateResult result;
      no_new([&] { result = Access::execute(*coordinator, generation); });
      return result;
    };
    if (scenario == 4) {
      PreparedFixtureAccess::inject_failure(f.h, 7);
      bool failed = false;
      try { run(); } catch (const std::runtime_error &) { failed = true; }
      require(failed && !f.h.ready() && Access::generation(*coordinator) == generation &&
                  Access::fields(*coordinator).size() == fields.size(),
              "late field mapping failure left healthy partial owner");
      continue;
    }
    if (scenario < 2) {
      PreparedFixtureAccess::inject_failure(f.h, 1);
      require(run().status == UpdateStatus::InvalidInput && before == TickSnapshot(f) &&
                  history == PlacementAccess::rebase_history(f.h), "field lifecycle rejection mutated execution");
      PreparedFixtureAccess::inject_failure(f.h, 0);
    }
    const auto result = run();
    std::cout << "exchange_fields_attempt,scenario=" << scenario << ",status=" << int(result.status)
              << ",budget=" << budget_name(result.failure.budget) << ",required=" << result.failure.required << std::endl;
    if (scenario >= 2) {
      require(result.status == (scenario == 2 ? UpdateStatus::CapacityExhausted : UpdateStatus::InvalidInput) &&
                  before == TickSnapshot(f) && history == PlacementAccess::rebase_history(f.h) && f.h.ready(),
              "field peak or focal retirement rejection mutated execution");
      if (scenario == 2) require(result.failure.budget == Budget::Fields && result.failure.required == 9,
                                 "retired field capacity funded new claims");
      continue;
    }
    require(result.status == UpdateStatus::Ok && result.generation == generation + 1 &&
                s.completed_ticks == ticks && s.live.fields == next_ids.size() &&
                Access::fields(*coordinator).size() == next_ids.size(),
            "field lifecycle did not publish complete unticked state");
    for (std::size_t i = 0; i + 1 < fields.size(); ++i)
      require(s.field_headers[fields[i].handle.record].record < 0, "unused outgoing field survived");
    for (const auto &field : Access::fields(*coordinator)) {
      const auto &actual = s.field_headers[field.handle.record];
      require(field.handle.context == actual.context && field.handle.address == actual.address &&
                  field.handle.incarnation == actual.incarnation, "field map lost actual native claim");
      if (field.field.kind == GroupKind::Membership)
        require(field.handle.record == fields.back().handle.record &&
                    field.handle.incarnation == fields.back().handle.incarnation,
                "retained membership/focal field was replaced");
      else for (const auto &old : fields)
        require(field.handle.record != old.handle.record, "new field reused outgoing claim");
    }
    require(backing.same_backing(metrics.sample()), "field exchange grew native backing");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation, parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed,
            "following tick rejected new field hierarchy");
  }
  std::cout << "exchange_fields,create=1,retire=1,retained_focus=1,field_claim_peak=1,"
               "focal_retirement_rejected=1,mappings=atomic,retry=1,late_unready=1,"
               "following_tick=1,backing=stable,cpp_new=0 PASS\n";
}
