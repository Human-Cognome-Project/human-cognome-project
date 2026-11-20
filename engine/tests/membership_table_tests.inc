void membership_table_values() {
  using Access=assembly::testing::MembershipFieldsAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  membership_fixture::Fixture f;
  auto p=cost_projection(f,2);
  const auto source=Access::catalogue(*p);
  const auto facts=[](const MembershipFields &value) {
    std::vector<std::string> out;
    for (const auto &row:detached_memberships(value))
      out.push_back(occurrence_text(row.occurrence)+"|"+std::to_string(int(row.kind))+"|"+
          warm::key(row.target.object)+"|"+std::string(row.target.revision.str())+"|"+std::to_string(row.count));
    return out;
  };
  const auto expected=facts(source);
  const auto live=membership_fields(*p);
  for (const auto index:{static_cast<std::uint32_t>(Access::targets(live).size()),UINT32_MAX}) {
    auto invalid=live;
    Access::index(invalid,index);
    bool refused=false;
    try {invalid.contribution(0);} catch (const warm::Error &error) {
      refused=std::string_view(error.what())=="membership: target index out of range";
    }
    check(refused,"public export refuses an invalid catalogue-local target index before access");
  }
  membership_fixture::Fixture one;
  one.tool(one.base,one.english).member_of={one.ref(one.fr)};
  const auto one_projection=project_letters(normalize_letters(one.base,{one.request(one.english,0,warm::Exposure::Aggregate)}));
  const auto small=membership_fields(one_projection);
  {
    auto table=source;
    const auto original=Access::targets(table).front();
    auto revised=original; revised.revision=warm::new_uuid();
    auto rescope=original; rescope.object.scope=warm::new_uuid();
    auto readdress=original; readdress.object.address.push_back(codec::AddressElement{0,0,false});
    PreparationWork work(UINT64_MAX);
    const auto size=Access::targets(table).size();
    const auto old=Access::retain(table,original,work);
    const auto a=Access::retain(table,revised,work),b=Access::retain(table,rescope,work),c=Access::retain(table,readdress,work);
    check(old<size && a==size && b==size+1 && c==size+2 &&
              Access::targets(table)[a]==revised && Access::targets(table)[b]==rescope && Access::targets(table)[c]==readdress &&
              Access::retain(table,revised,work)==a,
          "target retention keys scope, complete address and revision while preserving first-use indices");
  }
  auto copy=source;
  check(facts(copy)==expected && Access::targets(copy).data()!=Access::targets(source).data(),
        "ordinary catalogue copies own independent tables and complete indexed values");
  for (std::size_t i=0;i<Access::targets(copy).size();++i)
    check(Access::targets(copy)[i].object.scope.str().data()!=Access::targets(source)[i].object.scope.str().data() &&
              Access::targets(copy)[i].object.address.data()!=Access::targets(source)[i].object.address.data() &&
              Access::targets(copy)[i].revision.str().data()!=Access::targets(source)[i].revision.str().data(),
          "copied target tables own all UUID/address/revision buffers independently");
  std::size_t assignment_sites=0;
  ReservationResult old_budget,full_budget;
  {
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    std::optional<MembershipFields> destination(Access::copy(small,domain));
    old_budget=domain->result();
    A::begin();
    *destination=source;
    assignment_sites=A::counts.attempts;
    full_budget=domain->result();
    check(facts(*destination)==expected && Access::targets(*destination).get_allocator().domain==domain,
          "copy assignment atomically replaces rows/table and preserves the destination domain");
    destination.reset();
    const auto after=A::end();
    check(!after.live && !after.blocks && !domain->result().live_bytes,"copy assignment releases old and replacement backing");
  }
  for (std::size_t fail=1;fail<=assignment_sites;++fail) {
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    auto destination=Access::copy(small,domain);
    const auto before=facts(destination);
    const auto *rows=Access::rows(destination).data();
    const auto *targets=Access::targets(destination).data();
    const auto live=domain->result().live_bytes;
    bool refused=false;
    A::begin(fail);
    try {destination=source;} catch (const std::bad_alloc &) {refused=true;}
    const auto after=A::end();
    check(refused && !after.live && !after.blocks && domain->result().live_bytes==live &&
              rows==Access::rows(destination).data() && targets==Access::targets(destination).data() && facts(destination)==before,
          "each copy-assignment allocation refusal preserves both original tables and releases partial copies");
    destination=source;
    check(facts(destination)==expected && facts(source)==expected,"healthy copy-assignment retry preserves independent source facts");
  }
  std::size_t quota_refusals=0;
  for (const bool bytes:{false,true}) {
    auto limit=bytes?old_budget.peak_bytes:old_budget.charged_work;
    const auto ceiling=bytes?full_budget.peak_bytes:full_budget.charged_work;
    while (limit<ceiling) {
      auto domain=std::make_shared<ReservationDomain>(ReservationBudget{bytes?limit:UINT64_MAX,bytes?UINT64_MAX:limit});
      auto destination=Access::copy(small,domain);
      const auto live=domain->result().live_bytes;
      const auto before=facts(destination);
      bool refused=false;
      try {destination=source;} catch (const ReservationStop &) {refused=true;}
      const auto result=domain->result();
      const auto next=bytes?result.required_bytes:result.required_work;
      check(refused && next>limit && next<=ceiling && result.live_bytes==live && facts(destination)==before,
            "each copy-assignment reservation refusal preserves old table/index associations");
      limit=next; ++quota_refusals;
    }
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{bytes?ceiling:UINT64_MAX,bytes?UINT64_MAX:ceiling});
    auto destination=Access::copy(small,domain);
    destination=source;
    check(facts(destination)==expected,"copy assignment succeeds at its exact observed reservation boundary");
  }
  for (const bool same:{false,true}) {
    auto left=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    auto right=same?left:std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    auto destination=Access::copy(small,left);
    auto value=Access::copy(source,right);
    const auto *table=Access::targets(value).data();
    const auto *rows=Access::rows(value).data();
    allocation_observation::calls=allocation_observation::bytes=0;
    allocation_observation::enabled=true;
    destination=std::move(value);
    auto *self=&destination; destination=std::move(*self);
    allocation_observation::enabled=false;
    check(!allocation_observation::calls && facts(destination)==expected && Access::targets(destination).data()==table &&
              Access::rows(destination).data()==rows && Access::targets(destination).get_allocator().domain==right &&
              (same || !left->result().live_bytes),
          "move assignment transfers both tables without allocation and releases displaced same/unequal-domain storage");
    auto moved=std::move(destination);
    auto *copy_self=&moved; moved=*copy_self;
    check(facts(moved)==expected,"move construction and self-copy preserve complete indexed values");
  }
  std::cout << "membership_table_values,assignment_allocation_refusals=" << assignment_sites
            << ",assignment_quota_refusals=" << quota_refusals << ",full_pin_identity=1,copy_move=1,healthy_retry=1 PASS\n";
}
