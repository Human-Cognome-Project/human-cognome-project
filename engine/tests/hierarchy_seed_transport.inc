// Inspection under #343: measure the installed path and exercise an existing
// native copy into isolated scratch. No runtime transport or accounting changes.
#ifdef HCP_TRANSFER_AUDIT
void hierarchy_transfer_audit_control(engine::Runtime &r) {
  using namespace taichi::lang;
  Ndarray array(&r.program(), PrimitiveType::f64, {2});
  const std::array<double, 2> input{{-1.25, 9.5}};
  std::array<double, 2> output{};
  auto journal = std::make_unique<transfer_audit::Journal>();
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
              journal->events[0].array == &array && journal->events[1].array == &array &&
              journal->events[0].bytes == sizeof(input) &&
              journal->events[1].bytes == sizeof(output) && input == output &&
              !std::memcmp(journal->payload.data(), input.data(), sizeof(input)) &&
              !std::memcmp(journal->payload.data() + sizeof(input), input.data(), sizeof(input)),
          "transfer wrapper direction/extent/payload or inactive control failed");
}

void hierarchy_seed_transport_case(engine::Runtime &r, HostPath path, int H, int bits) {
  using namespace taichi::lang;
  auto capacity = limits();
  capacity[Budget::CascadeNodes] = H;
  capacity[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      capacity, false, NumericalProfile::trial(52, bits));
  Fixture f(r, path, {{}, {}, {0, 1}}, {{2, 0}, {0, 3}, {2, 3}},
            {2, 3}, {1., 2.}, {-1., 1.}, 52, bits,
            false, false, false, false, 1024, {}, &capacity);
  const auto generation = f.h.prepared_storage().generation;
  const auto ticks = f.h.prepared_storage().completed_ticks;
  auto journal = std::make_unique<transfer_audit::Journal>();
  LiveHierarchyResult result;
  {
    transfer_audit::Scope scope(*journal);
    no_new([&] { result = f.h.install_live_hierarchy(generation, f.spec()); });
  }
  require(result.status == CascadeStatus::Ok && result.nodes == 3,
          "transport inspection installation failed");

  // Pointer identity identifies the actual seed upload; do not reconstruct an
  // assumed call count from reported totals. Payloads were copied at call time.
  const auto &live = f.h.inspect_live_hierarchy();
  const transfer_audit::Event *seed = nullptr, *values = nullptr, *valid = nullptr;
  std::uint64_t upload = 0, readback = 0;
  for (std::size_t i = 0; i < journal->count; ++i) {
    const auto &e = journal->events[i];
    (e.upload ? upload : readback) += e.bytes;
    if (e.upload && e.host == live.slots.data()) {
      require(!seed, "ambiguous live seed upload");
      seed = &e;
    }
    if (!e.upload && e.bytes == std::size_t(7 * H) * sizeof(double) &&
        e.array->dtype == PrimitiveType::f64 && e.array->get_nelement() == std::size_t(7 * H)) {
      require(!values, "ambiguous centroid value readback");
      values = &e;
    }
    if (!e.upload && e.bytes == std::size_t(H) * sizeof(int) &&
        e.array->dtype == PrimitiveType::i32 && e.array->get_nelement() == std::size_t(H)) {
      require(!valid, "ambiguous centroid validity readback");
      valid = &e;
    }
  }
  require(seed && values && valid && seed->bytes == std::size_t(8 * H) * sizeof(double),
          "actual centroid inspection and live seed transfers were not observed");
  require(upload == result.upload_bytes && result.readback_bytes == 248 &&
              readback == result.readback_bytes + values->bytes + valid->bytes,
          "unexpected actual/reported installation transfer accounting");
  const auto *expected = journal->payload.data() + seed->offset;
  std::vector<double> source(7 * H), output(8 * H), zero(8 * H);
  std::memcpy(source.data(), journal->payload.data() + values->offset, values->bytes);
  for (int bank = 0; bank < 2; ++bank)
    for (int col = 0; col < 4; ++col)
      for (int row = 0; row < H; ++row) {
        const double v = row < int(result.nodes) ? source[(col == 3 ? 6 : col + 3) * H + row] : 0.;
        require(!std::memcmp(expected + ((bank * 4 + col) * H + row) * sizeof(double),
                             &v, sizeof(v)), "seed upload disagrees with published active-prefix mapping");
      }
  if (bits == 31) {
    bool differs = false;
    for (int row = 0; row < int(result.nodes); ++row)
      for (int col = 0; col < 3; ++col)
        differs |= source[col * H + row] != source[(col + 3) * H + row];
    require(differs, "precision control cannot distinguish trial from published centre");
  }

  const auto before = state_hash(f);
  const auto controls = TickExecutionAccess::controls(f.h);
  Ndarray scratch(&r.program(), PrimitiveType::f64, {8 * H});
  Ndarray poisoned(&r.program(), PrimitiveType::f64, {7 * H});
  const auto copy = [&](Ndarray &from, int rows) {
    const auto src = from.get_device_allocation(), dst = scratch.get_device_allocation();
    require(src.device && src.device == dst.device, "native copy crossed devices");
    engine::upload(scratch, zero.data(), zero.size() * sizeof(double));
    for (int bank = 0; bank < 2; ++bank)
      for (int col = 0; col < 4; ++col)
        src.device->memcpy_internal(
            dst.get_ptr(std::size_t((bank * 4 + col) * H) * sizeof(double)),
            src.get_ptr(std::size_t((col == 3 ? 6 : col + 3) * H) * sizeof(double)),
            std::size_t(rows) * sizeof(double));
    engine::readback(scratch, output.data(), output.size() * sizeof(double));
  };
  copy(*values->array, int(result.nodes));
  require(!std::memcmp(output.data(), expected, seed->bytes),
          "native active-prefix copy differs from actual seed upload");

  // Inactive source capacity is not part of the candidate. Poison only a
  // separate allocation to demonstrate why a full-capacity copy is incorrect.
  for (int col = 0; col < 7; ++col)
    for (int row = int(result.nodes); row < H; ++row)
      source[col * H + row] = 1234. + col + row;
  engine::upload(poisoned, source.data(), values->bytes);
  copy(poisoned, int(result.nodes));
  require(!std::memcmp(output.data(), expected, seed->bytes),
          "active-prefix copy consumed inactive capacity");
  copy(poisoned, H);
  require(std::memcmp(output.data(), expected, seed->bytes),
          "full-capacity negative control did not expose stale padding");
  require(before == state_hash(f) && controls == TickExecutionAccess::controls(f.h) && f.h.ready() &&
              f.h.prepared_storage().generation == generation &&
              f.h.prepared_storage().completed_ticks == ticks,
          "isolated primitive witness changed installed execution");
  std::cout << "hierarchy_seed_transport,H=" << H << ",nodes=" << result.nodes
            << ",field_bits=" << bits << ",reported_upload=" << result.upload_bytes
            << ",actual_upload=" << upload << ",reported_readback=" << result.readback_bytes
            << ",actual_readback=" << readback
            << ",inspection_readback=" << values->bytes + valid->bytes
            << ",seed_upload=" << seed->bytes << ",active_native_copy_bytes=" << 64 * result.nodes
            << ",observed_calls=" << journal->count
            << ",boundary_new_calls=0,native_copy_bit_exact=1,padding_control=1,state_unchanged=1 PASS\n";
}
#endif

void hierarchy_seed_transport(engine::Runtime &r, HostPath path) {
  require(!r.on_cuda(), "hierarchy seed transport inspection qualifies CPU only");
#ifdef HCP_TRANSFER_AUDIT
  hierarchy_transfer_audit_control(r);
  hierarchy_seed_transport_case(r, path, 8, 52);
  hierarchy_seed_transport_case(r, path, 70, 52);
  hierarchy_seed_transport_case(r, path, 70, 31);
#else
  throw std::runtime_error("hierarchy seed transfer audit requires Linux GNU/Clang linker wrapping");
#endif
}
