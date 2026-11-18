// Initial C3 transport: actual source/destination padding, private guards,
// former-host-map reference, refusal/retry, failure and following tick history.
#ifdef HCP_TRANSFER_AUDIT
using TransferJournal = transfer_audit::Journal;
void hierarchy_transfer_audit_control(engine::Runtime &r) {
  using namespace taichi::lang;
  Ndarray array(&r.program(), PrimitiveType::f64, {2});
  const std::array<double, 2> input{{-1.25, 9.5}};
  std::array<double, 2> output{};
  auto journal = std::make_unique<TransferJournal>();
  engine::upload(array, input.data(), sizeof(input));
  require(journal->count == 0, "inactive transfer journal recorded a call");
  {
    transfer_audit::Scope scope(*journal);
    no_new([&] {
      engine::upload(array, input.data(), sizeof(input));
      engine::readback(array, output.data(), sizeof(output));
    });
  }
  engine::readback(array, output.data(), sizeof(output));
  require(journal->count == 2 && journal->used == 2 * sizeof(input) &&
              journal->events[0].upload && !journal->events[1].upload &&
              input == output &&
              !std::memcmp(journal->payload.data(), input.data(), sizeof(input)) &&
              !std::memcmp(journal->payload.data() + sizeof(input), input.data(), sizeof(input)),
          "transfer wrapper known-payload or inactive control failed");
}
std::unique_ptr<Fixture> seed_fixture(engine::Runtime &r, HostPath path, int H, int bits) {
  auto capacity = limits();
  capacity[Budget::CascadeNodes] = H;
  capacity[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      capacity, false, NumericalProfile::trial(52, bits));
  return std::make_unique<Fixture>(r, path,
      std::vector<std::vector<int>>{{}, {}, {0, 1}},
      std::vector<std::vector<std::uint64_t>>{{2, 0}, {0, 3}, {2, 3}},
      std::vector<std::uint64_t>{2, 3}, std::vector<double>{1., 2.},
      std::vector<double>{-1., 1.}, 52, bits, false, false, false, false,
      1024, std::vector<double>{}, &capacity);
}
LiveHierarchyResult seed_install(Fixture &f, TransferJournal &journal,
                                std::uint64_t work = UINT64_MAX) {
  LiveHierarchyResult result;
  transfer_audit::Scope scope(journal);
  no_new([&] { result = f.h.install_live_hierarchy(
      f.h.prepared_storage().generation, f.spec(), work); });
  return result;
}
template <class V> bool seed_same_bits(const V &a, const V &b) {
  return a.size() == b.size() && !std::memcmp(a.data(), b.data(), a.size() * sizeof(a[0]));
}
void seed_same_execution(Fixture &a, Fixture &b) {
  a.h.inspect_prepared(); b.h.inspect_prepared();
  const auto &x = a.h.inspect_live_hierarchy(), &y = b.h.inspect_live_hierarchy();
  require(seed_same_bits(a.h.particles64(), b.h.particles64()) &&
              seed_same_bits(a.h.groups64(), b.h.groups64()) &&
              seed_same_bits(a.h.prepared_storage().motion64(), b.h.prepared_storage().motion64()) &&
              seed_same_bits(x.slots, y.slots) && seed_same_bits(x.previous, y.previous) &&
              x.selectors == y.selectors && x.node_count == y.node_count,
          "native seeds/tick history differ from former-host-map reference");
}
void seed_guard_controls(Fixture &f, const CascadeResult &current, const CascadeResult &old) {
  const auto before = state_hash(f);
  auto *destination = LiveHierarchyAccess::input_array(f.h, 0);
  const auto reject = [&](const CascadeResult &q) {
    for (int operation = 0; operation < 3; ++operation) {
      bool refused = false;
      try {
        if (operation == 0) LiveHierarchyAccess::seed_descriptors(f.h, q);
        else if (operation == 1) LiveHierarchyAccess::copy_seed(f.h, q, *destination);
        else f.h.inspect_centroid_cascade(q);
      } catch (const std::logic_error &) { refused = true; }
      require(refused && f.h.ready() && state_hash(f) == before,
              "stale private/public centroid observation accepted or mutated execution");
    }
  };
  reject(old);
  for (int field = 0; field < 6; ++field) {
    auto q = current;
    if (field == 0) ++q.query;
    if (field == 1) ++q.binding;
    if (field == 2) ++q.context;
    if (field == 3) ++q.generation;
    if (field == 4) ++q.completed_ticks;
    if (field == 5) q.status = CascadeStatus::InvalidHierarchy;
    reject(q);
  }
}
void hierarchy_seed_transport_case(engine::Runtime &r, HostPath path, int H, int bits) {
  auto reference = seed_fixture(r, path, H, bits);
  auto legacy = std::make_unique<TransferJournal>();
  legacy->legacy = true;
  const auto reference_result = seed_install(*reference, *legacy);
  require(reference_result.status == CascadeStatus::Ok && legacy->copied,
          "former-host-map reference installation failed");

  auto owner = seed_fixture(r, path, H, bits);
  auto &f = *owner;
  const auto generation = f.h.prepared_storage().generation;
  const auto binding = f.h.bind_weighted_centroid_cascade(generation, f.spec());
  const auto old = f.h.query_centroid_cascade(generation);
  require(binding.status == CascadeStatus::Ok && old.status == CascadeStatus::Ok,
          "source padding setup query failed");
  auto inspection = std::make_unique<TransferJournal>();
  {
    transfer_audit::Scope scope(*inspection);
    f.h.inspect_centroid_cascade(old);
  }
  require(inspection->count == 2, "public inspection lost its two downloads");
  auto *source = inspection->events[0].array;
  auto *valid = inspection->events[1].array;
  std::vector<double> source_values(7 * H);
  std::vector<int> validity(H, 77);
  std::fill_n(validity.begin(), 3, 1);
  std::memcpy(source_values.data(), inspection->payload.data(), 7 * H * sizeof(double));
  for (int col = 0; col < 7; ++col)
    for (int row = 3; row < H; ++row) source_values[col * H + row] = 1234. + col + row;
  engine::upload(*source, source_values.data(), source_values.size() * sizeof(double));
  engine::upload(*valid, validity.data(), validity.size() * sizeof(int));
  auto *destination = LiveHierarchyAccess::input_array(f.h, 0);
  std::vector<double> dirty(8 * H, -9876.5), observed(8 * H);
  engine::upload(*destination, dirty.data(), dirty.size() * sizeof(double));
  engine::readback(*destination, observed.data(), observed.size() * sizeof(double));
  require(seed_same_bits(dirty, observed), "dirty destination setup did not reach native storage");

  const auto before = state_hash(f);
  auto short_journal = std::make_unique<TransferJournal>();
  const auto short_result = seed_install(f, *short_journal, reference_result.preparation_visits - 1);
  require(short_result.status == CascadeStatus::CapacityExhausted && f.h.ready() &&
              !f.h.live_hierarchy_installed() && state_hash(f) == before && !short_journal->copied,
          "short installation work mutated committed state or lost healthy retry");
  for (std::size_t i = 0; i < short_journal->count; ++i)
    require(!short_journal->events[i].initialization && !short_journal->events[i].copy,
            "work refusal reached destination initialization/copy");
  engine::readback(*destination, observed.data(), observed.size() * sizeof(double));
  require(seed_same_bits(dirty, observed), "short-work refusal changed dirty destination");

  auto journal = std::make_unique<TransferJournal>();
  const auto result = seed_install(f, *journal, reference_result.preparation_visits);
  require(result.status == CascadeStatus::Ok && journal->copied && result.nodes == 3 &&
              result.preparation_visits == reference_result.preparation_visits,
          "exact-work native seed retry failed");
  std::uint64_t upload = 0, readback = 0, initialized = 0, copied = 0;
  int fills = 0, copies = 0;
  for (std::size_t i = 0; i < journal->count; ++i) {
    const auto &e = journal->events[i];
    if (e.initialization) { initialized += e.bytes; ++fills; require(e.array == destination, "wrong fill owner"); }
    else if (e.copy) { copied += e.bytes; ++copies; require(e.array == destination, "wrong copy owner"); }
    else {
      (e.upload ? upload : readback) += e.bytes;
      require(e.array != destination, "installer retained host seed upload/readback");
    }
  }
  require(upload == result.upload_bytes && readback == result.readback_bytes && readback == 248 &&
              fills == 1 && initialized == std::uint64_t(64 * H) && copies == 1 && copied == 192 &&
              upload == std::uint64_t(H == 8 ? 66496 : 72448),
          "native initialization/copy or actual/reported transfer totals disagree");
  inspection = std::make_unique<TransferJournal>();
  {
    transfer_audit::Scope scope(*inspection);
    f.h.inspect_centroid_cascade(journal->query);
  }
  require(inspection->count == 2 && inspection->used == std::size_t(60 * H),
          "explicit public inspection lost full value/validity bytes");
  const auto &candidate = f.h.inspect_centroid_cascade(journal->query);
  const auto &live = f.h.inspect_live_hierarchy();
  bool precision_difference = false;
  for (int bank = 0; bank < 2; ++bank)
    for (int column = 0; column < 4; ++column)
      for (int row = 0; row < H; ++row) {
        const double expected = row < 3 ? candidate.values[(column + 3) * H + row] : 0.;
        require(!std::memcmp(&live.slots[(bank * 4 + column) * H + row], &expected, sizeof(double)),
                "native bank differs from public published prefix/zero destination padding");
      }
  for (int column = 0; column < 7; ++column)
    for (int row = 3; row < H; ++row)
      require(candidate.values[column * H + row] == source_values[column * H + row] &&
                  candidate.valid[row] == 77, "actual source padding was not retained as poison");
  for (int column = 0; column < 3; ++column)
    for (int row = 0; row < 3; ++row)
      precision_difference |= candidate.values[column * H + row] != candidate.values[(column + 3) * H + row];
  require(bits != 31 || precision_difference, "Q31 control lacks trial/publication distinction");
  seed_guard_controls(f, journal->query, old);
  seed_same_execution(f, *reference);

  for (double x : {-2., -3.}) {
    const auto prior = f.h.inspect_live_hierarchy();
    for (auto *fixture : {&f, reference.get()}) {
      fixture->h.inspect_prepared();
      fixture->h.particle64(kPosX, fixture->in.particle_rows[0].record) = x;
      PreparedFixtureAccess::upload_body_fixture64(fixture->h);
      fixture->control->set(1, fixture->nodes[0].field.record);
      Parameters64 p; p.dt = 0; p.gravity = 0;
      for (int tick = 0; tick < 8; ++tick) no_new([&] { fixture->h.tick64(p); });
    }
    const auto &after = f.h.inspect_live_hierarchy();
    for (int node : {0, 2})
      for (int column = 0; column < 4; ++column) {
        const double previous = prior.slots[(prior.selectors[node] * 4 + column) * H + node];
        require(after.selectors[node] != prior.selectors[node] &&
                    !std::memcmp(&after.previous[column * H + node], &previous, sizeof(double)),
                "following publication lost the actual prior XYZ/mass packet history");
      }
    seed_same_execution(f, *reference);
  }
  bool stale = false;
  try { f.h.inspect_centroid_cascade(journal->query); }
  catch (const std::logic_error &) { stale = true; }
  require(stale && f.h.ready(), "tick did not invalidate the installation observation");
  std::cout << "hierarchy_seed_transport,H=" << H << ",nodes=3,field_bits=" << bits
            << ",actual_upload=" << upload << ",actual_readback=" << readback
            << ",native_initialization_bytes=" << initialized << ",native_copy_request_bytes=" << copied
            << ",host_transfer_reduction=" << 124 * H
            << ",boundary_new_calls=0,dirty_destination=1,poisoned_source_padding=1,both_banks_bit_exact=1"
               ",exact_short_retry=1,currentness_guards=21,public_inspection=1,tick_history=16 PASS\n";
}
void hierarchy_seed_failures(engine::Runtime &r, HostPath path) {
  using Fault = TransferJournal::Fault;
  for (auto fault : {Fault::BeforeCopy, Fault::AfterCopy, Fault::FinalSync}) {
    auto f = seed_fixture(r, path, 8, 31);
    auto journal = std::make_unique<TransferJournal>();
    journal->fault = fault;
    bool failed = false;
    try { seed_install(*f, *journal); }
    catch (const std::runtime_error &) { failed = true; }
    require(failed && !f->h.ready() && !f->h.live_hierarchy_installed(),
            "copy/final-sync exception escaped the installation failure boundary");
    require(f->h.install_live_hierarchy(f->h.prepared_storage().generation, f->spec()).status == CascadeStatus::Unready,
            "failed installation resumed an unready execution");
    int fills = 0;
    for (std::size_t i = 0; i < journal->count; ++i) fills += journal->events[i].initialization;
    require(fills == 1 && (fault == Fault::BeforeCopy || journal->copied),
            "failure control did not reach actual native mutation");
  }
  std::cout << "hierarchy_seed_failure,copy_entry_after_fill=unready,copy_completion=unready,final_sync=unready PASS\n";
}
#endif
void hierarchy_seed_transport(engine::Runtime &r, HostPath path) {
  require(!r.on_cuda(), "hierarchy seed transport evidence qualifies CPU only");
#ifdef HCP_TRANSFER_AUDIT
  hierarchy_transfer_audit_control(r);
  hierarchy_seed_transport_case(r, path, 8, 52);
  hierarchy_seed_transport_case(r, path, 70, 52);
  hierarchy_seed_transport_case(r, path, 70, 31);
  hierarchy_seed_failures(r, path);
#else
  throw std::runtime_error("hierarchy seed transfer audit requires Linux GNU/Clang linker wrapping");
#endif
}
