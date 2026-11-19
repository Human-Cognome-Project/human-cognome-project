// Actual refresh transport, full reserved metadata and shared commit callers.
// Linux linker instrumentation is confined to this test executable.
#ifdef HCP_TRANSFER_AUDIT
auto refresh_metadata_inputs(Harness &h) {
  std::array<std::vector<unsigned char>,3> out;
  const std::array<int,3> inputs{3,5,6};
  for (std::size_t i=0; i<inputs.size(); ++i) {
    auto *array = LiveHierarchyAccess::input_array(h,inputs[i]);
    out[i].resize(array->get_nelement()*array->get_element_size());
    engine::readback(*array,out[i].data(),out[i].size());
  }
  return out;
}
void refresh_metadata_dirty_tail(Harness &h) {
  auto values = refresh_metadata_inputs(h);
  const std::array<int,3> inputs{3,5,6};
  // These fixtures leave the final child/source/comparison rows unused. Poison
  // only their final eight bytes, never an active descriptor or native packet.
  for (std::size_t i=0; i<inputs.size(); ++i) {
    require(values[i].size() > 8,"metadata tail control has no reserved capacity");
    std::fill(values[i].end()-8,values[i].end(),0xa5);
    engine::upload(*LiveHierarchyAccess::input_array(h,inputs[i]),values[i].data(),values[i].size());
  }
}
const transfer_audit::Event &refresh_metadata_stage(Harness &h, const transfer_audit::Journal &journal, int input) {
  using namespace taichi::lang;
  const int argument = input == 3 ? 7 : input == 5 ? 9 : 10;
  const auto kernels = LiveHierarchyAccess::refresh_kernels(h);
  const auto pointer = engine::testing::CompiledKernelAccess::context(*kernels[0]).array_ptrs.at(
      {argument,TypeFactory::DATA_PTR_POS_IN_NDARRAY});
  require(pointer == engine::testing::CompiledKernelAccess::context(*kernels[1]).array_ptrs.at(
      {argument,TypeFactory::DATA_PTR_POS_IN_NDARRAY}),"refresh kernels lost their shared frozen candidate input");
  auto *live = LiveHierarchyAccess::input_array(h,input);
  const transfer_audit::Event *stage = nullptr;
  for (std::size_t i=0; i<journal.count; ++i) {
    const auto &e = journal.events[i];
    require(!e.upload || e.array != live,"refresh still uploads selected live metadata");
    if (e.upload && pointer == reinterpret_cast<void *>(e.array->get_device_allocation_ptr_as_int())) {
      require(!stage,"multiple selected metadata staging uploads"); stage = &e;
    }
  }
  require(stage && stage->array != live && stage->array->dtype == live->dtype &&
          stage->array->shape == live->shape && stage->bytes > 8 &&
          stage->bytes == live->get_nelement()*live->get_element_size(),
          "missing or incompatible full metadata staging upload");
  return *stage;
}
std::uint64_t refresh_metadata_observe(Harness &h, const transfer_audit::Journal &journal,
                                     const UpdateResult &result, const char *caller, std::uint64_t exact_word=0) {
  require(result.status == UpdateStatus::Ok && journal.refresh_commits == 1,
          "metadata observation did not span exactly one successful shared commit");
  std::uint64_t uploaded=0, read=0, copied=0;
  for (std::size_t i=0; i<journal.count; ++i) {
    const auto &e = journal.events[i];
    if (!e.initialization && !e.copy) (e.upload ? uploaded : read) += e.bytes;
  }
  require(uploaded == result.upload_bytes && read == result.readback_bytes,
          "actual/reported shared commit host transfer bytes differ");
  for (int input : {3,5,6}) {
    const auto &stage = refresh_metadata_stage(h,journal,input);
    auto *live = LiveHierarchyAccess::input_array(h,input);
    std::vector<unsigned char> source(stage.bytes), destination(stage.bytes);
    engine::readback(*stage.array,source.data(),source.size());
    engine::readback(*live,destination.data(),destination.size());
    require(source == destination && !std::memcmp(source.data(),journal.payload.data()+stage.offset,source.size()),
            "native staged/live metadata differs from exact call-time payload or leaves stale padding");
    if (input == 5 && exact_word) {
      bool found = false;
      for (std::size_t i=0; i<source.size(); i+=sizeof(std::uint64_t)) {
        std::uint64_t value; std::memcpy(&value,source.data()+i,sizeof(value)); found |= value == exact_word;
      }
      require(found,"odd wide u64 provenance was rounded before native transport");
    }
    copied += stage.bytes;
  }
  require(copied == result.native_copy_bytes && copied == journal.refresh_copy_bytes,
          "reported native copy request bytes differ from the three full native payloads");
  std::cout << "refresh_metadata,caller=" << caller << ",events=" << journal.count
    << ",host_upload=" << uploaded << ",host_readback=" << read
    << ",reported_native_copy=" << copied << ",full_payload_equal=1 PASS\n";
  return copied;
}
bool refresh_metadata_remapped(Harness &h) {
  const auto &s = h.prepared_storage();
  for (int i=0; i<h.particle_capacity()+h.field_capacity(); ++i)
    if (s.validation[i] > 0 && s.destinations[i] != (i<h.particle_capacity() ? i : i-h.particle_capacity())) return true;
  return false;
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
  refresh_compiler_controls(f);
  auto journal = std::make_unique<transfer_audit::Journal>();
  RefreshRequest same(f.h);
  const auto unchanged = state_hash(f);
  const auto initial_inputs = refresh_metadata_inputs(f.h);
  {
    transfer_audit::Scope trace(*journal);
    const auto noop = refresh_apply(f,same);
    require(noop.status == UpdateStatus::NoChange && !noop.native_copy_bytes,"identical refresh performed native metadata copies");
  }
  require(!journal->refresh_commits && state_hash(f) == unchanged && refresh_metadata_inputs(f.h) == initial_inputs,
          "no-op refresh changed committed state or native inputs");
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
    refresh_metadata_dirty_tail(f.h);
    const auto before = state_hash(f);
    const auto before_inputs = refresh_metadata_inputs(f.h);
    const auto generation = f.h.prepared_storage().generation;
    journal->count = journal->used = 0;
    {
      transfer_audit::Scope trace(*journal);
      const auto refused = refresh_apply(f,request,commands,0);
      require(refused.status == UpdateStatus::CapacityExhausted && !refused.native_copy_bytes,
              "zero refresh work did not refuse without native copies");
    }
    require(!journal->refresh_commits && f.h.ready() && state_hash(f) == before && refresh_metadata_inputs(f.h) == before_inputs,
            "short refresh changed committed state or dirty native tails");
    journal->count = journal->used = 0;
    UpdateResult result;
    { transfer_audit::Scope trace(*journal); result = refresh_apply(f,request,commands); }
    refresh_ok(result);
    require(result.generation == generation+1 && f.h.prepared_storage().generation == result.generation,
            "refresh did not publish exactly one coherent native generation");
    const auto current = state_hash(f);
    const auto current_inputs = refresh_metadata_inputs(f.h);
    const auto copied = refresh_metadata_observe(f.h,*journal,result,"installed_refresh",wide ? first : 0);
    // Accepted inspection 9043bcb8: actual host totals before the replacement.
    const std::uint64_t baseline = (H == 8 ? 113144 : 120336) - (path == HostPath::CpuDirect ? 6704 : 0);
    require(result.upload_bytes+copied == baseline && result.readback_bytes == 12 &&
            copied == std::uint64_t(560+8192+8*H),"actual host saving differs from accepted source-pinned baseline");
    require(f.h.ready() && state_hash(f) == current && refresh_metadata_inputs(f.h) == current_inputs,
            "metadata observation mutated committed state");
    std::cout << "refresh_transport,H=" << H << ",profile_field_bits=" << bits
      << ",root_bits=" << request.nodes.back().retained_fraction_bits << ",u64_wide=" << wide
      << ",phase=" << phase << ",generation=" << result.generation << ",host_upload=" << result.upload_bytes
      << ",host_readback=" << result.readback_bytes << ",removed_host_upload=" << copied
      << ",reported_native_copy=" << result.native_copy_bytes << ",dirty_tails_cleared=1 PASS\n";
    const auto ticks = f.h.prepared_storage().completed_ticks;
    Parameters64 parameters; parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
    no_new([&] { f.h.tick64(parameters); });
    require(f.h.ready() && f.h.prepared_storage().completed_ticks == ticks+1,"following actual native tick failed");
    journal->refresh_commits = 0; journal->refresh_copy_bytes = 0;
  }
}
void refresh_transport_failures(engine::Runtime &runtime, HostPath path) {
  using Fault = transfer_audit::Journal::Fault;
  for (auto fault : {Fault::BeforeCopy,Fault::AfterCopy,Fault::FinalSync}) {
    Fixture f(runtime,path,{{}},{{1}},{1},{1.},{0.}); f.install();
    RefreshRequest request(f.h); request.nodes[0].retained_fraction_bits = 31;
    const auto generation = f.h.prepared_storage().generation;
    const auto ticks = f.h.prepared_storage().completed_ticks;
    auto journal = std::make_unique<transfer_audit::Journal>(); journal->fault = fault;
    bool failed = false;
    try { transfer_audit::Scope trace(*journal); refresh_apply(f,request); }
    catch (const std::runtime_error &) { failed = true; }
    require(failed && !f.h.ready() && f.h.prepared_storage().generation == generation &&
            f.h.prepared_storage().completed_ticks == ticks && refresh_apply(f,request).status == UpdateStatus::Unready,
            "postclaim refresh failure advertised a healthy/retryable generation");
    require((fault == Fault::BeforeCopy ? journal->refresh_copy_bytes == 0 : journal->refresh_copy_bytes > 0),
            "refresh fault did not cross its intended copy boundary");
    std::cout << "refresh_transport_failure,boundary=" << int(fault) << ",unready=1 PASS\n";
  }
}
void refresh_transport_inspection(engine::Runtime &runtime, HostPath path) {
  hierarchy_transfer_audit_control(runtime);
  refresh_transport_case(runtime,path,8,52,false);
  refresh_transport_case(runtime,path,70,52,false);
  refresh_transport_case(runtime,path,70,31,true);
  refresh_transport_failures(runtime,path);
  for (bool wrong_shape : {false,true}) {
    Fixture f(runtime,path,{{}},{{1}},{1},{1.},{0.}); f.install();
    RefreshRequest request(f.h); request.nodes[0].retained_fraction_bits = 31;
    auto *target = LiveHierarchyAccess::input_array(f.h,3);
    const auto dtype = target->dtype;
    const auto extent = target->shape[0];
    if (wrong_shape) ++target->shape[0];
    else target->dtype = taichi::lang::PrimitiveType::f32;
    const auto generation = f.h.prepared_storage().generation;
    bool refused = false;
    try { refresh_apply(f,request); } catch (const std::logic_error &) { refused = true; }
    // Restore only wrapper metadata for orderly teardown; never repair or
    // resume the execution that failed after claims.
    target->dtype = dtype; target->shape[0] = extent;
    require(refused && !f.h.ready() && f.h.prepared_storage().generation == generation &&
            refresh_apply(f,request).status == UpdateStatus::Unready,
            "incompatible native metadata destination bypassed postclaim guard");
    std::cout << "refresh_transport_guard,wrong_shape=" << wrong_shape << ",unready=1 PASS\n";
  }
}
#endif
