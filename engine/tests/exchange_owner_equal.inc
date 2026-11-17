// Both alternatives have one body and mass two. Untouched aggregate and byte
// copies retain every field, so repeated publication isolates semantic direction.
void exchange_owner_equal(engine::Runtime &r, HostPath path, bool recipe_only = false) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using Plan = assembly::testing::ExchangeCoordinatorAccess;
  using assembly::GroupKind;
  using namespace assembly::detail;
  letter_fixture::Fixture letters;
  const auto group = letters.add(letter_fixture::object("02.00.00.00.00"), "group", 0);
  letters.base.tools.at(letters.tools.at(letters.english)).member_of = {group};
  letters.base.focal_fields = {{warm::FieldKind::Membership,group.object}};
  const auto initial = assembly::project_letters(assembly::normalize_structural_letters(letters.base,
      {{0,letters.ref(letters.english),warm::Exposure::Aggregate},
       {1,letters.ref(letters.english),warm::Exposure::Aggregate},
       {2,letters.ref(letters.english),warm::Exposure::Parts}}));
  const auto byte = letters.base.tools.at(letters.tools.at(letters.english)).parents[0].target;
  const auto &byte_tool = letters.base.tools.at(letters.tools.at(byte.object));
  const std::vector<assembly::FieldKey> keys{
      {GroupKind::ParentValue,byte_tool.parents[0].target.object},
      {GroupKind::ParentValue,byte_tool.parents[1].target.object},
      {GroupKind::SiblingDefinition,byte.object}, {GroupKind::ParentValue,byte.object},
      {GroupKind::SiblingDefinition,letters.english}, {GroupKind::Membership,group.object}};
  auto limits = placement_limits();
  Fixture f(r,path,{{},{},{0,1},{},{3},{2,4}},
      {{0,0,1},{0,0,1},{0,0,2},{2,2,0},{2,2,0},{2,2,2}},
      {2,2,2},{2,2,2},{0,20,40},52,52,false,false,true,false,1024,{},&limits,true);
  f.install();
  std::vector<OccurrenceBinding> rows;
  for (const auto &o : initial.occurrences()) if (o.active)
    rows.push_back({o.key,f.in.particle_rows[o.key.source.entry]});
  std::vector<ExchangeFieldBinding> fields;
  for (std::size_t i=0; i<keys.size(); ++i) fields.push_back({keys[i],f.in.field_rows[i]});
  exchange_recipe_cost(f,initial,rows,fields);
  if (recipe_only) return;
  owner_lifetime(f,initial,rows,fields);
  auto owner = Access::adopt(f.h,initial,rows,fields);
  const assembly::OccurrenceKey key{initial.study(),{0,{}}};
  const auto payload = Access::declarations(*owner).front();
  const auto memberships = Access::memberships(*owner);
  const auto resources = Access::resources(*owner);
  const auto map_backing = Access::map_backing(*owner);
  require(payload->outgoing().size() == 1 && payload->incoming().size() == 1 && payload->mass() == 2,
          "equal-alternative fixture lost equal counts/mass");
  for (int round=0; round<16; ++round) {
    const auto &current = Access::projection(*owner);
    const auto target = current.exposure(0) == warm::Exposure::Aggregate
        ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    const auto expected = assembly::declare_letter_exchange(current,key,target);
    const auto selected = Access::declaration(*owner,key,target);
    require(selected.get() == payload.get() && Access::equivalent(*selected,expected,target == payload->from()),
            "equal alternatives changed immutable payload or full ordered meanings");
    bool rejected = false;
    try {
      Plan::stage_binding(Access::plan(*owner),selected,memberships,
          Access::mappings(*owner),Access::fields(*owner),current.exposure(0));
      Plan::capture(Access::plan(*owner),current);
    } catch (const warm::Error &) { rejected = true; }
    require(rejected && Plan::binding_status(Access::plan(*owner)) == UpdateStatus::InvalidInput,
            "equal row counts/mass authorized capture with the wrong explicit role");
    std::vector<ExchangeIncoming> incoming;
    for (const auto &source : expected.incoming())
      incoming.push_back({source.occurrence,source.definition,source.node,1,900+round,0.5,{1,1,true}});
    const auto before = TickSnapshot(f);
    const auto generation = Access::generation(*owner);
    OwnerPreparationResult full;
    no_new([&] {
      // First preparation partitions the spare parent pool. Measure work from
      // its normalized state before testing exact/one-short retries, as in the
      // single-owner staging sweep; scratch order is not committed authority.
      const auto first = Access::prepare(*owner,key,target,incoming,{},UINT64_MAX,UINT64_MAX,UINT64_MAX);
      require(first.status == PreparationStatus::Ok, "equal alternatives failed first preparation");
      full = Access::prepare(*owner,key,target,incoming,{},UINT64_MAX,UINT64_MAX,UINT64_MAX);
    });
    require(full.status == PreparationStatus::Ok && Plan::target(Access::plan(*owner)) == target &&
                Plan::declaration(Access::plan(*owner)) == payload.get(),
            "equal alternatives failed coherent directional preparation");
    rejected = false;
    try { Access::prepare(*owner,key,current.exposure(0),incoming,{}); }
    catch (const warm::Error &) { rejected = true; }
    require(rejected && !Access::pending(*owner) && Plan::consumed(Access::plan(*owner)) &&
                !Plan::sealed(Access::plan(*owner)) && before == TickSnapshot(f),
            "same-target refusal retained pending directional intent");
    for (int phase=0; phase<3; ++phase) {
      auto work = std::array<std::uint64_t,3>{full.staging.charged,full.capture.charged,full.commands.charged};
      require(work[phase], "directional preparation phase is uncharged");
      --work[phase];
      OwnerPreparationResult stopped;
      no_new([&] { stopped = Access::prepare(*owner,key,target,incoming,{},work[0],work[1],work[2]); });
      if (stopped.status != PreparationStatus::WorkExhausted || stopped.phase != static_cast<PreparationPhase>(phase))
        std::cerr << "equal_phase,round=" << round << ",expected=" << phase << ",actual=" << int(stopped.phase)
                  << ",status=" << int(stopped.status) << ",staging=" << stopped.staging.charged
                  << ",capture=" << stopped.capture.charged << ",commands=" << stopped.commands.charged
                  << ",full=" << full.staging.charged << '/' << full.capture.charged << '/' << full.commands.charged << '\n';
      require(stopped.status == PreparationStatus::WorkExhausted &&
                  stopped.phase == static_cast<PreparationPhase>(phase) && !Access::pending(*owner) &&
                  Plan::consumed(Access::plan(*owner)) && !Plan::sealed(Access::plan(*owner)) && before == TickSnapshot(f),
              "one-short directional phase retained authority or changed state");
    }
    no_new([&] {
      const auto retry = Access::prepare(*owner,key,target,incoming,{},
          full.staging.charged,full.capture.charged,full.commands.charged);
      require(retry.status == PreparationStatus::Ok && Plan::target(Access::plan(*owner)) == target,
              "equal alternatives failed exact directional preparation retry");
      require(Access::resources(*owner).total().charged_bytes() == resources.total().charged_bytes() &&
                  Access::resources(*owner).shared_allocations == 5,
              "binding alias duplicated census storage or shared identities");
    });
    const auto &commands = Plan::commands(Access::plan(*owner));
    RefreshRequest request(f.h);
    for (std::size_t node=0; node<request.nodes.size(); ++node) {
      auto &cohort = request.cohorts[node];
      cohort.erase(std::remove_if(cohort.begin(),cohort.end(),[&](const auto &ref) {
        return std::any_of(commands.begin(),commands.end(),[&](const auto &command) {
          return command.operation == UpdateOp::RemoveParticipation && command.row == ref.source.row;
        });
      }),cohort.end());
      for (std::size_t i=0; i<commands.size(); ++i)
        if (commands[i].operation == UpdateOp::InsertParticipation &&
            commands[i].second.handle.record == request.nodes[node].field.record)
          cohort.push_back({commands[i].first,{},int(i)});
    }
    const auto spec = request.spec();
    no_new([&] { Access::seal(*owner,spec); });
    UpdateResult result;
    no_new([&] { result = Access::execute(*owner,generation); });
    require(result.status == UpdateStatus::Ok && result.generation == generation+1 &&
                Access::projection(*owner).exposure(0) == target &&
                Access::projection(*owner).exposure(1) == warm::Exposure::Aggregate &&
                Access::projection(*owner).exposure(2) == warm::Exposure::Parts &&
                Access::map_backing(*owner) == map_backing,
            "equal-alternative publication changed direction, other copies or backing");
    Parameters64 parameters;
    parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    TickResult tick;
    no_new([&] { tick = f.h.tick_prepared(result.generation,parameters); });
    require(tick.status == TickStatus::Ok && !tick.coverage_changed,
            "equal-alternative following tick failed");
  }
  std::cout << "exchange_owner_equal,rounds=16,rows_per_side=1,mass_per_side=2,payloads=3,"
               "roles=explicit,binding_census_alias=deduplicated,wrong_role=refused,"
               "one_short_staging_capture_commands=refused,exact_retry=1,following_ticks=16 PASS\n";
}
