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

template<class T> void map_reservation_controls(const std::vector<T> &rows) {
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  using Map = ExchangeMap<T>;
  std::vector<T> one{rows.front()};
  if constexpr (std::is_same_v<T,OccurrenceBinding>) key_buffer(one.front())={3,5};
  else key_buffer(one.front())=*codec::decode_token_id("Az.0z");
  const auto equal = [](const T &a, const T &b) {
    if constexpr (std::is_same_v<T,OccurrenceBinding>) return a.occurrence==b.occurrence;
    else return a.field.kind==b.field.kind && a.field.object==b.field.object;
  };
  const auto element_bytes=sizeof(key_buffer(one.front())[0]);
  const auto initial_bytes=2*element_bytes+37;
  const auto uuid=[](const T &row) -> const warm::Uuid & {
    if constexpr (std::is_same_v<T,OccurrenceBinding>) return row.occurrence.study;
    else return row.field.object.scope;
  };
  Map reference(one); reference.reserve(3,4);
  const auto backing = [&](const Map &map, const auto &domain) {
    std::uint64_t bytes=map.storage().capacity()*sizeof(T);
    require(map.storage().get_allocator().domain == domain.get(),"map outer domain mismatch");
    for (const auto &row : map.storage()) {
      const auto &path=key_buffer(row);
      require(path.get_allocator().domain == domain,"map active/inactive path domain mismatch");
      const auto &text=uuid(row).str();
      require(text.get_allocator().domain==domain && text.size()==36 && text.data()[36]=='\0',
              "map active/inactive UUID domain or encoding mismatch");
      bytes += warm::recipe_text_backing(text);
      bytes += path.capacity()*element_bytes;
    }
    return bytes;
  };
  auto zero=std::make_shared<ReservationDomain>(ReservationBudget{0,UINT64_MAX});
  Map empty(zero);
  no_new([&] {
    bool stopped=false;
    try { empty.reserve(3,4); } catch (const ReservationStop &) { stopped=true; }
    require(stopped && !zero->result().live_bytes && empty.storage().empty(),
            "map outer rejection allocated nested paths");
  });
  const auto row_bytes=sizeof(T);
  auto growth=std::make_shared<ReservationDomain>(ReservationBudget{2*row_bytes+initial_bytes,UINT64_MAX});
  {
    Map value(one,growth);
    const auto *original=value.storage().data();
    no_new([&] {
      bool stopped=false;
      try { value.reserve(2,4); } catch (const ReservationStop &) { stopped=true; }
      require(stopped && growth->result().required_bytes>=3*row_bytes+initial_bytes &&
                  growth->result().live_bytes==row_bytes+initial_bytes && value.storage().data()==original &&
                  value.size()==1 && equal(value[0],one[0]),
              "map growth omitted old/new overlap or changed existing rows");
      stopped=false;
      try { Map denied(value,zero); } catch (const ReservationStop &) { stopped=true; }
      require(stopped && !zero->result().live_bytes,"map copy allocated before outer admission");
    });
  }
  require(!growth->result().live_bytes,"map growth denial leaked original backing");
  for (bool bytes : {true,false}) {
    auto d=std::make_shared<ReservationDomain>(ReservationBudget{
        bytes ? row_bytes : UINT64_MAX,bytes ? UINT64_MAX : row_bytes});
    A::begin();
    bool stopped=false;
    try { Map denied(one,d); } catch (const ReservationStop &) { stopped=true; }
    catch (...) { A::end(); throw; }
    const auto seen=A::end();
    require(stopped && seen.attempts==1 && !seen.live && !seen.blocks && !d->result().live_bytes &&
                (bytes ? d->result().required_bytes : d->result().required_work)==row_bytes+37,
            "map UUID allocated before admission or leaked its admitted outer table");
  }
  for (bool copying : {false,true}) {
    const auto construct = [&](const auto &d) {
      // Return a prvalue: an ordinary Map copy intentionally detaches.
      return copying ? Map(reference,d) : Map(one,d);
    };
    ReservationResult full;
    std::size_t sites=0;
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    A::begin();
    try {
      auto value=construct(domain); value.reserve(3,4);
      full=domain->result(); sites=A::counts.attempts;
      for (const auto &row : value.storage()) {
        const auto &text=uuid(row).str();
        const auto found=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==text.data(); });
        require(found!=std::end(A::table) && found->bytes==text.capacity()+1,
                "map UUID differs from independent allocation request");
      }
      require(A::counts.live==domain->result().live_bytes,"map retained unselected backing");
      require(full.live_bytes == backing(value,domain) && value.size()==1 &&
                  equal(value[0],one[0]) && value.storage().size()==3,
              "map construction/copy lost values or selected backing");
    } catch (...) { A::end(); throw; }
    auto released=A::end();
    require(!released.live && !released.blocks && !domain->result().live_bytes,"map construction leaked");
    for (bool bytes : {true,false}) {
      const auto total=bytes ? full.peak_bytes : full.charged_work;
      std::uint64_t allowance=0;
      while (allowance<total) {
        auto d=std::make_shared<ReservationDomain>(ReservationBudget{
            bytes ? allowance : UINT64_MAX,bytes ? UINT64_MAX : allowance});
        bool stopped=false;
        A::begin();
        try { auto value=construct(d); value.reserve(3,4); }
        catch (const ReservationStop &) { stopped=true; }
        catch (...) { A::end(); throw; }
        released=A::end();
        const auto result=d->result();
        const auto next=bytes ? result.required_bytes : result.required_work;
        require(stopped && result.status==(bytes ? ReservationStatus::ByteExhausted : ReservationStatus::WorkExhausted) &&
                    !result.live_bytes && !released.live && !released.blocks && next>allowance && next<=total,
                "map path refusal failed reclamation or progress");
        allowance=next;
      }
    }
    for (std::size_t at=1; at<=sites; ++at) {
      auto d=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
      bool failed=false;
      A::begin(at);
      try { auto value=construct(d); value.reserve(3,4); }
      catch (const std::bad_alloc &) { failed=true; }
      catch (...) { A::end(); throw; }
      released=A::end();
      require(failed && released.attempts==at && !released.live && !released.blocks && !d->result().live_bytes,
              "map path allocation failure leaked or missed injection");
    }
    auto exact=std::make_shared<ReservationDomain>(ReservationBudget{full.peak_bytes,full.charged_work});
    auto retry=construct(exact); retry.reserve(3,4);
    require(exact->result().live_bytes==full.live_bytes && exact->result().charged_work==full.charged_work &&
                equal(retry[0],one[0]),"map exact retry failed");
  }
  std::optional<Map> snapshot, retained;
  std::weak_ptr<ReservationDomain> watch;
  {
    auto first=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    auto second=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    watch=first;
    retained.emplace(reference,first);
    snapshot.emplace(*retained);
    const auto live=first->result().live_bytes;
    backing(*snapshot,std::shared_ptr<ReservationDomain>{});
    require(key_buffer(snapshot->storage()[2]).capacity()>=4 && first->result().live_bytes==live,
            "map snapshot lost inactive capacity or borrowed quota");
    Map other(reference,second);
    no_new([&] {
      *retained=other;
      retained->push_back(one.front());
      retained->erase(retained->begin());
      retained->swap(other);
      require(backing(*retained,second)==second->result().live_bytes &&
                  backing(other,first)==first->result().live_bytes,
              "map assignment/erase/swap lost domains or capacity");
      retained->swap(other);
    });
  }
  require(!watch.expired(),"map released domain before its outer/nested backing");
  retained.reset();
  require(watch.expired(),"map retained domain after destruction");
  snapshot->reserve(4,5);
  require(snapshot->size()==1 && equal(snapshot->operator[](0),one[0]),
          "map snapshot failed after domain destruction");
  backing(*snapshot,std::shared_ptr<ReservationDomain>{});
  {
    auto d=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    watch=d; retained.emplace(d); d.reset();
    require(!watch.expired(),"empty map lost domain before later reservation");
    retained->reserve(2,4);
  }
  retained.reset();
  require(watch.expired(),"empty map domain lifetime cycle");
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

