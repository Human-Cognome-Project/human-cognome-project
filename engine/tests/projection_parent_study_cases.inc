void projection_parent_study_cases() {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace P=projection_parent_study_observation;
  namespace A=allocation_lifetime;
  static_assert(std::is_same_v<decltype(std::declval<const LetterProjection &>().parent(0)),ParentContribution>,
      "public parent extraction returns a complete owning value");
  static_assert(!std::is_invocable_v<decltype(&ProjectionAccess::parent),const LetterProjection &,
      const ProjectionParentContribution &,std::size_t>,"parent readers must receive the owning projection, pool and checked index");
  const auto equal=[](const ParentContribution &a,const ParentContribution &b) {
    return a.occurrence==b.occurrence && a.field.kind==b.field.kind && a.field.object==b.field.object &&
        a.ordinal==b.ordinal && a.count==b.count;
  };
  for(const bool structural:{false,true})for(const bool selected:{false,true}) {
    auto source=std::make_unique<LetterProjection>([&] {
      Fixture fixture;
      const auto word=letter_fixture::object("02.00.20.00.2A");
      const auto character=fixture.base.tools.at(fixture.tools.at(fixture.english)).parents.front().target;
      fixture.add(word,std::string(12,'A'),24,std::vector<warm::ToolRef>(12,character));
      fixture.base.geometry.push_back({word,{},{{{},0.5}}});
      if(structural)fixture.base.focal_fields={{warm::FieldKind::SiblingDefinition,fixture.english}};
      const auto bundle=structural?normalize_structural_letters(fixture.base,
          {{UINT64_MAX,fixture.ref(word),warm::Exposure::Parts},{4,fixture.ref(fixture.english),warm::Exposure::Aggregate},
           {8,fixture.ref(fixture.french),warm::Exposure::Parts},{20,fixture.ref(fixture.accented),warm::Exposure::Parts},
           {30,fixture.ref(fixture.repeated),warm::Exposure::Aggregate}}):normalize_letters(fixture.base,
          {fixture.request(word,UINT64_MAX,warm::Exposure::Parts),fixture.request(fixture.english,4,warm::Exposure::Aggregate),
           fixture.request(fixture.french,8,warm::Exposure::Parts),fixture.request(fixture.accented,20,warm::Exposure::Parts),
           fixture.request(fixture.repeated,30,warm::Exposure::Aggregate)});
      return project_letters(bundle);
    }());
    auto domain=selected?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
        std::shared_ptr<ReservationDomain>{};
    std::weak_ptr<ReservationDomain> weak=domain;
    std::optional<LetterProjection> projection;
    if(selected)projection.emplace(Access::copy(*source,domain,structural));
    else {projection.emplace(*source);if(structural)Access::reserve(*projection);}
    P::independent<Access>(*source,*projection);
    source.reset();
    {
      const auto ordinary=*projection;
      P::independent<Access>(*projection,ordinary);
      P::inspect<Access>(ordinary,nullptr,[](const void *,std::size_t) {});
    }
    LetterProjection moved(std::move(*projection));projection.reset();projection.emplace(std::move(moved));
    std::vector<ParentContribution> expected,exported;
    for(const bool spare:{false,true})
      for(std::size_t i=0;i<ProjectionAccess::parent_count(*projection,spare);++i)
        expected.push_back(P::value<Access>(*projection,spare,i));
    exported.reserve(expected.size());std::vector<std::size_t> sites(expected.size());
    const auto selected_live=domain?domain->result().live_bytes:0;
    for(const auto index:{projection->parent_count(),SIZE_MAX}) {
      bool refused=false;
      try{const auto value=projection->parent(index);(void)value;}catch(const warm::Error &){refused=true;}
      check(refused,"public parent extraction checks the active bound and keeps the spare pool private");
    }
    A::begin();
    const auto before_observer=A::counts.attempts;
    const auto cost=P::inspect<Access>(*projection,domain.get(),[](const void *,std::size_t) {});
    check(A::counts.attempts==before_observer,"parent ownership witness makes no allocation");
    std::size_t row_index=0,refusals=0;
    std::uint64_t export_bytes=0,export_requests=0,active_export_bytes=0,active_export_requests=0;
    bool max_entry=false,multi_digit=false,multi_byte=false,repeated_target=false;
    for(const bool spare:{false,true}) {
      const auto &rows=Access::parent_rows(*projection,spare);
      PreparationWork malformed(UINT64_MAX);
      check(!P::matches<Access>(*projection,spare,SIZE_MAX,expected.front().occurrence,malformed) &&
            !P::provenance<Access>(*projection,spare,rows.size(),malformed),"parent witness bounds are checked in the owning projection");
      bool refused=false;
      try{ProjectionAccess::parent(*projection,spare,rows.size());}catch(const warm::Error &){refused=true;}
      check(refused,"production parent reader checks the selected pool bound");
      for(std::size_t i=0;i<rows.size();++i,++row_index) {
        const auto &row=ProjectionAccess::parent(*projection,spare,i);
        const auto &key=expected[row_index].occurrence;
        max_entry|=row.source.entry==UINT64_MAX;
        multi_digit|=row.source.path.size()==2 && row.source.path[0]==10;
        multi_byte|=row.source.path.size()==2 && row.source.path[1]>0;
        for(std::size_t j=0;j<i;++j)repeated_target|=rows[j].source==row.source &&
            ProjectionAccess::parent_target(*projection,spare,j)==ProjectionAccess::parent_target(*projection,spare,i) &&
            rows[j].ordinal!=row.ordinal;
        PreparationWork full(UINT64_MAX),numeric(UINT64_MAX);
        check(P::matches<Access>(*projection,spare,i,key,full) &&
              ProjectionAccess::parent_matches(*projection,spare,i,key.study,key.source,numeric) &&
              full.result().charged==numeric.result().charged,"production full-key and split-key parent readers agree with the same comparison work");
        for(const auto limit:{UINT64_C(0),numeric.result().charged-1}) {
          PreparationWork short_work(limit);bool refused=false;
          try{P::matches<Access>(*projection,spare,i,key,short_work);}catch(const PreparationStop &){refused=true;}
          check(refused,"parent header/numeric witness refuses zero/one-short work");
        }
        PreparationWork exact(numeric.result().charged);
        const auto before=A::counts.attempts;A::fail_at=before+1;
        const auto matched=P::matches<Access>(*projection,spare,i,key,exact);
        A::fail_at=0;
        check(matched && A::counts.attempts==before,"parent header/numeric witness retries with exact work and every allocation refused");
        auto foreign=key;foreign.study=warm::new_uuid();
        PreparationWork invalid(UINT64_MAX);
        check(!P::matches<Access>(*projection,spare,i,foreign,invalid) &&
              !ProjectionAccess::parent_matches(*projection,spare,i,foreign.study,foreign.source,invalid),
              "production readers reject a foreign study with identical parent entry/path");
        foreign=key;foreign.source.entry^=1;
        check(!P::matches<Access>(*projection,spare,i,foreign,invalid),"parent witness preserves the complete numeric entry");
        foreign=key;foreign.source.path.push_back(UINT32_MAX);
        check(!P::matches<Access>(*projection,spare,i,foreign,invalid),"parent witness preserves the complete numeric path");
        const auto export_before=A::counts;
        allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
        exported.push_back(P::value<Access>(*projection,spare,i));
        allocation_observation::enabled=false;
        sites[row_index]=A::counts.attempts-export_before.attempts;
        export_requests+=sites[row_index];export_bytes+=allocation_observation::bytes.load();
        if(!spare) {active_export_requests+=sites[row_index];active_export_bytes+=allocation_observation::bytes.load();}
        check(allocation_observation::calls.load()==sites[row_index] &&
              A::counts.live-export_before.live==allocation_observation::bytes.load(),
              "parent copy requests equal independently observed retained nested backing");
        for(std::size_t fail=1;fail<=sites[row_index];++fail) {
          const auto prior=A::counts;A::fail_at=prior.attempts+fail;bool refused=false;
          try{const auto value=P::value<Access>(*projection,spare,i);(void)value;}catch(const std::bad_alloc &){refused=true;}
          A::fail_at=0;
          check(refused && A::counts.live==prior.live && A::counts.blocks==prior.blocks &&
                (!domain || domain->result().live_bytes==selected_live),"every parent value-copy allocation failure unwinds and preserves selected storage");
          {const auto retry=P::value<Access>(*projection,spare,i);check(equal(retry,expected[row_index]),"parent value copy retries with exact full source/field/ordinal/count");}
          ++refusals;
        }
      }
    }
    check(max_entry && multi_digit && multi_byte && repeated_target,"parent controls include maximum entry, repeated targets and multi-digit/multibyte paths");
    check(!structural || cost.pools[1].rows,"structural selected/ordinary copies retain the inactive parent pool");
    projection.reset();domain.reset();
    bool detached=weak.expired();
    for(std::size_t i=0;i<exported.size();++i) {
      const auto &row=exported[i];
      detached &= equal(row,expected[i]) && !row.occurrence.study.str().get_allocator().domain &&
          !row.occurrence.source.path.get_allocator().domain && !row.field.object.scope.str().get_allocator().domain &&
          !row.field.object.address.get_allocator().domain;
    }
    check(detached,"complete parent values outlive source/recipe/projection/domain after copy and move");
    exported.clear();const auto released=A::end();
    check(!released.live && !released.blocks && released.acquired==released.released,"all parent observation and exported allocations are reclaimed");
    P::print("projection_parent_study_lifetime",structural?2:1,selected,cost);
    std::cout<<"projection_parent_copy_cost,recipe="<<(structural?2:1)<<",selected="<<selected
      <<",rows="<<expected.size()<<",requests="<<export_requests<<",requested="<<export_bytes
      <<",output_capacity_bytes="<<exported.capacity()*sizeof(ParentContribution)
      <<",active_export_requests="<<active_export_requests<<",active_export_requested="<<active_export_bytes
      <<",allocation_refusals="<<refusals<<",source_projection_domain_dead=1,exact_work_retry=1,after_destroy_requested=0 PASS\n";
  }
}
