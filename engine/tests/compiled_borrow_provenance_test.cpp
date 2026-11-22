#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

#include <cstdlib>
#include <filesystem>
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
using Access=engine::testing::CompiledKernelAccess;
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
engine::CompiledKernel kernel(engine::Runtime &runtime,const char *name) {
  IRBuilder body;
  auto *array=body.create_ndarray_arg_load({0},PrimitiveType::i32,1,0);
  auto *value=body.create_arg_load({1},PrimitiveType::i32,false,0);
  body.create_global_store(body.create_external_ptr(array,{body.get_int32(0)}),value);
  return engine::CompiledKernel(runtime,name,body.extract_ir(),
      {engine::Param::array(PrimitiveType::i32),engine::Param::scalar(PrimitiveType::i32)});
}
void launch(engine::Runtime &runtime,engine::CompiledKernel &k,Ndarray &array,int value) {
  auto call=k.invocation(true); auto &ctx=call.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0},array);
  engine::argument_int(ctx,1,value); k.launch(ctx); runtime.synchronize();
  int actual=0; engine::readback(array,&actual,sizeof(actual));
  check(actual==value,"provenance request changed surviving dispatch");
}
// Metadata-only unsupported record: no GPU code generation, registration or
// driver operation. It seeds a real native manager's existing memory-cache leg.
class ForeignData final : public CompiledKernelData {
 public:
  taichi::Arch arch() const override { return taichi::Arch::cuda; }
  std::unique_ptr<CompiledKernelData> clone() const override { return std::make_unique<ForeignData>(); }
 protected:
  Err load_impl(const CompiledKernelDataFile &) override { return Err::kUnknown; }
  Err dump_impl(CompiledKernelDataFile &) const override { return Err::kUnknown; }
};
class ForeignCompiler final : public KernelCompiler {
 public:
  explicit ForeignCompiler(int *calls):calls_(calls) {}
  IRNodePtr compile(const CompileConfig &,const Kernel &) const override { return std::make_unique<Block>(); }
  CKDPtr compile(const CompileConfig &,const DeviceCapabilityConfig &,const Kernel &,IRNode &) const override {
    ++*calls_; return std::make_unique<ForeignData>();
  }
 private:
  int *calls_;
};
bool unsupported_record(Program &program) {
  char directory[]="/tmp/hcp-borrow-provenance-XXXXXX";
  check(mkdtemp(directory),"provenance fixture directory failed");
  bool rejected=false;
  {
    int compile_calls=0;
    KernelCompilationManager::Config setup;
    setup.offline_cache_path=directory;
    setup.kernel_compiler=std::make_unique<ForeignCompiler>(&compile_calls);
    KernelCompilationManager manager(std::move(setup));
    IRBuilder body;
    Kernel definition(program,body.extract_ir(),"foreign_record");
    definition.finalize_params(); definition.finalize_rets();
    auto config=program.compile_config(); config.arch=taichi::Arch::cuda;
    const auto &foreign=manager.load_or_compile(config,program.get_device_caps(),definition);
    check(foreign.arch()==taichi::Arch::cuda && compile_calls==1,"foreign fixture did not seed its record");
    config.arch=taichi::Arch::x64;
    try { (void)manager.borrow_or_compile(config,program.get_device_caps(),definition); }
    catch (const std::invalid_argument &) { rejected=true; }
    check(compile_calls==1,"tracked mismatch bypassed the existing cached record");
    if (rejected) {
      const auto view=foreign.borrow_observation();
      check(view.status==OwnershipCountStatus::Unavailable && view.untracked_clients &&
                !view.tracked_borrows && !view.reclamation_supported,
            "unsupported tracked request created borrow metadata");
    }
    std::cout << "native_tracked_arch_provenance,actual_unsupported_rejected=" << rejected
              << ",cache_hit_compile_calls=" << compile_calls << ",gpu_execution=0 "
              << (rejected ? "PASS" : "FAIL") << '\n';
  }
  check(std::filesystem::is_empty(directory) && std::filesystem::remove(directory),
        "owned provenance fixture cleanup failed");
  return rejected;
}
void run() {
  engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=1;
  engine::Runtime runtime(settings); auto &program=runtime.program();
  check(runtime.arch()==taichi::Arch::x64,"provenance fixture requires actual x64 CPU");
  Ndarray a(&program,PrimitiveType::i32,{1}),b(&program,PrimitiveType::i32,{1});
  auto first=kernel(runtime,"arch_primary"),sibling=kernel(runtime,"arch_sibling");
  launch(runtime,first,a,7); launch(runtime,sibling,b,11);
  const auto &data=Access::data(first);
  const auto before=data.borrow_observation();
  check(!before.untracked_clients && before.tracked_borrows==1,"tracked seed was already raw");
  auto &manager=program.get_program_impl()->get_kernel_compilation_manager();
  constexpr auto unlimited=std::numeric_limits<std::size_t>::max();
  const auto counts=manager.ownership_counts(unlimited);
  const auto &llvm=dynamic_cast<const LlvmProgramImpl &>(*program.get_program_impl());
  const auto modules=llvm.jit_ownership_counts(unlimited);
  auto config=program.compile_config(); config.arch=taichi::Arch::cuda;
  const auto &raw=manager.load_or_compile(config,program.get_device_caps(),Access::definition(first));
  check(&raw==&data && raw.arch()==taichi::Arch::x64,"mismatched request did not return its actual cached x64 record");
  const auto observed=raw.borrow_observation();
  const bool marked=observed.untracked_clients;
  check(observed.record==before.record && observed.tracked_borrows==1,"raw mismatch changed tracked ownership");
  {
    auto tracked=manager.borrow_or_compile(program.compile_config(),program.get_device_caps(),Access::definition(first));
    check(&tracked.get()==&data && tracked.get().arch()==taichi::Arch::x64 &&
              tracked.observation().tracked_borrows==2 && tracked.observation().untracked_clients==marked,
          "supported hit changed actual record or cleared raw provenance");
  }
  const auto after=manager.ownership_counts(unlimited);
  const auto jit_after=llvm.jit_ownership_counts(unlimited);
  check(counts.status==OwnershipCountStatus::Present && after.status==OwnershipCountStatus::Present &&
            counts.pending_objects==after.pending_objects && counts.loaded_objects==after.loaded_objects &&
            counts.pending_entries==after.pending_entries && counts.disk_metadata_entries==after.disk_metadata_entries &&
            modules.status==OwnershipCountStatus::Present && jit_after.status==OwnershipCountStatus::Present &&
            modules.modules==jit_after.modules && modules.runtime_modules==jit_after.runtime_modules &&
            modules.kernel_modules==jit_after.kernel_modules,"mismatched raw hit materialized native state");
  launch(runtime,first,a,17); launch(runtime,sibling,b,19);
  std::cout << "native_raw_arch_provenance,same_x64_record=1,mismatched_config_marks_raw=" << marked
            << ",tracked_reacquire_preserves_marker=1,cache_and_jit_unchanged=1,sibling_dispatch=1,gpu_execution=0 "
            << (marked ? "PASS" : "FAIL") << '\n';
  const bool rejected=unsupported_record(program);
  check(marked && rejected,"actual compiled-record provenance check failed");
}
}  // namespace
int main() {
  try { run(); return 0; }
  catch (const std::exception &e) { std::cerr << "compiled borrow provenance: " << e.what() << '\n'; return 1; }
}