void projection_reservation_controls(const assembly::LetterProjection &projection, bool reserve=true) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  auto prepared=projection;
  if (reserve) Access::reserve_projection(prepared);
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> watch = domain;
  std::optional<assembly::LetterProjection> snapshot;
  std::uint64_t outer = 0;
  A::begin();
  try {
    {
      auto copy = Access::projection_copy(projection,domain);
      if (reserve) Access::reserve_projection(copy);
      outer = Access::projection_backing_bytes(copy);
      Access::projection_texts(copy,[&](const auto &text) {
        const auto observed=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==text.data(); });
        const auto bytes=observed==std::end(A::table) ? 0 : observed->bytes;
        require(bytes==warm::recipe_text_backing(text) &&
                    (!bytes || bytes==text.capacity()+1) && text.data()[text.size()]=='\0',
                "recipe text request omitted terminator or charged inline storage");
      });
      for (std::size_t i=0; i<copy.recipe().tools.size(); ++i)
        require(warm::tool_body(copy.recipe().tools[i])==warm::tool_body(projection.recipe().tools[i]),
                "recipe text copy changed immutable encoding");
      Access::projection_backing(copy,[&](const auto &table) { if (table.second) {
        const auto observed=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==table.first; });
        require(observed!=std::end(A::table) && observed->bytes==table.second,
                "projection table backing differs from independent request");
      }});
      Access::projection_paths(copy,[&](const auto &path) {
        require(path.get_allocator().domain == domain,"projection path detached during copy/reservation");
        const auto observed=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==path.data(); });
        require(path.empty() || (observed!=std::end(A::table) &&
                    observed->bytes==path.capacity()*sizeof(std::uint32_t)),
                "projection path backing differs from independent request");
        outer += path.capacity()*sizeof(std::uint32_t);
      });
      Access::projection_addresses(copy,[&](const auto &address) {
        require(address.get_allocator().domain == domain,"projection address detached during copy/reservation");
        const auto observed=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==address.data(); });
        require(address.empty() || (observed!=std::end(A::table) &&
                    observed->bytes==address.capacity()*sizeof(codec::AddressElement)),
                "projection address backing differs from independent request");
        outer += address.capacity()*sizeof(codec::AddressElement);
      });
      // Every retained projection allocation is now selected, including input identities.
      require(outer && domain->result().live_bytes == outer && A::counts.live == outer,
              "projection domain differs from independently observed complete retained backing");
      Access::projection_domains(copy,[&](const auto *d) {
        require(d == domain.get(), "explicit projection copy detached a table allocator");
      });
      // Also copy a populated inactive pool; owner adoption normally copies
      // before reserving it, so that alone would not exercise this branch.
      {
        auto other_domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
        auto reserved_copy=Access::projection_copy(copy,other_domain);
        if (reserve) Access::reserve_projection(reserved_copy);
        std::uint64_t bytes=Access::projection_backing_bytes(reserved_copy);
        Access::projection_paths(reserved_copy,[&](const auto &path) {
          require(path.get_allocator().domain == other_domain,"copied inactive projection path detached");
          bytes += path.capacity()*sizeof(std::uint32_t);
        });
        Access::projection_addresses(reserved_copy,[&](const auto &address) {
          require(address.get_allocator().domain == other_domain,"copied inactive projection address detached");
          bytes += address.capacity()*sizeof(codec::AddressElement);
        });
        Access::projection_domains(reserved_copy,[&](const auto *d) {
          require(d == other_domain.get(),"populated projection copy retained foreign backing domain");
        });
        require(Access::projection_equivalent(reserved_copy,copy),"populated projection copy changed exact rows");
        require(other_domain->result().live_bytes == bytes &&
                    Access::projection_parent_sizes(reserved_copy) == Access::projection_parent_sizes(copy),
                "explicit reserved projection copy lost or duplicated a parent pool");
      }
      snapshot.emplace(copy);
      require(Access::projection_equivalent(*snapshot,copy),"projection snapshot changed exact rows");
      Access::projection_addresses(*snapshot,[&](const auto &address) {
        require(!address.get_allocator().domain,"projection snapshot retained address domain");
      });
      Access::projection_paths(*snapshot,[&](const auto &path) {
        require(!path.get_allocator().domain,"projection snapshot retained path domain");
      });
      Access::projection_domains(*snapshot,[&](const auto *d) {
        require(!d, "ordinary projection snapshot borrowed source quota");
      });
      require(domain->result().live_bytes == outer && snapshot->study() == copy.study() &&
                  snapshot->occurrences().size() == copy.occurrences().size() &&
                  Access::projection_parent_sizes(*snapshot) == Access::projection_parent_sizes(copy),
              "projection snapshot changed source accounting or lost inactive contributions");
    }
    require(!domain->result().live_bytes, "projection destruction did not release its table bytes");
    domain.reset();
    require(watch.expired(), "detached projection snapshot retained its source domain");
    // Restore full parent capacity on the independent snapshot after source death.
    if (reserve) Access::reserve_projection(*snapshot);
    require(Access::projection_equivalent(*snapshot,prepared),"snapshot references depended on destroyed projection");
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
    if (reserve) Access::reserve_projection(copy);
    moved.emplace(std::move(copy));
    domain.reset();
  }
  require(!watch.expired(), "moved projection lost its allocation domain");
  require(Access::projection_equivalent(*moved,prepared),"moved projection changed references or parent pools");
  Access::projection_domains(*moved,[&](const auto *d) {
        require(d != nullptr, "moved projection detached its backing allocator");
      });
  Access::projection_paths(*moved,[&](const auto &path) {
    require(bool(path.get_allocator().domain),"moved projection detached path backing");
  });
  Access::projection_addresses(*moved,[&](const auto &address) {
    require(address.get_allocator().domain == watch.lock(),"moved projection detached address backing");
  });
  moved.reset();
  require(watch.expired(), "projection table allocator retained a lifetime cycle");

  // Sweep the complete explicit copy plus inactive-parent reservation, including
  // nested path/address requests, also copying an already populated spare pool.
  for (const auto *source : {&projection,static_cast<const assembly::LetterProjection *>(&prepared)}) {
    const auto construct = [&](const auto &d) {
      auto value=Access::projection_copy(*source,d);
      if (reserve) Access::reserve_projection(value);
      return value;
    };
    domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    ReservationResult full;
    { auto value=construct(domain); full=domain->result(); }
    for (bool bytes : {true,false}) {
      const auto total=bytes ? full.peak_bytes : full.charged_work;
      std::uint64_t allowance=0;
      while (allowance < total) {
        auto d=std::make_shared<ReservationDomain>(ReservationBudget{
            bytes ? allowance : UINT64_MAX, bytes ? UINT64_MAX : allowance});
        bool stopped=false;
        A::begin();
        try { auto value=construct(d); }
        catch (const ReservationStop &) { stopped=true; }
        catch (...) { A::end(); throw; }
        const auto released=A::end();
        const auto result=d->result();
        const auto next=bytes ? result.required_bytes : result.required_work;
        require(stopped && result.status == (bytes ? ReservationStatus::ByteExhausted : ReservationStatus::WorkExhausted) &&
                    !result.live_bytes && !released.live && !released.blocks && next > allowance && next <= total,
                "projection path rejection failed cleanup or progress");
        allowance=next;
      }
    }
    domain=std::make_shared<ReservationDomain>(ReservationBudget{full.peak_bytes,full.charged_work});
    auto retry=construct(domain);
    require(domain->result().live_bytes == full.live_bytes && domain->result().charged_work == full.charged_work &&
                retry.study() == projection.study() && retry.occurrences().size() == projection.occurrences().size(),
            "exact projection path construction retry failed");
    require(Access::projection_equivalent(retry,prepared),"projection exact retry changed references or pool order");
  }
}

