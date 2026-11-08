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

void owner_input_admission(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  const TickSnapshot before(f);
  AdoptionInputResult full;
  no_new([&] { full = Access::input_admission(projection,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX}); });
  require(full.status == AdoptionInputStatus::Ok && full.phase == AdoptionInputPhase::Complete &&
              full.caller_bytes >= full.snapshot_bytes && full.snapshot_bytes &&
              full.required_bytes == full.caller_bytes+full.snapshot_bytes &&
              full.copy.charged == full.snapshot_bytes && full.inspection.charged,
          "adoption input charge omitted source/snapshot or work");
  std::uint64_t boundaries = 0;
  no_new([&] {
    std::uint64_t allowance = 0;
    while (allowance < full.inspection.charged) {
      AdoptionInputResult result;
      auto rejected = Access::adopt(f.h,projection,rows,fields,{UINT64_MAX,allowance,UINT64_MAX},result);
      require(!rejected && result.status == AdoptionInputStatus::WorkExhausted &&
                  result.phase == AdoptionInputPhase::Inspection && result.inspection.charged <= allowance &&
                  result.inspection.next_required > allowance && result.inspection.next_required <= full.inspection.charged &&
                  !result.caller_bytes && !result.snapshot_bytes && !result.required_bytes && !result.copy.charged,
              "adoption inspection exhaustion allocated or published a partial charge");
      allowance = result.inspection.next_required; ++boundaries;
    }
    for (auto limit : {UINT64_C(0),full.required_bytes-1}) {
      AdoptionInputResult result;
      auto rejected = Access::adopt(f.h,projection,rows,fields,{limit,full.inspection.charged,UINT64_MAX},result);
      require(!rejected && result.status == AdoptionInputStatus::ByteExhausted &&
                  result.phase == AdoptionInputPhase::Copy && result.required_bytes == full.required_bytes &&
                  !result.copy.charged, "adoption byte exhaustion copied inputs or allocated owner");
    }
    for (auto limit : {UINT64_C(0),full.copy.charged-1}) {
      AdoptionInputResult result;
      auto rejected = Access::adopt(f.h,projection,rows,fields,{full.required_bytes,full.inspection.charged,limit},result);
      require(!rejected && result.status == AdoptionInputStatus::WorkExhausted &&
                  result.phase == AdoptionInputPhase::Copy && !result.copy.charged &&
                  result.copy.next_required == full.copy.charged,
              "adoption copy exhaustion allocated before admission");
    }
    PreparationWork meter(UINT64_MAX);
    auto total = UINT64_MAX;
    bool overflow = false;
    try { input_add(total,1,meter); } catch (const PreparationStop &stop) {
      overflow = stop.result.status == PreparationStatus::Overflow;
    }
    require(overflow && total == UINT64_MAX, "input byte sum overflow changed its accumulator");
    total = 0; overflow = false;
    try { input_elements(total,UINT64_MAX,2,meter); } catch (const PreparationStop &stop) {
      overflow = stop.result.status == PreparationStatus::Overflow;
    }
    require(overflow && !total, "input byte product overflow changed its accumulator");
  });
  // Capacity-only changes are caller occupancy, never copied logical payload.
  auto spare_rows = rows;
  const auto row_charge = Access::input_admission(projection,spare_rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  const auto old_capacity = spare_rows.capacity();
  spare_rows.reserve(old_capacity+17);
  const auto slack = Access::input_admission(projection,spare_rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  require(slack.caller_bytes-row_charge.caller_bytes ==
              (spare_rows.capacity()-old_capacity)*sizeof(OccurrenceBinding) &&
              slack.snapshot_bytes == row_charge.snapshot_bytes && slack.copy.charged == row_charge.copy.charged,
          "input gate omitted caller slack or copied unused rows");
  auto nested = rows;
  const auto nested_before = Access::input_admission(projection,nested,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  const auto path_capacity = nested.front().occurrence.source.path.capacity();
  nested.front().occurrence.source.path.reserve(path_capacity+19);
  const auto nested_after = Access::input_admission(projection,nested,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  require(nested_after.caller_bytes-nested_before.caller_bytes ==
              (nested.front().occurrence.source.path.capacity()-path_capacity)*sizeof(std::uint32_t) &&
              nested_after.snapshot_bytes == nested_before.snapshot_bytes,
          "input gate omitted nested caller slack");
  nested.front().occurrence.source.path.push_back(3);
  const auto populated = Access::input_admission(projection,nested,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  require(populated.caller_bytes == nested_after.caller_bytes &&
              populated.snapshot_bytes-nested_after.snapshot_bytes == sizeof(std::uint32_t),
          "input snapshot charge omitted populated nested path");
  auto text_projection = projection;
  auto &text = const_cast<warm::Bundle &>(text_projection.recipe()).tools.front().recipe;
  const auto text_before = Access::input_admission(text_projection,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  const auto text_capacity = text.capacity();
  text.append(33,'x');
  const auto text_after = Access::input_admission(text_projection,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX});
  require(text_after.snapshot_bytes-text_before.snapshot_bytes == 33 &&
              text_after.caller_bytes-text_before.caller_bytes == text.capacity()-text_capacity,
          "input snapshot charge omitted recipe text or confused size with capacity");
  AdoptionInputResult result;
  auto retry = Access::adopt(f.h,projection,rows,fields,
      {full.required_bytes,full.inspection.charged,full.copy.charged},result);
  require(retry && result.status == AdoptionInputStatus::Ok && !Access::pending(*retry) &&
              before == TickSnapshot(f), "exact adoption input retry failed or changed native state");
  std::cout << "adoption_input,selections=" << projection.recipe().selections.size()
            << ",inspection_boundaries=" << boundaries << ",inspection_work=" << full.inspection.charged
            << ",caller_bytes=" << full.caller_bytes << ",snapshot_bytes=" << full.snapshot_bytes
            << ",required_bytes=" << full.required_bytes << " PASS\n";
}

void reservation_allocator_controls() {
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  ReservationDomain domain({32,UINT64_MAX});
  {
    ReservedVector<int> values{ReservationAllocator<int>(&domain)};
    values.reserve(4); values.push_back(123);
    const auto *backing = values.data();
    bool rejected = false;
    no_new([&] { try { values.reserve(8); } catch (const ReservationStop &) { rejected = true; } });
    require(rejected && domain.result().status == ReservationStatus::ByteExhausted &&
                domain.result().required_bytes >= 48 && domain.result().live_bytes == 16 &&
                values.data() == backing && values.size() == 1 && values[0] == 123,
            "reservation growth omitted overlapping old backing or changed vector on rejection");
  }
  require(!domain.result().live_bytes, "reservation vector destruction did not release charge");
  ReservationDomain measured({UINT64_MAX,UINT64_MAX});
  A::begin();
  try {
    ReservedVector<bool> bits{ReservationAllocator<bool>(&measured)};
    ReservedVector<int> integers{ReservationAllocator<int>(&measured)};
    bits.reserve(65); integers.reserve(7);
    require(measured.result().live_bytes == A::counts.live && measured.result().charged_work == A::counts.live &&
                measured.result().live_bytes >= 9+7*sizeof(int),
            "reservation allocator omitted actual bit-word or integer requests");
  } catch (...) { A::end(); throw; }
  const auto finished = A::end();
  require(!finished.live && !measured.result().live_bytes && measured.result().peak_bytes == finished.peak,
          "reservation allocator failed exact request/release witness");
  ReservationDomain work({UINT64_MAX,16});
  ReservationAllocator<int> allocator(&work);
  auto *p = allocator.allocate(4); allocator.deallocate(p,4);
  no_new([&] {
    bool rejected = false;
    try { p = allocator.allocate(1); } catch (const ReservationStop &) { rejected = true; }
    require(rejected && work.result().status == ReservationStatus::WorkExhausted &&
                work.result().charged_work == 16 && work.result().required_work == 20 && !work.result().live_bytes,
            "released bytes incorrectly refunded cumulative allocation work");
    ReservationDomain overflow({UINT64_MAX,UINT64_MAX});
    ReservationAllocator<std::uint64_t> huge(&overflow);
    rejected = false;
    try { huge.allocate(SIZE_MAX); } catch (const ReservationStop &) { rejected = true; }
    require(rejected && overflow.result().status == ReservationStatus::Overflow &&
                !overflow.result().live_bytes && !overflow.result().charged_work,
            "reservation multiplication overflow reached underlying allocation");
    ReservationDomain sum({UINT64_MAX,UINT64_MAX});
    sum.before(UINT64_MAX,1); sum.acquired(UINT64_MAX);
    rejected = false;
    try { sum.before(1,1); } catch (const ReservationStop &) { rejected = true; }
    require(rejected && sum.result().status == ReservationStatus::Overflow &&
                sum.result().live_bytes == UINT64_MAX && sum.result().charged_work == UINT64_MAX,
            "reservation sum overflow wrapped an existing charge");
    sum.released(UINT64_MAX);
  });
  ReservationDomain failed({UINT64_MAX,UINT64_MAX});
  A::begin(1);
  bool rejected = false;
  try { ReservationAllocator<int>(&failed).allocate(4); }
  catch (const std::bad_alloc &) { rejected = true; }
  const auto failure = A::end();
  require(rejected && !failed.result().live_bytes && failed.result().charged_work == 16 && !failure.live,
          "underlying allocator failure acquired phantom bytes or refunded admitted work");
}

void map_reservation_controls(const std::vector<assembly::detail::OccurrenceBinding> &rows) {
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  using Map = ExchangeMap<OccurrenceBinding>;
  const std::vector<OccurrenceBinding> one{rows.front()};
  const auto row_bytes = sizeof(OccurrenceBinding);
  ReservationDomain zero({0,UINT64_MAX});
  Map empty(&zero);
  no_new([&] {
    bool rejected = false;
    try { empty.reserve(3,4); } catch (const ReservationStop &) { rejected = true; }
    require(rejected && zero.result().status == ReservationStatus::ByteExhausted &&
                !zero.result().live_bytes && empty.storage().empty(),
            "map outer rejection allocated nested slots or published storage");
  });
  ReservationDomain growth({2*row_bytes,UINT64_MAX});
  {
    Map map(one,&growth);
    const auto *backing = map.storage().data();
    no_new([&] {
      bool rejected = false;
      try { map.reserve(2,4); } catch (const ReservationStop &) { rejected = true; }
      require(rejected && growth.result().required_bytes >= 3*row_bytes &&
                  growth.result().live_bytes == row_bytes && growth.result().charged_work == row_bytes &&
                  map.storage().data() == backing && map.size() == 1 && map[0].occurrence == one[0].occurrence,
              "map growth omitted old/new overlap or changed committed rows");
    });
  }
  require(!growth.result().live_bytes, "map growth rejection leaked its original store");
  ReservationDomain measured({UINT64_MAX,UINT64_MAX});
  A::begin();
  try {
    Map map(one,&measured);
    map.reserve(3,4);
    require(measured.result().live_bytes == map.storage().capacity()*row_bytes &&
                A::counts.live > measured.result().live_bytes,
            "map accounting confused outer storage with untracked nested keys");
  } catch (...) { A::end(); throw; }
  const auto released = A::end();
  require(!released.live && !released.blocks && !measured.result().live_bytes,
          "map outer/nested storage was not reclaimed together");
  std::optional<Map> detached;
  {
    ReservationDomain first({UINT64_MAX,UINT64_MAX}), second({UINT64_MAX,UINT64_MAX});
    Map original(one,&first); original.reserve(3,4);
    const auto live = first.result().live_bytes;
    detached.emplace(original);
    require(!detached->storage().get_allocator().domain && first.result().live_bytes == live &&
                detached->storage().size() == original.storage().size() &&
                detached->storage()[2].occurrence.source.path.capacity() >= 4,
            "plain map snapshot borrowed owner domain or lost inactive nested reservation");
    Map copy(original,&second);
    require(second.result().live_bytes == copy.storage().capacity()*row_bytes &&
                copy.storage().get_allocator().domain == &second,
            "explicit map copy bypassed its requested allocation domain");
    no_new([&] {
      original = copy;
      require(original.storage().get_allocator().domain == &first,
              "map assignment replaced the destination domain");
      original.swap(copy);
      require(original.storage().get_allocator().domain == &second &&
                  copy.storage().get_allocator().domain == &first,
              "map swap left an allocator attached to foreign backing");
      bool rejected = false;
      try { Map denied(original,&zero); } catch (const ReservationStop &) { rejected = true; }
      require(rejected && !zero.result().live_bytes,
              "explicit map-copy rejection allocated before outer admission");
    });
  }
  // The copied snapshot remains usable after both source allocation domains die.
  detached->reserve(4,5);
  require(detached->size() == 1 && detached->operator[](0).occurrence == one[0].occurrence &&
              !detached->storage().get_allocator().domain,
          "detached map snapshot depended on a destroyed source domain");
}

void shared_reservation_controls() {
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  struct alignas(64) Object {
    int *destroyed;
    explicit Object(int *d, bool fail = false) : destroyed(d) { if (fail) throw 17; }
    ~Object() { ++*destroyed; }
  };
  int destroyed = 0;
  auto zero = std::make_shared<ReservationDomain>(ReservationBudget{0,UINT64_MAX});
  no_new([&] {
    bool rejected = false;
    try { auto p = std::allocate_shared<Object>(SharedReservationAllocator<Object>(zero),&destroyed); }
    catch (const ReservationStop &) { rejected = true; }
    require(rejected && !zero->result().live_bytes && !destroyed,
            "shared allocation bypassed admission before construction");
  });
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> watch = domain;
  std::weak_ptr<Object> weak;
  std::uint64_t bytes = 0;
  A::begin();
  try {
    auto p = std::allocate_shared<Object>(SharedReservationAllocator<Object>(domain),&destroyed);
    bytes = domain->result().live_bytes;
    require(bytes == A::counts.live && bytes >= sizeof(Object) &&
                domain->result().charged_work == bytes && !destroyed,
            "shared allocation charge differs from actual object/control-block request");
    weak = p;
    auto alias = p;
    p.reset();
    require(!destroyed && domain->result().live_bytes == bytes,
            "shared alias released live accounting early");
    alias.reset();
    require(destroyed == 1 && weak.expired() && domain->result().live_bytes == bytes &&
                A::counts.live == bytes,
            "weak reference lost its charged control-block allocation");
    domain.reset();
    require(!watch.expired(), "shared allocator let its domain die before deallocation");
    weak.reset();
    require(watch.expired(), "shared allocation domain formed a reference cycle");
  } catch (...) { A::end(); throw; }
  const auto released = A::end();
  require(!released.live && !released.blocks, "shared allocation was not reclaimed");

  // Constructor failure releases storage but never refunds prepaid request work.
  domain = std::make_shared<ReservationDomain>(ReservationBudget{bytes,bytes});
  bool failed = false;
  A::begin();
  try { auto p = std::allocate_shared<Object>(SharedReservationAllocator<Object>(domain),&destroyed,true); }
  catch (int value) { failed = value == 17; }
  catch (...) { A::end(); throw; }
  const auto unwound = A::end();
  require(failed && !unwound.live && !unwound.blocks && !domain->result().live_bytes &&
              domain->result().charged_work == bytes && destroyed == 1,
          "shared constructor failure leaked or refunded allocation work");
  no_new([&] {
    bool rejected = false;
    try { auto p = std::allocate_shared<Object>(SharedReservationAllocator<Object>(domain),&destroyed); }
    catch (const ReservationStop &) { rejected = true; }
    require(rejected && domain->result().status == ReservationStatus::WorkExhausted,
            "shared allocation reused already spent request work");
  });
  domain = std::make_shared<ReservationDomain>(ReservationBudget{bytes,bytes});
  { auto retry = std::allocate_shared<Object>(SharedReservationAllocator<Object>(domain),&destroyed); }
  require(!domain->result().live_bytes && destroyed == 2, "exact shared allocation retry failed");

  // Ordinary table copies detach accounting but retain shared pointee lifetime.
  using Table = std::vector<int,SharedReservationAllocator<int>>;
  std::optional<Table> snapshot;
  {
    auto table_domain = std::make_shared<ReservationDomain>(ReservationBudget{4*sizeof(int),UINT64_MAX});
    Table original{SharedReservationAllocator<int>(table_domain)};
    original.reserve(4); original.push_back(29);
    snapshot.emplace(original);
    require(!snapshot->get_allocator().domain && table_domain->result().live_bytes == 4*sizeof(int),
            "table snapshot borrowed or charged its source domain");
    const auto *backing = original.data();
    no_new([&] {
      bool rejected = false;
      try { original.reserve(8); } catch (const ReservationStop &) { rejected = true; }
      require(rejected && original.data() == backing && original[0] == 29 &&
                  table_domain->result().required_bytes >= 12*sizeof(int),
              "shared table growth omitted old/new allocation overlap");
    });
  }
  snapshot->push_back(31);
  require(snapshot->front() == 29, "detached table snapshot lost its contents");
}

void owner_reservation_admission(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  reservation_allocator_controls();
  map_reservation_controls(rows);
  shared_reservation_controls();
  const TickSnapshot before(f);
  ReservationResult full;
  {
    auto owner = Access::adopt(f.h,projection,rows,fields,{UINT64_MAX,UINT64_MAX},full);
    const auto storage = Access::resources(*owner);
    require(full.status == ReservationStatus::Ok && full.live_bytes && full.peak_bytes >= full.live_bytes &&
                full.charged_work > full.live_bytes &&
                full.live_bytes > storage.binding.vector_bytes+storage.coordinator.vector_bytes+Access::map_outer_bytes(*owner)+
                    Access::declarations(*owner).capacity()*sizeof(std::shared_ptr<const assembly::LetterExchangeContract>)+
                    storage.declarations.object_bytes+storage.memberships.object_bytes,
            "reservation domain omitted fixed buffers or repeated temporary setup");
  }
  std::uint64_t byte_boundaries = 0, work_boundaries = 0;
  for (bool bytes : {true,false}) {
    const auto total = bytes ? full.peak_bytes : full.charged_work;
    std::uint64_t allowance = 0;
    while (allowance < total) {
      ReservationResult stopped;
      A::begin();
      bool returned = false;
      try {
        auto owner = Access::adopt(f.h,projection,rows,fields,
            {bytes ? allowance : UINT64_MAX,bytes ? UINT64_MAX : allowance},stopped);
        returned = bool(owner);
      } catch (...) { A::end(); throw; }
      const auto released = A::end();
      const auto next = bytes ? stopped.required_bytes : stopped.required_work;
      require(!returned && stopped.status == (bytes ? ReservationStatus::ByteExhausted : ReservationStatus::WorkExhausted) &&
                  !stopped.live_bytes && !released.live && !released.blocks && next > allowance && next <= total &&
                  (bytes ? stopped.peak_bytes : stopped.charged_work) <= allowance &&
                  f.h.ready(), "constructor reservation exhaustion leaked allocations or failed progress");
      allowance = next;
      if (bytes) ++byte_boundaries; else ++work_boundaries;
    }
  }
  ReservationResult exact;
  auto retry = Access::adopt(f.h,projection,rows,fields,{full.peak_bytes,full.charged_work},exact);
  require(retry && exact.status == ReservationStatus::Ok && exact.live_bytes == full.live_bytes &&
              exact.peak_bytes == full.peak_bytes && exact.charged_work == full.charged_work &&
              before == TickSnapshot(f), "exact constructor reservation retry failed or changed native state");
  std::cout << "owner_reservations,selections=" << projection.recipe().selections.size()
            << ",byte_boundaries=" << byte_boundaries << ",work_boundaries=" << work_boundaries
            << ",live_bytes=" << full.live_bytes << ",peak_bytes=" << full.peak_bytes
            << ",allocation_work=" << full.charged_work << " PASS\n";
}

void owner_lifetime(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  namespace A = allocation_lifetime;
  owner_input_admission(f,projection,rows,fields);
  owner_reservation_admission(f,projection,rows,fields);
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
    auto domain = Access::reservation_domain(*owner);
    std::weak_ptr<const assembly::MembershipFields> weak_catalogue = catalogue;
    owner.reset();
    const auto aliased = A::counts.live;
    const auto alias_charge = domain->result().live_bytes;
    declaration.reset();
    const auto catalogue_only = A::counts.live;
    const auto catalogue_charge = domain->result().live_bytes;
    catalogue.reset();
    require(weak_catalogue.expired() && domain->result().live_bytes == catalogue_charge,
            "owner catalogue control block lost its charge before weak release");
    weak_catalogue.reset();
    require(alias_charge > catalogue_charge && catalogue_charge && !domain->result().live_bytes,
            "owner shared allocation charge failed alias/weak release accounting");
    domain.reset();
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
