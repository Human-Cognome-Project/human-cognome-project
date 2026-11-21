#include "prepared_owner_fixture.h"
#include "assembly/prepared_submission.h"
#include "snapshot.h"
#include "hierarchy_entry_trace.h"
#include "prepared_metrics.h"
#include "field/cascade_source.h"
#include "taichi/codegen/compiled_kernel_data.h"
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

// Linker observations of real HCP calls only. No production trace/fault API,
// private access, backend mutation, cache clearing or independent native owner.
namespace submission_trace {
using namespace taichi::lang;
struct Root { int id; AllocationRecord record; bool native_sets; };
struct Compilation { std::string name; const CompiledKernelData *data; };
struct Trace {
  engine::Runtime *runtime{};
  std::vector<Root> roots;
  std::vector<Compilation> compilations;
  std::size_t destroys{}, runtime_destroys{};
  std::size_t live_sets_before_finalize{};
  std::uint64_t root_requested_before_finalize{};
  bool emit{};
  std::string phase{"live"};
  unsigned emitted{};
  void next(const char *name) { phase=name; emitted=0; }
};
thread_local Trace *active{};
void check(bool ok, const char *text) { if (!ok) throw std::runtime_error(text); }
LlvmProgramImpl &impl(Program &p) {
  check(p.compile_config().arch == taichi::Arch::x64,"actual LLVM/x64 required");
  return *static_cast<LlvmProgramImpl *>(p.get_program_impl());
}
void candidate_roots(const Trace &t, std::size_t begin) {
  auto *device = t.runtime->program().get_compute_device();
  for (std::size_t i=begin; i<t.roots.size(); ++i)
    check(device->allocation_record(t.roots[i].record.token).status ==
              (t.roots[i].native_sets?AllocationRecordStatus::Live:AllocationRecordStatus::Retired),
          "observed native root lifetime mismatch");
}
}
extern "C" void real_runtime(engine::Runtime *, const engine::InstanceSettings &)
    __asm__("__real__ZN6engine7RuntimeC1ERKNS_16InstanceSettingsE");
extern "C" void wrap_runtime(engine::Runtime *, const engine::InstanceSettings &)
    __asm__("__wrap__ZN6engine7RuntimeC1ERKNS_16InstanceSettingsE");
extern "C" void wrap_runtime(engine::Runtime *r, const engine::InstanceSettings &s) {
  real_runtime(r,s);
  if (auto *t=submission_trace::active) {
    submission_trace::check(!t->runtime,"unexpected second Runtime"); t->runtime=r;
  }
}
extern "C" void real_runtime_destroy(engine::Runtime *)
    __asm__("__real__ZN6engine7RuntimeD1Ev");
extern "C" void wrap_runtime_destroy(engine::Runtime *)
    __asm__("__wrap__ZN6engine7RuntimeD1Ev");
extern "C" void wrap_runtime_destroy(engine::Runtime *r) {
  if (auto *t=submission_trace::active) {
    submission_trace::check(t->runtime==r,"foreign Runtime destruction");
    submission_trace::candidate_roots(*t,0);
    for (const auto &root:t->roots) if (root.native_sets) {
      ++t->live_sets_before_finalize;
      t->root_requested_before_finalize+=r->program().get_compute_device()->allocation_record(root.record.token).requested_bytes;
    }
    ++t->runtime_destroys; t->runtime=nullptr;
  }
  real_runtime_destroy(r);
}
using namespace taichi::lang;
SNodeTree *real_add_tree(Program *, std::unique_ptr<SNode>, bool)
    __asm__("__real__ZN6taichi4lang7Program14add_snode_treeESt10unique_ptrINS0_5SNodeESt14default_deleteIS3_EEb");
SNodeTree *wrap_add_tree(Program *, std::unique_ptr<SNode>, bool)
    __asm__("__wrap__ZN6taichi4lang7Program14add_snode_treeESt10unique_ptrINS0_5SNodeESt14default_deleteIS3_EEb");
SNodeTree *wrap_add_tree(Program *p, std::unique_ptr<SNode> root, bool compile_only) {
  auto *tree=real_add_tree(p,std::move(root),compile_only);
  if (auto *t=submission_trace::active) {
    submission_trace::check(t->runtime && &t->runtime->program()==p && !compile_only,
                            "foreign or unmaterialized tree");
    auto record=submission_trace::impl(*p).snode_root_backing_record(tree->id());
    submission_trace::check(record.status==AllocationRecordStatus::Live,"new root not live");
    const auto &children=tree->root()->ch;
    const bool sets=children.size()==2 && children[0]->type==SNodeType::pointer &&
        children[1]->type==SNodeType::pointer && children[0]->ch.size()==1 &&
        children[1]->ch.size()==1 && children[0]->ch[0]->type==SNodeType::bitmasked &&
        children[1]->ch[0]->type==SNodeType::bitmasked;
    t->roots.push_back({tree->id(),record,sets});
    std::cout << "submission_native_root,phase=" << t->phase << ",tree_id=" << tree->id()
              << ",allocation=" << record.token.allocation << ",native_sets=" << sets
              << ",requested=" << record.requested_bytes << ",extent=" << record.extent_bytes << " PASS\n";
  }
  return tree;
}
void real_destroy_tree(Program *, SNodeTree *)
    __asm__("__real__ZN6taichi4lang7Program18destroy_snode_treeEPNS0_9SNodeTreeE");
