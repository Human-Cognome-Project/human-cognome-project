#include "assembly/active_workbench.h"
#include "letter_projection_fixture.h"
#include "prepared_metrics.h"
#include "taichi/codegen/compiled_kernel_data.h"
#include "compiler_lifetime_trace.h"

#include <cstring>
#include <limits>
#include <set>

// Observe real legacy submissions through linker wrapping only. This witness
// verifies ordered source retirement without owning trees or adding a runtime API.
namespace legacy_trace {
using namespace taichi::lang;
void check(bool ok, const char *text) {
  if (!ok) throw std::runtime_error(text);
}
constexpr auto no_source = std::numeric_limits<std::size_t>::max();
struct Root {
  int id;
  AllocationRecord record;
  bool activity;
  std::size_t source;
  bool published{}, harness_destroyed{}, constructor_fault{};
};
struct Trace {
  engine::Runtime *runtime{};
  std::vector<Root> roots;
  std::vector<const CompiledKernelData *> compilations;
  std::size_t destroys{}, runtime_destroys{};
  std::size_t current_source{no_source}, retiring_source{no_source}, harness_destroys{};
  std::uint64_t serial{}, last_sync{}, last_launch{};
  bool in_harness{}, fail_activity_compile{}, fault_fired{};
  const char *phase{"empty"};
};
thread_local Trace *active{};
AllocationRecordStatus status(const Trace &t, const Root &root) {
  return t.runtime->program().get_compute_device()
      ->allocation_record(root.record.token).status;
}
void retired_since(const Trace &t, std::size_t begin) {
  for (std::size_t i = begin; i < t.roots.size(); ++i)
    check(status(t, t.roots[i]) == AllocationRecordStatus::Retired,
          "failed candidate retained native backing");
  for (std::size_t i = 0; i < begin; ++i)
    if (t.roots[t.roots[i].source].published)
      check(status(t, t.roots[i]) == AllocationRecordStatus::Live,
            "candidate retirement affected published sibling backing");
}
struct Sample {
  std::size_t adds{}, destroys{}, compiles{}, unique_data{}, sources{}, activities{};
  std::size_t tree_slots{}, free_ids{}, program_kernels{};
  std::uint64_t source_requested{}, activity_requested{};
  prepared_test::Sample native;
};
Sample sample(Trace &t, const char *phase) {
  check(t.runtime, "Runtime capture missing");
  auto &p = t.runtime->program();
  check(p.compile_config().arch == taichi::Arch::x64, "actual LLVM/x64 required");
  Sample s;
  compiler_trace::sample(p, phase);
  s.native = prepared_test::Metrics(*t.runtime, {}).sample();
  const auto host = p.lifecycle_storage();
  check(host.available && host.trees.available && host.kernels.available &&
            host.free_id_storage_available, "native host observation unavailable");
  s.tree_slots = host.trees.slots;
  s.free_ids = host.free_ids.entries;
  s.program_kernels = host.kernels.slots;
  s.adds = t.roots.size(); s.destroys = t.destroys;
  s.compiles = t.compilations.size();
  std::set<const CompiledKernelData *> unique;
  for (const auto *data : t.compilations) {
    check(data->arch() == taichi::Arch::x64, "compiled data lost before Program teardown");
    unique.insert(data);
  }
  s.unique_data = unique.size();
  for (const auto &root : t.roots) {
    const auto state = status(t, root);
    check(state == AllocationRecordStatus::Live || state == AllocationRecordStatus::Retired,
          "observed token became unknown");
    if (state != AllocationRecordStatus::Live) continue;
    if (root.activity) { ++s.activities; s.activity_requested += root.record.requested_bytes; }
    else { ++s.sources; s.source_requested += root.record.requested_bytes; }
  }
  std::cout << "legacy_native_sample,phase=" << phase << ",root_adds=" << s.adds
            << ",root_destroys=" << s.destroys << ",compile_calls=" << s.compiles
            << ",unique_compiled_objects=" << s.unique_data
            << ",program_kernel_slots=" << s.program_kernels
            << ",tree_slots=" << s.tree_slots << ",free_tree_ids=" << s.free_ids
            << ",live_source_roots=" << s.sources << ",live_activity_roots=" << s.activities
            << ",source_requested=" << s.source_requested
            << ",activity_requested=" << s.activity_requested
            << ",issued_snodes=" << s.native.snodes << ",runtime_requested=" << s.native.requested
            << ",pool_requested=" << s.native.pool.requested_bytes
            << ",pool_reserved=" << s.native.pool.reserved_bytes
            << ",compile_seconds=" << s.native.compilation << " PASS\n";
  return s;
}
void unchanged(const Sample &a, const Sample &b) {
  check(a.adds == b.adds && a.destroys == b.destroys && a.compiles == b.compiles &&
            a.unique_data == b.unique_data && a.tree_slots == b.tree_slots &&
            a.free_ids == b.free_ids && a.sources == b.sources && a.activities == b.activities &&
            a.native.same_backing(b.native), "early refusal or observation provisioned context");
}
}  // namespace legacy_trace

