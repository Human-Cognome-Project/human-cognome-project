#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &definition(const CompiledKernel &k) { return *k.kernel_; }
  static const taichi::lang::CompiledKernelData &data(const CompiledKernel &k) { return k.compiled_data(); }
};
}  // namespace engine::testing

namespace {
using namespace taichi::lang;
using Access = engine::testing::CompiledKernelAccess;
using Status = KernelCompilationManager::RemovalStatus;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
struct RegistrationFault {};
bool fail_registration = false;
JITModule *latest_module = nullptr;
const void *watched_data = nullptr, *watched_metadata = nullptr;
bool data_released = false, metadata_released = false;
void released(void *pointer, std::size_t) noexcept {
  if (pointer == watched_data) data_released = true;
  if (pointer == watched_metadata) metadata_released = true;
}
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &runtime, const char *key) {
  IRBuilder body;
  auto *array = body.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *value = body.create_arg_load({1}, PrimitiveType::i32, false, 0);
  body.create_global_store(body.create_external_ptr(array, {body.get_int32(0)}), value);
  return std::make_unique<engine::CompiledKernel>(runtime, key, body.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32),
                                 engine::Param::scalar(PrimitiveType::i32)});
}
void launch(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array,
            int value, bool prepared) {
  auto invocation = k.invocation(prepared);
  auto &ctx = invocation.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0}, array);
  engine::argument_int(ctx, 1, value); k.launch(ctx); runtime.synchronize();
  int actual = 0; engine::readback(array, &actual, sizeof(actual));
  check(actual == value, "removal changed following native dispatch");
}
bool same(const CompilerOwnershipCounts &a, const CompilerOwnershipCounts &b) {
  return a.status == b.status && a.owner == b.owner && a.complete == b.complete &&
      a.pending_entries == b.pending_entries && a.pending_objects == b.pending_objects &&
      a.disk_metadata_entries == b.disk_metadata_entries && a.loaded_objects == b.loaded_objects &&
      a.visited_entries == b.visited_entries;
}
bool same(const JitOwnershipCounts &a, const JitOwnershipCounts &b) {
  return a.status == b.status && a.owner == b.owner && a.complete == b.complete &&
      a.modules == b.modules && a.runtime_modules == b.runtime_modules &&
      a.kernel_modules == b.kernel_modules && a.visited_modules == b.visited_modules;
}
KernelCompilationManager::RemovalResult remove(KernelCompilationManager &manager,
    const std::string &key, std::size_t work) {
  allocation_observation::calls = 0; allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  const auto result = manager.remove_unregistered_kernel(key, work);
  allocation_observation::enabled = false;
  check(!allocation_observation::calls && !allocation_observation::bytes,
        "bounded removal allocated C++ storage");
  return result;
}
void refuses(KernelCompilationManager &manager, const std::string &key, Status status,
             std::size_t work = unlimited) {
  const auto before = manager.ownership_counts(unlimited);
  const auto result = remove(manager, key, work);
  check(result.status == status && same(before, manager.ownership_counts(unlimited)),
        "removal refusal changed native cache state or reason");
  check(result.visited_entries == (status == Status::Incomplete ? 0 : before.visited_entries),
        "removal work coverage was not explicit");
}
std::vector<void *> tasks(JITModule &module, const engine::CompiledKernel &k) {
  std::vector<void *> result;
  const auto &data = dynamic_cast<const LLVM::CompiledKernelData &>(Access::data(k));
  for (const auto &task : data.get_internal_data().compiled_data.tasks) {
    auto *address = module.lookup_function(task.name);
    check(address, "surviving task symbol disappeared"); result.push_back(address);
  }
  check(!result.empty(), "native store kernel has no task"); return result;
}
class CloneCompiler final : public KernelCompiler {
 public:
  explicit CloneCompiler(const CompiledKernelData &source) : source_(source) {}
  IRNodePtr compile(const CompileConfig &, const Kernel &) const override { return std::make_unique<Block>(); }
  CKDPtr compile(const CompileConfig &, const DeviceCapabilityConfig &, const Kernel &, IRNode &) const override {
    return source_.clone();
  }
 private:
  const CompiledKernelData &source_;
};
class CoverageFaultData final : public LLVM::CompiledKernelData {
 public:
  CoverageFaultData(const LLVM::CompiledKernelData &source, const bool *fail)
      : LLVM::CompiledKernelData(source.arch(), source.get_internal_data()), fail_(fail) {}
  Err check() const override {
    const auto result = LLVM::CompiledKernelData::check();
    if (*fail_) allocation_lifetime::fail_at = allocation_lifetime::counts.attempts + 2;
    return result;
  }
 private:
  const bool *fail_;
};
class CoverageCompiler final : public KernelCompiler {
 public:
  CoverageCompiler(const CompiledKernelData &source, const bool *fail)
      : source_(dynamic_cast<const LLVM::CompiledKernelData &>(source)), fail_(fail) {}
  IRNodePtr compile(const CompileConfig &, const Kernel &) const override { return std::make_unique<Block>(); }
  CKDPtr compile(const CompileConfig &, const DeviceCapabilityConfig &, const Kernel &, IRNode &) const override {
    return std::make_unique<CoverageFaultData>(source_, fail_);
  }
 private:
  const LLVM::CompiledKernelData &source_;
  const bool *fail_;
};
void unknown_coverage(engine::Runtime &runtime, const CompiledKernelData &source) {
  char directory[] = "/tmp/hcp-removal-unknown-XXXXXX";
  check(mkdtemp(directory), "unknown fixture directory failed");
  {
    bool fail = false;
    KernelCompilationManager::Config setup; setup.offline_cache_path = directory;
    setup.kernel_compiler = std::make_unique<CoverageCompiler>(source, &fail);
    KernelCompilationManager owner(std::move(setup));
    auto &program = runtime.program();
    IRBuilder seed_body; Kernel seed(program, seed_body.extract_ir(), "seed");
    seed.finalize_params(); seed.finalize_rets();
    { auto guard = owner.borrow_or_compile(program.compile_config(), program.get_device_caps(), seed); }
    // Seed primes the map buckets. Short keys use no string allocation. Arm the
    // failure after LLVM verification: node succeeds, metadata request throws.
    IRBuilder body; Kernel definition(program, body.extract_ir(), "unknown");
    definition.finalize_params(); definition.finalize_rets();
    fail = true; bool failed = false;
    try { (void)owner.borrow_or_compile(program.compile_config(), program.get_device_caps(), definition); }
    catch (const std::bad_alloc &) { failed = true; }
    allocation_lifetime::fail_at = 0; fail = false;
    check(failed && owner.ownership_counts(2).pending_objects == 2,
          "unknown fixture did not fail after cache insertion");
    refuses(owner, "unknown", Status::UnknownBorrowers, 2);
    // Explicit reacquisition can establish coverage for a record never exposed
    // by the failed call. Inspection/removal must not create it implicitly.
    {
      auto guard = owner.borrow_or_compile(program.compile_config(), program.get_device_caps(), definition);
      check(guard.observation().tracked_borrows == 1 && !guard.observation().untracked_clients,
            "explicit retry did not establish coverage");
      refuses(owner, "unknown", Status::LiveBorrows, 2);
    }
    check(remove(owner, "unknown", 2).status == Status::Removed, "covered unknown-record retry did not remove");
  }
  check(std::filesystem::is_empty(directory) && std::filesystem::remove(directory), "unknown fixture created disk state");
  std::cout << "native_removal_unknown,post_insert_metadata_failure=1,unknown_refusal=1"
               ",inspection_creates_no_coverage=1,explicit_retry_coverage=1,covered_removal=1 PASS\n";
}
class MetadataData final : public CompiledKernelData {
 public:
  explicit MetadataData(taichi::Arch arch) : arch_(arch) {}
  taichi::Arch arch() const override { return arch_; }
  std::unique_ptr<CompiledKernelData> clone() const override { return std::make_unique<MetadataData>(arch_); }
 protected:
  Err load_impl(const CompiledKernelDataFile &) override { return Err::kUnknown; }
  Err dump_impl(CompiledKernelDataFile &) const override { return Err::kUnknown; }
 private:
  taichi::Arch arch_;
};
class MetadataCompiler final : public KernelCompiler {
 public:
  explicit MetadataCompiler(taichi::Arch arch) : arch_(arch) {}
  IRNodePtr compile(const CompileConfig &, const Kernel &) const override { return std::make_unique<Block>(); }
  CKDPtr compile(const CompileConfig &, const DeviceCapabilityConfig &, const Kernel &, IRNode &) const override {
    return std::make_unique<MetadataData>(arch_);
  }
 private:
  taichi::Arch arch_;
};
std::uint64_t file_hash(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary); check(bool(stream), "owned disk fixture file missing");
  std::uint64_t hash = 14695981039346656037ull;
  char byte;
  while (stream.get(byte)) { hash ^= static_cast<unsigned char>(byte); hash *= 1099511628211ull; }
  check(stream.eof(), "owned disk fixture read failed"); return hash;
}
void disk_and_unsupported(engine::Runtime &runtime) {
  auto &program = runtime.program();
  auto source = kernel(runtime, "remove_serial_source");
  const auto &data = Access::data(*source);
  char directory[] = "/tmp/hcp-removal-fixture-XXXXXX";
  check(mkdtemp(directory), "owned removal directory failed");
  const std::filesystem::path path(directory);
  KernelCompilationManager::Config empty_config; empty_config.offline_cache_path = directory;
  {
    KernelCompilationManager empty(std::move(empty_config));
    refuses(empty, "missing", Status::NotFound, 0);
    check(std::filesystem::is_empty(path), "empty removal created offline files");
  }
  const std::string loaded_key = "remove-loaded", unloaded_key = "remove-unloaded";
  const auto object_file = path / (loaded_key + ".tic");
  {
    std::ofstream stream(object_file, std::ios::binary);
    check(data.dump(stream) == CompiledKernelData::Err::kNoError && bool(stream), "fixture serialization failed");
  }
  CacheData metadata;
  metadata.version[0] = TI_VERSION_MAJOR; metadata.version[1] = TI_VERSION_MINOR; metadata.version[2] = TI_VERSION_PATCH;
  metadata.kernels[loaded_key].kernel_key = loaded_key; metadata.kernels[unloaded_key].kernel_key = unloaded_key;
  const auto metadata_file = path / KernelCompilationManager::kMetadataFilename;
  taichi::write_to_binary_file(metadata, metadata_file.string());
  const auto object_before = file_hash(object_file), metadata_before = file_hash(metadata_file);
  {
    KernelCompilationManager::Config setup; setup.offline_cache_path = directory;
    setup.kernel_compiler = std::make_unique<CloneCompiler>(data);
    KernelCompilationManager owner(std::move(setup));
    refuses(owner, unloaded_key, Status::DiskBacked, 2);
    refuses(owner, loaded_key, Status::Incomplete, 1);
    Kernel disk(program, std::function<void()>([] {}), "remove_disk_request");
    disk.set_kernel_key_for_cache(loaded_key);
    auto config = program.compile_config(); config.offline_cache = true;
    const auto &loaded = owner.load_or_compile(config, program.get_device_caps(), disk);
    check(!loaded.get_handle() && owner.ownership_counts(2).loaded_objects == 1, "fixture did not load exactly one object");
    refuses(owner, loaded_key, Status::DiskBacked, 2);
    // Same key in the pending map must not erase a disk row (updated_data_ holds
    // its address). A tracked Block request supplies otherwise positive coverage.
    IRBuilder body; Kernel shadow(program, body.extract_ir(), "remove_shadow");
    shadow.set_kernel_key_for_cache(loaded_key); shadow.finalize_params(); shadow.finalize_rets();
    {
      auto guard = owner.borrow_or_compile(program.compile_config(), program.get_device_caps(), shadow);
      check(&guard.get() != &loaded && !guard.observation().untracked_clients, "pending shadow did not get independent coverage");
    }
    refuses(owner, loaded_key, Status::DiskBacked, 3);
    Kernel pending_disk(program, std::function<void()>([] {}), "remove_pending_disk");
    pending_disk.set_kernel_key_for_cache("remove-pending-disk");
    (void)owner.load_or_compile(config, program.get_device_caps(), pending_disk);
    refuses(owner, "remove-pending-disk", Status::DiskBacked, 4);
    check(owner.owns_compiled_kernel(disk, loaded) && !loaded.get_handle() &&
              owner.ownership_counts(4).loaded_objects == 1,
          "disk refusal replaced the loaded object or metadata association");
  }
  check(file_hash(object_file) == object_before && file_hash(metadata_file) == metadata_before,
        "removal mutated fixture disk bytes");
  check(std::filesystem::remove(object_file) && std::filesystem::remove(metadata_file) &&
            std::filesystem::remove(path), "owned disk fixture cleanup failed");

  // Actual architecture and actual LLVM type are both required. These synthetic
  // metadata-only records exercise refusal without GPU compilation or execution.
  for (auto arch : {taichi::Arch::x64, taichi::Arch::cuda}) {
    char foreign_directory[] = "/tmp/hcp-removal-foreign-XXXXXX";
    check(mkdtemp(foreign_directory), "owned unsupported directory failed");
    {
      KernelCompilationManager::Config setup; setup.offline_cache_path = foreign_directory;
      setup.kernel_compiler = std::make_unique<MetadataCompiler>(arch);
      KernelCompilationManager owner(std::move(setup));
      IRBuilder body; Kernel definition(program, body.extract_ir(), "remove_foreign");
      definition.finalize_params(); definition.finalize_rets();
      auto config = program.compile_config(); config.arch = arch;
      if (arch == taichi::Arch::x64) {
        auto guard = owner.borrow_or_compile(config, program.get_device_caps(), definition);
        check(!guard.observation().untracked_clients, "foreign type did not have tracked provenance");
      } else { (void)owner.load_or_compile(config, program.get_device_caps(), definition); }
      refuses(owner, definition.get_cached_kernel_key(), Status::Unsupported, 1);
    }
    check(std::filesystem::is_empty(foreign_directory) && std::filesystem::remove(foreign_directory),
          "unsupported refusal created files");
  }
  std::cout << "native_removal_storage,disk_metadata_refusal=1,loaded_refusal=1,pending_disk_refusal=1"
               ",disk_shadow_refusal=1,disk_bytes_unchanged=1,foreign_arch_and_type_refusal=1,gpu_execution=0 PASS\n";
}
void run(int workers, bool prepared) {
  engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
  engine::Runtime runtime(settings); auto &program = runtime.program();
  check(runtime.arch() == taichi::Arch::x64, "removal fixture requires actual x64 CPU");
  auto &manager = program.get_program_impl()->get_kernel_compilation_manager();
  const auto &llvm = dynamic_cast<const LlvmProgramImpl &>(*program.get_program_impl());
  Ndarray a(&program, PrimitiveType::i32, {1}), b(&program, PrimitiveType::i32, {1});
  auto first = kernel(runtime, "remove_first"), sibling = kernel(runtime, "remove_sibling");
  launch(runtime, *first, a, 7, prepared); auto *first_module = latest_module;
  launch(runtime, *sibling, b, 11, prepared); auto *sibling_module = latest_module;
  const auto first_tasks = tasks(*first_module, *first), sibling_tasks = tasks(*sibling_module, *sibling);
  const int first_id = Access::data(*first).get_handle()->get_launch_id();
  const int sibling_id = Access::data(*sibling).get_handle()->get_launch_id();
  const auto jit_before = llvm.jit_ownership_counts(unlimited);
  const auto rows_before = program.get_program_impl()->launch_registry_storage().contexts;
  refuses(manager, "no_such_record", Status::NotFound);
  allocation_lifetime::begin(); allocation_lifetime::on_released = released;
  unknown_coverage(runtime, Access::data(*first));
  auto candidate = kernel(runtime, "remove_candidate");
  auto shared = kernel(runtime, "remove_candidate");
  const auto &data = Access::data(*candidate);
  const auto coverage = data.borrow_observation();
  check(&Access::data(*shared) == &data && coverage.tracked_borrows == 2 &&
            !coverage.untracked_clients && !data.get_handle() && !data.registration_started(),
        "unregistered shared candidate was not positively tracked");
  const auto key = Access::definition(*candidate).get_cached_kernel_key();
  watched_data = &data;
  for (const auto &entry : allocation_lifetime::table) if (entry.pointer) {
    const auto begin = reinterpret_cast<std::uintptr_t>(entry.pointer);
    const auto record = reinterpret_cast<std::uintptr_t>(coverage.record);
    if (record >= begin && record - begin < entry.bytes) watched_metadata = entry.pointer;
  }
  check(watched_metadata, "candidate metadata allocation was not observed");
  allocation_lifetime::capture_acquisitions = false;
  refuses(manager, key, Status::LiveBorrows);
  candidate.reset(); refuses(manager, key, Status::LiveBorrows);
  shared.reset();
  check(!data_released && !metadata_released, "last borrow removed cached data implicitly");
  const auto before = manager.ownership_counts(unlimited);
  refuses(manager, key, Status::Incomplete, before.visited_entries - 1);
  const auto tracked_before = allocation_lifetime::counts;
  const auto result = remove(manager, key, before.visited_entries);
  const auto tracked_after = allocation_lifetime::counts;
  check(result.status == Status::Removed && result.visited_entries == before.visited_entries &&
            data_released && metadata_released && tracked_after.live < tracked_before.live,
        "successful removal did not destroy the exact record/metadata");
  const auto after = manager.ownership_counts(unlimited);
  check(before.pending_objects == 3 && after.pending_objects == 2 &&
            after.pending_entries + 1 == before.pending_entries &&
            !after.loaded_objects && after.disk_metadata_entries == before.disk_metadata_entries &&
            same(jit_before, llvm.jit_ownership_counts(unlimited)) &&
            program.get_program_impl()->launch_registry_storage().contexts == rows_before,
        "single removal changed another native owner");
  refuses(manager, key, Status::NotFound);
  allocation_lifetime::end(); allocation_lifetime::on_released = nullptr;
  std::cout << "native_removal_effect,workers=" << workers << ",path=" << (prepared ? "prepared" : "dynamic")
            << ",pending_before=3,pending_after=2,exact_data_destroyed=1,metadata_destroyed=1"
            << ",cpp_requests_released=" << tracked_after.released - tracked_before.released
            << ",cpp_requested_bytes_released=" << tracked_before.live - tracked_after.live
            << ",removal_new_requests=0,jit_and_rows_unchanged=1 PASS\n";

  // A new lifetime may reuse an allocator address. Absence + destruction above,
  // not pointer inequality, proves the following request must compile afresh.
  auto retry = kernel(runtime, "remove_candidate");
  check(!Access::data(*retry).get_handle() && !Access::data(*retry).registration_started() &&
            Access::data(*retry).borrow_observation().tracked_borrows == 1 &&
            manager.ownership_counts(unlimited).pending_objects == 3,
        "removed key did not acquire a fresh unregistered lifetime");
  launch(runtime, *retry, a, 17, prepared);
  check(Access::data(*retry).get_handle()->get_launch_id() == int(rows_before),
        "fresh retry did not append a new launch row");
  retry.reset(); refuses(manager, key, Status::RegistrationStarted);
  check(tasks(*first_module, *first) == first_tasks && tasks(*sibling_module, *sibling) == sibling_tasks &&
            Access::data(*first).get_handle()->get_launch_id() == first_id &&
            Access::data(*sibling).get_handle()->get_launch_id() == sibling_id,
        "removal/retry changed surviving handles/tasks");
  launch(runtime, *first, a, 23, prepared); launch(runtime, *sibling, b, 29, prepared);
  auto raw_candidate = kernel(runtime, "remove_raw");
  const auto raw_key = Access::definition(*raw_candidate).get_cached_kernel_key();
  auto mismatch = program.compile_config(); mismatch.arch = taichi::Arch::cuda;
  const auto &raw = manager.load_or_compile(mismatch, program.get_device_caps(), Access::definition(*raw_candidate));
  check(&raw == &Access::data(*raw_candidate) && raw.borrow_observation().untracked_clients,
        "mismatched raw exposure lost provenance");
  raw_candidate.reset(); refuses(manager, raw_key, Status::UnknownBorrowers);
  raw_candidate = kernel(runtime, "remove_raw"); raw_candidate.reset();
  refuses(manager, raw_key, Status::UnknownBorrowers);

  auto partial = kernel(runtime, "remove_partial_registration");
  const auto partial_key = Access::definition(*partial).get_cached_kernel_key();
  const auto partial_jit = llvm.jit_ownership_counts(unlimited);
  const auto partial_rows = program.get_program_impl()->launch_registry_storage().contexts;
  bool failed = false;
  {
    auto invocation = partial->invocation(true); auto &ctx = invocation.get();
    ctx.set_arg_ndarray({0}, a); ctx.set_arg_int({1}, 99);
    fail_registration = true;
    try {
      if (prepared) partial->prepare(ctx); else partial->launch(ctx);
    } catch (const RegistrationFault &) { failed = true; }
    fail_registration = false;
  }
  check(failed && !Access::data(*partial).get_handle() && Access::data(*partial).registration_started() &&
            program.get_program_impl()->launch_registry_storage().contexts == partial_rows + 1 &&
            same(partial_jit, llvm.jit_ownership_counts(unlimited)),
        "fault did not leave a protected partial registration");
  partial.reset(); refuses(manager, partial_key, Status::RegistrationStarted);
  refuses(manager, "remove_first", Status::RegistrationStarted);
  first.reset(); refuses(manager, "remove_first", Status::RegistrationStarted);
  const auto jit_controls = llvm.jit_ownership_counts(unlimited);
  disk_and_unsupported(runtime);
  check(same(jit_controls, llvm.jit_ownership_counts(unlimited)), "disk/unsupported refusal created JIT resources");
  check(tasks(*sibling_module, *sibling) == sibling_tasks &&
            Access::data(*sibling).get_handle()->get_launch_id() == sibling_id,
        "refusal controls changed surviving sibling");
  launch(runtime, *sibling, b, 31, prepared);
  std::cout << "native_removal_controls,workers=" << workers << ",path=" << (prepared ? "prepared" : "dynamic")
            << ",live_shared_refusal=1,short_work_refusal=1,repeated_absence=1,fresh_retry=1"
            << ",raw_sticky_refusal=1,registered_zero_borrow_refusal=1,partial_registration_refusal=1"
            << ",surviving_handles_tasks=1,sibling_dispatch=1,registered_retirement=0 PASS\n";
  std::cout << "native_removal_layout,hcp_wrapper=" << sizeof(engine::CompiledKernel)
            << ",native_base=" << sizeof(CompiledKernelData) << ",native_llvm=" << sizeof(LLVM::CompiledKernelData)
            << ",guard=" << sizeof(CompiledKernelBorrow) << " PASS\n";
}
}  // namespace

taichi::lang::JITModule *removal_real_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *removal_wrap_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *removal_wrap_jit(taichi::lang::LlvmRuntimeExecutor *executor,
                                        std::unique_ptr<llvm::Module> module) {
  if (fail_registration) throw RegistrationFault{};
  latest_module = removal_real_jit(executor, std::move(module)); return latest_module;
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "expected workers and prepared|dynamic");
    const int workers = std::stoi(argv[1]); const std::string path(argv[2]);
    check((workers == 1 || workers == 2) && (path == "prepared" || path == "dynamic"), "unsupported mode");
    run(workers, path == "prepared"); return 0;
  } catch (const std::exception &e) { std::cerr << "unregistered removal: " << e.what() << '\n'; return 1; }
}