void wrap_destroy_tree(Program *, SNodeTree *)
    __asm__("__wrap__ZN6taichi4lang7Program18destroy_snode_treeEPNS0_9SNodeTreeE");
void wrap_destroy_tree(Program *p, SNodeTree *tree) {
  real_destroy_tree(p,tree);
  if (auto *t=submission_trace::active) ++t->destroys;
}
const CompiledKernelData &real_compile(Program *, const CompileConfig &,
    const DeviceCapabilityConfig &, const Kernel &)
    __asm__("__real__ZN6taichi4lang7Program14compile_kernelERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
const CompiledKernelData &wrap_compile(Program *, const CompileConfig &,
    const DeviceCapabilityConfig &, const Kernel &)
    __asm__("__wrap__ZN6taichi4lang7Program14compile_kernelERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
const CompiledKernelData &wrap_compile(Program *p, const CompileConfig &c,
    const DeviceCapabilityConfig &caps, const Kernel &k) {
  const auto &data=real_compile(p,c,caps,k);
  if (auto *t=submission_trace::active) {
    submission_trace::check(t->runtime && &t->runtime->program()==p,"foreign compiled object");
    t->compilations.push_back({k.get_name(),&data});
    if (t->emit && t->emitted<2) {
      std::stringstream encoded;
      submission_trace::check(data.dump(encoded)==CompiledKernelData::Err::kNoError,"native artifact serialization failed");
      CompiledKernelDataFile decoded;
      submission_trace::check(decoded.load(encoded)==CompiledKernelDataFile::Err::kNoError,"native artifact decoding failed");
      const auto prefix=t->phase+"-"+std::to_string(t->emitted++);
      std::ofstream ir(prefix+".ll"), metadata(prefix+".json");
      ir << decoded.src_code(); metadata << decoded.metadata();
      submission_trace::check(bool(ir) && bool(metadata),"native artifact write failed");
      std::cout << "submission_native_artifact,phase=" << t->phase << ",name=" << k.get_name()
                << ",prefix=" << prefix << " PASS\n";
    }
  }
  return data;
}
namespace {
using namespace prepared_owner_fixture;
using submission_trace::check;
template<class V> bool bits(const V &a, const V &b) {
  return a.size()==b.size() && !std::memcmp(a.data(),b.data(),a.size()*sizeof(a[0]));
}
PreparedStudySubmission request(const Input &input, field::HostPath path) {
  PreparedStudySubmission out{input.source,input.bindings,capacity(),domain()}; out.path=path;
  for (auto &v:out.bindings.layout) v.node.layout=nullptr;
  return out;
}
struct Sample {
  std::size_t adds{}, destroys{}, compile_calls{}, unique_data{}, trees{}, free_ids{}, program_kernels{};
  std::size_t live_roots{}, retired_roots{}, live_set_roots{};
  std::uint64_t live_root_requested{};
  std::uint64_t kernel_capacity{}, tree_capacity{}, allocation_record_capacity{};
  double compile_seconds{};
  prepared_test::Sample native;
};
Sample sample(submission_trace::Trace &t, const char *phase) {
  check(t.runtime,"Runtime capture missing");
  auto &p=t.runtime->program();
  prepared_test::Metrics metrics(*t.runtime,{});
  Sample s; s.native=metrics.sample();
  const auto host=p.lifecycle_storage();
  check(host.available && host.trees.available && host.kernels.available &&
            host.free_id_storage_available,"native host observation unavailable");
  std::set<const CompiledKernelData *> data;
  for (const auto &c:t.compilations) {
    // Compilation manager owns these objects until Program teardown; caller
    // Kernel definitions and candidate Harnesses may already be destroyed.
    check(c.data->arch()==taichi::Arch::x64,"compiled object did not survive candidate teardown");
    data.insert(c.data);
  }
  s.adds=t.roots.size(); s.destroys=t.destroys;
  s.compile_calls=t.compilations.size(); s.unique_data=data.size();
  s.trees=host.trees.slots; s.free_ids=host.free_ids.entries;
  s.program_kernels=host.kernels.slots;
  s.kernel_capacity=host.kernels.capacity_bytes; s.tree_capacity=host.trees.capacity_bytes;
  const auto records=p.get_compute_device()->allocation_record_storage();
  check(records.available,"native allocation record storage unavailable");
  s.allocation_record_capacity=records.record_capacity_bytes;
  s.compile_seconds=p.get_total_compilation_time();
  for (const auto &root:t.roots) {
    const auto record=p.get_compute_device()->allocation_record(root.record.token);
    if (record.status==AllocationRecordStatus::Live) {
      ++s.live_roots; s.live_set_roots+=root.native_sets;
      s.live_root_requested+=record.requested_bytes;
    } else {
      check(record.status==AllocationRecordStatus::Retired,"observed root token became unknown");
      ++s.retired_roots;
    }
  }
  std::cout << "submission_native_sample,phase=" << phase << ",root_adds=" << s.adds
            << ",root_destroys=" << s.destroys << ",compile_calls=" << s.compile_calls
            << ",unique_compiled_objects=" << s.unique_data << ",tree_slots=" << s.trees
            << ",free_tree_ids=" << s.free_ids << ",tree_capacity_bytes=" << s.tree_capacity
            << ",program_kernel_slots=" << s.program_kernels
            << ",program_kernel_capacity_bytes=" << s.kernel_capacity
            << ",live_roots=" << s.live_roots << ",retired_roots=" << s.retired_roots
            << ",live_set_roots=" << s.live_set_roots << ",live_root_requested=" << s.live_root_requested
            << ",allocation_record_capacity_bytes=" << s.allocation_record_capacity
            << ",issued_snodes=" << s.native.snodes << ",runtime_requested=" << s.native.requested
            << ",pool_requested=" << s.native.pool.requested_bytes
            << ",pool_reserved=" << s.native.pool.reserved_bytes
            << ",compile_seconds=" << s.compile_seconds << " PASS\n";
  return s;
}
void same_activity(const Sample &a, const Sample &b) {
  check(a.adds==b.adds && a.destroys==b.destroys && a.compile_calls==b.compile_calls &&
            a.unique_data==b.unique_data && a.trees==b.trees && a.free_ids==b.free_ids &&
            a.native.snodes==b.native.snodes,"warmed or early-refused operation provisioned native context");
}
void run(int workers, field::HostPath path, bool emit) {
  Input live, candidate(true);
  auto live_request=request(live,path), pending=request(candidate,path);
  std::weak_ptr<const ActiveAssembly> live_pin=live.source, candidate_pin=candidate.source;
  submission_trace::Trace trace; trace.emit=emit;
  struct Scope {
    explicit Scope(submission_trace::Trace &t) { check(!submission_trace::active,"nested trace"); submission_trace::active=&t; }
    ~Scope() { submission_trace::active=nullptr; }
  } scope(trace);
  handoff_trace::Calls calls;
  {
    engine::InstanceSettings settings; settings.read_environment=false;
    settings.cpu_threads=workers; settings.snode_capacity=4096;
    ActiveSession session(settings);
    const auto empty=sample(trace,"empty_session");
    auto insufficient=pending;
    auto limits=insufficient.capacity.limits(); limits[field::Budget::ScratchBytes]=0;
    insufficient.capacity=field::PreparedCapacity(limits,insufficient.capacity.profile());
    const auto early=session.submit_prepared(std::move(insufficient),candidate.source->owner());
    check(!early.handle && early.failure.budget==field::Budget::ScratchBytes,"early scratch refusal lost");
    same_activity(empty,sample(trace,"early_scratch_refusal"));
    PreparedSubmissionResult result;
    { handoff_trace::Scope count(calls); result=session.submit_prepared(live_request,live.source->owner()); }
    check(result.handle && session.ready(*result.handle) && calls.combined==1 && !calls.standalone,
          "live sibling did not use one combined bootstrap");
    const auto handle=*result.handle; const auto owner=live.source->owner();
    field::Parameters64 parameters; parameters.dt=parameters.gravity=0;
    check(session.tick_prepared(handle,owner,parameters).status==field::TickStatus::Ok,"warm tick failed");
    const auto before=viewport::observe(session,owner,handle);
    const auto warm=sample(trace,"live_warm");
    const auto healthy=[&] {
      const auto current=viewport::observe(session,owner,handle);
      check(session.ready(handle) && session.completed_ticks(handle)==1 && current.tick==before.tick &&
                current.prepared_generation==before.prepared_generation &&
                current.body_records.size()==before.body_records.size() &&
                current.comparisons.size()==before.comparisons.size() &&
                bits(current.report.particles64,before.report.particles64) &&
                bits(current.report.groups64,before.report.groups64),"refusal changed live sibling");
      for (std::size_t i=0;i<current.body_records.size();++i)
        check(field::detail::same_handle(current.body_records[i],before.body_records[i]),"sibling body claim changed");
      for (std::size_t i=0;i<current.comparisons.size();++i) {
        const auto &a=current.comparisons[i], &b=before.comparisons[i];
        check(field::detail::same_handle(a.field,b.field) && a.comparison==b.comparison &&
                  a.level==b.level && a.publish==b.publish && a.retained_fraction_bits==b.retained_fraction_bits &&
                  a.calculation==b.calculation && a.storage==b.storage && a.cohort_begin==b.cohort_begin &&
                  a.cohort_count==b.cohort_count && a.child_begin==b.child_begin && a.child_count==b.child_count,
              "sibling field/comparison changed");
      }
    };
    healthy(); same_activity(warm,sample(trace,"warm_observation"));
    const auto duplicate=session.submit_prepared(live_request,owner);
    check(!duplicate.handle,"duplicate study published again");
    same_activity(warm,sample(trace,"duplicate_refusal")); healthy();
    const auto baseline_roots=trace.roots.size();
    for (const auto *phase:{"plan_refusal_1","plan_refusal_2","placement_refusal"}) {
      auto stopped=pending;
      const bool plan=std::string(phase)!="placement_refusal";
      if (plan) stopped.plan_work=0; else stopped.placement_work=0;
      const auto start=sample(trace,"before_refusal");
      trace.next(phase);
      { handoff_trace::Scope count(calls); result=session.submit_prepared(std::move(stopped),candidate.source->owner()); }
      check(!result.handle && pending.resources->result().live_bytes==0 && calls.combined==1 && !calls.standalone &&
                (plan ? result.preparation_status==AdmissionPlanStatus::WorkExhausted :
                 result.failure.budget==field::Budget::PlacementWork),"candidate refusal published or retained metadata");
      const auto after=sample(trace,phase);
      check(after.adds>start.adds && after.destroys>start.destroys &&
                after.live_roots>start.live_roots && after.live_set_roots>start.live_set_roots &&
                after.compile_calls>start.compile_calls && after.unique_data>start.unique_data,
            "inspection did not reach measured native provisioning/retention boundary");
      submission_trace::candidate_roots(trace,baseline_roots);
      auto *device=trace.runtime->program().get_compute_device();
      for (std::size_t i=0;i<baseline_roots;++i)
        check(device->allocation_record(trace.roots[i].record.token).status==AllocationRecordStatus::Live,
              "candidate teardown retired sibling backing");
      healthy();
    }
    trace.next("retry");
    { handoff_trace::Scope count(calls); result=session.submit_prepared(pending,candidate.source->owner()); }
    check(result.handle && session.ready(*result.handle) && calls.combined==2 && !calls.standalone,
          "healthy retry failed or changed native bootstrap authority");
    healthy(); const auto published=sample(trace,"retry_published");
    check(session.tick_prepared(*result.handle,candidate.source->owner(),parameters).status==field::TickStatus::Ok &&
              session.tick_prepared(handle,owner,parameters).status==field::TickStatus::Ok,
          "post-refusal sibling/candidate following ticks failed");
    check(session.completed_ticks(handle)==2 && session.completed_ticks(*result.handle)==1,
          "following tick counters differ");
    same_activity(published,sample(trace,"following_ticks"));
    live.source.reset(); candidate.source.reset(); live_request.source.reset(); pending.source.reset();
    check(!live_pin.expired() && !candidate_pin.expired(),"published source pin lost");
  }
  check(!trace.runtime && trace.runtime_destroys==1 && trace.destroys<trace.roots.size() &&
            live_pin.expired() && candidate_pin.expired(),"session teardown lost native/source release ordering");
  std::cout << "submission_refusal_inspection,workers=" << workers
            << ",path=" << (path==field::HostPath::BulkCopy?"bulk":"direct")
            << ",refusals=3,combined_commits=" << calls.combined
            << ",standalone_installs=" << calls.standalone
            << ",live_set_roots_before_finalize=" << trace.live_sets_before_finalize
            << ",live_root_requested_before_finalize=" << trace.root_requested_before_finalize
            << ",source_record_roots_retired=1,program_owned_set_roots_retained=1,compiled_objects_retained=1,sibling_unchanged=1"
               ",healthy_retry=1,following_ticks=2,source_pins_released=1 PASS\n";
}
}
int main(int argc,char **argv) {
  try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="--emit-artifacts"),"workers path [--emit-artifacts] required");
    const int workers=std::stoi(argv[1]);
    check(workers==1 || workers==2,"expected one/two workers");
    const auto mode=std::string(argv[2]); check(mode=="bulk" || mode=="direct","expected bulk/direct");
    run(workers,mode=="bulk"?field::HostPath::BulkCopy:field::HostPath::CpuDirect,argc==4);
    return 0;
  } catch(const std::exception &e) { std::cerr << "submission refusal inspection: " << e.what() << '\n'; return 1; }
}