extern "C" void real_runtime(engine::Runtime *, const engine::InstanceSettings &)
    __asm__("__real__ZN6engine7RuntimeC1ERKNS_16InstanceSettingsE");
extern "C" void wrap_runtime(engine::Runtime *, const engine::InstanceSettings &)
    __asm__("__wrap__ZN6engine7RuntimeC1ERKNS_16InstanceSettingsE");
extern "C" void wrap_runtime(engine::Runtime *r, const engine::InstanceSettings &s) {
  real_runtime(r, s);
  if (auto *t = legacy_trace::active) {
    legacy_trace::check(!t->runtime, "unexpected second Runtime"); t->runtime = r;
  }
}
extern "C" void real_runtime_destroy(engine::Runtime *) __asm__("__real__ZN6engine7RuntimeD1Ev");
extern "C" void wrap_runtime_destroy(engine::Runtime *) __asm__("__wrap__ZN6engine7RuntimeD1Ev");
extern "C" void wrap_runtime_destroy(engine::Runtime *r) {
  if (auto *t = legacy_trace::active) {
    legacy_trace::check(t->runtime == r, "foreign Runtime destruction");
    // ActiveSession has already cleared every Execution. Query copied tokens
    // while the independent Device still exists, never after real destruction.
    const auto s = legacy_trace::sample(*t, "before_runtime_finalize");
    legacy_trace::check(!s.sources && !s.activities && s.adds == 12 && s.destroys == 12 &&
                            t->harness_destroys == 5 && t->fault_fired,
                        "legacy source / Activity teardown incomplete");
    for (const auto &root : t->roots)
      legacy_trace::check(legacy_trace::status(*t, root) == taichi::lang::AllocationRecordStatus::Retired,
          "execution teardown lifetime mismatch");
    ++t->runtime_destroys; t->runtime = nullptr;
  }
  real_runtime_destroy(r);
}
using namespace taichi::lang;
SNodeTree *real_add_tree(Program *, std::unique_ptr<SNode>, bool)
    __asm__("__real__ZN6taichi4lang7Program14add_snode_treeESt10unique_ptrINS0_5SNodeESt14default_deleteIS3_EEb");
SNodeTree *wrap_add_tree(Program *, std::unique_ptr<SNode>, bool)
    __asm__("__wrap__ZN6taichi4lang7Program14add_snode_treeESt10unique_ptrINS0_5SNodeESt14default_deleteIS3_EEb");
SNodeTree *wrap_add_tree(Program *p, std::unique_ptr<SNode> root, bool compile_only) {
  auto *tree = real_add_tree(p, std::move(root), compile_only);
  if (auto *t = legacy_trace::active) {
    legacy_trace::check(t->runtime && &t->runtime->program() == p && !compile_only &&
        p->compile_config().arch == taichi::Arch::x64, "foreign or unmaterialized tree");
    const auto record = static_cast<LlvmProgramImpl *>(p->get_program_impl())
                            ->snode_root_backing_record(tree->id());
    legacy_trace::check(record.status == AllocationRecordStatus::Live, "new root not live");
    const auto &children = tree->root()->ch;
    const bool activity = children.size() == 2 && children[0]->type == SNodeType::pointer &&
        children[1]->type == SNodeType::pointer && children[0]->ch.size() == 1 &&
        children[1]->ch.size() == 1 && children[0]->ch[0]->type == SNodeType::bitmasked &&
        children[1]->ch[0]->type == SNodeType::bitmasked;
    legacy_trace::check(activity || (children.size() == 1 &&
        (children[0]->type == SNodeType::place || children[0]->type == SNodeType::dense)),
        "unexpected legacy source / Activity layout");
    if (!activity) t->current_source = t->roots.size();
    legacy_trace::check(t->current_source != legacy_trace::no_source, "Activity without source");
    t->roots.push_back({tree->id(), record, activity, t->current_source});
    std::cout << "legacy_native_root,phase=" << t->phase << ",tree_id=" << tree->id()
              << ",allocation=" << record.token.allocation << ",activity=" << activity
              << ",primary_step=" << (!activity && children[0]->type == SNodeType::dense)
              << ",requested=" << record.requested_bytes << ",extent=" << record.extent_bytes << " PASS\n";
  }
  return tree;
}
void real_destroy_tree(Program *, SNodeTree *)
    __asm__("__real__ZN6taichi4lang7Program18destroy_snode_treeEPNS0_9SNodeTreeE");
