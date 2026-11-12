#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/rhi/cpu/cpu_device.h"
#include <iostream>
#include <stdexcept>

using namespace taichi::lang;
namespace {
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
engine::CompiledKernel kernel(engine::Runtime &runtime) {
  IRBuilder ir;
  auto *a = ir.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *b = ir.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0);
  auto *n = ir.create_arg_load({2}, PrimitiveType::i32, false, 0);
  ir.create_global_store(ir.create_external_ptr(a, {ir.get_int32(0)}),
      ir.create_add(ir.create_global_load(ir.create_external_ptr(b, {ir.get_int32(0)})), n));
  return engine::CompiledKernel(runtime, "prepared_backing", ir.extract_ir(),
      {engine::Param::array(PrimitiveType::i32), engine::Param::array(PrimitiveType::i32),
       engine::Param::scalar(PrimitiveType::i32)});
}
void run() {
  engine::InstanceSettings settings;
  settings.cpu_threads = 1;
  engine::Runtime runtime(settings);
  auto &device = *runtime.program().get_compute_device();
  cpu::CpuDevice foreign;
  auto compiled = kernel(runtime);
  auto context = compiled.make_context();
  require(!context.prepared_backing(device).buffers_available,
          "unprepared context claimed backing coverage");
  auto owned = std::make_unique<Ndarray>(&runtime.program(), PrimitiveType::i32,
                                       std::vector<int>{4});
  auto allocation = owned->get_device_allocation();
  Ndarray part(allocation, PrimitiveType::i32, {2});
  int values[4]{3, 0, 0, 0};
  engine::upload(*owned, values, sizeof(values));
  context.set_arg_ndarray({0}, *owned);
  context.set_arg_ndarray({1}, part);
  context.set_arg_int({2}, 7);
  compiled.prepare(context);
  const auto view = context.prepared_backing(device);
  const auto a = context.prepared_array_record(device, 0);
  const auto b = context.prepared_array_record(device, 1);
  require(view.buffers_available && view.array_bindings_complete &&
              view.device_buffers_complete && !view.host_metadata_complete &&
              view.arguments.data && view.arguments.requested_bytes == context.arg_buffer_size &&
              view.results.requested_bytes == context.result_buffer_size &&
              view.runtime_context.data && view.runtime_context.requested_bytes &&
              view.context_object_bytes == sizeof(LaunchContextBuilder) &&
              view.prepared_object_bytes == sizeof(LaunchContextBuilder::Prepared) &&
              view.array_binding_capacity_bytes ==
                  context.array_bindings.capacity() * sizeof(LaunchContextBuilder::ArrayBinding) &&
              view.incremental_record_capacity_bytes ==
                  context.array_bindings.capacity() * sizeof(AllocationRecord),
          "prepared buffer/metadata coverage incorrect");
  require(a.status == AllocationRecordStatus::Live && a.token == b.token &&
              a.requested_bytes == sizeof(values) && b.extent_bytes == sizeof(values),
          "prepared views lost captured backing identity/extent");
  require(!context.prepared_backing(foreign).buffers_available &&
              context.prepared_array_record(foreign, 0).status == AllocationRecordStatus::Unknown &&
              context.prepared_array_record(device, 2).status == AllocationRecordStatus::Unknown &&
              context.prepared_array_record(device, 99).status == AllocationRecordStatus::Unknown,
          "foreign/scalar/out-of-range query accepted");
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const auto repeated = context.prepared_backing(device);
  const auto repeated_record = context.prepared_array_record(device, 0);
  allocation_observation::enabled = false;
  require(!allocation_observation::calls && repeated.arguments.data == view.arguments.data &&
              repeated_record.token == a.token, "inspection allocated or changed identity");
  compiled.launch(context);
  runtime.synchronize();
  engine::readback(*owned, values, sizeof(values));
  require(values[0] == 10, "prepared execution changed");
  auto moved = std::move(context);
  require(!context.prepared_backing(device).buffers_available &&
              moved.prepared_backing(device).arguments.data == view.arguments.data,
          "move lost owner buffers or left source coverage live");
  moved.set_prepared_int(2, 9);
  compiled.launch(moved);
  runtime.synchronize();
  engine::readback(*owned, values, sizeof(values));
  require(values[0] == 19, "moved prepared execution changed");

  const auto owner_token = owned->backing_record(device).token;
  DeviceAllocation proven_import;
  require(device.import_owned_memory(owner_token, sizeof(int), 2*sizeof(int), &proven_import) ==
              RhiResult::success, "owned import creation failed");
  Ndarray proven_view(proven_import, PrimitiveType::i32, {2});
  Ndarray oversized_view(proven_import, PrimitiveType::i32, {4});
  auto oversized = compiled.make_context();
  oversized.set_arg_ndarray({0},*owned);
  oversized.set_arg_ndarray({1},oversized_view);
  oversized.set_arg_int({2},0);
  compiled.prepare(oversized);  // No launch: the view exceeds its registration.
  require(!oversized.prepared_backing(device).array_bindings_complete &&
              oversized.prepared_array_backing_record(device,1).status == AllocationRecordStatus::Unknown,
          "full owner extent hid an oversized import view");
  auto proven = compiled.make_context();
  proven.set_arg_ndarray({0}, *owned);
  proven.set_arg_ndarray({1}, proven_view);
  proven.set_arg_int({2}, 2);
  compiled.prepare(proven);
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const auto proven_backing = proven.prepared_array_backing_record(device, 1);
  const auto proven_summary = proven.prepared_backing(device);
  allocation_observation::enabled = false;
  const auto proven_registration = proven.prepared_array_record(device, 1);
  require(!allocation_observation::calls && proven_summary.array_bindings_complete &&
              !proven_summary.host_metadata_complete &&
              proven_backing.token == owner_token && proven_backing.extent_bytes == sizeof(values) &&
              proven_registration.ownership == AllocationOwnership::Imported &&
              proven_registration.extent_bytes == 2*sizeof(int) &&
              !(proven_registration.token == proven_backing.token),
          "prepared import conflated registration with canonical backing or allocated");
  require(proven.prepared_array_backing_record(foreign,1).status == AllocationRecordStatus::Unknown &&
              proven.prepared_array_backing_record(device,2).status == AllocationRecordStatus::Unknown &&
              proven.prepared_array_backing_record(device,99).status == AllocationRecordStatus::Unknown,
          "canonical query accepted foreign/scalar/out-of-range binding");
  values[1] = 5;
  engine::upload(*owned, values, sizeof(values));
  compiled.launch(proven);
  runtime.synchronize();
  engine::readback(*owned, values, sizeof(values));
  require(values[0] == 7, "prepared partial import used wrong offset");
  auto proven_moved = std::move(proven);
  require(!proven.prepared_backing(device).buffers_available &&
              proven_moved.prepared_array_backing_record(device,1).token == owner_token,
          "move lost canonical backing or retained source coverage");
  proven_moved.set_prepared_int(2,3);
  compiled.launch(proven_moved);
  runtime.synchronize();
  engine::readback(*owned, values, sizeof(values));
  require(values[0] == 8, "moved partial import execution changed");
  device.dealloc_memory(proven_import);
  require(!proven_moved.prepared_backing(device).array_bindings_complete &&
              proven_moved.prepared_array_record(device,1).status == AllocationRecordStatus::Retired &&
              proven_moved.prepared_array_backing_record(device,1).status == AllocationRecordStatus::Unknown &&
              moved.prepared_backing(device).array_bindings_complete,
          "retired import still certified backing or retired owner");
  DeviceAllocation remaining_import;
  require(device.import_owned_memory(owner_token,0,sizeof(values),&remaining_import) ==
              RhiResult::success, "remaining import failed");
  Ndarray remaining_view(remaining_import,PrimitiveType::i32,{4});
  auto remaining = compiled.make_context();
  remaining.set_arg_ndarray({0},remaining_view);
  remaining.set_arg_ndarray({1},part);
  remaining.set_arg_int({2},0);
  compiled.prepare(remaining);

  auto external = compiled.make_context();
  external.set_arg_ndarray({0}, *owned);
  external.set_arg_external_array_with_shape({0}, reinterpret_cast<uintptr_t>(values),
                                             sizeof(values), {4});
  external.set_arg_ndarray({1}, part);
  external.set_arg_int({2}, 0);
  compiled.prepare(external);
  require(external.prepared_backing(device).buffers_available &&
              !external.prepared_backing(device).array_bindings_complete &&
              external.prepared_array_backing_record(device, 0).status == AllocationRecordStatus::Unavailable &&
              external.prepared_array_record(device, 0).status == AllocationRecordStatus::Unavailable,
          "external rebind retained prior owned record");

  // Explicit CPU construction/cast is confined to this CPU test fixture.
  auto &cpu = static_cast<taichi::lang::cpu::CpuDevice &>(device);
  auto imported = cpu.import_memory(cpu.get_alloc_info(allocation).ptr, sizeof(values));
  Ndarray imported_view(imported, PrimitiveType::i32, {4});
  auto import_context = compiled.make_context();
  import_context.set_arg_ndarray({0}, imported_view);
  import_context.set_arg_ndarray({1}, part);
  import_context.set_arg_int({2}, 0);
  compiled.prepare(import_context);
  require(!import_context.prepared_backing(device).array_bindings_complete &&
              import_context.prepared_array_backing_record(device, 0).status == AllocationRecordStatus::Unknown &&
              import_context.prepared_array_record(device, 0).ownership == AllocationOwnership::Imported,
          "import registration certified owned backing");
  cpu.dealloc_memory(imported);
  require(import_context.prepared_array_record(device, 0).status == AllocationRecordStatus::Retired &&
              moved.prepared_backing(device).array_bindings_complete,
          "import retirement affected original owner");
  owned.reset();
  require(remaining.prepared_array_record(device,0).status == AllocationRecordStatus::Live &&
              remaining.prepared_array_backing_record(device,0).status == AllocationRecordStatus::Unknown &&
              !remaining.prepared_backing(device).array_bindings_complete,
          "live registration certified retired backing");
  device.dealloc_memory(remaining_import);
  require(!moved.prepared_backing(device).array_bindings_complete &&
              moved.prepared_array_record(device, 0).status == AllocationRecordStatus::Retired,
          "prepared binding recaptured released array");
  moved.prepared->usable = false;
  require(!moved.prepared_backing(device).buffers_available,
          "unusable invocation certified coverage");
  auto unsupported = compiled.make_context();
  unsupported.prepared = std::make_unique<LaunchContextBuilder::Prepared>();
  require(!unsupported.prepared_backing(device).buffers_available &&
              unsupported.prepared_array_record(device, 0).status == AllocationRecordStatus::Unavailable,
          "default unsupported prepared state claimed coverage");
  std::cout << "argument_request=" << view.arguments.requested_bytes
            << " result_request=" << view.results.requested_bytes
            << " record_capacity=" << view.incremental_record_capacity_bytes << '\n';
}
}
int main() {
  try { run(); std::cout << "prepared_backing_test PASS\n"; return 0; }
  catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
