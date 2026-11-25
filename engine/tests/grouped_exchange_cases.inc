void grouped_exchange(engine::Runtime &runtime,HostPath path) {
  for(const int scenario:{0,1,2}) {
    auto limits=placement_limits();if(scenario==1)limits[Budget::Particles]=4;
    Fixture f(runtime,path,{{}},{{2,2,2}},{2,2,2},{2,2,2},{-10,10,100},
        52,52,false,false,true,false,1024,{},&limits,true);
    f.install();
    const auto a=f.in.particle_rows[0],b=f.in.particle_rows[1],survivor=f.in.particle_rows[2];
    PreparedFixtureAccess::inject_value64(f.h,kVelX,a.record,-0.25);
    PreparedFixtureAccess::inject_value64(f.h,kVelX,b.record,0.75);
    PreparedFixtureAccess::inject_value64(f.h,kVelX,survivor.record,0.125);
    runtime.synchronize();f.h.inspect_prepared();
    std::array<double,kParticleFieldCount> kept{};
    for(int k=0;k<kParticleFieldCount;++k)kept[k]=f.h.particle64(k,survivor.record);
    std::vector<UpdateCommand> commands;
    for(int i=0;i<2;++i) {auto c=placement_definition(901+i);c.exact_mass=2;commands.push_back(c);}
    for(int i=0;i<2;++i) {UpdateCommand c;c.operation=UpdateOp::RetireParticle;c.first=UpdateRef::existing(f.in.particle_rows[i]);commands.push_back(c);}
    for(int i=0;i<2;++i) {UpdateCommand c;c.operation=UpdateOp::RemoveParticipation;c.row=i;c.generation=f.h.prepared_storage().generation;commands.push_back(c);}
    for(int i=0;i<2;++i) {
      UpdateCommand c;c.operation=UpdateOp::InsertParticipation;c.first=UpdateRef::pending(i);
      c.second=UpdateRef::existing(f.in.field_rows[0]);c.position=1+i;c.ordinal=i;c.share=1;c.exact_count=2;commands.push_back(c);
    }
    std::vector<PlacementIntrinsic> inputs{{1,1,2,0.25,{1,1,true}},{0,0,2,0.25,{1,1,true}}};
    const std::vector<engine::RecordHandle> outgoing{a,b};
    const std::vector<PlacementExchangeGroup> groups{{0,1,0,1},{1,1,1,1}};
    RefreshRequest request(f.h);request.cohorts[0].erase(request.cohorts[0].begin(),request.cohorts[0].begin()+2);
    request.cohorts[0].push_back({UpdateRef::pending(0),{},6});
    request.cohorts[0].push_back({UpdateRef::pending(1),{},7});
    const auto hierarchy=request.spec();
    const auto generation=f.h.prepared_storage().generation,tick=f.h.prepared_storage().completed_ticks;
    const TickSnapshot before(f);prepared_test::Metrics metrics(runtime,PreparedFixtureAccess::nodes(f.h));const auto backing=metrics.sample();
    const auto invoke=[&](const auto &selected,std::uint64_t expected,std::uint64_t work=UINT64_MAX) {
      UpdateResult result;no_new([&]{result=PlacementAccess::grouped_exchange(f.h,expected,commands,inputs,hierarchy,outgoing,selected,work);});return result;
    };
    if(scenario==0) {
      auto invalid=groups;invalid[1].outgoing_begin=0;
      require(invoke(invalid,generation).status==UpdateStatus::InvalidInput && before==TickSnapshot(f),"overlapping outgoing groups changed native state");
      invalid=groups;invalid[1].incoming_begin=0;
      require(invoke(invalid,generation).status==UpdateStatus::InvalidInput && before==TickSnapshot(f),"overlapping incoming groups changed native state");
      invalid=groups;invalid.pop_back();
      require(invoke(invalid,generation).status==UpdateStatus::InvalidInput && before==TickSnapshot(f),"partial grouped coverage accepted");
      require(invoke(groups,generation-1).status==UpdateStatus::Stale && before==TickSnapshot(f),"stale grouped request changed native state");
      require(invoke(groups,generation,0).status==UpdateStatus::CapacityExhausted && before==TickSnapshot(f),"group work refusal changed native state");
      // Total mass still agrees, but transferring mass between logical groups
      // would silently change their individual meaning and motion.
      auto mismatched_commands=commands;auto mismatched_inputs=inputs;
      mismatched_commands[0].exact_mass=1;mismatched_commands[1].exact_mass=3;
      for(auto &v:mismatched_inputs)v.mass=v.command?3:1;
      UpdateResult mismatched;
      no_new([&]{mismatched=PlacementAccess::grouped_exchange(f.h,generation,mismatched_commands,mismatched_inputs,hierarchy,outgoing,groups);});
      require(mismatched.status==UpdateStatus::InvalidInput && before==TickSnapshot(f),"grouped exchange redistributed mass between logical occurrences");
    }
    if(scenario==2) {
      PreparedFixtureAccess::inject_failure(f.h,2);bool threw=false;
      try{invoke(groups,generation);}catch(const std::exception &){threw=true;}
      require(threw && !f.h.ready(),"grouped postclaim failure remained healthy");continue;
    }
    const auto result=invoke(groups,generation);
    if(scenario==1) {
      require(result.status==UpdateStatus::CapacityExhausted && result.failure.budget==Budget::Particles &&
        result.failure.required==5 && !result.claims.sampled && before==TickSnapshot(f) && f.h.ready(),"grouped final-fit/peak-overflow refusal failed");continue;
    }
    require(result.status==UpdateStatus::Ok && result.generation==generation+1 && f.h.prepared_storage().completed_ticks==tick &&
      result.claims.sampled && result.claims.particles_before==3 && result.claims.particles_peak==5 && result.claims.particles_after==3,
      "grouped transaction did not publish one generation with observed overlap");
    f.h.inspect_prepared();
    for(int i=0;i<2;++i) {
      const auto h=f.h.prepared_storage().resolved[i];
      require(f.h.particle64(kPosX,h.record)==(i?10:-10) && f.h.particle64(kVelX,h.record)==(i?0.75:-0.25),
              "grouped replacement averaged distinct logical motion or used field focus");
    }
    for(int k=0;k<kParticleFieldCount;++k) {
      const auto current=f.h.particle64(k,survivor.record);
      require(!std::memcmp(&current,&kept[k],sizeof(current)),"grouped transaction changed survivor bits");
    }
    require(backing.same_backing(metrics.sample()),"grouped replacement changed native backing/compilation counters");
    Parameters64 parameters;parameters.gravity=parameters.dt=parameters.contact_stiffness=parameters.reach_strength=0;
    require(f.h.tick_prepared(result.generation,parameters).status==TickStatus::Ok,"following grouped tick failed");
  }
  std::cout<<"grouped_exchange,logical_groups=2,distinct_motion=1,own_targets=1,survivor_bits=1,one_generation=1,"
      "claims_peak=5,partition_refusals=3,per_group_mass_refused=1,stale_and_work_refused=1,postclaim_unready=1,following_tick=1,backing=stable,cpp_new=0 PASS\n";
}
