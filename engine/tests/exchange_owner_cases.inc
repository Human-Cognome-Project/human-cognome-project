void exchange_owner(engine::Runtime &r, HostPath path) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using Plan = assembly::testing::ExchangeCoordinatorAccess;
  using assembly::GroupKind;
  letter_fixture::Fixture letters;
  const auto group = letters.add(letter_fixture::object("02.00.00.00.00"), "group", 0);
  letters.base.tools.at(letters.tools.at(letters.accented)).member_of = {group};
  letters.base.focal_fields = {{warm::FieldKind::Membership, group.object}};
  const auto recipe = assembly::normalize_structural_letters(letters.base,
      {{0, letters.ref(letters.accented), warm::Exposure::Aggregate}});
  const auto initial = assembly::project_letters(recipe);
  const assembly::OccurrenceKey key{initial.study(), {0,{}}};
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
  Fixture f(r,path,{{},{0},{1}},{{4},{4},{4}},{4},{4},{0},52,52,
            false,false,true,false,1024,{},&limits,true);
  f.install();
  std::vector<assembly::detail::OccurrenceBinding> rows{{key, f.in.particle_rows[0]}};
  std::vector<assembly::detail::ExchangeFieldBinding> fields;
  for (int i = 0; i < 3; ++i) fields.push_back({keys[6+i], f.in.field_rows[i]});
  owner_lifetime(f,initial,rows,fields);
  {
    auto probe = Access::adopt(f.h,initial,rows,fields);
    const auto before_charge = Access::storage(*probe);
    // Test-only capacity probe: preserve every immutable address value while
    // adding unused reservation in one recipe copy. No native state is changed.
    auto &address = const_cast<warm::Bundle &>(Access::projection(*probe).recipe()).tools[0].ref.object.address;
    const auto old_capacity = address.capacity();
    address.reserve(old_capacity+17);
    const auto after_charge = Access::storage(*probe);
    require(after_charge.charged_bytes() - before_charge.charged_bytes() ==
                (address.capacity()-old_capacity)*sizeof(codec::AddressElement) &&
                after_charge.text_bytes == before_charge.text_bytes &&
                after_charge.object_bytes == before_charge.object_bytes,
            "projection census omitted nested object-address capacity");
  }
  auto owner = Access::adopt(f.h, initial, rows, fields);
  {
    auto probe = Access::adopt(f.h,initial,rows,fields);
    auto before = Access::resources(*probe);
    auto &path = Access::inactive_path(*probe);
    const auto capacity = path.capacity();
    path.reserve(capacity+17);
    auto after = Access::resources(*probe);
    require(after.total().charged_bytes()-before.total().charged_bytes() ==
                (path.capacity()-capacity)*sizeof(std::uint32_t) &&
                after.maps.vector_bytes-before.maps.vector_bytes ==
                (path.capacity()-capacity)*sizeof(std::uint32_t),
            "owner census omitted inactive nested map capacity");
    before = after;
    auto &commands = Access::field_command_buffer(*probe);
    const auto command_capacity = commands.capacity();
    commands.reserve(command_capacity+17);
    after = Access::resources(*probe);
    require(after.total().charged_bytes()-before.total().charged_bytes() ==
                (commands.capacity()-command_capacity)*sizeof(int),
            "owner census omitted field-command capacity");
    auto overflow = after;
    overflow.owner.object_bytes = UINT64_MAX;
    bool rejected = false;
    try { overflow.total(); } catch (const warm::Error &) { rejected = true; }
    require(rejected && overflow.owner.object_bytes == UINT64_MAX,
            "owner aggregate overflow accepted or mutated report");
  }
  assembly::detail::OwnerStorage resources;
  no_new([&] { resources = Access::resources(*owner); });
  require(resources.shared_allocations == 3 && resources.maps.vector_bytes &&
              resources.binding.vector_bytes && resources.coordinator.vector_bytes &&
              resources.declarations.vector_bytes && resources.memberships.vector_bytes &&
              resources.total().charged_bytes() > Access::storage(*owner).charged_bytes(),
          "owner census omitted retained component families");
  {
    // Bounded map behavior independent of native claims. Clearing/erasing keeps
    // nested capacity; oversized keys and full stores reject without appending.
    assembly::detail::ExchangeMap<assembly::detail::OccurrenceBinding> map;
    map.reserve(2,2);
    auto nested = key;
    nested.source.path = {3,5};
    no_new([&] {
      map.push_key(key,rows[0].handle);
      map.push_key(nested,rows[0].handle);
      map.erase(map.begin());
      map.push_key(key,rows[0].handle);
    });
    require(map.size() == 2 && map[0].occurrence == nested && map[1].occurrence == key,
            "map erasure changed logical order");
    bool rejected = false;
    try { map.push_key(key,rows[0].handle); } catch (const warm::Error &) { rejected = true; }
    require(rejected && map.size() == 2, "full identity map appended a row");
    rejected = false;
    try { assembly::detail::exchange_map_capacity(SIZE_MAX,1); }
    catch (const warm::Error &) { rejected = true; }
    require(rejected, "candidate map reservation overflow accepted");
    map.clear();
    nested.source.path.resize(3);
    rejected = false;
    try { map.push_key(nested,rows[0].handle); } catch (const warm::Error &) { rejected = true; }
    require(rejected && map.size() == 0, "oversized identity grew map backing");
    auto copy = map;
    nested.source.path.resize(2);
    no_new([&] { copy.push_key(nested,rows[0].handle); map = copy; });
    require(map.size() == 1 && map[0].occurrence == nested,
            "copied empty map lost nested reservation or retry identity");
  }
  const auto memberships = Access::memberships(*owner);
  const auto declarations = Access::declarations(*owner);
  require(declarations.size() == 2, "owner did not retain both exchange declarations");
  const auto *reserved_plan = &Access::plan(*owner);
  const auto plan_backing = Plan::backing(*reserved_plan);
  const auto *reserved_binding = Plan::binding(*reserved_plan);
  const auto binding_backing = Plan::binding_backing(*reserved_plan);
  require(Plan::binding_status(*reserved_plan) == UpdateStatus::InvalidInput,
          "adoption exposes an uncaptured binding");
  require(!Access::pending(*owner), "new owner exposes an unprepared plan");
  const auto reserved = Access::storage(*owner);
  const auto map_backing = Access::map_backing(*owner);
  assembly::ProjectionStorage overflow{UINT64_MAX,0,0,0};
  bool charge_rejected = false;
  try { overflow.add({1,0,0,0}); } catch (const warm::Error &) { charge_rejected = true; }
  require(charge_rejected && overflow.object_bytes == UINT64_MAX && !overflow.vector_bytes && !overflow.text_bytes,
          "projection charge overflow published a partial report");
  require(reserved.object_bytes == 2 * sizeof(assembly::LetterProjection) && reserved.parent_slots == 10 &&
              reserved.vector_bytes && reserved.text_bytes, "projection storage omitted retained surfaces");
  const auto *first_view = &Access::projection(*owner), *second_view = &Access::spare_projection(*owner);
  taichi::lang::SNode projection_root(0,taichi::lang::SNodeType::root);
  const std::vector<assembly::StudyNode> projection_nodes{{&projection_root,-1,assembly::ScaleRole::Root}};
  const auto &s = f.h.prepared_storage();
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  const auto same_handle = [](auto a, auto b) {
    return a.context == b.context && a.address == b.address && a.record == b.record &&
           a.incarnation == b.incarnation;
  };
  // Caller construction remains outside the guard. Successful owner preparation
  // (including its first call and retries), sealing, execution and ticks are guarded.
  for (int round = 0; round <= 32; ++round) {
    const bool compression = round % 2;
    const auto target = compression ? warm::Exposure::Aggregate : warm::Exposure::Parts;
    const auto *previous = &Access::projection(*owner);
    const auto old_rows = Access::mappings(*owner);
    const auto old_fields = Access::fields(*owner);
    const auto old_outgoing = Access::outgoing_report(*owner);
    const auto old_balance = Access::balance_report(*owner);
    const auto generation = s.generation, ticks = s.completed_ticks;
    const auto unchanged = [&] {
      if (&Access::projection(*owner) != previous || Access::generation(*owner) != generation ||
          Access::outgoing_report(*owner) != old_outgoing || Access::balance_report(*owner) != old_balance ||
          Access::mappings(*owner).size() != old_rows.size() || Access::fields(*owner).size() != old_fields.size())
        return false;
      for (std::size_t i = 0; i < old_rows.size(); ++i)
        if (!(Access::mappings(*owner)[i].occurrence == old_rows[i].occurrence) ||
            !same_handle(Access::mappings(*owner)[i].handle, old_rows[i].handle)) return false;
      for (std::size_t i = 0; i < old_fields.size(); ++i)
        if (Access::fields(*owner)[i].field.kind != old_fields[i].field.kind ||
            !(Access::fields(*owner)[i].field.object == old_fields[i].field.object) ||
            !same_handle(Access::fields(*owner)[i].handle, old_fields[i].handle)) return false;
      return true;
    };
    const auto declaration = assembly::declare_letter_exchange(*previous,key,target);
    std::shared_ptr<const assembly::LetterExchangeContract> selected;
    no_new([&] { selected = Access::declaration(*owner,key,target); });
    require(selected.get() == declarations[compression ? 1 : 0].get() &&
                Access::equivalent(*selected,declaration),
            "retained declaration differs from freshly derived alternatives");
    no_new([&] { Plan::stage_binding(Access::plan(*owner),selected,memberships,old_rows,old_fields); });
    require(Plan::binding_status(Access::plan(*owner)) == UpdateStatus::InvalidInput,
            "map staging exposed an uncaptured binding");
    no_new([&] { Plan::capture(Access::plan(*owner),*previous); });
    require(Plan::binding_status(Access::plan(*owner)) == UpdateStatus::Ok && unchanged(),
            "allocation-free source capture failed or published owner data");
    std::vector<assembly::detail::ExchangeIncoming> incoming;
    for (const auto &source : declaration.incoming())
      incoming.push_back({source.occurrence, source.definition, source.node, 1,
                          int((compression ? 910 : 900) + incoming.size()), 0.5, {1,1,true}});
    const std::vector<int> next_ids = compression ? std::vector<int>{6,7,8}
                                                 : std::vector<int>{0,1,2,3,4,5,8};
    std::vector<assembly::detail::ExchangeNewField> additions;
    for (int id : next_ids) if (id != 8) additions.push_back({keys[id],1,50+id});
    bool rejected = false;
    try { Access::prepare(*owner,key,previous->exposure(0),incoming,additions); }
    catch (const warm::Error &) { rejected = true; }
    require(rejected && unchanged() && !Access::pending(*owner), "failed preparation changed committed owner");
    no_new([&] { Access::stage_projection(*owner,key,target); });
    require(Access::storage(*owner).charged_bytes() == reserved.charged_bytes(),
            "projection reservation grew during preparation");
    no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
    if (round == 0) {
      auto oversized = additions;
      oversized.back().field.object.address.resize(128);
      rejected = false;
      try { Access::prepare(*owner,key,target,incoming,oversized); }
      catch (const warm::Error &) { rejected = true; }
      require(rejected && unchanged() && !Access::pending(*owner) &&
                  Access::map_backing(*owner) == map_backing,
              "oversized field staging grew backing or published partial owner");
      no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
      auto foreign = key, unknown = key, nested = key;
      foreign.study = warm::new_uuid();
      unknown.source.entry = UINT64_MAX;
      nested.source.path.push_back(0);
      for (const auto &invalid_key : {foreign,unknown,nested}) {
        rejected = false;
        try { Access::prepare(*owner,invalid_key,target,incoming,additions); }
        catch (const warm::Error &) { rejected = true; }
        require(rejected && !Access::pending(*owner) && unchanged(),
                "retained declaration accepted a foreign, unknown or nested owner");
        no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
      }
      rejected = false;
      try { Access::prepare(*owner,key,static_cast<warm::Exposure>(99),incoming,additions); }
      catch (const warm::Error &) { rejected = true; }
      require(rejected && !Access::pending(*owner) && unchanged(), "invalid retained alternative accepted");
      no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
    }
    // Fail after binding creation and partial command construction, then retry.
    // This must also invalidate a previously prepared, unconsumed transaction.
    auto invalid = incoming;
    invalid.back().radius = -1;
    rejected = false;
    try { Access::prepare(*owner,key,target,invalid,additions); }
    catch (const warm::Error &) { rejected = true; }
    require(rejected && !Access::pending(*owner) && unchanged(),
            "partial preparation retained pending intent or changed owner");
    require(Access::execute(*owner,generation).status == UpdateStatus::InvalidInput,
            "failed preparation allowed execution of old intent");
    no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
    require(&Access::plan(*owner) == reserved_plan && Plan::backing(*reserved_plan) == plan_backing,
            "transaction preparation replaced reserved backing");
    require(Plan::declaration(*reserved_plan) == selected.get(), "binding copied retained declaration");
    require(Plan::memberships(*reserved_plan) == memberships.get(), "binding replaced membership catalogue");
    // Force capture failure after masses were collected, without changing live
    // source data. The partially overwritten reusable view must be invalid.
    auto invalid_fields = old_fields;
    invalid_fields.back().handle.incarnation = 0;
    rejected = false;
    try { Plan::rebind(Access::plan(*owner),*previous,selected,old_rows,invalid_fields); }
    catch (const warm::Error &) { rejected = true; }
    require(rejected && Plan::binding_status(*reserved_plan) == UpdateStatus::InvalidInput && unchanged(),
            "partial binding capture retained authority or changed committed owner");
    no_new([&] { Access::prepare(*owner,key,target,incoming,additions); });
    require(Plan::binding(*reserved_plan) == reserved_binding &&
                Plan::binding_backing(*reserved_plan) == binding_backing &&
                Plan::binding_status(*reserved_plan) == UpdateStatus::Ok,
            "binding retry replaced reserved capture backing or remained invalid");
    require(unchanged(), "preparation published exposure early");
    // Prospective field hierarchy includes retained membership and pending claims.
    const auto &coordinator = Access::plan(*owner);
    const auto &commands = Plan::commands(coordinator);
    std::vector<LiveRefreshNode> nodes;
    std::vector<int> children;
    std::vector<LiveRefreshContribution> contributions;
    for (int id : next_ids) {
      const auto &planned = Plan::planned_fields(coordinator);
      const auto where = std::find_if(planned.begin(), planned.end(), [&](const auto &v) {
        return v.field.kind == keys[id].kind && v.field.object == keys[id].object;
      });
      require(where != planned.end(), "missing owner prospective field");
      const auto command = Plan::field_commands(coordinator)[where - planned.begin()];
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
        const auto index = int(std::find(next_ids.begin(),next_ids.end(),child) - next_ids.begin());
        children.push_back(index);
        node.level = std::max(node.level, nodes[index].level + 2);
      }
      node.child_count = child_ids.size();
      node.cohort_begin = contributions.size();
      for (std::size_t row = 0; row < s.live.edges; ++row) {
        const bool removed = std::any_of(commands.begin(),commands.end(),[&](const auto &c) {
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
          contributions.push_back({c.first,{},int(i)});
      }
      node.cohort_count = contributions.size() - node.cohort_begin;
      nodes.push_back(node);
    }
    const field::LiveRefreshSpec hierarchy{nodes.data(),nodes.size(),children.data(),children.size(),
                                          contributions.data(),contributions.size()};
    const auto charge = Plan::hierarchy_charge(*reserved_plan,hierarchy);
    const auto input_bytes = nodes.size()*sizeof(field::LiveRefreshNode) +
        children.size()*sizeof(int) + contributions.size()*sizeof(field::LiveRefreshContribution);
    require(charge.input_bytes == input_bytes && charge.required_work == input_bytes &&
                charge.required_bytes == charge.retained_bytes+input_bytes && charge.retained_bytes,
            "hierarchy sealing omitted caller/destination payload charge");
    const auto unsealed_native = TickSnapshot(f);
    no_new([&] {
      using Status = assembly::detail::HierarchySealStatus;
      auto unreadable = hierarchy;
      unreadable.nodes = reinterpret_cast<const field::LiveRefreshNode *>(std::uintptr_t(1));
      require(Access::seal(*owner,unreadable,{charge.required_bytes-1,charge.required_work}).status ==
                  Status::ByteExhausted,
              "hierarchy byte shortage read caller payload or was accepted");
      require(Access::seal(*owner,unreadable,{charge.required_bytes,charge.required_work-1}).status ==
                  Status::WorkExhausted,
              "hierarchy work shortage read caller payload or was accepted");
      auto oversized = hierarchy;
      oversized.node_count = SIZE_MAX;
      const auto overflow = Access::seal(*owner,oversized,{UINT64_MAX,UINT64_MAX});
      require(overflow.status == Status::Overflow && !overflow.required_bytes && !overflow.input_bytes &&
                  !Plan::sealed(*reserved_plan) && Access::pending(*owner),
              "hierarchy overflow published partial charge or consumed intent");
    });
    require(unchanged() && unsealed_native == TickSnapshot(f) &&
                Plan::backing(*reserved_plan) == plan_backing,
            "hierarchy budget rejection changed owner/native state or backing");
    no_new([&] {
      require(Access::seal(*owner,hierarchy,{charge.required_bytes,charge.required_work}).status ==
                  assembly::detail::HierarchySealStatus::Ok,
              "exact hierarchy budget retry failed");
    });
    require(Plan::backing(*reserved_plan) == plan_backing,
            "hierarchy sealing replaced reserved backing");
    require(unchanged(), "hierarchy sealing published exposure early");
    const auto before = TickSnapshot(f);
    const auto run = [&](std::uint64_t expected, std::uint64_t work = UINT64_MAX) {
      UpdateResult result;
      no_new([&] { result = Access::execute(*owner,expected,work); });
      return result;
    };
    require(run(generation+1).status == UpdateStatus::Stale && unchanged() && before == TickSnapshot(f),
            "stale owner operation changed state");
    require(run(generation,0).status == UpdateStatus::CapacityExhausted && unchanged() && before == TickSnapshot(f),
            "owner work exhaustion changed state");
    PreparedFixtureAccess::inject_failure(f.h,1);
    require(run(generation).status == UpdateStatus::InvalidInput && unchanged() && before == TickSnapshot(f),
            "ordinary owner rejection changed state");
    PreparedFixtureAccess::inject_failure(f.h,0);
    if (round == 32) {
      PreparedFixtureAccess::inject_failure(f.h,8);
      bool failed = false;
      try { run(generation); } catch (const std::runtime_error &) { failed = true; }
      require(failed && !f.h.ready() && unchanged(), "late exposure failure left healthy partial owner");
      require(run(generation).status == UpdateStatus::Unready, "unready owner allowed another operation");
      break;
    }
    const auto result = run(generation);
    require(result.status == UpdateStatus::Ok && result.generation == generation+1 &&
                s.completed_ticks == ticks && Access::generation(*owner) == result.generation &&
                !Access::pending(*owner), "owner did not publish one complete unticked exchange");
    const auto &current = Access::projection(*owner);
    require(Access::map_backing(*owner) == map_backing,
            "exchange map publication changed reserved nested backing");
    no_new([&] {
      require(Access::resources(*owner).total().charged_bytes() == resources.total().charged_bytes(),
              "owner capacity charge changed across preparation/publication");
    });
    require(&current == (round % 2 ? first_view : second_view), "owner replaced a reserved projection view");
    require(current.study() == initial.study() && current.recipe().ref.selection == recipe.ref.selection &&
                current.recipe().ref.revision == recipe.ref.revision &&
                current.recipe().selections[0].exposure == warm::Exposure::Aggregate &&
                current.exposure(0) == target, "owner changed identity/recipe or lost live exposure");
    require(current.occurrences().size() == initial.occurrences().size(), "owner lost alternative inventory");
    const auto structural = current.bind_structure(projection_nodes);
    require(structural.selections.size() == 1 && structural.selections[0].exposure ==
                (compression ? assembly::Exposure::Aggregate : assembly::Exposure::Parts),
            "structural handoff retained initial exposure");
    std::size_t active = 0;
    for (std::size_t i = 0; i < current.occurrences().size(); ++i) {
      const auto &o = current.occurrences()[i];
      require(o.key == initial.occurrences()[i].key && o.definition == initial.occurrences()[i].definition,
              "owner changed stable occurrence identity");
      const auto count = std::count_if(Access::mappings(*owner).begin(),Access::mappings(*owner).end(),
          [&](const auto &v) { return v.occurrence == o.key; });
      require(count == int(o.active), "active projection and native mapping disagree");
      active += o.active;
    }
    require(active == (compression ? 1 : 2) && current.parents().size() == (compression ? 1 : 4),
            "owner active contributions did not follow exposure");
    const auto memberships = assembly::membership_fields(current);
    require(memberships.contributions().size() == active, "owner inherited membership did not follow exposure");
    for (const auto &m : memberships.contributions())
      require(m.count == (compression ? 4 : 2), "owner membership lost exact source mass");
    for (const auto &v : Access::fields(*owner))
      require(same_handle(v.handle,s.field_headers[v.handle.record]), "owner field handle is stale");
    for (const auto &v : Access::mappings(*owner))
      require(same_handle(v.handle,s.particle_headers[v.handle.record]), "owner occurrence handle is stale");
    const auto focal = std::find_if(Access::fields(*owner).begin(),Access::fields(*owner).end(),
        [&](const auto &v) { return v.field.kind == GroupKind::Membership && v.field.object == group.object; });
    require(focal != Access::fields(*owner).end() && same_handle(focal->handle,fields.back().handle),
            "owner lost retained focus");
    require(Access::outgoing_report(*owner) == Plan::outgoing_report(Access::plan(*owner)) &&
                Access::balance_report(*owner) == Plan::balance_report(Access::plan(*owner)),
            "owner report does not match committed exchange");
    require(run(result.generation).status == UpdateStatus::InvalidInput, "owner replayed consumed plan");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation,parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed &&
                backing.same_backing(metrics.sample()), "owner following tick failed or native backing grew");
  }
  std::cout << "exchange_owner,rounds=32,identity=stable,recipe=immutable,exposure=live,"
               "mappings=atomic,reports=atomic,reject_retry=1,late_unready=1,"
               "following_ticks=32,backing=stable,projection_prepare_cpp_new=0,"
               "coordinator_backing=stable,hierarchy_seal_cpp_new=0,partial_prepare_retry=1,"
               "declaration_select_cpp_new=0,declarations=retained,"
               "binding_backing=stable,partial_binding_retry=1,"
               "membership_catalogue=retained,owner_prepare_cpp_new=0,source_capture_cpp_new=0,execute_cpp_new=0 PASS\n";
  std::cout << "projection_storage,views=2,parent_slots=" << reserved.parent_slots
            << ",objects=" << reserved.object_bytes << ",vectors=" << reserved.vector_bytes
            << ",text_capacity=" << reserved.text_bytes << ",charge=" << reserved.charged_bytes() << '\n';
}