void recipe_payload_controls() {
  membership_fixture::Fixture letters;
  // Unreferenced valid tools exercise absent, empty, inline and heap text without
  // changing the letter notation contract or the requested occurrence graph.
  for (int i=0; i<4; ++i) {
    auto tool=letters.base.tools.front();
    tool.ref.object=letter_fixture::object("02.00.20.01.0"+std::to_string(i));
    tool.parents.clear(); tool.member_of.clear(); tool.mass=1;
    tool.recipe=i==3 ? warm::RecipeText(97,'r') : warm::RecipeText("short");
    if (i==0) tool.notation.reset();
    else if (i==1) tool.notation=warm::RecipeText{};
    else if (i==2) tool.notation="q\"\\";
    else tool.notation=warm::RecipeText(131,'n');
    letters.base.tools.push_back(std::move(tool));
  }
  auto recipe=assembly::normalize_letters(letters.base,
      {letters.request(letters.accented,0,warm::Exposure::Aggregate),
       letters.request(letters.english,1,warm::Exposure::Parts),
       letters.request(letters.french,2,warm::Exposure::Parts)});
  recipe.contacts.push_back({recipe.occurrences.front().id,recipe.occurrences.back().id});
  recipe.contacts.push_back({recipe.selections[1].parts.front(),recipe.selections[2].parts.front()});
  require(!recipe.contacts.back().a.path.empty() && !recipe.contacts.back().b.path.empty(),
          "legacy contact fixture lacks byte paths");
  recipe.geometry.front().centroid={0.25,0.5,0.75};
  recipe.geometry.front().spheres.front()={{0.5,0.25,0.125},0.625};
  warm::validate(recipe);
  auto projection=assembly::project_letters(recipe);
  require(!recipe.occurrences.empty() && !recipe.contacts.empty() && !recipe.geometry.empty(),
          "legacy recipe address fixture lost populated fields");
  assembly::testing::ExchangeOwnerAccess::populate_input_tables(projection);
  projection_reservation_controls(projection,false);
  letters.base.focal_fields={{warm::FieldKind::Membership,letters.en}};
  auto structural=assembly::normalize_structural_letters(letters.base,
      {{0,letters.ref(letters.accented),warm::Exposure::Parts},
       {1,letters.ref(letters.english),warm::Exposure::Aggregate}});
  const auto &parts=structural.selections.front().parts;
  require(parts.size()>=2 && !parts[0].path.empty() && !parts[1].path.empty(),
          "structural contact fixture lacks byte paths");
  structural.contacts.push_back({parts[0],parts[1]});
  structural.contacts.push_back({{1,{}},parts[0]});
  warm::validate(structural);
  auto structural_projection=assembly::project_letters(structural);
  assembly::testing::ExchangeOwnerAccess::populate_input_tables(structural_projection);
  projection_reservation_controls(structural_projection);
}

