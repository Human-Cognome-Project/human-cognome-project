#include "engine/record_map.h"

#include <array>
#include <atomic>
#include <limits>
#include <stdexcept>

#include "engine/transfer.h"
#include "taichi/util/bit.h"

namespace engine {
namespace {
using namespace taichi::lang;
enum Command { Define, Inspect, SetKind, Release };
const std::vector<int> arg_input{0}, arg_output{1};

std::uint64_t next_context() {
  static std::atomic<std::uint64_t> next{1};
  auto value = next.load();
  for (;;) {
    if (value == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("record context identity exhausted");
    if (next.compare_exchange_weak(value, value + 1))
      return value;
  }
}
void operation(IRBuilder &b, SNode *node, Stmt *index, SNodeOpType op) {
  auto *ptr = b.create_global_ptr(node, {index});
  ptr->is_cell_access = true;
  b.insert(Stmt::make_typed<SNodeOpStmt>(op, node, ptr));
}
Stmt *active(IRBuilder &b, SNode *node, Stmt *index) {
  auto *ptr = b.create_global_ptr(node, {index});
  ptr->is_cell_access = true;
  return b.insert(
      Stmt::make_typed<SNodeOpStmt>(SNodeOpType::is_active, node, ptr));
}
Stmt *internal(IRBuilder &b, const char *name, std::vector<Stmt *> args) {
  return b.insert(Stmt::make_typed<InternalFuncStmt>(name, args));
}
Stmt *read(IRBuilder &b, SNode *node, Stmt *index) {
  return b.create_global_load(b.create_global_ptr(node, {index}));
}
void write(IRBuilder &b, SNode *node, Stmt *index, Stmt *value) {
  b.create_global_store(b.create_global_ptr(node, {index}), value);
}
void result(IRBuilder &b, ArgLoadStmt *out, int column, Stmt *value) {
  b.create_global_store(b.create_external_ptr(out, {b.get_int32(column)}),
                        b.create_cast(value, PrimitiveType::i64));
}
void status(IRBuilder &b, ArgLoadStmt *out, RecordStatus value) {
  result(b, out, 0, b.get_int32(static_cast<int>(value)));
}
void initialize_output(IRBuilder &b, ArgLoadStmt *out, RecordStatus value) {
  for (int i = 0; i < 5; ++i)
    result(b, out, i, b.get_int32(i == 1 ? -1 : 0));
  status(b, out, value);
}
RecordResult failed(RecordStatus status) {
  return {status, {}, 0, 0};
}
}  // namespace

struct PreparedRecordMap::Impl {
  struct Tree {
    Program *program{};
    SNodeTree *node{};
    ~Tree() {
      if (node) {
        program->synchronize();
        program->destroy_snode_tree(node);
      }
    }
  } tree;
  Runtime &runtime;
  const std::uint64_t context{next_context()};
  RecordBindings binding;
  std::size_t live{0};
  std::uint64_t serial{0}, committed{0};
  bool usable{false};
  std::unique_ptr<Ndarray> input, output;
  std::unique_ptr<Ndarray> batch_input, batch_output;
  std::vector<std::int64_t> staged_batch, returned_batch;
  CompiledKernel mutate, find, batch;
  std::unique_ptr<LaunchContextBuilder> mutate_context, find_context,
      batch_context;

