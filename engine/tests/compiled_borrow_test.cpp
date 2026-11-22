#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &definition(const CompiledKernel &k) { return *k.kernel_; }
  static const taichi::lang::CompiledKernelData &data(const CompiledKernel &k) { return k.compiled_data(); }
  static auto borrow(const CompiledKernel &k) { return k.borrow_.observation(); }
  static const void *context(const CompiledKernel &k) { return k.bound_context_.get(); }
};
}  // namespace engine::testing

namespace {
using namespace taichi::lang;
using Access=engine::testing::CompiledKernelAccess;
void check(bool ok, const char *what) { if (!ok) throw std::runtime_error(what); }
struct Refused {};
bool fail_acquired=false;
const CompiledKernelData *failed_data=nullptr;
const void *watched_definition=nullptr, *watched_context=nullptr, *watched_control=nullptr;
const CompiledKernelData *watched_data=nullptr;
bool definition_order=false, context_order=false, control_released=false;
void released(void *p, std::size_t) noexcept {
  if (p==watched_control) control_released=true;
  if (p==watched_context)
    context_order=watched_data->borrow_observation().tracked_borrows==2 && !definition_order;
  if (p==watched_definition)
    definition_order=watched_data->borrow_observation().tracked_borrows==2 && context_order;
}
template<class F> void refused(F f) {
  try { f(); } catch (const Refused &) { return; }
  throw std::runtime_error("expected injected constructor/refusal unwind");
}
template<class F> void rejects(F f) {
  try { f(); } catch (const std::logic_error &) { return; }
  throw std::runtime_error("empty/expired/unsupported borrow accepted");
}
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &r, const char *name) {
  IRBuilder b;
  auto *array=b.create_ndarray_arg_load({0},PrimitiveType::i32,1,0);
  auto *value=b.create_arg_load({1},PrimitiveType::i32,false,0);
  b.create_global_store(b.create_external_ptr(array,{b.get_int32(0)}),value);
  return std::make_unique<engine::CompiledKernel>(r,name,b.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32),engine::Param::scalar(PrimitiveType::i32)});
}
void launch(engine::Runtime &r,engine::CompiledKernel &k,Ndarray &array,int value) {
  auto call=k.invocation(true); auto &ctx=call.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0},array);
  engine::argument_int(ctx,1,value); k.launch(ctx); r.synchronize();
  int actual=0; engine::readback(array,&actual,sizeof(actual));
  check(actual==value,"surviving prepared dispatch changed");
}
std::size_t count(const CompiledKernelData &data) { return data.borrow_observation().tracked_borrows; }
void run() {
  static_assert(!std::is_copy_constructible_v<CompiledKernelBorrow>);
  static_assert(std::is_nothrow_move_constructible_v<CompiledKernelBorrow>);
  static_assert(std::is_nothrow_move_assignable_v<engine::CompiledKernel>);
  CompiledKernelBorrow escaped;
  std::size_t control_request=0;
  {
    engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=1;
    engine::Runtime runtime(settings); auto &p=runtime.program();
    Ndarray a(&p,PrimitiveType::i32,{1}), b(&p,PrimitiveType::i32,{1});
    allocation_lifetime::begin(); allocation_lifetime::on_released=released;
    auto first=kernel(runtime,"borrow_first");
    launch(runtime,*first,a,7);
    const auto &original=Access::data(*first);
    const auto initial=original.borrow_observation();
    check(initial.status==OwnershipCountStatus::Present &&
              initial.owner==&p.get_program_impl()->get_kernel_compilation_manager() &&
              initial.tracked_borrows==1 &&
              !initial.untracked_clients && !initial.reclamation_supported,"first native borrow missing");
    watched_definition=&Access::definition(*first); watched_context=Access::context(*first);
    watched_data=&original;
    for (const auto &entry:allocation_lifetime::table) if (entry.pointer) {
      const auto begin=reinterpret_cast<std::uintptr_t>(entry.pointer);
      const auto record=reinterpret_cast<std::uintptr_t>(initial.record);
      if (record>=begin && record-begin<entry.bytes) {
        watched_control=entry.pointer; control_request=entry.bytes;
      }
    }
    check(watched_control && control_request,"native control allocation not observed");
    allocation_lifetime::capture_acquisitions=false;
    refused([&] {
      auto candidate=kernel(runtime,"borrow_first");
      check(&Access::data(*candidate)==&original && count(original)==2,"shared borrow acquisition missing");
      throw Refused{};
    });
    check(count(original)==1,"refused shared borrower survived unwind");
    auto shared=kernel(runtime,"borrow_first"); launch(runtime,*shared,a,11);
    check(count(original)==2,"successful shared retry missing");
    auto other=kernel(runtime,"borrow_other"); launch(runtime,*other,b,13);
    const auto &different=Access::data(*other);
    engine::CompiledKernel moved(std::move(*other));
    check(!Access::borrow(*other).record && count(different)==1,"move construction duplicated a borrow");
    *first=std::move(moved);
    check(definition_order && context_order && count(original)==1 && count(different)==1 &&
              !Access::borrow(moved).record,"move assignment released borrow before dependencies");
    auto *same_first=first.get(); *first=std::move(*same_first);
    check(count(different)==1,"self move changed native borrow count");
    rejects([&] { (void)moved.make_context(); });
    launch(runtime,*shared,a,17); launch(runtime,*first,b,19);

    // Measure whole native API hits separately from guard-only operations.
    const auto caps=p.get_device_caps();
    allocation_observation::calls=0; allocation_observation::bytes=0;
    allocation_observation::enabled=true;
    auto extra=p.compile_kernel_borrow(p.compile_config(),caps,Access::definition(*shared));
    allocation_observation::enabled=false;
    const auto hit_calls=allocation_observation::calls.load();
    const auto hit_bytes=allocation_observation::bytes.load();
    allocation_observation::calls=0; allocation_observation::bytes=0;
    allocation_observation::enabled=true;
    auto transfer=std::move(extra);
    auto *same_transfer=&transfer; transfer=std::move(*same_transfer);
    const auto repeated=transfer.observation();
    transfer=CompiledKernelBorrow{};
    allocation_observation::enabled=false;
    check(!extra.has_record() && repeated.record==initial.record &&
              repeated.tracked_borrows==2 && count(original)==1 &&
              !allocation_observation::calls && !allocation_observation::bytes,
          "guard move/observation/release allocated or changed counts");
    std::cout << "native_borrow_cache_hit,api_calls=" << hit_calls << ",api_bytes=" << hit_bytes
              << ",guard_move_observe_release_calls=0,record_unchanged="
              << (repeated.record==initial.record) << " PASS\n";
    rejects([&] { (void)extra.get(); });

    fail_acquired=true;
    refused([&] { auto failed=kernel(runtime,"borrow_failure"); });
    check(failed_data && !count(*failed_data) && !failed_data->borrow_observation().untracked_clients,
          "post-acquisition constructor unwind leaked a borrow");
    {
      auto retry=kernel(runtime,"borrow_failure");
      check(&Access::data(*retry)==failed_data && count(*failed_data)==1,"constructor retry did not reuse native record");
      launch(runtime,*retry,b,23);
    }
    check(!count(*failed_data),"successful retry release missing");
    refused([&] {
      struct Owner {
        std::unique_ptr<engine::CompiledKernel> k;
        explicit Owner(engine::Runtime &r):k(kernel(r,"borrow_first")) { throw Refused{}; }
      } owner(runtime);
    });
    check(count(original)==1,"enclosing constructor unwind leaked HCP borrow");
    launch(runtime,*shared,a,29);

    const auto &raw=p.compile_kernel(p.compile_config(),p.get_device_caps(),Access::definition(*shared));
    check(&raw==&original && raw.borrow_observation().untracked_clients && count(raw)==1,
          "raw Program reference was not marked permanently untracked");
    allocation_observation::calls=0; allocation_observation::bytes=0;
    allocation_observation::enabled=true;
    (void)p.compile_kernel(p.compile_config(),caps,Access::definition(*shared));
    allocation_observation::enabled=false;
    std::cout << "native_raw_cache_hit,api_calls=" << allocation_observation::calls.load()
              << ",api_bytes=" << allocation_observation::bytes.load() << " PASS\n";
    auto &manager=p.get_program_impl()->get_kernel_compilation_manager();
    (void)manager.load_or_compile(p.compile_config(),p.get_device_caps(),Access::definition(*first));
    check(different.borrow_observation().untracked_clients,"direct raw manager client was not marked");
    auto clone=raw.clone();
    check(clone->borrow_observation().status==OwnershipCountStatus::Unavailable &&
              clone->borrow_observation().untracked_clients && !clone->borrow_observation().reclamation_supported,
          "unmanaged cloned data appeared reclaimable");
    auto config=p.compile_config(); config.arch=taichi::Arch::cuda;
    rejects([&] { (void)p.compile_kernel_borrow(config,p.get_device_caps(),Access::definition(*shared)); });
    escaped=p.compile_kernel_borrow(p.compile_config(),p.get_device_caps(),Access::definition(*shared));
    check(escaped.observation().untracked_clients && count(original)==2,"tracked retry cleared raw-client marker");
    shared.reset(); first.reset(); other.reset();
    check(count(original)==1 && !count(different),"HCP teardown did not release exactly one guard");
  }
  check(escaped.has_record() && escaped.observation().status==OwnershipCountStatus::Absent &&
            !escaped.observation().owner && escaped.observation().tracked_borrows==1 && !control_released,
        "expired guard metadata was freed with native data");
  rejects([&] { (void)escaped.get(); });
  escaped=CompiledKernelBorrow{};
  check(control_released,"final expired guard did not release its metadata");
  allocation_lifetime::on_released=nullptr;
  (void)allocation_lifetime::end();
  std::cout << "native_borrow_controls,shared_acquire_release=1,move_order=1,constructor_unwind=1"
               ",retry_sibling_dispatch=1,raw_clients_ineligible=1,expired_guard_checked=1"
               ",guard_move_observe_release_calls=0,reclamation_supported=0 PASS\n";
  std::cout << "native_borrow_overhead,hcp_wrapper=" << sizeof(engine::CompiledKernel)
            << ",native_base=" << sizeof(CompiledKernelData)
            << ",native_llvm=" << sizeof(LLVM::CompiledKernelData)
            << ",guard=" << sizeof(CompiledKernelBorrow)
            << ",control_request=" << control_request << " PASS\n";
}
}  // namespace
CompiledKernelBorrow borrow_real_compile(Program *,const CompileConfig &,const DeviceCapabilityConfig &,const Kernel &)
    __asm__("__real__ZN6taichi4lang7Program21compile_kernel_borrowERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
CompiledKernelBorrow borrow_wrap_compile(Program *,const CompileConfig &,const DeviceCapabilityConfig &,const Kernel &)
    __asm__("__wrap__ZN6taichi4lang7Program21compile_kernel_borrowERKNS0_13CompileConfigERKNS0_22DeviceCapabilityConfigERKNS0_6KernelE");
CompiledKernelBorrow borrow_wrap_compile(Program *p,const CompileConfig &c,const DeviceCapabilityConfig &caps,const Kernel &k) {
  auto result=borrow_real_compile(p,c,caps,k);
  if (fail_acquired) {
    fail_acquired=false; failed_data=&result.get();
    check(failed_data->borrow_observation().tracked_borrows==1,"fault did not follow acquisition");
    throw Refused{};
  }
  return result;
}
int main() {
  try { run(); return 0; }
  catch (const std::exception &e) { std::cerr << "compiled borrow: " << e.what() << '\n'; return 1; }
}
