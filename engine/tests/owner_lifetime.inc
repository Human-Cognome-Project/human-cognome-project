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

void unique_reservation_controls() {
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  struct alignas(64) Object {
    ReservedVector<int> nested;
    int *destroyed;
    Object(ReservationDomain *domain, int *d, bool fail)
        : nested(ReservationAllocator<int>(domain)), destroyed(d) {
      // The object's own storage must already be live during member work.
      if (domain) require(domain->result().live_bytes == sizeof(Object),
                          "unique object acquired its bytes after construction");
      nested.reserve(4);
      if (fail) throw 17;
    }
    ~Object() { ++*destroyed; }
  };
  int entered = 0, destroyed = 0;
  const auto make = [&](const auto &domain, bool fail = false) {
    return reserve_unique<Object>(domain,[&](auto *p) {
      ++entered;
      ::new (static_cast<void *>(p)) Object(domain.get(),&destroyed,fail);
    });
  };
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{sizeof(Object)-1,UINT64_MAX});
  no_new([&] {
    bool rejected = false;
    try { auto p = make(domain); } catch (const ReservationStop &) { rejected = true; }
    require(rejected && !entered && !destroyed && !domain->result().live_bytes,
            "unique allocation denial entered construction or acquired bytes");
  });
  // Exact object allowance cannot also fund nested construction; unwind must
  // reclaim the admitted outer block without calling a nonexistent destructor.
  domain = std::make_shared<ReservationDomain>(ReservationBudget{sizeof(Object),UINT64_MAX});
  A::begin();
  bool rejected = false;
  try { auto p = make(domain); } catch (const ReservationStop &) { rejected = true; }
  catch (...) { A::end(); throw; }
  auto released = A::end();
  require(rejected && entered == 1 && !destroyed && !domain->result().live_bytes &&
              domain->result().charged_work == sizeof(Object) && released.attempts == 1 && !released.live,
          "unique nested denial leaked or refunded the outer request");
  domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  A::begin();
  bool failed = false;
  try { auto p = make(domain,true); } catch (int value) { failed = value == 17; }
  catch (...) { A::end(); throw; }
  released = A::end();
  const auto bytes = domain->result().charged_work;
  require(failed && entered == 2 && !destroyed && !released.live && !released.blocks &&
              !domain->result().live_bytes && bytes == sizeof(Object)+4*sizeof(int),
          "unique constructor failure leaked object/members or refunded work");
  domain = std::make_shared<ReservationDomain>(ReservationBudget{bytes,bytes});
  A::begin(1);
  failed = false;
  try { auto p = make(domain); } catch (const std::bad_alloc &) { failed = true; }
  catch (...) { A::end(); throw; }
  released = A::end();
  require(failed && entered == 2 && !domain->result().live_bytes &&
              domain->result().charged_work == sizeof(Object) && !released.live,
          "unique allocator failure constructed an object or acquired phantom bytes");
  domain = std::make_shared<ReservationDomain>(ReservationBudget{bytes,bytes});
  std::weak_ptr<ReservationDomain> watch = domain;
  A::begin();
  try {
    auto original = make(domain);
    require(domain->result().live_bytes == A::counts.live && A::counts.live == bytes &&
                reinterpret_cast<std::uintptr_t>(original.get())%alignof(Object) == 0,
            "unique allocation charge/alignment differs from observed requests");
    ReservedUnique<Object> moved = std::move(original);
    ReservedUnique<Object> swapped;
    moved.swap(swapped);
    require(!original && !moved && swapped, "unique move/swap lost ownership");
    domain.reset();
    require(!watch.expired(), "unique deleter lost domain before object destruction");
    swapped.reset();
    require(watch.expired() && destroyed == 1,
            "unique reset retained domain or failed object destruction");
  } catch (...) { A::end(); throw; }
  released = A::end();
  require(!released.live && !released.blocks, "unique teardown leaked backing");
  // Null-domain standalone fixtures still use matching allocation/deallocation.
  { auto standalone = make(std::shared_ptr<ReservationDomain>{}); }
  require(destroyed == 2, "unbudgeted unique fixture did not destroy its object");
}

