void projection_definition_cost(int mode) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  namespace D=projection_definition_observation;
  auto fixture=std::make_unique<Fixture>();
  const bool structural=mode!=0;
  if(structural)fixture->base.focal_fields={{warm::FieldKind::SiblingDefinition,fixture->english}};
  auto bundle=std::make_unique<warm::Bundle>(structural?
      normalize_structural_letters(fixture->base,
          {{0,fixture->ref(fixture->english),warm::Exposure::Aggregate},
           {10,fixture->ref(fixture->english),warm::Exposure::Parts},
           {20,fixture->ref(fixture->french),warm::Exposure::Parts},
           {30,fixture->ref(fixture->accented),warm::Exposure::Aggregate},
           {40,fixture->ref(fixture->repeated),warm::Exposure::Parts}}):
      normalize_letters(fixture->base,
          {fixture->request(fixture->english,0,warm::Exposure::Aggregate),
           fixture->request(fixture->english,10,warm::Exposure::Parts),
           fixture->request(fixture->french,20,warm::Exposure::Parts),
           fixture->request(fixture->accented,30,warm::Exposure::Aggregate),
           fixture->request(fixture->repeated,40,warm::Exposure::Parts)}));
  const auto version=bundle->recipe_version;
  struct Expected {warm::OccurrenceId id;warm::ToolRef definition;int node;bool active;
    std::optional<warm::ToolRef> character,complete;};
  std::vector<Expected> expected;
  const auto collect=[&](const auto &rows) {
    for(const auto &row:rows) {
      const auto selection=std::find_if(bundle->selections.begin(),bundle->selections.end(),
          [&](const auto &s){return s.entry==row.id.entry;});
      const auto &roots=rows;
      const auto root=std::find_if(roots.begin(),roots.end(),[&](const auto &r){return r.id.entry==row.id.entry && r.id.path.empty();});
      const auto &tool=fixture->tool(*bundle,root->definition);
      Expected value{row.id,fixture->ref(row.definition),row.node,
          row.id.path.empty()==(selection->exposure==warm::Exposure::Aggregate),{}, {}};
      if(!row.id.path.empty())value.character=tool.parents.at(row.id.path[0]).target;
      else if(tool.parents.size()==1)value.complete=tool.parents.front().target;
      expected.push_back(std::move(value));
    }
  };
  if(structural)collect(bundle->structural_occurrences);else collect(bundle->occurrences);
  std::sort(expected.begin(),expected.end(),[](const auto &a,const auto &b){return a.id<b.id;});
  const auto matches=[&](const ProjectedOccurrence &row,std::size_t i,const warm::Uuid &study) {
    const auto &e=expected[i];
    bool exact=row.key.study==study && row.key.source==e.id && row.definition==e.definition &&
        row.node==e.node && row.active==e.active && row.slot==(structural?-1:int(i)) &&
        bool(row.byte)==bool(e.character) && row.complete_character==e.complete;
    if(row.byte)exact &= row.byte->owner.study==study && row.byte->owner.source.entry==e.id.entry &&
        row.byte->owner.source.path.empty() && row.byte->character==*e.character &&
        row.byte->character_ordinal==e.id.path[0] && row.byte->byte_ordinal==e.id.path[1];
    return exact;
  };
  std::unique_ptr<LetterProjection> source;
  if(mode>=2)source=std::make_unique<LetterProjection>(project_letters(*bundle));
  auto domain=mode==2?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
      std::shared_ptr<ReservationDomain>{};
  std::weak_ptr<ReservationDomain> weak=domain;
  std::optional<LetterProjection> projection;
  std::vector<ProjectedOccurrence> exported;exported.reserve(expected.size());
  std::vector<std::size_t> sites(expected.size());
  A::begin();
  allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
  try {
    if(mode<2)projection.emplace(project_letters(*bundle));
    else if(mode==2)projection.emplace(Access::copy(*source,domain,false));
    else projection.emplace(*source);
  }catch(...){allocation_observation::enabled=false;A::end();throw;}
  allocation_observation::enabled=false;
  const auto construction=A::counts;
  const auto requested=allocation_observation::bytes.load();
  bool attributed=true;
  allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
  const auto cost=D::inspect(*projection,domain.get(),[&](const void *pointer,std::size_t bytes) {
    const auto *found=std::find_if(std::begin(A::table),std::end(A::table),[&](const auto &e){return e.pointer==pointer;});
    attributed &= found!=std::end(A::table) && found->bytes==bytes;
  });
  const auto study=projection_study_observation::inspect(*projection,domain.get(),[&](const void *pointer,std::size_t bytes) {
    const auto *found=std::find_if(std::begin(A::table),std::end(A::table),[&](const auto &e){return e.pointer==pointer;});
    attributed &= found!=std::end(A::table) && found->bytes==bytes;
  });
  const auto lookup_calls=allocation_observation::calls.load();allocation_observation::enabled=false;
  check(attributed && !lookup_calls && cost.rows==expected.size() && !cost.requests && !cost.bytes && !cost.pin_capacity_bytes,
        "projected definition indices borrow only the projection's already owned exact recipe");
  check(!study.requests && !study.bytes && !study.uuid_capacity_bytes && study.header_bytes,
        "numeric rows use the existing owned study header with no repeated row-study storage");
  {
    auto &stored=assembly::testing::ProjectionIndicesAccess::rows(*projection).front();
    const auto definition_index=stored.definition_index,source_index=stored.source_index;
    std::uint32_t other=0;
    while(other<projection->recipe().tools.size() &&
          projection->recipe().tools[other].ref.object==expected.front().definition.object)++other;
    check(other<projection->recipe().tools.size(),"wrong in-range definition control has a distinct source object");
    for(int fault=0;fault<4;++fault) {
      if(fault==0)stored.definition_index=UINT32_MAX;
      if(fault==1)stored.definition_index=other;
      if(fault==2)stored.source_index=UINT32_MAX;
      if(fault==3)stored.source_index=source_index==0?1:0;
      const auto before=A::counts;
      PreparationWork work(UINT64_MAX);
      check(!assembly::detail::ProjectionAccess::resolve(*projection,0,&work),
            "malformed definition/source index including a wrong in-range tool refuses contextual read");
      bool refused=false;
      try{auto value=projection->occurrence(0);}catch(const warm::Error &){refused=true;}
      stored.definition_index=definition_index;stored.source_index=source_index;
      check(refused && A::counts.live==before.live && A::counts.blocks==before.blocks &&
                matches(projection->occurrence(0),0,projection->study()),
            "malformed index export refuses before copying and healthy retry preserves the source");
    }
    PreparationWork full(UINT64_MAX);
    const auto *tool=assembly::detail::ProjectionAccess::resolve(*projection,0,&full);
    check(tool && tool->ref==expected.front().definition,"production contextual read resolves the exact full pin");
    for(const auto limit:{UINT64_C(0),full.result().charged-1}) {
      PreparationWork denied(limit);bool refused=false;
      try{assembly::detail::ProjectionAccess::resolve(*projection,0,&denied);}catch(const PreparationStop &){refused=true;}
      check(refused,"production definition read refuses zero/one-short work");
    }
    PreparationWork exact(full.result().charged);
    check(assembly::detail::ProjectionAccess::resolve(*projection,0,&exact)==tool,
          "production definition read retries with exact work");
    const auto node=stored.node;stored.node+=1;
    check(!assembly::detail::ProjectionAccess::resolve(*projection,0),"production read rejects the wrong source node");
    stored.node=node;
    auto path=stored.source.path;stored.source.path.push_back(UINT32_MAX);
    check(!assembly::detail::ProjectionAccess::resolve(*projection,0),"production read compares the complete source path");
    stored.source.path=std::move(path);
    check(!assembly::detail::ProjectionAccess::resolve(*projection,projection->occurrence_count()) &&
          !assembly::detail::ProjectionAccess::resolve(*projection,SIZE_MAX),"contextual lookup rejects malformed row indices");
    bool refused=false;try{auto value=projection->occurrence(projection->occurrence_count());}catch(const warm::Error &){refused=true;}
    check(refused,"public occurrence export checks its row index");
  }
  {
    const auto row=projection->occurrence(0);
    PreparationWork full(UINT64_MAX);
    const auto index=D::lookup(*projection,row,projection->recipe().ref,full);
    check(index<projection->recipe().tools.size() && projection->recipe().tools[index].ref==expected.front().definition,
          "checked recipe lookup resolves exact occurrence, definition and node meaning");
    auto bad=row;bad.key.study=warm::new_uuid();
    PreparationWork wrong(UINT64_MAX);
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"equal numeric keys in a foreign study refuse");
    bad=row;bad.key.source.entry=UINT64_MAX;
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"missing occurrence entry refuses");
    bad=row;bad.key.source.path.push_back(UINT32_MAX);
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"different full occurrence path refuses");
    bad=row;bad.node+=1;
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"changed source node refuses");
    bad=row;bad.definition.revision=warm::new_uuid();
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"same definition object with a stale revision refuses");
    bad=row;bad.definition.object.scope=warm::new_uuid();
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"different definition scope refuses");
    bad=row;bad.definition.object.address.push_back(codec::AddressElement{0,0,false});
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"different complete definition address refuses");
    bad=row;bad.definition=fixture->ref(fixture->french);
    check(D::lookup(*projection,bad,projection->recipe().ref,wrong)==SIZE_MAX,"another valid tool cannot replace the occurrence's definition");
    auto recipe=projection->recipe().ref;recipe.revision=warm::new_uuid();
    check(D::lookup(*projection,row,recipe,wrong)==SIZE_MAX,"different recipe revision refuses identical row facts");
    recipe=projection->recipe().ref;recipe.selection=warm::new_uuid();
    check(D::lookup(*projection,row,recipe,wrong)==SIZE_MAX,"different recipe selection refuses identical row facts");
    for(const auto limit:{UINT64_C(0),full.result().charged-1}) {
      PreparationWork denied(limit);bool refused=false;
      try{D::lookup(*projection,row,projection->recipe().ref,denied);}catch(const PreparationStop &){refused=true;}
      check(refused,"definition lookup refuses zero/one-short work");
    }
    PreparationWork exact(full.result().charged);
    check(D::lookup(*projection,row,projection->recipe().ref,exact)==index,"definition lookup retries with its exact work allowance");
  }
  bool exact=projection->occurrence_count()==expected.size();
  for(std::size_t i=0;i<expected.size();++i)exact &= matches(projection->occurrence(i),i,projection->study());
  allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
  for(std::size_t i=0;i<expected.size();++i){const auto before=A::counts.attempts;exported.push_back(projection->occurrence(i));sites[i]=A::counts.attempts-before;}
  allocation_observation::enabled=false;
  const auto export_calls=allocation_observation::calls.load(),export_bytes=allocation_observation::bytes.load();
  std::size_t refusals=0;
  const auto selected_live=domain?domain->result().live_bytes:0;
  for(std::size_t i=0;i<sites.size();++i)for(std::size_t fail=1;fail<=sites[i];++fail) {
    const auto before=A::counts;A::fail_at=before.attempts+fail;bool refused=false;
    try{auto value=projection->occurrence(i);}catch(const std::bad_alloc &){refused=true;}
    A::fail_at=0;
    check(refused && A::counts.live==before.live && A::counts.blocks==before.blocks &&
              (!domain || domain->result().live_bytes==selected_live),"every detached row-copy refusal preserves projection and prior values");
    {const auto retry=projection->occurrence(i);check(matches(retry,i,projection->study()),"detached row-copy healthy retry retains full provenance and identity");}
    ++refusals;
  }
  source.reset();bundle.reset();fixture.reset();
  for(std::size_t i=0;i<expected.size();++i)exact &= matches(projection->occurrence(i),i,projection->study());
  projection.reset();domain.reset();
  const auto detached_pin=[](const warm::ToolRef &ref){return !ref.object.scope.str().get_allocator().domain &&
      !ref.object.address.get_allocator().domain && !ref.revision.str().get_allocator().domain;};
  for(std::size_t i=0;i<exported.size();++i) {
    const auto &row=exported[i];
    exact &= matches(row,i,exported.front().key.study) && !row.key.study.str().get_allocator().domain &&
        !row.key.source.path.get_allocator().domain && detached_pin(row.definition) &&
        (!row.byte || (!row.byte->owner.study.str().get_allocator().domain && detached_pin(row.byte->character))) &&
        (!row.complete_character || detached_pin(*row.complete_character));
  }
  const bool detached=weak.expired() && A::counts.live==export_bytes;
  exported.clear();const auto released=A::end();
  check(exact && detached && !released.live && !released.blocks && released.acquired==released.released,
        "projection and detached public rows own complete definitions/provenance through source/projection/domain death");
  std::cout << "projection_definition_cost,mode=" << mode << ",recipe=" << version << ",rows=" << cost.rows
            << ",active=" << cost.active << ",unique_definitions=" << cost.unique_definitions
            << ",pin_requests=" << cost.requests << ",pin_requested=" << cost.bytes
            << ",pin_capacity_bytes=" << cost.pin_capacity_bytes << ",index_capacity_bytes=" << cost.index_capacity_bytes << ",outer_bytes=" << cost.outer_bytes
            << ",construction_requests=" << construction.attempts << ",construction_requested=" << requested
            << ",retained=" << construction.live << ",peak=" << construction.peak << ",lookup_work=" << cost.lookup_work
            << ",lookup_allocations=" << lookup_calls << ",recipe_definition_owned=1 PASS\n";
  std::cout << "projection_row_copy_cost,mode=" << mode << ",rows=" << cost.rows << ",requests=" << export_calls
            << ",requested=" << export_bytes << ",output_capacity_bytes=" << exported.capacity()*sizeof(ProjectedOccurrence)
            << ",allocation_refusals=" << refusals << ",source_projection_domain_dead=1,after_destroy_requested=0 PASS\n";
  std::cout << "projection_study_cost,mode=" << mode << ",recipe=" << version << ",rows=" << study.rows
            << ",capacity=" << study.capacity << ",row_uuid_requests=" << study.requests << ",row_uuid_requested=" << study.bytes
            << ",header_uuid_requested=" << study.header_bytes << ",uuid_member_capacity_bytes=" << study.uuid_capacity_bytes
            << ",outer_requested=" << study.outer_bytes << ",byte_owner_uuid_requests=" << study.byte_owner_requests
            << ",byte_owner_uuid_requested=" << study.byte_owner_bytes << ",header_comparison_work=" << study.comparison_work
            << ",source_validation_work=" << study.source_work << ",comparison_allocations=" << lookup_calls
            << ",source_projection_domain_dead=1,after_destroy_requested=0,duplicate_row_study_backing=0 PASS\n";
}
