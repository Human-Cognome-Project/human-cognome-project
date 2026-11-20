void projection_parent_target_cases() {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace T=projection_parent_target_observation;
  namespace S=projection_parent_study_observation;
  namespace A=allocation_lifetime;
  static_assert(!std::is_invocable_v<decltype(&T::resolve),const LetterProjection &,
      const ProjectionParentContribution &,PreparationWork &>,"selector reads require owning projection, pool and checked row index");
  for(const bool structural:{false,true})for(const bool selected:{false,true}) {
    auto source=std::make_unique<LetterProjection>([&] {
      Fixture f;const auto word=letter_fixture::object("02.00.20.00.2A");
      const auto character=f.base.tools.at(f.tools.at(f.english)).parents.front().target;
      f.add(word,std::string(12,'A'),24,std::vector<warm::ToolRef>(12,character));
      f.base.geometry.push_back({word,{},{{{},0.5}}});
      if(structural)f.base.focal_fields={{warm::FieldKind::SiblingDefinition,f.english}};
      return project_letters(structural?normalize_structural_letters(f.base,
          {{UINT64_MAX,f.ref(word),warm::Exposure::Parts},{4,f.ref(f.english),warm::Exposure::Aggregate},
           {8,f.ref(f.french),warm::Exposure::Aggregate},{20,f.ref(f.accented),warm::Exposure::Parts},
           {30,f.ref(f.repeated),warm::Exposure::Aggregate}}):normalize_letters(f.base,
          {f.request(word,UINT64_MAX,warm::Exposure::Parts),f.request(f.english,4,warm::Exposure::Aggregate),
           f.request(f.french,8,warm::Exposure::Aggregate),f.request(f.accented,20,warm::Exposure::Parts),
           f.request(f.repeated,30,warm::Exposure::Aggregate)}));
    }());
    if(structural)Access::reserve(*source);
    std::array<std::array<T::Selectors,256>,2> selectors{};
    for(const bool spare:{false,true})for(std::size_t i=0;i<ProjectionAccess::parent_count(*source,spare);++i) {
      check(i<selectors[spare].size(),"target experiment has bounded temporary selector storage");
      PreparationWork setup(UINT64_MAX);const auto pair=T::select(*source,spare,i,setup);
      check(bool(pair),"actual retained parent resolves to independent source and recipe-parent selectors");
      selectors[spare][i]=*pair;
    }
    auto domain=selected?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
        std::shared_ptr<ReservationDomain>{};
    std::weak_ptr<ReservationDomain> weak=domain;
    std::optional<LetterProjection> projection;
    if(selected)projection.emplace(Access::copy(*source,domain,false));else projection.emplace(*source);
    source.reset();
    {LetterProjection moved(std::move(*projection));projection.reset();projection.emplace(std::move(moved));}
    std::vector<ParentContribution> output;std::vector<warm::ObjectKey> detached_targets;
    bool wrong_source=false,repeated_parent=false,full_path=false,max_entry=false,multi_digit=false;
    std::size_t rows=0,faults=0;std::uint64_t guard_work=0;
    for(const bool spare:{false,true})for(std::size_t i=0;i<ProjectionAccess::parent_count(*projection,spare);++i) {
      auto &actual=assembly::testing::ProjectionIndicesAccess::parents(*projection,spare)[i];
      const auto pair=selectors[spare][i];PreparationWork work(UINT64_MAX);
      const auto *target=T::resolve(*projection,spare,i,work);
      check(target && actual.occurrence_index==pair.occurrence && actual.parent_index==pair.parent &&
            target==&ProjectionAccess::tool(*projection,pair.occurrence).parents[pair.parent].target,
            "copied/moved production selectors retain exact recipe-target meaning through source death");
      ++rows;guard_work+=work.result().charged;
      max_entry|=actual.source.entry==UINT64_MAX;
      multi_digit|=actual.source.path.size()==2 && actual.source.path[0]==10;
      for(const auto limit:{UINT64_C(0),work.result().charged-1}) {
        PreparationWork short_work(limit);bool refused=false;
        try{T::resolve(*projection,spare,i,short_work);}catch(const PreparationStop &){refused=true;}
        check(refused,"selector guard refuses zero/one-short work before returning a target");
      }
      PreparationWork retry(work.result().charged);A::begin(1);
      const auto *resolved=T::resolve(*projection,spare,i,retry);
      const auto released=A::end();
      check(resolved==target && !released.attempts && !released.live && !released.blocks,
            "selector guard retries with exact work and all allocations refused");
      OccurrenceKey key{projection->study(),actual.source};PreparationWork matching(UINT64_MAX);
      check(T::matches(*projection,spare,i,key,matching),"full namespace/source associates only within its owning projection");
      key.study=warm::new_uuid();
      check(!T::matches(*projection,spare,i,key,matching),"same numeric selectors and source in a foreign study refuse association");
      for(const auto bad:{T::Selectors{UINT32_MAX,pair.parent},T::Selectors{pair.occurrence,UINT32_MAX}}) {
        actual.occurrence_index=bad.occurrence;actual.parent_index=bad.parent;
        PreparationWork invalid(UINT64_MAX);
        check(!T::resolve(*projection,spare,i,invalid),"production read refuses out-of-range source or parent selectors");
        if(!spare) {
          bool refused=false;try{const auto value=projection->parent(i);(void)value;}catch(const warm::Error &){refused=true;}
          check(refused,"public parent export refuses malformed selectors before copying output");
        }
        actual.occurrence_index=pair.occurrence;actual.parent_index=pair.parent;
      }
      if(!wrong_source)for(std::size_t j=0;j<projection->occurrence_count();++j) {
        if(j==pair.occurrence)continue;
        const auto &other=ProjectionAccess::row(*projection,j);const auto *tool=ProjectionAccess::resolve(*projection,j);
        if(!tool || other.active==spare || pair.parent>=tool->parents.size())continue;
        const auto &edge=tool->parents[pair.parent];
        if(edge.ordinal!=actual.ordinal || edge.contribution!=actual.count || !(edge.target==*target))continue;
        actual.occurrence_index=std::uint32_t(j);
        PreparationWork invalid(UINT64_MAX);
        check(!T::resolve(*projection,spare,i,invalid),
              "wrong valid source with identical full target/ordinal/count refuses the independent full source check");
        if(!spare) {
          bool refused=false;try{const auto value=projection->parent(i);(void)value;}catch(const warm::Error &){refused=true;}
          check(refused,"public export rejects a wrong valid source sharing the full target");
        }
        actual.occurrence_index=pair.occurrence;
        wrong_source=true;break;
      }
      if(!repeated_parent) {
        const auto &tool=ProjectionAccess::tool(*projection,pair.occurrence);
        for(std::size_t j=0;j<tool.parents.size();++j) {
          const auto &edge=tool.parents[j];
          if(j==pair.parent || edge.ordinal==actual.ordinal || edge.contribution!=actual.count || !(edge.target==*target))continue;
          actual.parent_index=std::uint32_t(j);
          PreparationWork invalid(UINT64_MAX);
          check(!T::resolve(*projection,spare,i,invalid),
                "wrong valid repeated target selector refuses its distinct independently retained ordinal");
          if(!spare) {
            bool refused=false;try{const auto value=projection->parent(i);(void)value;}catch(const warm::Error &){refused=true;}
            check(refused,"public export rejects another valid repeated-target ordinal");
          }
          actual.parent_index=pair.parent;
          repeated_parent=true;break;
        }
      }
      const auto kind=actual.kind;const auto ordinal=actual.ordinal;
      const auto count=actual.count,entry=actual.source.entry;
      for(int fault=0;fault<4;++fault) {
        if(fault==0)actual.kind=GroupKind::SiblingDefinition;
        if(fault==1)actual.ordinal^=1;
        if(fault==2)actual.count^=1;
        if(fault==3)actual.source.entry^=1;
        PreparationWork invalid(UINT64_MAX);
        check(!T::resolve(*projection,spare,i,invalid),"selector experiment refuses wrong kind, ordinal, count and full entry");
        actual.kind=kind;actual.ordinal=ordinal;actual.count=count;actual.source.entry=entry;++faults;
        PreparationWork healthy(UINT64_MAX);
        check(T::resolve(*projection,spare,i,healthy)==target,"healthy selector retry restores exact target after each retained-fact refusal");
      }
      for(std::size_t k=0;k<actual.source.path.size();++k) {
        actual.source.path[k]^=1;PreparationWork invalid(UINT64_MAX);
        check(!T::resolve(*projection,spare,i,invalid),"independent full source path refuses each changed character/byte ordinal");
        actual.source.path[k]^=1;full_path=true;++faults;
      }
      auto &occurrence=assembly::testing::ProjectionIndicesAccess::rows(*projection)[actual.occurrence_index];
      occurrence.active=!occurrence.active;PreparationWork wrong_role(UINT64_MAX);
      check(!T::resolve(*projection,spare,i,wrong_role),"steady production parent reader rejects the wrong pool exposure role");
      occurrence.active=!occurrence.active;++faults;
      output.push_back(S::value<Access>(*projection,spare,i));detached_targets.push_back(target->object);
    }
    PreparationWork invalid(UINT64_MAX);
    check(!T::resolve(*projection,false,SIZE_MAX,invalid),"selector reader checks the selected parent pool bound");
    check(wrong_source && repeated_parent && full_path && max_entry && multi_digit,
          "target controls cover repeated/shared targets, full paths, maximum entry and multi-digit ordinals");
    projection.reset();domain.reset();bool detached=weak.expired();
    for(std::size_t i=0;i<output.size();++i)detached &= output[i].field.object==detached_targets[i] &&
        !output[i].field.object.scope.str().get_allocator().domain && !output[i].field.object.address.get_allocator().domain &&
        !detached_targets[i].scope.str().get_allocator().domain && !detached_targets[i].address.get_allocator().domain;
    check(detached,"actual public values and independently copied recipe targets survive all source/projection/domain owners");
    std::cout<<"projection_parent_target_cases,recipe="<<(structural?2:1)<<",selected="<<selected
      <<",rows="<<rows<<",faults="<<faults<<",guard_work="<<guard_work
      <<",wrong_valid_source=1,wrong_repeated_parent=1,full_path=1,exact_work_retry=1,guard_allocations=0"
      <<",source_projection_domain_dead=1,retained_reader_bytes=0,production_reader=1 PASS\n";
  }
}
