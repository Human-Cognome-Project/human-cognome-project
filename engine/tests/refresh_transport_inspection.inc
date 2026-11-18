// Actual refresh transfers and isolated native-copy comparison. Runtime
// transport remains unchanged; only independent scratch destinations are written.
#ifdef HCP_TRANSFER_AUDIT
auto refresh_transport_inputs(Fixture &f) {
  std::array<std::vector<unsigned char>,3> out;
  const std::array<int,3> inputs{3,5,6};
  for (std::size_t i=0; i<inputs.size(); ++i) {
    auto *array = LiveHierarchyAccess::input_array(f.h,inputs[i]);
    out[i].resize(array->get_nelement()*array->get_element_size());
    engine::readback(*array,out[i].data(),out[i].size());
  }
  return out;
}
void refresh_transport_payload(Fixture &f, const transfer_audit::Journal &journal,
                               int input, std::uint64_t exact_word, std::uint64_t &candidate) {
  using namespace taichi::lang;
  auto *live = LiveHierarchyAccess::input_array(f.h,input);
  const transfer_audit::Event *commit = nullptr, *stage = nullptr;
  for (std::size_t i=0; i<journal.count; ++i) {
    const auto &e = journal.events[i];
    if (e.upload && e.array == live) {
      require(!commit,"multiple refresh commit payloads for selected array"); commit = &e;
    }
  }
  require(commit,"missing selected refresh commit payload");
  for (std::size_t i=0; i<journal.count; ++i) {
    const auto &e = journal.events[i];
    if (e.upload && e.array != live && e.host == commit->host && e.bytes == commit->bytes) {
      require(!stage,"ambiguous refresh staging payload"); stage = &e;
    }
  }
  require(stage && stage < commit && stage->array->dtype == live->dtype &&
          stage->array->shape == live->shape && commit->bytes > 8 &&
          commit->bytes == live->get_nelement()*live->get_element_size() &&
          !std::memcmp(journal.payload.data()+stage->offset,journal.payload.data()+commit->offset,commit->bytes),
          "stage/commit input payload, extent or order differs");
  std::vector<unsigned char> source(commit->bytes), destination(commit->bytes), copied(commit->bytes);
  engine::readback(*stage->array,source.data(),source.size());
  engine::readback(*live,destination.data(),destination.size());
  require(source == destination && !std::memcmp(source.data(),journal.payload.data()+stage->offset,source.size()),
          "native candidate/commit input was mutated after staging");
  if (input == 5 && exact_word) {
    bool found = false;
    for (std::size_t i=0; i<source.size(); i+=sizeof(std::uint64_t)) {
      std::uint64_t value; std::memcpy(&value,source.data()+i,sizeof(value));
      found |= value == exact_word;
    }
    require(found,"wide u64 source provenance was rounded before transport");
  }
  Ndarray scratch(&f.runtime.program(),live->dtype,live->shape);
  const auto from = stage->array->get_device_allocation(), to = scratch.get_device_allocation();
  require(from.device && from.device == to.device && from != to && to != live->get_device_allocation(),
          "scratch copy does not have an independent same-device destination");
  f.runtime.program().fill_ndarray_fast_u32(&scratch,0xa5a5a5a5);
  engine::readback(scratch,copied.data(),copied.size());
  require(copied != destination,"dirty scratch control does not distinguish real payload");
  from.device->memcpy_internal(to.get_ptr(),from.get_ptr(),commit->bytes-8);
  engine::readback(scratch,copied.data(),copied.size());
  require(copied != destination,"truncated native copy failed to expose stale destination padding");
  from.device->memcpy_internal(to.get_ptr(),from.get_ptr(),commit->bytes);
  engine::readback(scratch,copied.data(),copied.size());
  require(copied == destination,"full native scratch copy differs bitwise from actual commit payload");
  candidate += commit->bytes;
  std::cout << "refresh_transport_array,input=" << input << ",bytes=" << commit->bytes
            << ",stage_commit_equal=1,native_equal=1,truncated_refused=1\n";
}
void refresh_transport_case(engine::Runtime &runtime, HostPath path, int H, int bits, bool wide) {
  const std::uint64_t first = wide ? (std::uint64_t{1}<<54)+1 : 2;
  const std::uint64_t second = wide ? (std::uint64_t{1}<<54)+3 : 3;
  auto capacity = limits(); capacity[Budget::CascadeNodes] = H;
  capacity[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      capacity,false,NumericalProfile::trial(52,bits));
  Fixture f(runtime,path,{{},{},{0,1}},{{first,0},{0,second},{first,second}},
      {first,second},{1.,2.},{-1.,1.},52,bits,false,false,false,false,1024,{},&capacity);
  f.install();
  refresh_compiler_controls(f); // Both compiled kernels keep these metadata inputs read-only.
  auto journal = std::make_unique<transfer_audit::Journal>();
  RefreshRequest same(f.h);
  const auto unchanged = state_hash(f);
  const auto initial_inputs = refresh_transport_inputs(f);
  {
    transfer_audit::Scope trace(*journal);
    require(refresh_apply(f,same).status == UpdateStatus::NoChange,"identical refresh is not a no-op");
  }
  require(state_hash(f) == unchanged && refresh_transport_inputs(f) == initial_inputs,
          "no-op refresh changed committed state or native input tables");
  for (int phase=0; phase<2; ++phase) {
    RefreshRequest request(f.h);
    std::vector<UpdateCommand> commands;
    if (!phase) request.nodes.back().retained_fraction_bits = bits == 52 ? 31 : 52;
    else {
      const auto &s = f.h.prepared_storage(); const auto body = f.in.particle_rows[0];
      for (std::size_t row=0; row<s.live.edges; ++row) if (s.edges[row].first.record == body.record) {
        const auto &edge = s.edges[row];
        UpdateCommand cmd; cmd.operation = UpdateOp::ReplaceParticipation;
        cmd.row = row; cmd.generation = s.generation;
        cmd.first = UpdateRef::existing(edge.first); cmd.second = UpdateRef::existing(edge.second);
        cmd.ordinal = edge.ordinal; cmd.exact_count = first-1;
        cmd.share = float(double(first-1)/double(first));
        request.replaced(row,int(commands.size())); commands.push_back(cmd);
      }
      require(commands.size() == 2,"count edit does not cover leaf and parent source provenance");
    }
    const auto before = state_hash(f);
    const auto before_inputs = refresh_transport_inputs(f);
    const auto generation = f.h.prepared_storage().generation;
    journal->count = journal->used = 0;
    {
      transfer_audit::Scope trace(*journal);
      require(refresh_apply(f,request,commands,0).status == UpdateStatus::CapacityExhausted,
              "zero refresh work was not refused");
    }
    require(f.h.ready() && state_hash(f) == before && refresh_transport_inputs(f) == before_inputs,
            "short refresh changed committed state or native input tables");
    for (std::size_t i=0; i<journal->count; ++i) for (int input : {3,5,6})
      require(!journal->events[i].upload || journal->events[i].array != LiveHierarchyAccess::input_array(f.h,input),
              "refused refresh uploaded live input tables");
    journal->count = journal->used = 0;
    UpdateResult result;
    {
      transfer_audit::Scope trace(*journal); result = refresh_apply(f,request,commands);
    }
    refresh_ok(result);
    require(result.generation == generation+1 && f.h.prepared_storage().generation == result.generation,
            "refresh did not publish exactly one coherent native generation");
    std::uint64_t uploaded=0, read=0, candidate=0;
    for (std::size_t i=0; i<journal->count; ++i) {
      const auto &e = journal->events[i];
      require(!e.initialization && !e.copy,"unexpected native initialization/copy during unchanged runtime refresh");
      (e.upload ? uploaded : read) += e.bytes;
    }
    if (uploaded != result.upload_bytes || read != result.readback_bytes)
      std::cerr << "refresh_transfer_totals,actual_upload=" << uploaded << ",reported_upload=" << result.upload_bytes
                << ",actual_readback=" << read << ",reported_readback=" << result.readback_bytes << '\n';
    require(uploaded == result.upload_bytes && read == result.readback_bytes,"actual/reported refresh bytes differ");
    const auto current = state_hash(f);
    const auto current_inputs = refresh_transport_inputs(f);
    for (int input : {3,5,6}) refresh_transport_payload(f,*journal,input,wide ? first : 0,candidate);
    require(f.h.ready() && state_hash(f) == current && refresh_transport_inputs(f) == current_inputs,
            "scratch comparison mutated the actual owner or native input tables");
    std::cout << "refresh_transport,H=" << H << ",profile_field_bits=" << bits
      << ",root_bits=" << request.nodes.back().retained_fraction_bits << ",u64_wide=" << wide
      << ",phase=" << phase << ",generation=" << result.generation << ",events=" << journal->count
      << ",upload=" << uploaded << ",readback=" << read
      << ",duplicate_commit_upload=" << candidate << ",scratch_native_copy=" << candidate
      << ",runtime_saving=0 PASS\n";
    const auto ticks = f.h.prepared_storage().completed_ticks;
    Parameters64 parameters; parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    no_new([&] { f.h.tick64(parameters); });
    require(f.h.ready() && f.h.prepared_storage().completed_ticks == ticks+1,"following actual native tick failed");
  }
}
void refresh_transport_inspection(engine::Runtime &runtime, HostPath path) {
  hierarchy_transfer_audit_control(runtime);
  refresh_transport_case(runtime,path,8,52,false);
  refresh_transport_case(runtime,path,70,52,false);
  refresh_transport_case(runtime,path,70,31,true);
}
#endif
