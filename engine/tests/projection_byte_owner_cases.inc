void projection_byte_owner_cases() {
  namespace B=projection_byte_owner_observation;
  using namespace assembly::detail;
  for(const bool structural:{false,true}) {
    Fixture fixture;
    const auto word=letter_fixture::object("02.00.20.00.2A");
    const auto character=fixture.base.tools.at(fixture.tools.at(fixture.english)).parents.front().target;
    fixture.add(word,std::string(12,'A'),24,std::vector<warm::ToolRef>(12,character));
    fixture.base.geometry.push_back({word,{},{{{},0.5}}});
    if(structural)fixture.base.focal_fields={{warm::FieldKind::SiblingDefinition,fixture.french}};
    const auto bundle=structural?normalize_structural_letters(fixture.base,
        {{UINT64_MAX,fixture.ref(word),warm::Exposure::Parts},{4,fixture.ref(fixture.english),warm::Exposure::Aggregate},
         {8,fixture.ref(fixture.french),warm::Exposure::Aggregate},{20,fixture.ref(fixture.accented),warm::Exposure::Parts},
         {30,fixture.ref(fixture.repeated),warm::Exposure::Parts}}):normalize_letters(fixture.base,
        {fixture.request(word,UINT64_MAX,warm::Exposure::Parts),fixture.request(fixture.english,4,warm::Exposure::Aggregate),
         fixture.request(fixture.french,8,warm::Exposure::Aggregate),fixture.request(fixture.accented,20,warm::Exposure::Parts),
         fixture.request(fixture.repeated,30,warm::Exposure::Parts)});
    auto projection=project_letters(bundle);
    const auto foreign=project_letters(bundle);
    std::size_t bytes=0;bool multi_digit=false,multi_byte=false,inactive=false;
    std::uint64_t replay_work=0;
    for(std::size_t i=0;i<projection.occurrence_count();++i) {
      const auto &row=ProjectionAccess::row(projection,i);if(!row.byte)continue;
      ++bytes;inactive|=!row.active;
      multi_digit|=row.source.entry==UINT64_MAX && row.source.path[0]==10;multi_byte|=row.source.path[1]>0;
      PreparationWork lookup(UINT64_MAX);
      const auto owner=ProjectionAccess::byte_owner_index(projection,i,&lookup);
      const auto value=projection.occurrence(i);
      PreparationWork full(UINT64_MAX);
      check(B::matches(projection,i,owner,value.byte->owner,full),
            "checked byte/owner indices preserve the actual complete owner and character/byte relation");
      replay_work+=full.result().charged;
      for(const auto allowance:{UINT64_C(0),lookup.result().charged-1}) {
        PreparationWork limited(allowance);bool refused=false;
        try{ProjectionAccess::byte_owner_index(projection,i,&limited);}catch(const PreparationStop &){refused=true;}
        check(refused,"production byte-owner resolution refuses zero/one-short work");
      }
      PreparationWork exact_lookup(lookup.result().charged);
      allocation_lifetime::begin(1);
      const auto resolved_owner=ProjectionAccess::byte_owner_index(projection,i,&exact_lookup);
      const auto lookup_released=allocation_lifetime::end();
      check(resolved_owner==owner && !lookup_released.attempts && !lookup_released.live && !lookup_released.blocks,
            "production owner index resolves with exact work and every allocation refused");
      const auto foreign_value=foreign.occurrence(i);
      PreparationWork invalid(UINT64_MAX);
      check(!B::matches(projection,i,owner,foreign_value.byte->owner,invalid) &&
            value.byte->owner.source==foreign_value.byte->owner.source,
            "foreign study with identical byte-owner numeric key refuses contextual association");
      std::size_t other=0;
      while(other<projection.occurrence_count() &&
            (!ProjectionAccess::row(projection,other).source.path.empty() || other==owner))++other;
      check(other<projection.occurrence_count() && !B::relation(projection,i,other,invalid),
            "wrong in-range aggregate owner index refuses even when byte definitions are shared");
      check(!B::relation(projection,i,i,invalid) && !B::relation(projection,i,SIZE_MAX,invalid) &&
            !B::relation(projection,SIZE_MAX,owner,invalid),"byte role and malformed indices refuse owner association");
      auto wrong=value.byte->owner;wrong.source.path.push_back(0);
      check(!B::matches(projection,i,owner,wrong,invalid),"complete owner path remains part of the association boundary");
      for(const auto allowance:{UINT64_C(0),full.result().charged-1}) {
        PreparationWork limited(allowance);bool refused=false;
        try{B::matches(projection,i,owner,value.byte->owner,limited);}catch(const PreparationStop &){refused=true;}
        check(refused,"byte-owner index replay refuses zero/one-short work");
      }
      PreparationWork exact(full.result().charged);
      allocation_lifetime::begin(1);
      const auto matched=B::matches(projection,i,owner,value.byte->owner,exact);
      const auto released=allocation_lifetime::end();
      check(matched && !released.attempts && !released.live && !released.blocks,
            "byte-owner replay succeeds with exact work and every allocation refused");
      if(bytes==1) {
        auto &stored=assembly::testing::ProjectionIndicesAccess::rows(projection)[i];
        const auto original_owner=stored.byte->owner_index;
        const auto character_ordinal=stored.byte->character_ordinal,byte_ordinal=stored.byte->byte_ordinal;
        for(int fault=0;fault<5;++fault) {
          if(fault==0)stored.byte->owner_index=UINT32_MAX;
          if(fault==1)stored.byte->owner_index=std::uint32_t(i);
          if(fault==2)stored.byte->owner_index=std::uint32_t(other);
          if(fault==3)stored.byte->character_ordinal^=1;
          if(fault==4)stored.byte->byte_ordinal^=1;
          const auto refused_owner=ProjectionAccess::byte_owner_index(projection,i);
          bool refused=false;try{auto output=projection.occurrence(i);}catch(const warm::Error &){refused=true;}
          stored.byte->owner_index=original_owner;stored.byte->character_ordinal=character_ordinal;stored.byte->byte_ordinal=byte_ordinal;
          check(refused && refused_owner==projection.occurrence_count(),
                "production owner/source/ordinal guards refuse malformed in-range association before owning export");
          check(ProjectionAccess::byte_owner_index(projection,i)==owner,
                "healthy production byte-owner resolution retries after each provenance fault");
        }
        const auto revision=stored.byte->character.revision;
        stored.byte->character.revision=warm::new_uuid();
        check(!B::relation(projection,i,owner,invalid),"byte character full revision cannot change under the owner index");
        stored.byte->character.revision=revision;
        const auto source=stored.source_index;stored.source_index=UINT32_MAX;
        check(!B::relation(projection,i,owner,invalid),"owner replay retains the independent indexed byte source guard");
        stored.source_index=source;
        check(B::relation(projection,i,owner,invalid),"healthy byte-owner relationship retries after provenance/source refusal");
      }
    }
    check(multi_digit && multi_byte && inactive,"owner inspection covers maximum entry, repeated characters, multi-digit path and inactive bytes");
    std::cout << "projection_byte_owner_cases,recipe=" << (structural?2:1) << ",bytes=" << bytes
              << ",replay_work=" << replay_work << ",foreign_study_refusal=1,wrong_owner_refusal=1"
              << ",full_provenance=1,exact_no_new_retry=1,runtime_saving=0 PASS\n";
  }
}