void projection_reservation_controls(const assembly::LetterProjection &projection) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> watch = domain;
  std::optional<assembly::LetterProjection> snapshot;
  std::uint64_t outer = 0;
  A::begin();
  try {
    {
      auto copy = Access::projection_copy(projection,domain);
      Access::reserve_projection(copy);
      outer = Access::projection_table_bytes(copy);
      require(outer && domain->result().live_bytes == outer && A::counts.live > outer,
              "projection table domain omitted backing or charged nested payload");
      for (const auto *d : Access::projection_domains(copy))
        require(d == domain.get(), "explicit projection copy detached a table allocator");
      snapshot.emplace(copy);
      for (const auto *d : Access::projection_domains(*snapshot))
        require(!d, "ordinary projection snapshot borrowed source quota");
      require(domain->result().live_bytes == outer && snapshot->study() == copy.study() &&
                  snapshot->occurrences().size() == copy.occurrences().size() &&
                  Access::projection_parent_sizes(*snapshot) == Access::projection_parent_sizes(copy),
              "projection snapshot changed source accounting or lost inactive contributions");
    }
    require(!domain->result().live_bytes, "projection destruction did not release its table bytes");
    domain.reset();
    require(watch.expired(), "detached projection snapshot retained its source domain");
    // Restore full parent capacity on the independent snapshot after source death.
    Access::reserve_projection(*snapshot);
    require(snapshot->study() == projection.study() &&
                snapshot->occurrences().front().key == projection.occurrences().front().key,
            "detached projection snapshot depended on destroyed source storage");
    snapshot.reset();
  } catch (...) { A::end(); throw; }
  const auto released = A::end();
  require(!released.live && !released.blocks, "projection copy/reservation leaked storage");

  domain = std::make_shared<ReservationDomain>(ReservationBudget{0,UINT64_MAX});
  A::begin();
  bool rejected = false;
  try { auto copy = Access::projection_copy(projection,domain); }
  catch (const ReservationStop &) { rejected = true; }
  catch (...) { A::end(); throw; }
  const auto denied = A::end();
  require(rejected && !domain->result().live_bytes && !domain->result().charged_work &&
              !denied.live && !denied.blocks,
          "projection copy denial leaked preceding recipe or selected table allocations");
  // Moving a bounded value keeps its domain; copying the value detaches it.
  std::optional<assembly::LetterProjection> moved;
  {
    domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    watch = domain;
    auto copy = Access::projection_copy(projection,domain);
    moved.emplace(std::move(copy));
    domain.reset();
  }
  require(!watch.expired(), "moved projection lost its allocation domain");
  for (const auto *d : Access::projection_domains(*moved))
    require(d != nullptr, "moved projection detached its backing allocator");
  moved.reset();
  require(watch.expired(), "projection table allocator retained a lifetime cycle");
}

void catalogue_reservation_controls(const assembly::LetterProjection &projection) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  ReservationResult full;
  std::optional<assembly::MembershipFields> snapshot;
  std::weak_ptr<ReservationDomain> watch;
  {
    auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    watch = domain;
    {
      auto catalogue = Access::catalogue(projection,domain);
      full = domain->result();
      const auto &rows = catalogue.contributions();
      require(rows.get_allocator().domain == domain &&
                  full.live_bytes == rows.capacity()*sizeof(rows[0]) &&
                  full.charged_work > full.live_bytes,
              "catalogue omitted retained table or failed to release graph nodes");
      snapshot.emplace(catalogue);
      require(!snapshot->contributions().get_allocator().domain &&
                  domain->result().live_bytes == full.live_bytes,
              "catalogue value snapshot retained or charged its source domain");
    }
    require(!domain->result().live_bytes, "catalogue graph/table storage survived teardown");
  }
  require(watch.expired(), "detached catalogue snapshot extended domain lifetime");
  auto standalone = Access::catalogue(projection,{});
  require(snapshot->study() == standalone.study() &&
              snapshot->contributions().size() == standalone.contributions().size(),
          "catalogue snapshot lost identity or rows after domain death");
  for (std::size_t i=0; i<standalone.contributions().size(); ++i) {
    const auto &a=snapshot->contributions()[i], &b=standalone.contributions()[i];
    require(a.occurrence == b.occurrence && a.field.kind == b.field.kind &&
                a.field.object == b.field.object && a.target == b.target && a.count == b.count,
            "domain-bound catalogue changed membership semantics");
  }
  for (bool bytes : {true,false}) {
    const auto total = bytes ? full.peak_bytes : full.charged_work;
    std::uint64_t allowance=0;
    while (allowance < total) {
      auto domain = std::make_shared<ReservationDomain>(ReservationBudget{
          bytes ? allowance : UINT64_MAX, bytes ? UINT64_MAX : allowance});
      bool stopped=false;
      A::begin();
      try { auto catalogue=Access::catalogue(projection,domain); }
      catch (const ReservationStop &) { stopped=true; }
      catch (...) { A::end(); throw; }
      const auto released=A::end();
      const auto result=domain->result();
      const auto next=bytes ? result.required_bytes : result.required_work;
      require(stopped && result.status == (bytes ? ReservationStatus::ByteExhausted : ReservationStatus::WorkExhausted) &&
                  !result.live_bytes && !released.live && !released.blocks && next > allowance && next <= total,
              "catalogue graph allocation rejection failed cleanup/progress");
      allowance=next;
    }
  }
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{full.peak_bytes,full.charged_work});
  auto retry=Access::catalogue(projection,domain);
  require(domain->result().status == ReservationStatus::Ok &&
              domain->result().live_bytes == full.live_bytes &&
              domain->result().charged_work == full.charged_work,
          "exact catalogue construction retry failed");
}

