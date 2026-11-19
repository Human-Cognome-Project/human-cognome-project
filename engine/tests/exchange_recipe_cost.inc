// Actual adopted views, with observer scratch allocated before measurement.
void exchange_recipe_cost(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  owner_recipe_parent_lifetime(f,projection,rows,fields);
  using Access = assembly::testing::ExchangeOwnerAccess;
  namespace A = allocation_lifetime;
  struct Buffer { const void *data; std::size_t bytes; };
  std::size_t slots = 0;
  Access::recipe_backing(projection.recipe(),[&](auto, auto, auto) { ++slots; });
  std::vector<Buffer> committed, spare;
  committed.reserve(slots); spare.reserve(slots);
  auto source = std::make_unique<assembly::LetterProjection>(Access::projection_copy(projection,{}));
  const auto expected = projection.recipe();
  const auto entry = expected.selections.front().entry;
  const auto initial_exposure = projection.exposure(entry);
  const auto target = initial_exposure == warm::Exposure::Aggregate ? warm::Exposure::Parts : warm::Exposure::Aggregate;
  const assembly::OccurrenceKey key{projection.study(),{entry,{}}};
  // Same outer pin does not justify dropping changed inner meaning.
  auto changed = expected;
  changed.tools.front().ref.revision = warm::new_uuid();
  require(!Access::recipe_equivalent(expected,changed), "recipe comparison ignored definition revision");
  changed = expected;
  const auto parent_tool = std::find_if(changed.tools.begin(),changed.tools.end(),[](const auto &t) { return !t.parents.empty(); });
  require(parent_tool != changed.tools.end(), "recipe fixture has no parent provenance");
  parent_tool->parents.front().target.revision = warm::new_uuid();
  require(!Access::recipe_equivalent(expected,changed), "recipe comparison ignored full parent target pin");
  changed = expected; changed.geometry.front().spheres.front().radius += 1;
  require(!Access::recipe_equivalent(expected,changed), "recipe comparison ignored intrinsic geometry");
  changed = expected; changed.structural_occurrences.front().node += 1;
  require(!Access::recipe_equivalent(expected,changed), "recipe comparison ignored source node binding");
  allocation_lifetime_controls();
  const TickSnapshot before(f);
  A::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  A::Counts retained, released;
  std::uint64_t calls = 0, requested = 0, recipe_bytes = 0, recipe_blocks = 0, recipe_request = 0;
  std::uint64_t borrow_calls = 0, stage_calls = 0;
  exchange_parent_observation::Cost parent_cost;
  try {
    auto owner = Access::adopt(f.h,*source,rows,fields);
    allocation_observation::enabled = false;
    calls = allocation_observation::calls.load(); requested = allocation_observation::bytes.load();
    retained = A::counts;
    source.reset();
    const auto &current = Access::projection(*owner), &next = Access::spare_projection(*owner);
    const auto &declarations=Access::declarations(*owner);
    const warm::Tool *borrowed_tool=nullptr;
    no_new([&] {
      for (const auto &declaration:declarations)
        parent_cost.add(exchange_parent_observation::inspect_pinned<Access>(current,*declaration));
      assembly::detail::PreparationWork work(UINT64_MAX);
      borrowed_tool=exchange_parent_observation::lookup(current,*declarations.front(),
          declarations.front()->outgoing().front(),work);
      require(borrowed_tool,"caller-source death lost borrowed recipe parent authority");
    });
    const auto *domain = Access::reservation_domain(*owner).get();
    const auto append = [&](const auto &recipe, auto &out) {
      Access::recipe_backing(recipe,[&](const void *data, std::size_t bytes, const auto *selected) {
        require(selected == domain && out.size() < out.capacity(), "recipe backing escaped selected domain or inspection scratch");
        if (!bytes) return;
        const auto index = A::bucket(const_cast<void *>(data));
        const A::Entry *request = nullptr;
        for (std::size_t j=0; j<65536; ++j) {
          const auto &candidate = A::table[(index+j)%65536];
          if (!candidate.used) break;
          if (candidate.pointer == data) { request = &candidate; break; }
        }
        require(request && request->bytes == bytes, "recipe buffer differs from actual independent C++ request");
        out.push_back({data,bytes});
      });
    };
    no_new([&] { append(current.recipe(),committed); append(next.recipe(),spare); });
    require(committed.size() == spare.size() && !committed.empty(), "projection recipes differ in retained buffer shape");
    require(&current.recipe() == &next.recipe() && &current.recipe() != &projection.recipe(),
            "owner views do not share a privately detached recipe");
    for (std::size_t i=0; i<committed.size(); ++i) {
      require(committed[i].bytes == spare[i].bytes && committed[i].data == spare[i].data,
              "owner recipe backing differs between views");
      for (std::size_t j=0; j<committed.size(); ++j)
        require(i == j || committed[i].data != committed[j].data,
                "shared recipe buffer was attributed twice");
      recipe_bytes += spare[i].bytes; ++recipe_blocks;
    }
    const auto current_base = reinterpret_cast<std::uintptr_t>(&current);
    const auto next_base = reinterpret_cast<std::uintptr_t>(&next);
    const auto recipe_address = reinterpret_cast<std::uintptr_t>(&current.recipe());
    recipe_request = observed_shared_request(&current.recipe());
    bool current_request = false, next_request = false;
    for (const auto &request : A::table) {
      if (request.pointer == &current) current_request = request.bytes == sizeof(current);
      if (request.pointer == &next) next_request = request.bytes == sizeof(next);
    }
    require(current_request && next_request && recipe_request >= sizeof(warm::Bundle) &&
                !(recipe_address >= current_base && recipe_address-current_base < sizeof(current)) &&
                !(recipe_address >= next_base && recipe_address-next_base < sizeof(next)),
            "shared recipe request overlaps mutable projection storage");
    const auto *borrowed = &current.recipe();
    no_new([&] {
      require(Access::recipe_equivalent(*borrowed,expected) && Access::recipe_equivalent(*borrowed,next.recipe()),
              "adopted recipes lost complete values after caller-source teardown");
    });
    borrow_calls = allocation_observation::calls.load();
    no_new([&] { Access::stage_projection(*owner,key,target); });
    stage_calls = allocation_observation::calls.load();
    no_new([&] {
      require(current.exposure(entry) == initial_exposure && next.exposure(entry) == target &&
                  current.recipe().selections.front().exposure == expected.selections.front().exposure &&
                  next.recipe().selections.front().exposure == expected.selections.front().exposure &&
                  Access::recipe_equivalent(*borrowed,expected) && Access::recipe_equivalent(*borrowed,next.recipe()),
              "live exposure staging changed immutable recipe or committed exposure");
      assembly::detail::PreparationWork work(UINT64_MAX);
      require(exchange_parent_observation::lookup(next,*declarations.front(),
                  declarations.front()->outgoing().front(),work)==borrowed_tool,
              "prospective exposure changed immutable parent backing");
      for (const auto *view : {&current,&next}) {
        const auto &buffers = view == &current ? committed : spare;
        std::size_t i = 0;
        Access::recipe_backing(view->recipe(),[&](const void *data, std::size_t bytes, auto) {
          if (!bytes) return;
          require(i < buffers.size() && data == buffers[i].data && bytes == buffers[i].bytes,
                  "exposure staging changed retained recipe backing");
          ++i;
        });
        require(i == buffers.size(), "exposure staging lost recipe buffers");
      }
    });
    require(A::counts.attempts == retained.attempts && A::counts.live == retained.live,
            "recipe inspection/staging changed owner request lifetime");
    const auto bootstrap = Access::bootstrap_tracker(*owner).result().live_bytes;
    auto alias = Access::recipe_owner(current);
    std::weak_ptr<const warm::Bundle> weak_recipe = alias;
    std::weak_ptr<assembly::detail::ReservationDomain> weak = Access::reservation_domain(*owner);
    owner.reset();
    no_new([&] {
      require(std::any_of(alias->tools.begin(),alias->tools.end(),[&](const auto &tool) {
                return &tool==borrowed_tool && !tool.parents.empty();
              }),"explicit recipe pin did not retain borrowed parents after owner death");
    });
    require(!weak_recipe.expired() && !weak.expired() && Access::recipe_equivalent(*alias,expected) &&
                A::counts.live == recipe_bytes+recipe_request+bootstrap,
            "recipe alias lost meaning or retained mutable views after owner teardown");
    alias.reset();
    require(weak_recipe.expired() && !weak.expired() && domain->result().live_bytes == recipe_request &&
                A::counts.live == recipe_request+bootstrap && A::counts.blocks == 3,
            "weak recipe lost its shared allocation or selected-domain lifetime");
    weak_recipe.reset();
    require(weak.expired() && A::counts.live == bootstrap && A::counts.blocks == 2,
            "recipe views outlived owner or bootstrap weak-storage witness");
    weak.reset();
    released = A::end();
  } catch (...) { allocation_observation::enabled = false; if (A::active) A::end(); throw; }
  require(calls == retained.attempts && !released.live && !released.blocks && released.acquired == released.released &&
              before == TickSnapshot(f), "recipe inspection leaked C++ requests or changed native state");
  std::cout << "exchange_parent_lifetime,selections=" << expected.selections.size()
            << ",sources=" << parent_cost.sources << ",parents=" << parent_cost.parents
            << ",parent_requested=" << parent_cost.bytes() << ",parent_requests=" << parent_cost.requests
            << ",lookup_work=" << parent_cost.lookup_work
            << ",caller_dead=1,staged_exposure=1,explicit_pin_after_owner_death=1,weak_release=1,private_parent_backing=0 PASS\n";
  std::cout << "exchange_recipe_cost,selections=" << expected.selections.size()
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",owner_retained=" << retained.live << ",owner_peak=" << retained.peak
            << ",recipe_blocks_unique=" << recipe_blocks << ",recipe_requested_unique=" << recipe_bytes
            << ",recipe_header_bytes=" << sizeof(warm::Bundle) << ",projection_object_bytes=" << sizeof(assembly::LetterProjection)
            << ",recipe_payload_request=" << recipe_request << ",shared_recipe_views=2,borrow_calls=" << borrow_calls << ",stage_calls=" << stage_calls
            << ",after_destroy_requested=" << released.live << " PASS\n";
}
