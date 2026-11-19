// Native ownership control for the private declaration, separate from the
// actual constructor cost window. The caller's trusted maps remain unchanged.
void owner_recipe_parent_lifetime(Fixture &f,const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  auto caller=std::make_unique<assembly::LetterProjection>(projection);
  const auto entry=projection.recipe().selections.front().entry;
  const assembly::OccurrenceKey key{projection.study(),{entry,{}}};
  const auto target=projection.exposure(entry)==warm::Exposure::Aggregate ? warm::Exposure::Parts : warm::Exposure::Aggregate;
  const auto expected=assembly::declare_letter_exchange(projection,key,target);
  const TickSnapshot before(f);
  std::size_t sources=0,parents=0,refusals=0;
  A::begin();
  try {
    {
      auto owner=Access::adopt(f.h,*caller,rows,fields);
      auto declaration=Access::declarations(*owner).front();
      const auto &pin=Access::contract_recipe(*declaration);
      const auto &current=Access::projection(*owner);
      require(pin && pin.get()==&current.recipe() && pin.get()!=&caller->recipe() &&
                  pin.use_count()==long(2+Access::declarations(*owner).size()),
              "private declarations do not each pin the exact detached owner recipe once");
      for (const auto &contract:Access::declarations(*owner)) {
        require(Access::contract_recipe(*contract).get()==pin.get(),"private declaration borrowed another owner's recipe");
        for (const auto *side:{&contract->outgoing(),&contract->incoming()}) for (const auto &s:*side) {
          const auto index=Access::source_index(s);
          require(s.parents.empty() && !s.parents.capacity() && index<pin->tools.size() &&
                      pin->tools[index].ref==s.definition &&
                      &Access::contract_parents(*contract,s)==&pin->tools[index].parents,
                  "private source retained duplicate parents or resolved an unpinned index");
          ++sources; parents+=Access::contract_parents(*contract,s).size();
        }
      }
      caller.reset();
      auto detached=*declaration;
      auto assigned=expected;
      assigned=*declaration;
      auto selected=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
      auto explicit_copy=Access::contract_copy(*declaration,selected);
      require(!Access::contract_recipe(detached) && !Access::contract_recipe(assigned) &&
                  !Access::contract_recipe(explicit_copy) && Access::equivalent(detached,expected) &&
                  Access::equivalent(assigned,expected) && Access::equivalent(explicit_copy,expected),
              "public or explicit-domain copy retained private recipe borrowing");
      no_new([&] {
        require(declaration->matches(detached.outgoing().data(),detached.outgoing().size(),
                    detached.incoming().data(),detached.incoming().size(),declaration->match_work()) &&
                    !detached.matches(declaration->outgoing().data(),declaration->outgoing().size(),
                    declaration->incoming().data(),declaration->incoming().size(),UINT64_MAX),
                "raw matching inferred a private index without its declaration lifetime");
        require(Access::equivalent(*declaration,detached),"pinned and detached declaration facts differ");
      });
      auto changed=detached.outgoing();
      ++changed.front().parents.front().contribution;
      require(!declaration->matches(changed.data(),changed.size(),detached.incoming().data(),
                  detached.incoming().size(),UINT64_MAX),"pinned matching ignored changed parent facts");
      for (int defect=0;defect<3;++defect) {
        auto source=declaration->outgoing().front();
        if (defect==0) Access::source_index(source,SIZE_MAX);
        if (defect==1) Access::source_index(source,(Access::source_index(source)+1)%pin->tools.size());
        if (defect==2) source.definition.revision=warm::new_uuid();
        bool refused=false;
        try { Access::contract_parents(*declaration,source); } catch (const warm::Error &) {refused=true;}
        require(refused,"private recipe index escaped bounds or full definition guard"); ++refusals;
      }
      // Equal nominal pins do not authorize borrowing a different Bundle.
      auto foreign=std::make_shared<const warm::Bundle>(current.recipe());
      PreparationWork foreign_work(UINT64_MAX);
      bool refused=false;
      try { Access::pinned_contract(current,key,target,selected,foreign_work,foreign); }
      catch (const warm::Error &) {refused=true;}
      require(refused,"private declaration accepted another recipe with the same outer pin"); ++refusals;
      foreign.reset();
      const auto baseline=selected->result().live_bytes;
      PreparationWork full_work(UINT64_MAX);
      {
        auto private_value=Access::pinned_contract(current,key,target,selected,full_work,pin);
        auto moved=std::move(private_value);
        require(Access::contract_recipe(moved).get()==pin.get(),"private declaration move lost its recipe pin");
        moved=expected;
        require(!Access::contract_recipe(moved) && Access::equivalent(moved,expected),
                "value assignment retained private recipe sharing");
      }
      require(selected->result().live_bytes==baseline,"private value construction/assignment leaked selected backing");
      for (auto allowance:{UINT64_C(0),full_work.result().charged-1}) {
        PreparationWork work(allowance);
        refused=false;
        try { Access::pinned_contract(current,key,target,selected,work,pin); }
        catch (const PreparationStop &stop) {refused=stop.result.status==PreparationStatus::WorkExhausted;}
        require(refused && selected->result().live_bytes==baseline &&
                    pin.use_count()==long(2+Access::declarations(*owner).size()),
                "private construction work refusal leaked a recipe pin or partial source");
        ++refusals;
      }
      {
        PreparationWork exact(full_work.result().charged);
        auto retried=Access::pinned_contract(current,key,target,selected,exact,pin);
        require(exact.result().charged==full_work.result().charged && Access::equivalent(retried,expected),
                "exact private construction retry lost ordered parent facts");
      }
      require(selected->result().live_bytes==baseline,"exact private construction retry retained scratch backing");
      Access::stage_projection(*owner,key,target);
      require(Access::equivalent(*declaration,expected) &&
                  Access::contract_recipe(*declaration).get()==&Access::spare_projection(*owner).recipe(),
              "exposure staging changed the private declaration's immutable parents");
      std::weak_ptr<const warm::Bundle> weak_recipe=pin;
      std::weak_ptr<const assembly::LetterExchangeContract> weak_declaration=declaration;
      std::weak_ptr<ReservationDomain> weak_domain=Access::reservation_domain(*owner);
      owner.reset();
      no_new([&] {
        require(Access::contract_recipe(*declaration).use_count()==1 &&
                    !weak_recipe.expired() && Access::equivalent(*declaration,expected),
                "independent private declaration lost its recipe after owner death");
      });
      declaration.reset();
      require(weak_declaration.expired() && weak_recipe.expired() && !weak_domain.expired(),
              "private declaration retained a cycle or released weak allocator storage early");
      weak_declaration.reset();
      weak_recipe.reset();
      require(weak_domain.expired() && Access::equivalent(detached,expected) &&
                  Access::equivalent(assigned,expected) && Access::equivalent(explicit_copy,expected),
              "detached copies retained the source recipe/domain or lost parent values");
      weak_domain.reset();
    }
    const auto released=A::end();
    require(!released.live && !released.blocks && released.acquired==released.released,
            "private parent ownership control leaked allocation/domain/bootstrap backing");
  } catch (...) {if (A::active) A::end(); throw;}
  require(before==TickSnapshot(f),"private recipe-parent control changed native execution");
  std::cout << "owner_recipe_parents,selections=" << projection.recipe().selections.size()
            << ",sources=" << sources << ",parents=" << parents << ",refusals=" << refusals
            << ",pin_per_declaration=1,detached_copies=3,owner_death=1,weak_release=1 PASS\n";
}
