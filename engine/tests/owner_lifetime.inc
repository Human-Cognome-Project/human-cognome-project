void allocation_lifetime_controls() {
  namespace A = allocation_lifetime;
  void *outside = ::operator new(19);
  A::begin();
  void *first = ::operator new(8);
  void *array = ::operator new[](24);
  void *aligned = ::operator new(64,std::align_val_t(64));
  ::operator delete(outside);
  const auto peak = A::counts;
  ::operator delete(first,std::size_t(8));
  ::operator delete[](array);
  ::operator delete(aligned,std::size_t(64),std::align_val_t(64));
  const auto done = A::end();
  require(peak.attempts == 3 && peak.live == 96 && peak.peak == 96 &&
              done.acquired == 3 && done.released == 3 && !done.live && !done.blocks,
          "allocation lifetime observer failed byte/delete controls");
  A::begin(2);
  first = ::operator new(7);
  bool failed = false;
  try { array = ::operator new[](13); } catch (const std::bad_alloc &) { failed = true; }
  ::operator delete(first);
  const auto failure = A::end();
  require(failed && failure.attempts == 2 && failure.acquired == 1 &&
              failure.released == 1 && failure.peak == 7 && !failure.live,
          "allocation lifetime observer failed injected-failure control");
}

void owner_lifetime(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  namespace A = allocation_lifetime;
  allocation_lifetime_controls();
  const TickSnapshot before(f);
  const auto generation = f.h.prepared_storage().generation;
  const auto ticks = f.h.prepared_storage().completed_ticks;
  A::begin();
  A::Counts retained, complete;
  std::uint64_t capacity_charge = 0;
  try {
    auto owner = Access::adopt(f.h,projection,rows,fields);
    retained = A::counts;
    capacity_charge = Access::resources(*owner).total().charged_bytes();
    // External aliases deliberately outlive the owner: neither the shared
    // control blocks nor their nested allocations may be reported as reclaimed.
    auto declaration = Access::declarations(*owner).front();
    auto catalogue = Access::memberships(*owner);
    owner.reset();
    const auto aliased = A::counts.live;
    declaration.reset();
    const auto catalogue_only = A::counts.live;
    catalogue.reset();
    complete = A::end();
    require(aliased > catalogue_only && catalogue_only > 0,
            "shared owner data did not survive until final alias release");
  } catch (...) { if (A::active) A::end(); throw; }
  require(retained.attempts && retained.live && retained.peak > retained.live &&
              !complete.live && !complete.blocks && complete.acquired == complete.released,
          "owner adoption/teardown failed peak or full reclamation witness");
  std::size_t failure_peak = 0;
  for (std::size_t at = 1; at <= retained.attempts; ++at) {
    A::begin(at);
    bool failed = false;
    try { auto owner = Access::adopt(f.h,projection,rows,fields); }
    catch (const std::bad_alloc &) { failed = true; }
    catch (...) { A::end(); throw; }
    const auto count = A::end();
    failure_peak = std::max(failure_peak,count.peak);
    require(failed && count.attempts == at && count.acquired == at-1 &&
                !count.live && !count.blocks && count.acquired == count.released,
            "owner allocation failure leaked storage or missed its injection point");
    require(f.h.ready() && f.h.prepared_storage().generation == generation &&
                f.h.prepared_storage().completed_ticks == ticks,
            "owner construction failure changed execution authority");
  }
  // Fresh adoption after every allocation site has failed remains usable.
  A::begin();
  try { auto retry = Access::adopt(f.h,projection,rows,fields); }
  catch (...) { A::end(); throw; }
  const auto retry = A::end();
  require(!retry.live && !retry.blocks && retry.attempts == retained.attempts &&
              before == TickSnapshot(f),
          "owner allocation-failure sweep changed native state or prevented retry");
  std::cout << "owner_lifetime,selections=" << projection.recipe().selections.size()
            << ",allocation_sites=" << retained.attempts
            << ",retained_requested_bytes=" << retained.live
            << ",peak_requested_bytes=" << retained.peak
            << ",capacity_charge=" << capacity_charge
            << ",failure_peak_requested_bytes=" << failure_peak
            << ",failure_sites_checked=" << retained.attempts
            << ",remaining_blocks=" << retry.blocks << " PASS\n";
}