void catalogue_reservation_controls(const assembly::LetterProjection &projection) {
  using Access = assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A = allocation_lifetime;
  auto observed_domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  A::begin();
  try {
    auto value=Access::catalogue(projection,observed_domain);
    Access::catalogue_uuids(value,[&](const auto &uuid) {
      const auto &text=uuid.str();
      const auto found=std::find_if(std::begin(A::table),std::end(A::table),
          [&](const auto &entry) { return entry.pointer==text.data(); });
      require(text.get_allocator().domain==observed_domain && found!=std::end(A::table) &&
                  found->bytes==text.capacity()+1 && text.data()[text.size()]=='\0',
              "catalogue UUID request, domain or terminator differs from observation");
    });
    require(A::counts.live==observed_domain->result().live_bytes,
            "catalogue retained unselected storage");
  } catch (...) { A::end(); throw; }
  const auto observed=A::end();
  require(!observed.live && !observed.blocks && !observed_domain->result().live_bytes,
          "catalogue UUID observation leaked storage");
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
      std::uint64_t path_bytes=0;
      for (const auto &row : rows) {
        const auto &path=row.occurrence.source.path;
        require(path.get_allocator().domain == domain,"catalogue contribution path detached");
        path_bytes += path.capacity()*sizeof(std::uint32_t);
      }
      require(rows.get_allocator().domain == domain && path_bytes &&
                  full.live_bytes > rows.capacity()*sizeof(rows[0])+path_bytes &&
                  full.charged_work > full.live_bytes,
              "catalogue omitted retained table or failed to release graph nodes");
      std::uint64_t address_bytes=0;
      Access::catalogue_addresses(catalogue,[&](const auto &address) {
        require(address.get_allocator().domain == domain,"catalogue address detached during collection");
        address_bytes += address.capacity()*sizeof(codec::AddressElement);
      });
      std::uint64_t uuid_bytes=0;
      Access::catalogue_uuids(catalogue,[&](const auto &uuid) {
        require(uuid.str().get_allocator().domain==domain,"catalogue UUID detached");
        uuid_bytes += warm::recipe_text_backing(uuid.str());
      });
      require(full.live_bytes == rows.capacity()*sizeof(rows[0])+path_bytes+address_bytes+uuid_bytes,
              "catalogue omitted address backing or charged temporary payload");
      snapshot.emplace(catalogue);
      Access::catalogue_uuids(*snapshot,[&](const auto &uuid) {
        require(!uuid.str().get_allocator().domain,"catalogue snapshot retained UUID domain");
      });
      Access::catalogue_addresses(*snapshot,[&](const auto &address) {
        require(!address.get_allocator().domain,"catalogue snapshot retained address domain");
      });
      for (const auto &row : snapshot->contributions())
        require(!row.occurrence.source.path.get_allocator().domain,"catalogue snapshot retained path domain");
      require(!snapshot->contributions().get_allocator().domain &&
                  domain->result().live_bytes == full.live_bytes,
              "catalogue value snapshot retained or charged its source domain");
    }
    require(!domain->result().live_bytes, "catalogue graph/table storage survived teardown");
  }
  require(watch.expired(), "detached catalogue snapshot extended domain lifetime");
  auto standalone = Access::catalogue(projection,{});
  require(Access::catalogue_equivalent(*snapshot,standalone),
          "catalogue snapshot lost exact rows after domain death");
  std::optional<assembly::MembershipFields> moved;
  {
    auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    watch=domain;
    auto value=Access::catalogue(projection,domain);
    moved.emplace(std::move(value));
    require(domain->result().live_bytes == full.live_bytes,"catalogue move changed accounting");
    for (const auto &row : moved->contributions())
      require(row.occurrence.source.path.get_allocator().domain == domain,"catalogue move detached path");
  }
  require(!watch.expired(),"moved catalogue lost allocation domain");
  require(Access::catalogue_equivalent(*moved,standalone),"catalogue move changed exact rows");
  Access::catalogue_uuids(*moved,[&](const auto &uuid) {
    require(uuid.str().get_allocator().domain==watch.lock(),"catalogue move detached UUID");
  });
  Access::catalogue_addresses(*moved,[&](const auto &address) {
    require(address.get_allocator().domain == watch.lock(),"catalogue move detached address");
  });
  moved.reset();
  require(watch.expired(),"catalogue path allocator retained a lifetime cycle");
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
      if (!allowance) require(!released.attempts,"catalogue allocated before first UUID admission");
      allowance=next;
    }
  }
  auto domain = std::make_shared<ReservationDomain>(ReservationBudget{full.peak_bytes,full.charged_work});
  auto retry=Access::catalogue(projection,domain);
  require(domain->result().status == ReservationStatus::Ok &&
              domain->result().live_bytes == full.live_bytes &&
              domain->result().charged_work == full.charged_work && Access::catalogue_equivalent(retry,standalone),
          "exact catalogue construction retry failed");
}