void wrap_destroy_tree(Program *, SNodeTree *)
    __asm__("__wrap__ZN6taichi4lang7Program18destroy_snode_treeEPNS0_9SNodeTreeE");
void wrap_destroy_tree(Program *p, SNodeTree *tree) {
  auto *t = legacy_trace::active;
  std::size_t index = legacy_trace::no_source;
  if (t) {
    for (std::size_t i = t->roots.size(); i-- > 0;)
      if (t->roots[i].id == tree->id() &&
          legacy_trace::status(*t, t->roots[i]) == AllocationRecordStatus::Live) { index = i; break; }
    legacy_trace::check(index != legacy_trace::no_source, "retiring unobserved/live tree");
    const auto &root = t->roots[index];
    if (root.activity) {
      legacy_trace::check(legacy_trace::status(*t, t->roots[root.source]) == AllocationRecordStatus::Live,
                          "source retired before dependent Activity");
      if (t->in_harness) t->retiring_source = root.source;
    } else {
      legacy_trace::check(t->last_sync > t->last_launch &&
                          (root.harness_destroyed || root.constructor_fault),
                          "source retired before drain / dependent Harness destruction");
      std::cout << "legacy_source_retirement,phase=" << t->phase << ",tree_id=" << root.id
                << ",published=" << root.published << ",after_harness=" << root.harness_destroyed
                << ",constructor_fault=" << root.constructor_fault << ",drained=1 PASS\n";
    }
  }
  real_destroy_tree(p, tree);
  if (t) {
    ++t->destroys;
    legacy_trace::check(legacy_trace::status(*t, t->roots[index]) == AllocationRecordStatus::Retired,
                        "native destruction did not retire old token");
  }
}
void real_sync(Program *) __asm__("__real__ZN6taichi4lang7Program11synchronizeEv");
void wrap_sync(Program *) __asm__("__wrap__ZN6taichi4lang7Program11synchronizeEv");
void wrap_sync(Program *p) {
  real_sync(p);
  if (auto *t = legacy_trace::active) t->last_sync = ++t->serial;
}
void real_launch(Program *, const CompiledKernelData &, LaunchContextBuilder &)
    __asm__("__real__ZN6taichi4lang7Program13launch_kernelERKNS0_18CompiledKernelDataERNS0_20LaunchContextBuilderE");
void wrap_launch(Program *, const CompiledKernelData &, LaunchContextBuilder &)
    __asm__("__wrap__ZN6taichi4lang7Program13launch_kernelERKNS0_18CompiledKernelDataERNS0_20LaunchContextBuilderE");