  Impl(Runtime &r, int potential, int records, int batch_capacity)
      : runtime(r) {
    if (potential <= 0 || potential > (1 << 30) || records < 0 ||
        records > potential || batch_capacity < 0 || batch_capacity > (1 << 20))
      throw std::invalid_argument("invalid prepared record capacities");
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    auto &declarations = root->dense(Axis(0), potential);
    auto &code = declarations.insert_children(SNodeType::place);
    code.dt = PrimitiveType::u8;
    const int padded = int(taichi::bit::least_pot_bound(std::max(records, 1)));
    auto &pool = root->pointer(Axis(0), padded);
    pool.insert_children(SNodeType::place).dt = PrimitiveType::i8;
    auto &presence = root->bitmasked(Axis(0), padded);
    auto &address = presence.insert_children(SNodeType::place);
    address.dt = PrimitiveType::i32;
    auto &incarnation = presence.insert_children(SNodeType::place);
    incarnation.dt = PrimitiveType::i64;
    binding = {&presence, &address,  &incarnation, &code,
               &pool,     potential, records,      batch_capacity};
    tree.program = &r.program();
    tree.node = r.program().add_snode_tree(std::move(root), false);
    input = std::make_unique<Ndarray>(&r.program(), PrimitiveType::i64,
                                      std::vector<int>{5});
    output = std::make_unique<Ndarray>(&r.program(), PrimitiveType::i64,
                                       std::vector<int>{5});
    staged_batch.resize(2 + 5 * std::size_t(std::max(batch_capacity, 1)));
    returned_batch.resize(1 + 5 * std::size_t(std::max(batch_capacity, 1)));
    batch_input =
        std::make_unique<Ndarray>(&r.program(), PrimitiveType::i64,
                                  std::vector<int>{int(staged_batch.size())});
    batch_output =
        std::make_unique<Ndarray>(&r.program(), PrimitiveType::i64,
                                  std::vector<int>{int(returned_batch.size())});
    for (auto *array :
         {input.get(), output.get(), batch_input.get(), batch_output.get()})
      r.program().fill_ndarray_fast_u32(array, 0);

    // Exactly the requested number of native nodes, not padded capacity. Native
    // pointer deactivation inserts the fork's GC and retains its prepared
    // lists.
    for (bool on : {true, false}) {
      IRBuilder b;
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(records),
                                      false, r.cpu_threads());
      {
        auto guard = b.get_loop_guard(loop);
        operation(b, &pool, b.get_loop_index(loop),
                  on ? SNodeOpType::activate : SNodeOpType::deactivate);
      }
      CompiledKernel prepare(r,
                             "record_pool_prepare_" + std::to_string(pool.id) +
                                 (on ? "_on" : "_off"),
                             b.extract_ir(), {});
      auto ctx = prepare.make_context();
      prepare.launch(ctx);
    }
    r.synchronize();