// This fixture has only five-element addresses (15 requested bytes), empty or
// two-ordinal paths, UUID text, table/node blocks and a larger callable. The
// Addresses uniquely request 15 bytes. UUID payload and comparison formatting
// both request 37; classify prepaid requests by work delta, track their pointers,
// then check their count against the independent graph structure below.
namespace catalogue_address_witness {
inline thread_local assembly::detail::ReservationDomain *domain{};
inline thread_local std::uint64_t previous_work{}, live{}, peak{}, acquired{}, released{};
inline thread_local std::uint64_t uuid_live{}, uuid_peak{}, uuid_acquired{}, uuid_released{};
inline thread_local void *uuid_blocks[1024]{};
inline thread_local std::uint64_t unselected_uuid_sized{};
inline thread_local bool prepaid=true;
void acquire(void *pointer, std::size_t bytes) noexcept {
  const auto work=domain->result().charged_work;
  if (bytes == 5*sizeof(codec::AddressElement)) {
    prepaid &= work-previous_work == bytes;
    live += bytes; acquired += bytes; peak=std::max(peak,live);
  }
  if (bytes == 37) {
    if (work-previous_work == bytes) {
      auto slot=std::find(std::begin(uuid_blocks),std::end(uuid_blocks),nullptr);
      if (slot==std::end(uuid_blocks)) std::abort();
      *slot=pointer;
      uuid_live += bytes; uuid_acquired += bytes; uuid_peak=std::max(uuid_peak,uuid_live);
    } else ++unselected_uuid_sized;
  }
  previous_work=work;
}
void release(void *pointer, std::size_t bytes) noexcept {
  if (bytes == 5*sizeof(codec::AddressElement)) { live-=bytes; released+=bytes; }
  if (bytes == 37) {
    auto slot=std::find(std::begin(uuid_blocks),std::end(uuid_blocks),pointer);
    if (slot!=std::end(uuid_blocks)) {
      *slot=nullptr; uuid_live-=bytes; uuid_released+=bytes;
    }
  }
}
}