void contract_reservation_controls(const assembly::LetterProjection &projection) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  const assembly::OccurrenceKey key{projection.study(),{projection.recipe().selections.front().entry,{}}};
  const auto target = projection.exposure(key.source.entry) == warm::Exposure::Aggregate
      ? warm::Exposure::Parts : warm::Exposure::Aggregate;
  const auto reference = assembly::declare_letter_exchange(projection,key,target);
  for (bool copying : {false,true}) {
    const auto construct = [&](const auto &domain) {
      return copying ? Access::contract_copy(reference,domain) : Access::contract(projection,key,target,domain);
    };
    ReservationResult full;
    std::optional<assembly::LetterExchangeContract> snapshot, moved;
    std::weak_ptr<ReservationDomain> watch;
    {
      auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
      watch = domain;
      auto contract = construct(domain);
      full = domain->result();
      std::uint64_t backing=contract.owner().source.path.capacity()*sizeof(std::uint32_t);
      require(contract.owner().source.path.get_allocator().domain == domain,
              "declaration owner path detached");
      for (const auto &entry : Access::contract_tables(contract)) backing += entry.second;
      for (const auto *side : {&contract.outgoing(),&contract.incoming()})
        for (const auto &source : *side) {
          require(source.parents.get_allocator().domain == domain,
                  "declaration parent backing detached during construction/copy");
          backing += source.parents.capacity()*sizeof(warm::Parent);
          require(source.occurrence.source.path.get_allocator().domain == domain,
                  "declaration source path detached");
          backing += source.occurrence.source.path.capacity()*sizeof(std::uint32_t);
        }
      require(full.live_bytes == backing && backing && Access::equivalent(contract,reference),
              "declaration omitted table backing or changed its structural values");
      snapshot.emplace(contract);
      for (const auto *side : {&snapshot->outgoing(),&snapshot->incoming()})
        for (const auto &source : *side)
          require(!source.parents.get_allocator().domain && !source.occurrence.source.path.get_allocator().domain,
                  "declaration snapshot retained nested parent domain");
      require(!snapshot->owner().source.path.get_allocator().domain && !snapshot->outgoing().get_allocator().domain && !snapshot->incoming().get_allocator().domain &&
                  !snapshot->leaves().get_allocator().domain && domain->result().live_bytes == backing,
              "declaration snapshot retained or charged its source domain");
      moved.emplace(std::move(contract));
    }
    require(!watch.expired() && Access::equivalent(*moved,reference),
            "moved declaration lost source domain or structural values");
    moved.reset();
    require(watch.expired() && Access::equivalent(*snapshot,reference),
            "detached declaration snapshot failed after source domain death");
    for (bool bytes : {true,false}) {
      const auto total=bytes ? full.peak_bytes : full.charged_work;
      std::uint64_t allowance=0;
      while (allowance < total) {
        auto domain=std::make_shared<ReservationDomain>(ReservationBudget{
            bytes ? allowance : UINT64_MAX,bytes ? UINT64_MAX : allowance});
        bool stopped=false;
        A::begin();
        try { auto contract=construct(domain); }
        catch (const ReservationStop &) { stopped=true; }
        catch (...) { A::end(); throw; }
        const auto released=A::end();
        const auto result=domain->result();
        const auto next=bytes ? result.required_bytes : result.required_work;
        require(stopped && result.status == (bytes ? ReservationStatus::ByteExhausted : ReservationStatus::WorkExhausted) &&
                    !result.live_bytes && !released.live && !released.blocks && next > allowance && next <= total,
                "declaration construction/copy rejection failed cleanup or progress");
        allowance=next;
      }
    }
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{full.peak_bytes,full.charged_work});
    auto retry=construct(domain);
    require(Access::equivalent(retry,reference) && domain->result().live_bytes == full.live_bytes &&
                domain->result().charged_work == full.charged_work,
            "exact declaration construction/copy retry failed");
  }
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
  unique_reservation_controls();
  projection_reservation_controls(projection);
  catalogue_reservation_controls(projection);
  contract_reservation_controls(projection);
  const TickSnapshot before(f);
  ReservationResult full;
  {
    auto owner = Access::adopt(f.h,projection,rows,fields,{UINT64_MAX,UINT64_MAX},full);
    const auto storage = Access::resources(*owner);
    std::uint64_t unique_bytes = 0;
    for (const auto &entry : Access::unique_objects(*owner)) unique_bytes += entry.second;
    require(full.status == ReservationStatus::Ok && full.live_bytes && full.peak_bytes >= full.live_bytes &&
                full.charged_work > full.live_bytes &&
                full.live_bytes > storage.binding.vector_bytes+storage.coordinator.vector_bytes+Access::map_outer_bytes(*owner)+
                    Access::declarations(*owner).capacity()*sizeof(std::shared_ptr<const assembly::LetterExchangeContract>)+
                    storage.declarations.object_bytes+storage.memberships.object_bytes+unique_bytes+
                    Access::projection_table_bytes(Access::projection(*owner))+
                    Access::projection_table_bytes(Access::spare_projection(*owner)),
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

void owner_projection_setup(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  const TickSnapshot before(f);
  const auto generation = f.h.prepared_storage().generation;
  const auto ticks = f.h.prepared_storage().completed_ticks;
  const auto parents = Access::projection_parent_sizes(projection);
  ProjectionSetupResult full;
  {
    auto owner = Access::adopt(f.h,projection,rows,fields,UINT64_MAX,full);
    require(owner && full.phase == ProjectionSetupPhase::Complete && full.work.charged,
            "projection setup did not finish its explicit work gate");
  }
  std::uint64_t allowance = 0, boundaries = 0;
  std::array<bool,5> phases{};
  while (allowance < full.work.charged) {
    ProjectionSetupResult stopped;
    A::begin();
    bool returned = false;
    try {
      auto owner = Access::adopt(f.h,projection,rows,fields,allowance,stopped);
      returned = bool(owner);
    } catch (...) { A::end(); throw; }
    const auto released = A::end();
    require(!returned && stopped.work.status == PreparationStatus::WorkExhausted &&
                stopped.work.charged <= allowance && stopped.work.next_required > allowance &&
                stopped.work.next_required <= full.work.charged && !released.live && !released.blocks &&
                (allowance || !released.attempts) &&
                f.h.ready() && f.h.prepared_storage().generation == generation &&
                f.h.prepared_storage().completed_ticks == ticks,
            "projection setup rejection leaked, allocated before zero gate or changed native authority");
    const auto phase = static_cast<std::size_t>(stopped.phase);
    require(phase < phases.size(), "projection setup rejection reported completion");
    phases[phase] = true;
    allowance = stopped.work.next_required;
    ++boundaries;
  }
  for (const auto visited : phases) require(visited, "projection setup sweep omitted a phase");
  ProjectionSetupResult short_result, exact;
  require(!Access::adopt(f.h,projection,rows,fields,full.work.charged-1,short_result),
          "one-short projection setup allowance succeeded");
  auto retry = Access::adopt(f.h,projection,rows,fields,full.work.charged,exact);
  require(retry && exact.phase == ProjectionSetupPhase::Complete &&
              exact.work.status == PreparationStatus::Ok && exact.work.charged == full.work.charged &&
              before == TickSnapshot(f) && parents == Access::projection_parent_sizes(projection),
          "exact projection setup retry failed or changed caller/native state");
  std::cout << "owner_projection_setup,selections=" << projection.recipe().selections.size()
            << ",boundaries=" << boundaries << ",work=" << full.work.charged << " PASS\n";
}

void owner_lifetime(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  namespace A = allocation_lifetime;
  owner_input_admission(f,projection,rows,fields);
  owner_reservation_admission(f,projection,rows,fields);
  owner_projection_setup(f,projection,rows,fields);
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
    const auto census = Access::resources(*owner);
    capacity_charge = census.total().charged_bytes();
    // On this qualified standard library allocate_shared colocates each object
    // and control block. Find its whole actual new request independently of the
    // reservation allocator; do not guess control-block layout or use nested census.
    const auto containing_request = [](const void *object) {
      const auto address = reinterpret_cast<std::uintptr_t>(object);
      for (const auto &entry : A::table) {
        const auto base = reinterpret_cast<std::uintptr_t>(entry.pointer);
        if (entry.pointer && address >= base && address-base < entry.bytes) return entry.bytes;
      }
      throw std::runtime_error("shared pointee missing from independent allocation observer");
    };
    std::uint64_t shared_bytes = containing_request(Access::memberships(*owner).get());
    for (const auto &p : Access::declarations(*owner)) shared_bytes += containing_request(p.get());
    const auto table_bytes = Access::declarations(*owner).capacity()*
        sizeof(std::shared_ptr<const assembly::LetterExchangeContract>);
    std::uint64_t unique_bytes = 0;
    for (const auto &entry : Access::unique_objects(*owner)) {
      const auto request = containing_request(entry.first);
      require(request == entry.second, "unique allocation request differs from object storage size");
      unique_bytes += request;
    }
    std::uint64_t projection_bytes = 0;
    for (const auto *p : {&Access::projection(*owner),&Access::spare_projection(*owner)})
      for (const auto &entry : Access::projection_tables(*p)) if (entry.second) {
        const auto request = containing_request(entry.first);
        require(request == entry.second, "projection table differs from observed allocation request");
        projection_bytes += request;
      }
    const auto &membership_rows = Access::memberships(*owner)->contributions();
    std::uint64_t declaration_bytes=0;
    for (const auto &declaration : Access::declarations(*owner)) {
      for (const auto &entry : Access::contract_tables(*declaration)) if (entry.second) {
        require(containing_request(entry.first) == entry.second,
                "declaration table differs from observed allocation request");
        declaration_bytes += entry.second;
      }
      for (const auto *side : {&declaration->outgoing(),&declaration->incoming()})
        for (const auto &source : *side) {
          const auto bytes=source.parents.capacity()*sizeof(warm::Parent);
          require(!bytes || containing_request(source.parents.data()) == bytes,
                  "declaration parent table differs from observed allocation request");
          declaration_bytes += bytes;
          const auto &path=source.occurrence.source.path;
          const auto path_bytes=path.capacity()*sizeof(std::uint32_t);
          require(!path_bytes || containing_request(path.data()) == path_bytes,
                  "declaration path differs from observed allocation request");
          declaration_bytes += path_bytes;
        }
    }
    const auto membership_bytes = membership_rows.capacity()*sizeof(membership_rows[0]);
    require(!membership_bytes || containing_request(membership_rows.data()) == membership_bytes,
            "catalogue table differs from observed allocation request");
    require(Access::reservations(*owner).live_bytes == census.binding.vector_bytes+
                census.coordinator.vector_bytes+Access::map_outer_bytes(*owner)+table_bytes+shared_bytes+unique_bytes+projection_bytes+membership_bytes+declaration_bytes,
            "owner domain differs from independently observed selected allocations");
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
    require(weak_catalogue.expired() && domain->result().live_bytes == catalogue_charge-membership_bytes,
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