    IRBuilder b;
    auto *in = b.create_ndarray_arg_load(arg_input, PrimitiveType::i64, 1, 0);
    auto *out = b.create_ndarray_arg_load(arg_output, PrimitiveType::i64, 1, 0);
    const auto item = [&](int i) {
      return b.create_global_load(b.create_external_ptr(in, {b.get_int32(i)}));
    };
    auto *command = b.create_cast(item(0), PrimitiveType::i32);
    auto *key = b.create_cast(item(1), PrimitiveType::i32);
    auto *kind = b.create_cast(item(2), PrimitiveType::u8);
    auto *row = b.create_cast(item(3), PrimitiveType::i32);
    auto *ticket = item(4);
    initialize_output(b, out, RecordStatus::Stale);
    {
      auto defining = b.get_if_guard(
          b.create_if(b.create_cmp_eq(command, b.get_int32(Define))), true);
      status(b, out, RecordStatus::AlreadyDefined);
      auto vacant = b.get_if_guard(
          b.create_if(b.create_cmp_eq(read(b, &code, key), b.get_int32(0))),
          true);
      auto *slot =
          internal(b, "prepared_node_acquire_index", {b.get_int32(pool.id)});
      status(b, out, RecordStatus::CapacityExhausted);
      {
        auto bad = b.get_if_guard(
            b.create_if(b.create_cmp_eq(slot, b.get_int32(-2))), true);
        status(b, out, RecordStatus::Unready);
      }
      auto available = b.get_if_guard(
          b.create_if(b.create_cmp_ge(slot, b.get_int32(0))), true);
      operation(b, &presence, slot, SNodeOpType::activate);
      write(b, &address, slot, key);
      write(b, &incarnation, slot, ticket);
      write(b, &code, key, kind);
      status(b, out, RecordStatus::Ok);
      result(b, out, 1, slot);
      result(b, out, 2, ticket);
      result(b, out, 3, kind);
    }
    {
      auto existing = b.get_if_guard(
          b.create_if(b.create_cmp_ne(command, b.get_int32(Define))), true);
      auto present =
          b.get_if_guard(b.create_if(active(b, &presence, row)), true);
      auto matches = b.get_if_guard(
          b.create_if(b.create_logical_and(
              b.create_cmp_eq(read(b, &address, row), key),
              b.create_cmp_eq(read(b, &incarnation, row), ticket))),
          true);
      status(b, out, RecordStatus::Ok);
      result(b, out, 1, row);
      result(b, out, 2, ticket);
      result(b, out, 3, read(b, &code, key));
      {
        auto update = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(SetKind))), true);
        write(b, &code, key, kind);
        result(b, out, 3, kind);
      }
      {
        auto releasing = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(Release))), true);
        auto *released = internal(b, "prepared_node_release_index",
                                  {b.get_int32(pool.id), row});
        auto *ok = b.create_if(b.create_cmp_eq(released, b.get_int32(0)));
        {
          auto success = b.get_if_guard(ok, true);
          write(b, &code, key, b.get_int32(0));
          operation(b, &presence, row, SNodeOpType::deactivate);
        }
        {
          auto failed = b.get_if_guard(ok, false);
          status(b, out, RecordStatus::Unready);
        }
      }
    }
    const std::vector<Param> params{Param::array(PrimitiveType::i64),
                                    Param::array(PrimitiveType::i64)};
    mutate =
        CompiledKernel(r, "record_map_mutate_" + std::to_string(presence.id),
                       b.extract_ir(), params);
    IRBuilder search;
    auto *search_in =
        search.create_ndarray_arg_load(arg_input, PrimitiveType::i64, 1, 0);
    auto *search_out =
        search.create_ndarray_arg_load(arg_output, PrimitiveType::i64, 1, 0);
    auto *wanted =
        search.create_cast(search.create_global_load(search.create_external_ptr(
                               search_in, {search.get_int32(1)})),
                           PrimitiveType::i32);
    initialize_output(search, search_out, RecordStatus::NotFound);
    auto *loop = search.create_struct_for(&presence, false, r.cpu_threads());
    {
      auto guard = search.get_loop_guard(loop);
      auto *slot = search.get_loop_index(loop);
      search.create_atomic_add(
          search.create_external_ptr(search_out, {search.get_int32(4)}),
          search.get_int64(1));
      auto same =
          search.get_if_guard(search.create_if(search.create_cmp_eq(
                                  read(search, &address, slot), wanted)),
                              true);
      status(search, search_out, RecordStatus::Ok);
      result(search, search_out, 1, slot);
      result(search, search_out, 2, read(search, &incarnation, slot));
      result(search, search_out, 3, read(search, &code, wanted));
    }
    find = CompiledKernel(r, "record_map_find_" + std::to_string(presence.id),
                          search.extract_ir(), params);
    mutate_context =
        std::make_unique<LaunchContextBuilder>(mutate.make_context());
    find_context = std::make_unique<LaunchContextBuilder>(find.make_context());
    for (auto *ctx : {mutate_context.get(), find_context.get()}) {
      ctx->set_arg_ndarray(arg_input, *input);
      ctx->set_arg_ndarray(arg_output, *output);
    }
    mutate.prepare(*mutate_context);
    find.prepare(*find_context);
    build_batch();
    // Warm real dispatches without claiming a record or creating geometry.
    execute({Inspect, 0, 0, 0, 0}, false);
    execute({0, 0, 0, 0, 0}, true);
    staged_batch[0] = Define;
    staged_batch[1] = 0;
    execute_batch(0, nullptr);
    usable = true;
  }

  void build_batch() {
    IRBuilder b;
    auto *in = b.create_ndarray_arg_load({0}, PrimitiveType::i64, 1, 0);
    auto *out = b.create_ndarray_arg_load({1}, PrimitiveType::i64, 1, 0);
    const auto input_at = [&](Stmt *index) {
      return b.create_global_load(b.create_external_ptr(in, {index}));
    };
    const auto output_at = [&](Stmt *index, Stmt *value) {
      b.create_global_store(b.create_external_ptr(out, {index}),
                            b.create_cast(value, PrimitiveType::i64));
    };
    auto *command = b.create_cast(input_at(b.get_int32(0)), PrimitiveType::i32);
    auto *count = b.create_cast(input_at(b.get_int32(1)), PrimitiveType::i32);
    auto *global = b.create_external_ptr(out, {b.get_int32(0)});
    b.create_global_store(global,
                          b.get_int64(static_cast<int>(RecordStatus::Ok)));
    {
      auto *loop = b.create_range_for(b.get_int32(0), count, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *base =
          b.create_add(b.get_int32(2), b.create_mul(i, b.get_int32(5)));
      auto *key = b.create_cast(input_at(base), PrimitiveType::i32);
      {
        auto define = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(Define))), true);
        auto duplicate = b.get_if_guard(
            b.create_if(b.create_cmp_ne(read(b, binding.declaration, key),
                                        b.get_int32(0))),
            true);
        b.create_global_store(
            global,
            b.get_int64(static_cast<int>(RecordStatus::AlreadyDefined)));
      }
      {
        auto release = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(Release))), true);
        auto *row = b.create_cast(input_at(b.create_add(base, b.get_int32(2))),
                                  PrimitiveType::i32);
        auto *present = b.create_if(active(b, binding.presence, row));
        {
          auto absent = b.get_if_guard(present, false);
          b.create_global_store(
              global, b.get_int64(static_cast<int>(RecordStatus::Stale)));
        }
        {
          auto exists = b.get_if_guard(present, true);
          auto mismatch = b.get_if_guard(
              b.create_if(b.create_logical_or(
                  b.create_cmp_ne(read(b, binding.address, row), key),
                  b.create_cmp_ne(
                      read(b, binding.incarnation, row),
                      input_at(b.create_add(base, b.get_int32(3)))))),
              true);
          b.create_global_store(
              global, b.get_int64(static_cast<int>(RecordStatus::Stale)));
        }
      }
    }
    {
      auto *end = b.create_select(
          b.create_cmp_eq(b.create_global_load(global), b.get_int64(0)), count,
          b.get_int32(0));
      auto *loop = b.create_range_for(b.get_int32(0), end, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto still_valid =
          b.get_if_guard(b.create_if(b.create_cmp_eq(
                             b.create_global_load(global), b.get_int64(0))),
                         true);
      auto *input_base =
          b.create_add(b.get_int32(2), b.create_mul(i, b.get_int32(5)));
      auto *output_base =
          b.create_add(b.get_int32(1), b.create_mul(i, b.get_int32(5)));
      auto *key = b.create_cast(input_at(input_base), PrimitiveType::i32);
      auto *kind =
          b.create_cast(input_at(b.create_add(input_base, b.get_int32(1))),
                        PrimitiveType::u8);
      auto *ticket = input_at(b.create_add(input_base, b.get_int32(3)));
      output_at(output_base, b.get_int32(0));
      {
        auto defining = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(Define))), true);
        auto *slot = internal(b, "prepared_node_acquire_index",
                              {b.get_int32(binding.allocator->id)});
        auto *valid = b.create_if(b.create_logical_and(
            b.create_cmp_ge(slot, b.get_int32(0)),
            b.create_cmp_lt(slot, b.get_int32(binding.record_capacity))));
        {
          auto good = b.get_if_guard(valid, true);
          operation(b, binding.presence, slot, SNodeOpType::activate);
          write(b, binding.address, slot, key);
          write(b, binding.incarnation, slot, ticket);
          write(b, binding.declaration, key, kind);
          output_at(b.create_add(output_base, b.get_int32(1)), slot);
          output_at(b.create_add(output_base, b.get_int32(2)), ticket);
          output_at(b.create_add(output_base, b.get_int32(3)), kind);
        }
        {
          auto bad = b.get_if_guard(valid, false);
          b.create_global_store(
              global, b.get_int64(static_cast<int>(RecordStatus::Unready)));
        }
      }
      {
        auto releasing = b.get_if_guard(
            b.create_if(b.create_cmp_eq(command, b.get_int32(Release))), true);
        auto *row =
            b.create_cast(input_at(b.create_add(input_base, b.get_int32(2))),
                          PrimitiveType::i32);
        auto *recycled = internal(b, "prepared_node_recycle_index",
                                  {b.get_int32(binding.allocator->id), row});
        auto *valid = b.create_if(b.create_cmp_eq(recycled, b.get_int32(0)));
        {
          auto good = b.get_if_guard(valid, true);
          output_at(b.create_add(output_base, b.get_int32(1)), row);
          output_at(b.create_add(output_base, b.get_int32(2)), ticket);
          output_at(b.create_add(output_base, b.get_int32(3)),
                    read(b, binding.declaration, key));
          write(b, binding.declaration, key, b.get_int32(0));
          operation(b, binding.presence, row, SNodeOpType::deactivate);
        }
        {
          auto bad = b.get_if_guard(valid, false);
          b.create_global_store(
              global, b.get_int64(static_cast<int>(RecordStatus::Unready)));
        }
      }
    }
    {
      auto collect = b.get_if_guard(
          b.create_if(b.create_logical_and(
              b.create_logical_and(
                  b.create_cmp_eq(command, b.get_int32(Release)),
                  b.create_cmp_gt(count, b.get_int32(0))),
              b.create_cmp_eq(b.create_global_load(global), b.get_int64(0)))),
          true);
      internal(b, "prepared_node_collect",
               {b.get_int32(binding.allocator->id)});
    }
    batch = CompiledKernel(
        runtime, "record_map_batch_" + std::to_string(binding.presence->id),
        b.extract_ir(),
        {Param::array(PrimitiveType::i64), Param::array(PrimitiveType::i64)});
    batch_context =
        std::make_unique<LaunchContextBuilder>(batch.make_context());
    batch_context->set_arg_ndarray({0}, *batch_input);
    batch_context->set_arg_ndarray({1}, *batch_output);
    batch.prepare(*batch_context);
  }

  RecordStatus execute_batch(std::size_t count, RecordResult *answers) {
    try {
      upload(*batch_input, staged_batch.data(),
             (2 + 5 * count) * sizeof(std::int64_t));
      batch.launch(*batch_context);
      runtime.synchronize();
      readback(*batch_output, returned_batch.data(),
               (1 + 5 * count) * sizeof(std::int64_t));
      if (returned_batch[0] < 0 ||
          returned_batch[0] > static_cast<int>(RecordStatus::Unready))
        throw std::runtime_error("invalid native batch result");
      const auto status = static_cast<RecordStatus>(returned_batch[0]);
      if (status == RecordStatus::Unready)
        usable = false;
      for (std::size_t i = 0; i < count; ++i) {
        const auto *out = returned_batch.data() + 1 + 5 * i;
        answers[i] =
            status == RecordStatus::Ok
                ? RecordResult{status,
                               {context, std::uint64_t(out[2]),
                                int(staged_batch[2 + 5 * i]), int(out[1])},
                               int(out[3]),
                               0}
                : failed(status);
      }
      return status;
    } catch (...) {
      usable = false;
      throw;
    }
  }

  RecordResult execute(const std::array<std::int64_t, 5> &request,
                       bool lookup) {
    try {
      upload(*input, request.data(), sizeof(request));
      (lookup ? find : mutate).launch(lookup ? *find_context : *mutate_context);
      runtime.synchronize();
      std::array<std::int64_t, 5> answer{};
      readback(*output, answer.data(), sizeof(answer));
      if (answer[0] < 0 || answer[0] > static_cast<int>(RecordStatus::Unready))
        throw std::runtime_error("invalid native record result");
      const auto s = static_cast<RecordStatus>(answer[0]);
      if (s == RecordStatus::Unready)
        usable = false;
      return {
          s,
          {context, std::uint64_t(answer[2]), int(request[1]), int(answer[1])},
          int(answer[3]),
          std::uint64_t(answer[4])};
    } catch (...) {
      usable = false;
      throw;
    }
  }
  RecordResult invalid(const RecordHandle &h) const {
    if (!usable)
      return failed(RecordStatus::Unready);
    if (h.context != context)
      return failed(RecordStatus::WrongContext);
    if (h.address < 0 || h.address >= binding.potential_capacity ||
        h.record < 0 || h.record >= binding.record_capacity ||
        h.incarnation == 0 ||
        h.incarnation > std::uint64_t(std::numeric_limits<std::int64_t>::max()))
      return failed(RecordStatus::Stale);
    return failed(RecordStatus::Ok);
  }
};

