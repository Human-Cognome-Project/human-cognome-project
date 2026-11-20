void projection_study_identity_cases() {
  namespace S=projection_study_observation;
  using namespace assembly::detail;
  for(const bool structural:{false,true}) {
    Fixture fixture;
    const auto long_word=letter_fixture::object("02.00.20.00.2A");
    const auto character=fixture.base.tools.at(fixture.tools.at(fixture.english)).parents.front().target;
    fixture.add(long_word,std::string(12,'A'),24,std::vector<warm::ToolRef>(12,character));
    fixture.base.geometry.push_back({long_word,{},{{{},0.5}}});
    if(structural)fixture.base.focal_fields={{warm::FieldKind::SiblingDefinition,fixture.english}};
    const auto bundle=structural?normalize_structural_letters(fixture.base,
        {{UINT64_MAX,fixture.ref(long_word),warm::Exposure::Parts},
         {2,fixture.ref(fixture.english),warm::Exposure::Aggregate}}):
        normalize_letters(fixture.base,
            {fixture.request(long_word,UINT64_MAX,warm::Exposure::Parts),
             fixture.request(fixture.english,2,warm::Exposure::Aggregate)});
    const auto projection=project_letters(bundle),foreign_projection=project_letters(bundle);
    check(projection.study()!=foreign_projection.study(),"equal recipe and numeric keys retain independent study scopes");
    const auto &rows=ProjectionAccess::rows(projection);
    bool max_entry=false,two_digits=false;
    for(std::size_t i=0;i<rows.size();++i) {
      const auto &row=rows[i];
      const auto value=projection.occurrence(i);
      PreparationWork full(UINT64_MAX);
      check(S::matches(projection.study(),row.key.source,value.key,full) && value.key==row.key,
            "owned full key equals its exact projection header plus numeric occurrence");
      auto foreign=value.key;foreign.study=foreign_projection.study();
      PreparationWork denied_context(UINT64_MAX);
      check(!S::matches(projection.study(),row.key.source,foreign,denied_context),
            "same entry/path in another study refuses header association");
      foreign=value.key;foreign.source.entry^=1;
      check(!S::matches(projection.study(),row.key.source,foreign,denied_context),"different full entry refuses header association");
      foreign=value.key;foreign.source.path.push_back(UINT32_MAX);
      check(!S::matches(projection.study(),row.key.source,foreign,denied_context),"different full numeric path refuses header association");
      for(const auto limit:{UINT64_C(0),full.result().charged-1}) {
        PreparationWork denied(limit);bool refused=false;
        try{S::matches(projection.study(),row.key.source,value.key,denied);}catch(const PreparationStop &){refused=true;}
        check(refused,"header/numeric key comparison refuses zero/one-short work");
      }
      PreparationWork exact(full.result().charged);
      allocation_lifetime::begin(1);
      const auto matched=S::matches(projection.study(),row.key.source,value.key,exact);
      const auto no_allocation=allocation_lifetime::end();
      check(matched && !no_allocation.attempts && !no_allocation.live && !no_allocation.blocks,
            "exact key-comparison retry works with every C++ allocation refused");
      allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
      const auto full_text=occurrence_text(value.key);
      allocation_observation::enabled=false;
      const auto calls=allocation_observation::calls.load(),bytes=allocation_observation::bytes.load();
      allocation_observation::calls=allocation_observation::bytes=0;allocation_observation::enabled=true;
      const auto split_text=occurrence_text(projection.study(),row.key.source);
      allocation_observation::enabled=false;
      check(full_text==split_text && allocation_observation::calls==calls && allocation_observation::bytes==bytes,
            "header/numeric rendering retains exact opaque text and its existing allocation cost");
      max_entry |= row.key.source.entry==UINT64_MAX;
      two_digits |= row.key.source.path.size()==2 && row.key.source.path[0]==10;
    }
    check(max_entry && two_digits && rows.front().key.source.entry==2,
          "actual source preserves max entry, multi-digit path ordinals and numeric ordering");
    std::cout << "projection_study_identity_cases,recipe=" << (structural?2:1) << ",rows=" << rows.size()
              << ",foreign_study_refusal=1,full_path_text=1,exact_work_retry=1,comparison_allocations=0,runtime_saving=0 PASS\n";
  }
}
