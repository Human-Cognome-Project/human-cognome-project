#include "engine/runtime.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace taichi::lang;
void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
class NoCompile final : public KernelCompiler {
 public:
  IRNodePtr compile(const CompileConfig &, const Kernel &) const override {
    throw std::runtime_error("count fixture unexpectedly compiled");
  }
  CKDPtr compile(const CompileConfig &, const DeviceCapabilityConfig &,
                const Kernel &, IRNode &) const override {
    throw std::runtime_error("count fixture unexpectedly generated code");
  }
};
void run() {
  engine::InstanceSettings settings; settings.read_environment = false;
  settings.cpu_threads = 1;
  engine::Runtime runtime(settings);
  auto &program = runtime.program();
  check(runtime.arch() == taichi::Arch::x64, "CPU count fixture required");
  const auto &impl = dynamic_cast<const LlvmProgramImpl &>(*program.get_program_impl());
  const auto before = impl.jit_ownership_counts(std::numeric_limits<std::size_t>::max());
  check(before.status == OwnershipCountStatus::Present && before.runtime_modules == 1 &&
            !before.kernel_modules, "fresh runtime module classification failed");

  IRBuilder builder;
  builder.create_return(builder.get_int32(7));
  Kernel definition(program, builder.extract_ir(), "count_fixture_serial_source");
  definition.insert_ret(PrimitiveType::i32);
  definition.finalize_params(); definition.finalize_rets();
  const auto &data = program.compile_kernel(program.compile_config(), program.get_device_caps(), definition);

  char directory[] = "/tmp/hcp-loaded-counts-XXXXXX";
  check(mkdtemp(directory), "loaded count fixture directory failed");
  const std::filesystem::path path(directory);
  const std::string key = "count-fixture-loaded";
  const auto object_file = path / (key + ".tic");
  {
    std::ofstream stream(object_file, std::ios::binary);
    check(data.dump(stream) == CompiledKernelData::Err::kNoError && bool(stream),
          "compiled fixture serialization failed");
  }
  CacheData metadata;
  metadata.version[0] = TI_VERSION_MAJOR;
  metadata.version[1] = TI_VERSION_MINOR;
  metadata.version[2] = TI_VERSION_PATCH;
  metadata.kernels[key].kernel_key = key;
  metadata.kernels["count-fixture-unloaded"].kernel_key = "count-fixture-unloaded";
  const auto metadata_file = path / KernelCompilationManager::kMetadataFilename;
  taichi::write_to_binary_file(metadata, metadata_file.string());
  {
    KernelCompilationManager::Config config;
    config.offline_cache_path = path.string();
    config.kernel_compiler = std::make_unique<NoCompile>();
    KernelCompilationManager owner(std::move(config));
    const auto unloaded = owner.ownership_counts(2);
    check(unloaded.status == OwnershipCountStatus::Present && !unloaded.complete &&
              unloaded.disk_metadata_entries == 2 && !unloaded.loaded_objects,
          "count query loaded a disk object");
    // This explicit load is outside observation. An AST definition selects the
    // existing disk-cache path; the supplied fixture key avoids unrelated hashing.
    Kernel disk_definition(program, std::function<void()>([] {}), "count_fixture_disk_request");
    disk_definition.set_kernel_key_for_cache(key);
    auto compile_config = program.compile_config(); compile_config.offline_cache = true;
    const auto &loaded = owner.load_or_compile(compile_config, program.get_device_caps(), disk_definition);
    const auto exact = owner.ownership_counts(2);
    const auto short_work = owner.ownership_counts(1);
    const auto repeat = owner.ownership_counts(2);
    check(owner.owns_compiled_kernel(disk_definition, loaded) &&
              exact.status == OwnershipCountStatus::Present && !exact.complete && exact.owner == &owner &&
              !exact.pending_entries && !exact.pending_objects && exact.disk_metadata_entries == 2 &&
              exact.loaded_objects == 1 && exact.visited_entries == 2 &&
              short_work.status == OwnershipCountStatus::Incomplete && !short_work.visited_entries &&
              !short_work.loaded_objects && repeat.loaded_objects == 1 && repeat.owner == exact.owner,
          "loaded object counts or short-work behavior failed");
    const auto &same = owner.load_or_compile(compile_config, program.get_device_caps(), disk_definition);
    check(&same == &loaded && !loaded.get_handle(), "count query registered or replaced loaded data");
  }
  const auto after = impl.jit_ownership_counts(before.modules);
  check(after.status == OwnershipCountStatus::Present && after.owner == before.owner &&
            after.modules == before.modules && !after.kernel_modules,
        "disk count fixture created an executable module");
  check(std::filesystem::remove(object_file) && std::filesystem::remove(metadata_file) &&
            std::filesystem::remove(path), "owned loaded fixture cleanup failed");
  std::cout << "native_loaded_counts,metadata_entries=2,loaded_before=0,loaded_after_explicit_load=1"
               ",short_work_incomplete=1,repeat_unchanged=1,compile_on_load=0,new_jit_modules=0 PASS\n";
}
}  // namespace
int main() {
  try { run(); return 0; }
  catch (const std::exception &e) { std::cerr << "native loaded counts: " << e.what() << '\n'; return 1; }
}