PreparedRecordMap::PreparedRecordMap(Runtime &r, int v, int p, int batch)
    : impl_(std::make_unique<Impl>(r, v, p, batch)) {
}
PreparedRecordMap::~PreparedRecordMap() = default;
RecordResult PreparedRecordMap::define(int address, int kind) {
  auto &s = *impl_;
  if (!s.usable)
    return failed(RecordStatus::Unready);
  if (address < 0 || address >= s.binding.potential_capacity || kind < 1 ||
      kind > 255)
    return failed(RecordStatus::InvalidInput);
  if (s.serial == std::uint64_t(std::numeric_limits<std::int64_t>::max()))
    return failed(RecordStatus::CapacityExhausted);
  auto answer =
      s.execute({Define, address, kind, 0, std::int64_t(s.serial + 1)}, false);
  if (answer.status == RecordStatus::Ok) {
    ++s.serial;
    ++s.live;
    ++s.committed;
  }
  return answer;
}
RecordResult PreparedRecordMap::lookup(int address) {
  auto &s = *impl_;
  if (!s.usable)
    return failed(RecordStatus::Unready);
  if (address < 0 || address >= s.binding.potential_capacity)
    return failed(RecordStatus::InvalidInput);
  return s.execute({0, address, 0, 0, 0}, true);
}
RecordResult PreparedRecordMap::inspect(const RecordHandle &h) {
  auto valid = impl_->invalid(h);
  if (valid.status != RecordStatus::Ok)
    return valid;
  return impl_->execute(
      {Inspect, h.address, 0, h.record, std::int64_t(h.incarnation)}, false);
}
RecordResult PreparedRecordMap::set_kind(const RecordHandle &h, int kind) {
  auto &s = *impl_;
  auto valid = s.invalid(h);
  if (valid.status != RecordStatus::Ok)
    return valid;
  if (kind < 1 || kind > 255)
    return failed(RecordStatus::InvalidInput);
  auto result = s.execute(
      {SetKind, h.address, kind, h.record, std::int64_t(h.incarnation)}, false);
  if (result.status == RecordStatus::Ok)
    ++s.committed;
  return result;
}
RecordResult PreparedRecordMap::release(const RecordHandle &h) {
  auto &s = *impl_;
  auto valid = s.invalid(h);
  if (valid.status != RecordStatus::Ok)
    return valid;
  auto result = s.execute(
      {Release, h.address, 0, h.record, std::int64_t(h.incarnation)}, false);
  if (result.status == RecordStatus::Ok) {
    --s.live;
    ++s.committed;
  }
  return result;
}
RecordStatus PreparedRecordMap::define_batch(const int *addresses,
                                             const int *kinds,
                                             std::size_t count,
                                             RecordResult *answers) {
  auto &s = *impl_;
  if (!s.usable)
    return RecordStatus::Unready;
  if (count > std::size_t(s.binding.batch_capacity))
    return RecordStatus::CapacityExhausted;
  if (!count)
    return RecordStatus::Ok;
  if (!addresses || !kinds || !answers)
    return RecordStatus::InvalidInput;
  for (std::size_t i = 0; i < count; ++i) {
    if (addresses[i] < 0 || addresses[i] >= s.binding.potential_capacity ||
        kinds[i] < 1 || kinds[i] > 255)
      return RecordStatus::InvalidInput;
    for (std::size_t j = 0; j < i; ++j)
      if (addresses[j] == addresses[i])
        return RecordStatus::AlreadyDefined;
  }
  if (count > std::size_t(s.binding.record_capacity) - s.live ||
      count >
          std::uint64_t(std::numeric_limits<std::int64_t>::max()) - s.serial)
    return RecordStatus::CapacityExhausted;
  s.staged_batch[0] = Define;
  s.staged_batch[1] = std::int64_t(count);
  for (std::size_t i = 0; i < count; ++i) {
    auto *entry = s.staged_batch.data() + 2 + 5 * i;
    entry[0] = addresses[i];
    entry[1] = kinds[i];
    entry[2] = 0;
    entry[3] = std::int64_t(s.serial + i + 1);
    entry[4] = 0;
  }
  const auto status = s.execute_batch(count, answers);
  if (status == RecordStatus::Ok) {
    s.serial += count;
    s.live += count;
    ++s.committed;
  }
  return status;
}

