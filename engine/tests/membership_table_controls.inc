// Public/count/value-only fixtures also run against the pinned pre-table source.
void membership_table_cost_controls() {
  using Access=assembly::testing::MembershipFieldsAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  for (int mode=0;mode<3;++mode) for (bool selected:{false,true}) {
    const std::size_t count=mode==0?0:mode==1?1:12;
    const auto fixture=[](int kind) {
      auto f=std::make_unique<membership_fixture::Fixture>();
      f->tool(f->base,f->english).member_of.clear();
      if (kind!=0) f->tool(f->base,f->english).member_of.push_back(f->ref(f->fr));
      return f;
    };
    const auto project=[&](const auto &f) {
      std::vector<assembly::LetterRequest> requests;
      for (std::size_t i=0;i<(count?count:1);++i)
        requests.push_back(f->request(f->english,i,warm::Exposure::Aggregate));
      return std::make_unique<LetterProjection>(project_letters(normalize_letters(f->base,requests)));
    };
    auto f=fixture(mode);
    auto p=project(f);
    const auto study=p->study();
    const auto target=f->ref(f->fr);
    std::vector<MembershipContribution> values; values.reserve(count);
    auto domain=selected?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
        std::shared_ptr<ReservationDomain>{};
    std::weak_ptr<ReservationDomain> weak=domain;
    PreparationWork work(UINT64_MAX);
    std::optional<MembershipFields> fields;
    A::begin();
    allocation_observation::calls=allocation_observation::bytes=0;
    allocation_observation::enabled=true;
    try {
      if (selected) fields.emplace(Access::bounded(*p,false,domain,work));
      else fields.emplace(membership_fields(*p));
    } catch (...) { allocation_observation::enabled=false; A::end(); throw; }
    allocation_observation::enabled=false;
    const auto requested=allocation_observation::bytes.load();
    const auto construction=A::counts;
    bool exact=fields->contribution_count()==count;
    for (std::size_t i=0;i<count;++i) values.push_back(fields->contribution(i));
    p.reset(); f.reset(); fields.reset(); domain.reset();
    for (std::size_t i=0;i<count;++i)
      exact &= values[i].occurrence.study==study && values[i].occurrence.source.entry==i &&
          values[i].occurrence.source.path.empty() && values[i].kind==GroupKind::Membership &&
          values[i].target==target && values[i].count==2;
    values.clear();
    const auto released=A::end();
    check(exact && weak.expired() && !released.live && !released.blocks && released.acquired==released.released,
          "empty/single/repeated factory preserves separate rows and detached values through source/catalogue/domain death");
    f=fixture(mode); p=project(f);
    if (selected) {
      for (const auto limit:{UINT64_C(0),work.result().charged-1}) {
        PreparationWork denied_work(limit);
        bool stopped=false;
        auto retry_domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
        try { Access::bounded(*p,false,retry_domain,denied_work); }
        catch (const PreparationStop &) {stopped=true;}
        check(stopped && !retry_domain->result().live_bytes,"empty/single/repeated zero/one-short work releases partial tables");
        PreparationWork exact_work(work.result().charged);
        {check(Access::bounded(*p,false,retry_domain,exact_work).contribution_count()==count,
               "empty/single/repeated exact work retries with the complete row count");}
        check(!retry_domain->result().live_bytes,"empty/single/repeated exact retry fully releases");
      }
    }
    for (std::size_t fail=1;fail<=construction.attempts;++fail) {
      auto denied=selected?std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX}):
          std::shared_ptr<ReservationDomain>{};
      PreparationWork attempt(UINT64_MAX);
      bool refused=false;
      A::begin(fail);
      try {
        if (selected) Access::bounded(*p,false,denied,attempt);
        else membership_fields(*p);
      } catch (const std::bad_alloc &) {refused=true;}
      const auto after=A::end();
      check(refused && after.attempts==fail && !after.live && !after.blocks &&
                after.acquired==after.released && (!denied || !denied->result().live_bytes),
            "every empty/single/repeated construction allocation refusal releases partial tables and pins");
    }
    std::cout << "membership_small_cost,mode=" << mode << ",selected=" << selected << ",rows=" << count
              << ",requests=" << construction.attempts << ",requested=" << requested
              << ",retained=" << construction.live << ",peak=" << construction.peak
              << ",work=" << work.result().charged << ",catalogue_object_bytes=" << sizeof(MembershipFields)
              << ",allocation_refusals=" << construction.attempts << ",detached=1,after_destroy_requested=0 PASS\n";
  }
}
