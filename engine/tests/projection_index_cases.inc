void projection_index_case(int mode,bool structural,bool selected) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  auto fixture=std::make_unique<Fixture>();
  if(structural)fixture->base.focal_fields={{warm::FieldKind::SiblingDefinition,fixture->english}};
  auto bundle=std::make_unique<warm::Bundle>(fixture->base);
  if(mode==0) {
    bundle->recipe_version=structural?2:1;
    bundle->nodes={{-1,warm::ScaleRole::Root}};
  } else {
    const int count=mode==1?1:12;
    std::vector<LetterRequest> legacy;
    std::vector<StructuralLetterRequest> declarations;
    for(int i=0;i<count;++i) {
      const auto exposure=i%2?warm::Exposure::Parts:warm::Exposure::Aggregate;
      legacy.push_back(fixture->request(fixture->english,std::uint64_t(2+8*i),exposure));
      declarations.push_back({std::uint64_t(2+8*i),fixture->ref(fixture->english),exposure});
    }
    *bundle=structural?normalize_structural_letters(*bundle,declarations):normalize_letters(*bundle,legacy);
  }
  if(mode==0) {
    A::begin();bool refused=false;
    try{auto value=project_letters(*bundle);}catch(const warm::Error &error){
      refused=std::string_view(error.what())=="empty/oversized recipe";
    }
    const auto released=A::end();
    check(refused && !released.live && !released.blocks && released.acquired==released.released,
          "empty recipe retains the existing refusal before projection publication");
    std::cout << "projection_empty_refusal,recipe=" << (structural?2:1)
              << ",retained=0,after_destroy_requested=0 PASS\n";
    return;
  }
  struct Expected {warm::OccurrenceId id;warm::ToolRef definition;int node;bool active;
    std::optional<warm::ToolRef> complete;};
  std::vector<Expected> expected;
  const auto collect=[&](const auto &rows) {
    for(const auto &row:rows) {
      const auto selection=std::find_if(bundle->selections.begin(),bundle->selections.end(),
          [&](const auto &s){return s.entry==row.id.entry;});
      expected.push_back({row.id,fixture->ref(row.definition),row.node,
          row.id.path.empty()==(selection->exposure==warm::Exposure::Aggregate),
          row.id.path.empty()?std::optional<warm::ToolRef>(fixture->base.tools.at(fixture->tools.at(fixture->english)).parents.front().target):
              std::optional<warm::ToolRef>{}});
    }
  };
  if(structural)collect(bundle->structural_occurrences);else collect(bundle->occurrences);
  std::sort(expected.begin(),expected.end(),[](const auto &a,const auto &b){return a.id<b.id;});
  auto prototype=selected?std::make_unique<LetterProjection>(project_letters(*bundle)):nullptr;
  auto domain=selected?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
      std::shared_ptr<ReservationDomain>{};
  std::weak_ptr<ReservationDomain> weak=domain;
  std::vector<ProjectedOccurrence> values;values.reserve(expected.size());
  std::optional<LetterProjection> projection;
  A::begin();
  allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
  try {
    if(selected)projection.emplace(Access::copy(*prototype,domain,false));
    else projection.emplace(project_letters(*bundle));
  }catch(...){allocation_observation::enabled=false;A::end();throw;}
  allocation_observation::enabled=false;
  const auto construction=A::counts;
  const auto requested=allocation_observation::bytes.load();
  check(projection->occurrence_count()==expected.size(),"empty/single/repeated projection retains every alternative");
  allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
  for(std::size_t i=0;i<expected.size();++i)values.push_back(projection->occurrence(i));
  allocation_observation::enabled=false;
  const auto export_calls=allocation_observation::calls.load(),export_bytes=allocation_observation::bytes.load();
  const auto matches=[&](const ProjectedOccurrence &row,std::size_t i) {
    const auto &e=expected[i];
    return row.key.source==e.id && row.definition==e.definition && row.node==e.node && row.active==e.active &&
        row.slot==(structural?-1:int(i)) && row.complete_character==e.complete && bool(row.byte)==!e.id.path.empty() &&
        (!row.byte || (row.byte->owner.study==row.key.study && row.byte->owner.source.entry==e.id.entry &&
            row.byte->owner.source.path.empty() && row.byte->character==e.definition &&
            row.byte->character_ordinal==0 && row.byte->byte_ordinal==0));
  };
  for(std::size_t i=0;i<values.size();++i)
    check(matches(values[i],i) && values[i].key.study==projection->study(),
          "small-case owning export retains full numeric identity, revision, node, activity and byte provenance");
  projection.reset();prototype.reset();bundle.reset();fixture.reset();domain.reset();
  check(weak.expired() && A::counts.live==export_bytes,"small-case exports detach from projection recipe/domain ownership");
  for(std::size_t i=0;i<values.size();++i)check(matches(values[i],i),"small-case public values survive every source owner");
  values.clear();const auto released=A::end();
  check(!released.live && !released.blocks && released.acquired==released.released,"small-case lifetime fully releases actual requests");
  std::cout << "projection_index_case,mode=" << mode << ",recipe=" << (structural?2:1) << ",selected=" << selected
            << ",rows=" << expected.size() << ",requests=" << construction.attempts << ",requested=" << requested
            << ",retained=" << construction.live << ",peak=" << construction.peak
            << ",export_requests=" << export_calls << ",export_requested=" << export_bytes
            << ",output_capacity_bytes=" << values.capacity()*sizeof(ProjectedOccurrence)
            << ",source_projection_domain_dead=1,after_destroy_requested=0 PASS\n";
}