RecordStatus PreparedRecordMap::release_batch(const RecordHandle *handles,
                                              std::size_t count,
                                              RecordResult *answers) {
  auto &s = *impl_;
  if (!s.usable)
    return RecordStatus::Unready;
  if (count > std::size_t(s.binding.batch_capacity))
    return RecordStatus::CapacityExhausted;
  if (!count)
    return RecordStatus::Ok;
  if (!handles || !answers)
    return RecordStatus::InvalidInput;
  for (std::size_t i = 0; i < count; ++i) {
    const auto invalid = s.invalid(handles[i]);
    if (invalid.status != RecordStatus::Ok)
      return invalid.status;
    for (std::size_t j = 0; j < i; ++j)
      if (handles[j].record == handles[i].record)
        return RecordStatus::InvalidInput;
  }
  s.staged_batch[0] = Release;
  s.staged_batch[1] = std::int64_t(count);
  for (std::size_t i = 0; i < count; ++i) {
    auto *entry = s.staged_batch.data() + 2 + 5 * i;
    entry[0] = handles[i].address;
    entry[1] = 0;
    entry[2] = handles[i].record;
    entry[3] = std::int64_t(handles[i].incarnation);
    entry[4] = 0;
  }
  const auto status = s.execute_batch(count, answers);
  if (status == RecordStatus::Ok) {
    s.live -= count;
    ++s.committed;
  }
  return status;
}

const RecordBindings &PreparedRecordMap::bindings() const {
  return impl_->binding;
}
std::size_t PreparedRecordMap::size() const {
  return impl_->live;
}
std::uint64_t PreparedRecordMap::generation() const {
  return impl_->committed;
}
std::uint64_t PreparedRecordMap::context() const {
  return impl_->context;
}
bool PreparedRecordMap::ready() const {
  return impl_->usable;
}
std::size_t PreparedRecordMap::prepared_bytes() const {
  std::size_t bytes = 0;
  for (auto *ctx : {impl_->mutate_context.get(), impl_->find_context.get(),
                    impl_->batch_context.get()}) {
    bytes += ctx->arg_buffer_size + ctx->result_buffer_size +
             ctx->prepared->device_bytes + ctx->prepared->host_bytes +
             ctx->array_bindings.capacity() *
                 sizeof(LaunchContextBuilder::ArrayBinding) +
             ctx->prepared->scalars.capacity() *
                 sizeof(LaunchContextBuilder::Prepared::Scalar);
  }
  return bytes;
}
}  // namespace engine
