void exchange_owner_multiple(engine::Runtime &r, HostPath path) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using Plan = assembly::testing::ExchangeCoordinatorAccess;
  using assembly::GroupKind;
  letter_fixture::Fixture letters;
  const auto group = letters.add(letter_fixture::object("02.00.00.00.00"), "group", 0);
  letters.base.tools.at(letters.tools.at(letters.accented)).member_of = {group};
  letters.base.focal_fields = {{warm::FieldKind::Membership,group.object}};
  const auto recipe = assembly::normalize_structural_letters(letters.base,
      {{0,letters.ref(letters.accented),warm::Exposure::Aggregate},
       {1,letters.ref(letters.accented),warm::Exposure::Aggregate},
       {2,letters.ref(letters.accented),warm::Exposure::Parts},
       {3,letters.ref(letters.accented),warm::Exposure::Parts}});
  const auto initial = assembly::project_letters(recipe);
  auto limits = placement_limits();
  Fixture f(r,path,{{},{},{0,1},{},{},{3,4},{},{6},{2,5,7}},
      {{0,0,1,0,1,0},{0,0,1,0,1,0},{0,0,2,0,2,0},
       {0,0,0,1,0,1},{0,0,0,1,0,1},{0,0,0,2,0,2},
       {4,4,0,0,0,0},{4,4,0,0,0,0},{4,4,2,2,2,2}},
      {4,4,2,2,2,2},{4,4,2,2,2,2},{0,20,40,60,80,100},
      52,52,false,true,true,false,1024,{},&limits,true);
  f.install();
  std::vector<assembly::detail::OccurrenceBinding> rows;
  for (const auto &o : initial.occurrences()) if (o.active) {
    const auto index = o.key.source.entry < 2 ? o.key.source.entry
        : 2 + 2 * (o.key.source.entry-2) + o.key.source.path[1];
    rows.push_back({o.key,f.in.particle_rows[index]});
  }
  const auto character = letters.base.tools.at(letters.tools.at(letters.accented)).parents[0].target;
  const auto &ct = letters.base.tools.at(letters.tools.at(character.object));
  std::vector<assembly::detail::ExchangeFieldBinding> fields;
  for (int i = 0; i < 2; ++i) {
    const auto &bt = letters.base.tools.at(letters.tools.at(ct.parents[i].target.object));
    for (int j = 0; j < 2; ++j)
      fields.push_back({{GroupKind::ParentValue,bt.parents[j].target.object},f.in.field_rows[3*i+j]});
    fields.push_back({{GroupKind::SiblingDefinition,bt.ref.object},f.in.field_rows[3*i+2]});
  }
  fields.push_back({{GroupKind::ParentValue,character.object},f.in.field_rows[6]});
  fields.push_back({{GroupKind::SiblingDefinition,letters.accented},f.in.field_rows[7]});
  fields.push_back({{GroupKind::Membership,group.object},f.in.field_rows[8]});
  auto owner = Access::adopt(f.h,initial,rows,fields);
  const auto *reserved_plan = &Access::plan(*owner);
  const auto plan_backing = Plan::backing(*reserved_plan);
  const auto reserved = Access::storage(*owner);
  require(reserved.parent_slots == 40, "multiple owner omitted inactive parent storage");
  prepared_test::Metrics metrics(r,PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  const auto same = [](auto a, auto b) {
    return a.context == b.context && a.address == b.address && a.record == b.record && a.incarnation == b.incarnation;
  };
  for (int round = 0; round < 32; ++round) {
    const std::uint64_t entry = round % 2 ? 2 : 0;
    const auto *before_view = &Access::projection(*owner);
    const auto target = before_view->exposure(entry) == warm::Exposure::Aggregate
        ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    const assembly::OccurrenceKey key{initial.study(),{entry,{}}};
    const auto declaration = assembly::declare_letter_exchange(*before_view,key,target);
    std::vector<assembly::detail::ExchangeIncoming> incoming;
    for (const auto &source : declaration.incoming())
      incoming.push_back({source.occurrence,source.definition,source.node,1,
                          int(900+10*entry+(target == warm::Exposure::Parts ? 0 : 4)+incoming.size()),0.5,{1,1,true}});
    const auto old_rows = Access::mappings(*owner);
    const auto old_outgoing = Access::outgoing_report(*owner);
    const auto old_balance = Access::balance_report(*owner);
    const auto generation = f.h.prepared_storage().generation, ticks = f.h.prepared_storage().completed_ticks;
    f.h.inspect_prepared();
    const auto old_values = f.h.particles64();
    no_new([&] { Access::stage_projection(*owner,key,target); });
    require(&Access::projection(*owner) == before_view &&
                Access::storage(*owner).charged_bytes() == reserved.charged_bytes(),
            "multi-occurrence projection preparation published or grew storage");
    Access::prepare(*owner,key,target,incoming,{});
    require(&Access::plan(*owner) == reserved_plan && Plan::backing(*reserved_plan) == plan_backing,
            "multi-occurrence preparation replaced transaction backing");
    const auto &commands = Plan::commands(Access::plan(*owner));
    RefreshRequest request(f.h);
    for (std::size_t node = 0; node < request.nodes.size(); ++node) {
      auto &cohort = request.cohorts[node];
      cohort.erase(std::remove_if(cohort.begin(),cohort.end(),[&](const auto &ref) {
        return std::any_of(commands.begin(),commands.end(),[&](const auto &c) {
          return c.operation == UpdateOp::RemoveParticipation && c.row == ref.source.row;
        });
      }),cohort.end());
      for (std::size_t i = 0; i < commands.size(); ++i)
        if (commands[i].operation == UpdateOp::InsertParticipation &&
            commands[i].second.handle.record == request.nodes[node].field.record)
          cohort.push_back({commands[i].first,{},int(i)});
    }
    no_new([&] { Access::seal(*owner,request.spec()); });
    require(Plan::backing(*reserved_plan) == plan_backing,
            "multi-occurrence sealing replaced transaction backing");
    const auto before = TickSnapshot(f);
    UpdateResult result;
    PreparedFixtureAccess::inject_failure(f.h,1);
    no_new([&] { result = Access::execute(*owner,generation); });
    require(result.status == UpdateStatus::InvalidInput && before == TickSnapshot(f) &&
                &Access::projection(*owner) == before_view && Access::outgoing_report(*owner) == old_outgoing &&
                Access::balance_report(*owner) == old_balance, "multi-occurrence rejection changed committed state");
    PreparedFixtureAccess::inject_failure(f.h,0);
    no_new([&] { result = Access::execute(*owner,generation); });
    require(result.status == UpdateStatus::Ok && result.generation == generation+1 &&
                f.h.prepared_storage().completed_ticks == ticks, "multi-occurrence exchange failed");
    const auto &current = Access::projection(*owner);
    require(current.study() == initial.study() && current.exposure(entry) == target &&
                current.exposure(1) == warm::Exposure::Aggregate && current.exposure(3) == warm::Exposure::Parts,
            "multi-occurrence owner changed independent exposure");
    for (std::uint64_t e : {0u,1u,2u,3u}) if (e != entry)
      require(current.exposure(e) == before_view->exposure(e), "owner lost another edited occurrence's exposure");
    f.h.inspect_prepared();
    for (const auto &v : Access::mappings(*owner)) {
      require(same(v.handle,f.h.prepared_storage().particle_headers[v.handle.record]), "owner map lost native claim");
      if (v.occurrence.source.entry == entry) continue;
      const auto old = std::find_if(old_rows.begin(),old_rows.end(),[&](const auto &p) { return p.occurrence == v.occurrence; });
      require(old != old_rows.end() && same(old->handle,v.handle), "independent occurrence handle changed");
      for (int k = 0; k < kParticleFieldCount; ++k)
        require(f.h.particle64(k,v.handle.record) == old_values[k*f.h.particle_capacity()+v.handle.record],
                "independent occurrence numerical state changed");
    }
    for (const auto &v : fields) {
      const auto current_field = std::find_if(Access::fields(*owner).begin(),Access::fields(*owner).end(),
          [&](const auto &x) { return x.field.kind == v.field.kind && x.field.object == v.field.object; });
      require(current_field != Access::fields(*owner).end() && same(current_field->handle,v.handle),
              "shared field or focal identity changed");
    }
    std::size_t parent_count = 0, active = 0;
    for (const auto &o : current.occurrences()) if (o.active) {
      ++active;
      const auto &tool = letters.base.tools.at(letters.tools.at(o.definition.object));
      parent_count += tool.parents.size();
      for (const auto &p : tool.parents)
        require(std::count_if(current.parents().begin(),current.parents().end(),[&](const auto &v) {
          return v.occurrence == o.key && v.ordinal == p.ordinal && v.field.object == p.target.object && v.count == p.contribution;
        }) == 1, "reused parent pool lost or duplicated a contribution");
    }
    require(parent_count == current.parents().size() && active == Access::mappings(*owner).size() &&
                assembly::membership_fields(current).contributions().size() == active,
            "multi-occurrence contribution inventory differs from exposure");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation,parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed && backing.same_backing(metrics.sample()) &&
                Access::storage(*owner).charged_bytes() == reserved.charged_bytes(),
            "multi-occurrence tick failed or retained storage grew");
  }
  std::cout << "exchange_owner_multiple,occurrences=4,rounds=32,edited=2,survivors=preserved,"
               "shared_fields=preserved,retry=1,projection_prepare_cpp_new=0,backing=stable PASS\n";
  std::cout << "projection_storage_multiple,parent_slots=" << reserved.parent_slots
            << ",charge=" << reserved.charged_bytes() << '\n';
}