void catalogue_graph_address_controls() {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  namespace W=catalogue_address_witness;
  letter_fixture::Fixture letters;
  const auto left=letters.add(letter_fixture::object("02.00.00.01.00"),"left",0);
  const auto right=letters.add(letter_fixture::object("02.00.00.02.00"),"right",0);
  const auto top=letters.add(letter_fixture::object("02.00.00.03.00"),"top",0);
  letters.base.tools.at(letters.tools.at(letters.english)).member_of={left,right,top};
  letters.base.tools.at(letters.tools.at(left.object)).member_of={top};
  letters.base.tools.at(letters.tools.at(right.object)).member_of={top};
  letters.base.focal_fields={{warm::FieldKind::Membership,left.object}};
  auto projection=assembly::project_letters(assembly::normalize_structural_letters(letters.base,
      {{0,letters.ref(letters.english),warm::Exposure::Aggregate}}));
  for (const auto &t : projection.recipe().tools)
    require(t.ref.object.address.size()==5,"graph witness requires five-element addresses");
  const auto reference=Access::catalogue(projection,{});
  const auto address_size=5*sizeof(codec::AddressElement);
  // Four visited graph vertices have state/reached keys. Their ancestor sets
  // contain 3+1+1+0 unique targets despite the diamond and direct top edge.
  const auto temporary=(projection.recipe().tools.size()+2*4+5)*address_size;
  const auto retained=reference.contributions().size()*2*address_size;
  const auto temporary_uuid=(projection.recipe().tools.size()+2*4+5)*37;
  const auto retained_uuid=(3+reference.contributions().size()*4)*37;
  require(reference.contributions().size()==projection.occurrences().size()*3,
          "diamond catalogue failed per-occurrence deduplication");
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  const auto observe=[&] {
    W::domain=domain.get(); W::previous_work=W::live=W::peak=W::acquired=W::released=0;
    W::uuid_live=W::uuid_peak=W::uuid_acquired=W::uuid_released=W::unselected_uuid_sized=0; W::prepaid=true;
    std::fill(std::begin(W::uuid_blocks),std::end(W::uuid_blocks),nullptr);
    A::on_acquired=W::acquire; A::on_released=W::release;
    A::begin();
  };
  const auto finish=[] {
    A::on_acquired=nullptr; A::on_released=nullptr; return A::end();
  };
  observe();
  try {
    {
      auto value=Access::catalogue(projection,domain);
      require(Access::catalogue_equivalent(value,reference) && W::prepaid &&
                  W::acquired==temporary+retained && W::peak==temporary+retained &&
                  W::live==retained && W::released==temporary &&
                  W::uuid_acquired==temporary_uuid+retained_uuid &&
                  W::uuid_peak==temporary_uuid+retained_uuid &&
                  W::uuid_live==retained_uuid && W::uuid_released==temporary_uuid,
              "graph address requests, prepayment or transient overlap differ from independent fixture");
    }
  } catch (...) { A::on_acquired=nullptr; A::on_released=nullptr; A::end(); throw; }
  A::on_acquired=nullptr; A::on_released=nullptr;
  const auto released=A::end();
  const auto address_peak=W::peak, uuid_peak=W::uuid_peak, comparison_requests=W::unselected_uuid_sized;
  require(!released.live && !released.blocks && !W::live && W::acquired==W::released &&
              !W::uuid_live && W::uuid_acquired==W::uuid_released &&
              !domain->result().live_bytes,"graph address witness leaked temporary or retained backing");
  // Reuse the complete refusal/retry and lifetime controls with diamond ancestry.
  catalogue_reservation_controls(projection);
  for (std::size_t at=1; at<=released.attempts; ++at) {
    domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    A::begin(at);
    bool failed=false;
    try { auto value=Access::catalogue(projection,domain); }
    catch (const std::bad_alloc &) { failed=true; }
    catch (...) { A::end(); throw; }
    const auto observed=A::end();
    require(failed && observed.attempts==at && !observed.live && !observed.blocks &&
                !domain->result().live_bytes,"diamond allocation failure leaked graph backing");
  }
  domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  { auto value=Access::catalogue(projection,domain);
    require(Access::catalogue_equivalent(value,reference),"diamond allocation failures prevented retry"); }
  // Duplicate tool insertion keeps the first exact value and reclaims the rejected key.
  auto &recipe=Access::catalogue_recipe(projection);
  recipe.tools.push_back(recipe.tools.front());
  domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  // Same-sized detached control must not be counted as admitted UUID backing.
  observe();
  try {
    warm::Uuid attached(projection.study(),domain), detached(projection.study());
    require(W::uuid_acquired==37 && W::uuid_live==37 && W::unselected_uuid_sized==1,
            "UUID witness did not distinguish same-sized detached allocation");
  } catch (...) { finish(); throw; }
  const auto calibration=finish();
  require(!calibration.live && !calibration.blocks && !W::uuid_live && W::uuid_released==37,
          "UUID witness calibration leaked storage");
  domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  observe();
  try {
    auto value=Access::catalogue(projection,domain);
    require(Access::catalogue_equivalent(value,reference) && W::prepaid &&
                W::uuid_acquired==temporary_uuid+retained_uuid+37 &&
                W::uuid_released==temporary_uuid+37 && W::uuid_live==retained_uuid &&
                W::acquired==temporary+retained+address_size &&
                W::released==temporary+address_size && W::live==retained,
            "duplicate graph key changed values or failed to prepay/release UUID and address");
  } catch (...) { finish(); throw; }
  const auto duplicate=finish();
  require(!domain->result().live_bytes && !duplicate.live && !duplicate.blocks &&
              !W::uuid_live && W::uuid_acquired==W::uuid_released,
          "duplicate graph key retained allocation");
  const auto duplicate_budget=domain->result();
  // Introduce a cycle only after normal recipe validation; collection must reject
  // it and reclaim every preceding node/key allocation.
  auto top_it=std::find_if(recipe.tools.begin(),recipe.tools.end(),[&](const auto &t){return t.ref==top;});
  require(top_it!=recipe.tools.end(),"graph cycle fixture lost ancestor");
  top_it->member_of={left};
  domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  observe();
  bool rejected=false;
  try { auto value=Access::catalogue(projection,domain); }
  catch (const warm::Error &) { rejected=true; }
  catch (...) { finish(); throw; }
  const auto failed=finish();
  require(W::prepaid && !W::uuid_live && W::uuid_acquired==W::uuid_released,
          "cyclic graph UUID requests were not prepaid or reclaimed");
  require(rejected && !domain->result().live_bytes && !failed.live && !failed.blocks,
          "cyclic catalogue failed rejection or complete reclamation");
  top_it->member_of.clear();
  domain=std::make_shared<ReservationDomain>(ReservationBudget{duplicate_budget.peak_bytes,duplicate_budget.charged_work});
  { auto value=Access::catalogue(projection,domain);
    require(Access::catalogue_equivalent(value,reference) &&
                domain->result().charged_work==duplicate_budget.charged_work,
            "cycle repair failed exact-budget retry"); }
  require(!domain->result().live_bytes,"cycle repair retry retained backing");
  std::cout << "catalogue_graph_addresses,temporary_bytes=" << temporary
            << ",retained_bytes=" << retained << ",address_peak=" << address_peak
            << ",temporary_uuid_bytes=" << temporary_uuid << ",retained_uuid_bytes=" << retained_uuid
            << ",uuid_peak=" << uuid_peak << ",unselected_37_byte_requests=" << comparison_requests
            << ",failure_sites=" << released.attempts << " PASS\n";
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
    // Observe both collection and explicit copies without allocating in the visitor.
    auto observed_domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
    A::begin();
    try {
      auto value=construct(observed_domain);
      Access::contract_uuids(value,[&](const auto &uuid) {
        const auto &text=uuid.str();
        const auto found=std::find_if(std::begin(A::table),std::end(A::table),
            [&](const auto &entry) { return entry.pointer==text.data(); });
        require(text.get_allocator().domain==observed_domain && found!=std::end(A::table) &&
                    found->bytes==text.capacity()+1 && text.data()[text.size()]=='\0',
                "declaration UUID request or terminator differs from independent observation");
      });
      require(A::counts.live==observed_domain->result().live_bytes,
              "declaration retained unselected allocation");
    } catch (...) { A::end(); throw; }
    const auto observed=A::end();
    require(!observed.live && !observed.blocks && !observed_domain->result().live_bytes,
            "observed declaration leaked backing");
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
      Access::contract_addresses(contract,[&](const auto &address) {
        require(address.get_allocator().domain==domain,"declaration reference address detached");
        backing += address.capacity()*sizeof(codec::AddressElement);
      });
      Access::contract_uuids(contract,[&](const auto &uuid) {
        require(uuid.str().get_allocator().domain==domain,"declaration UUID detached");
        backing += warm::recipe_text_backing(uuid.str());
      });
      require(full.live_bytes == backing && backing && Access::equivalent(contract,reference),
              "declaration omitted table backing or changed its structural values");
      snapshot.emplace(contract);
      Access::contract_uuids(*snapshot,[&](const auto &uuid) {
        require(!uuid.str().get_allocator().domain,"declaration snapshot retained UUID domain");
      });
      Access::contract_addresses(*snapshot,[&](const auto &address) {
        require(!address.get_allocator().domain,"declaration snapshot retained address domain");
      });
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
    Access::contract_uuids(*moved,[&](const auto &uuid) {
      require(uuid.str().get_allocator().domain==watch.lock(),"moved declaration lost UUID domain");
    });
    Access::contract_addresses(*moved,[&](const auto &address) {
      require(address.get_allocator().domain==watch.lock(),"moved declaration lost address domain");
    });
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
        if (!allowance) require(!released.attempts,"declaration allocated before first UUID admission");
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
  map_reservation_controls(fields);
  shared_reservation_controls();
  unique_reservation_controls();
  projection_reservation_controls(projection);
  recipe_payload_controls();
  catalogue_reservation_controls(projection);
  catalogue_graph_address_controls();
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
                    Access::projection_backing_bytes(Access::projection(*owner))+
                    Access::projection_backing_bytes(Access::spare_projection(*owner)),
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
  std::array<bool,static_cast<std::size_t>(ProjectionSetupPhase::Complete)> phases{};
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
  // Constructors borrow caller tables only until their domain-owned maps exist.
  const auto same_handle=[](const auto &a,const auto &b) {
    return a.context==b.context && a.address==b.address && a.record==b.record &&
        a.incarnation==b.incarnation;
  };
  const auto same_rows=[&](const auto &a,const auto &b) {
    return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin(),[&](const auto &x,const auto &y) {
      return x.occurrence==y.occurrence && same_handle(x.handle,y.handle);
    });
  };
  const auto same_fields=[&](const auto &a,const auto &b) {
    return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin(),[&](const auto &x,const auto &y) {
      return x.field.kind==y.field.kind && x.field.object==y.field.object && same_handle(x.handle,y.handle);
    });
  };
  {
    auto independent=[&] {
      auto caller_rows=rows;
      auto caller_fields=fields;
      const auto *row_backing=caller_rows.data();
      const auto *field_backing=caller_fields.data();
      auto owner=Access::adopt(f.h,projection,caller_rows,caller_fields);
      require(caller_rows.data()==row_backing && caller_fields.data()==field_backing &&
                  same_rows(caller_rows,rows) && same_fields(caller_fields,fields),
              "adoption consumed or changed caller arguments");
      return owner;
    }();
    require(same_rows(Access::mappings(*independent),rows) &&
                same_fields(Access::fields(*independent),fields),
            "owner mappings depended on destroyed caller arguments");
  }
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
    std::uint64_t map_uuid_bytes=0;
    Access::map_uuids(*owner,[&](const auto &uuid) {
      const auto &text=uuid.str();
      const auto bytes=warm::recipe_text_backing(text);
      require(text.get_allocator().domain==Access::reservation_domain(*owner) &&
                  containing_request(text.data())==bytes && bytes==text.capacity()+1,
              "owner map UUID differs from independent allocation request");
      map_uuid_bytes += bytes;
    });
    std::uint64_t map_path_bytes=0;
    Access::map_paths(*owner,[&](const auto &path) {
      const auto bytes=path.capacity()*sizeof(std::uint32_t);
      require(path.get_allocator().domain == Access::reservation_domain(*owner) &&
                  (!bytes || containing_request(path.data())==bytes),
              "map active/inactive path differs from observed allocation request");
      map_path_bytes += bytes;
    });
    std::uint64_t map_address_bytes=0;
    Access::map_addresses(*owner,[&](const auto &address) {
      const auto bytes=address.capacity()*sizeof(codec::AddressElement);
      require(address.get_allocator().domain == Access::reservation_domain(*owner) &&
                  (!bytes || containing_request(address.data())==bytes),
              "map active/inactive address differs from observed allocation request");
      map_address_bytes += bytes;
    });
    std::uint64_t projection_bytes = 0;
    for (const auto *p : {&Access::projection(*owner),&Access::spare_projection(*owner)})
      Access::projection_backing(*p,[&](const auto &entry) { if (entry.second) {
        const auto request = containing_request(entry.first);
        require(request == entry.second, "projection table differs from observed allocation request");
        projection_bytes += request;
      }});
    for (const auto *p : {&Access::projection(*owner),&Access::spare_projection(*owner)})
      Access::projection_paths(*p,[&](const auto &path) {
        const auto bytes=path.capacity()*sizeof(std::uint32_t);
        require(!bytes || containing_request(path.data()) == bytes,
                "projection path differs from observed allocation request");
        projection_bytes += bytes;
      });
    for (const auto *p : {&Access::projection(*owner),&Access::spare_projection(*owner)})
      Access::projection_addresses(*p,[&](const auto &address) {
        const auto bytes=address.capacity()*sizeof(codec::AddressElement);
        require(address.get_allocator().domain == Access::reservation_domain(*owner) &&
                    (!bytes || containing_request(address.data()) == bytes),
                "projection address differs from observed allocation request");
        projection_bytes += bytes;
      });
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
    for (const auto &declaration : Access::declarations(*owner))
      Access::contract_addresses(*declaration,[&](const auto &address) {
        const auto bytes=address.capacity()*sizeof(codec::AddressElement);
        require(address.get_allocator().domain==Access::reservation_domain(*owner) &&
                    (!bytes || containing_request(address.data())==bytes),
                "declaration address differs from observed allocation request");
        declaration_bytes += bytes;
      });
    for (const auto &declaration : Access::declarations(*owner))
      Access::contract_uuids(*declaration,[&](const auto &uuid) {
        const auto &text=uuid.str();
        const auto bytes=warm::recipe_text_backing(text);
        require(text.get_allocator().domain==Access::reservation_domain(*owner) &&
                    containing_request(text.data())==bytes && bytes==text.capacity()+1,
                "declaration UUID differs from observed allocation request");
        declaration_bytes += bytes;
      });
    auto membership_bytes = membership_rows.capacity()*sizeof(membership_rows[0]);
    require(!membership_bytes || containing_request(membership_rows.data()) == membership_bytes,
            "catalogue table differs from observed allocation request");
    for (const auto &row : membership_rows) {
      const auto &path=row.occurrence.source.path;
      const auto bytes=path.capacity()*sizeof(std::uint32_t);
      require(!bytes || containing_request(path.data()) == bytes,
              "catalogue path differs from observed allocation request");
      membership_bytes += bytes;
    }
    Access::catalogue_uuids(*Access::memberships(*owner),[&](const auto &uuid) {
      const auto &text=uuid.str();
      const auto bytes=warm::recipe_text_backing(text);
      require(text.get_allocator().domain==Access::reservation_domain(*owner) &&
                  containing_request(text.data())==bytes && bytes==text.capacity()+1,
              "catalogue UUID differs from observed allocation request");
      membership_bytes += bytes;
    });
    Access::catalogue_addresses(*Access::memberships(*owner),[&](const auto &address) {
      const auto bytes=address.capacity()*sizeof(codec::AddressElement);
      require(address.get_allocator().domain == Access::reservation_domain(*owner) &&
                  (!bytes || containing_request(address.data()) == bytes),
              "catalogue address differs from observed allocation request");
      membership_bytes += bytes;
    });
    require(Access::reservations(*owner).live_bytes == census.binding.vector_bytes+
                census.coordinator.vector_bytes+Access::map_outer_bytes(*owner)+table_bytes+shared_bytes+unique_bytes+projection_bytes+membership_bytes+declaration_bytes+map_path_bytes+map_address_bytes+map_uuid_bytes,
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
  require(retained.attempts && retained.live && retained.peak >= retained.live && retained.released &&
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