void wrap_launch(Program *p, const CompiledKernelData &data, LaunchContextBuilder &context) {
  real_launch(p, data, context);
  if (auto *t = legacy_trace::active) t->last_launch = ++t->serial;
}
extern "C" void real_harness_destroy(field::Harness *) __asm__("__real__ZN5field7HarnessD1Ev");
extern "C" void wrap_harness_destroy(field::Harness *) __asm__("__wrap__ZN5field7HarnessD1Ev");
extern "C" void wrap_harness_destroy(field::Harness *h) {
  auto *t = legacy_trace::active;
  if (!t) { real_harness_destroy(h); return; }
  legacy_trace::check(!t->in_harness, "unexpected nested Harness destruction");
  const bool drained = t->last_sync > t->last_launch;
  t->in_harness = true; t->retiring_source = legacy_trace::no_source;
  real_harness_destroy(h);
  legacy_trace::check(t->retiring_source != legacy_trace::no_source, "Harness did not retire Activity");
  auto &source = t->roots[t->retiring_source];
  legacy_trace::check(legacy_trace::status(*t, source) == AllocationRecordStatus::Live &&
                        (!source.published || drained), "source/dependent drain ordering failed");
  source.harness_destroyed = true;
  ++t->harness_destroys; t->in_harness = false;
}
const CompiledKernelData &real_compile(Program *, const CompileConfig &,
    const DeviceCapabilityConfig &, const Kernel &)
    __asm__("__real__ZN6taichi4lang7Program14compile_kernelERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
const CompiledKernelData &wrap_compile(Program *, const CompileConfig &,
    const DeviceCapabilityConfig &, const Kernel &)
    __asm__("__wrap__ZN6taichi4lang7Program14compile_kernelERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
const CompiledKernelData &wrap_compile(Program *p, const CompileConfig &c,
    const DeviceCapabilityConfig &caps, const Kernel &k) {
  compiler_trace::capture(&k, compiler_trace::Kind::Definition);
  if (auto *t = legacy_trace::active;
      t && t->fail_activity_compile && k.get_name().find("activity_mark_centroids_") == 0) {
    t->fail_activity_compile = false; t->fault_fired = true;
    t->roots[t->current_source].constructor_fault = true;
    throw std::bad_alloc{};
  }
  const auto &data = real_compile(p, c, caps, k);
  compiler_trace::compiled(*p, k, data);
  if (auto *t = legacy_trace::active) t->compilations.push_back(&data);
  return data;
}
namespace {
using namespace assembly;
using legacy_trace::check;
template<class F> void rejects(F f) {
  bool rejected = false;
  try { f(); } catch (const warm::Error &) { rejected = true; }
  check(rejected, "expected checked public refusal");
}
bool bits(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() && !std::memcmp(a.data(), b.data(), a.size() * sizeof(float));
}
void run(int workers, field::HostPath path) {
  letter_fixture::Fixture fixture;
  const auto source = [&](warm::Exposure exposure, bool requires_local = false) {
    return ActiveAssembly::assemble(normalize_letters(fixture.base,
        {fixture.request(fixture.english, 0, exposure)}), warm::new_uuid(), requires_local);
  };
  const auto live = source(warm::Exposure::Aggregate);
  const auto candidate = source(warm::Exposure::Parts);
  const auto unavailable = source(warm::Exposure::Aggregate, true);
  legacy_trace::Trace trace;
  compiler_trace::Trace compiler;
  compiler_trace::Scope compiler_scope(compiler);
  struct Scope {
    explicit Scope(legacy_trace::Trace &t) {
      check(!legacy_trace::active, "nested trace"); legacy_trace::active = &t;
    }
    ~Scope() { legacy_trace::active = nullptr; }
  } scope(trace);
  ActiveReport retained;
  {
    engine::InstanceSettings settings; settings.read_environment = false;
    settings.cpu_threads = workers; settings.snode_capacity = 4096;
    ActiveSession session(settings);
    field::Parameters parameters; parameters.dt = parameters.gravity = 0;
    const auto empty = legacy_trace::sample(trace, "empty");
    rejects([&] { session.submit(unavailable, parameters, path); });
    legacy_trace::unchanged(empty, legacy_trace::sample(trace, "unavailable_refusal"));
    trace.phase = "live_submit";
    const auto handle = session.submit(live, parameters, path);
    trace.roots[0].published = true;
    session.tick(handle, parameters);
    const auto before = session.report(handle, live.owner());
    const auto warm = legacy_trace::sample(trace, "live_warm");
    check(warm.sources == 1 && warm.activities == 1 && warm.adds == 2, "unexpected legacy owners");
    const auto healthy = [&] {
      const auto report = session.report(handle, live.owner());
      check(session.ready(handle) && session.completed_ticks(handle) == 1 &&
          bits(report.particles, before.particles) && bits(report.groups, before.groups) &&
          bits(report.determined, before.determined), "failed submission changed warmed sibling");
    };
    healthy();
    rejects([&] { session.report(handle, candidate.owner()); });
    auto stale = handle; stale.generation = warm::new_uuid();
    rejects([&] { session.report(stale, live.owner()); });
    rejects([&] { session.submit(live, parameters, path); });
    legacy_trace::unchanged(warm, legacy_trace::sample(trace, "duplicate_refusal"));
    for (int attempt = 0; attempt < 2; ++attempt) {
      trace.phase = attempt ? "nonfinite_parameter_refusal" : "negative_parameter_refusal";
      const auto start = legacy_trace::sample(trace, "before_refusal");
      auto invalid = parameters;
      invalid.dt = attempt ? std::numeric_limits<float>::quiet_NaN() : -1.f;
      bool rejected = false;
      try { session.submit(candidate, invalid, path); }
      catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "invalid parameter did not reach real Harness validation");
      const auto after = legacy_trace::sample(trace, trace.phase);
      check(after.adds == start.adds + 2 && after.destroys == start.destroys + 2 &&
          after.sources == start.sources && after.activities == start.activities &&
          after.compiles > start.compiles && after.unique_data > start.unique_data,
          "legacy late failure lifetime changed");
      check(!trace.roots[start.adds].activity && trace.roots[start.adds + 1].activity &&
          legacy_trace::status(trace, trace.roots[start.adds]) == AllocationRecordStatus::Retired &&
          legacy_trace::status(trace, trace.roots[start.adds + 1]) == AllocationRecordStatus::Retired,
          "failed candidate source / Activity lifetime mismatch");
      legacy_trace::retired_since(trace, warm.adds);
      auto absent = handle; absent.study = candidate.study(); absent.generation = candidate.generation();
      rejects([&] { session.report(absent, candidate.owner()); });
      healthy();
    }
    trace.phase = "healthy_retry";
    const auto retry_roots = trace.roots.size();
    const auto retry = session.submit(candidate, parameters, path);
    trace.roots[retry_roots].published = true;
    check(session.ready(retry) && !session.completed_ticks(retry), "retry not ready");
    healthy();
    const auto stable = legacy_trace::sample(trace, "retry_ready");
    session.tick(retry, parameters); session.tick(handle, parameters);
    check(session.completed_ticks(retry) == 1 && session.completed_ticks(handle) == 2,
          "following ticks failed");
    retained = session.report(retry, candidate.owner());
    check(!retained.particles.empty(), "retry report absent");
    legacy_trace::unchanged(stable, legacy_trace::sample(trace, "following_ticks"));
    const auto live_completed = session.report(handle, live.owner());
    for (std::size_t i = retry_roots; i < trace.roots.size(); ++i)
      check(legacy_trace::status(trace, trace.roots[i]) == AllocationRecordStatus::Live,
            "successful retry backing retired before following ticks");

    // A real Activity constructor unwinds after registration, before FixedStudy
    // can publish its owned Harness. The linker fault adds no production switch.
    const auto partial = source(warm::Exposure::Parts);
    const auto begin = trace.roots.size();
    trace.phase = "activity_constructor_failure"; trace.fail_activity_compile = true;
    bool stopped = false;
    try { session.submit(partial, parameters, path); }
    catch (const std::bad_alloc &) { stopped = true; }
    check(stopped && trace.fault_fired && trace.roots.size() == begin + 2,
          "partial constructor failure did not reach registered trees");
    legacy_trace::retired_since(trace, begin);
    auto absent = handle; absent.study = partial.study(); absent.generation = partial.generation();
    rejects([&] { session.report(absent, partial.owner()); });
    const auto current = session.report(retry, candidate.owner());
    const auto live_current = session.report(handle, live.owner());
    check(session.completed_ticks(handle) == 2 && session.completed_ticks(retry) == 1 &&
          session.ready(handle) && session.ready(retry) &&
          bits(current.particles, retained.particles) && bits(current.groups, retained.groups) &&
          bits(current.determined, retained.determined) &&
          bits(live_current.particles, live_completed.particles) &&
          bits(live_current.groups, live_completed.groups) &&
          bits(live_current.determined, live_completed.determined),
          "constructor failure affected published siblings");
    legacy_trace::sample(trace, "partial_constructor_refusal");
    trace.phase = "partial_constructor_retry";
    const auto next = trace.roots.size();
    const auto recovered = session.submit(partial, parameters, path);
    trace.roots[next].published = true;
    session.tick(recovered, parameters);
    check(session.ready(recovered) && session.completed_ticks(recovered) == 1,
          "constructor failure retry did not tick");
    legacy_trace::sample(trace, "partial_constructor_retry");
    trace.phase = "session_teardown";
  }
  check(!trace.runtime && trace.runtime_destroys == 1 && !retained.particles.empty(),
        "session teardown or independently owning report failed");
  compiler_trace::finish(compiler);
  std::cout << "legacy_submission_inspection,workers=" << workers
            << ",path=" << (path == field::HostPath::BulkCopy ? "bulk" : "direct")
            << ",real_parameter_refusals=2,partial_constructor_refusals=1"
               ",source_roots_live_before_finalize=0,source_roots_retired=6"
               ",activity_roots_retired=6,harness_destroys=5,sibling_unchanged=1,healthy_retry=1"
               ",following_ticks=2,constructor_retry_tick=1 PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 3, "workers path required");
    const int workers = std::stoi(argv[1]);
    const std::string mode = argv[2];
    check((workers == 1 || workers == 2) && (mode == "bulk" || mode == "direct"),
          "expected one/two workers and bulk/direct");
    run(workers, mode == "bulk" ? field::HostPath::BulkCopy : field::HostPath::CpuDirect);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "legacy submission inspection: " << e.what() << '\n'; return 1;
  }
}
