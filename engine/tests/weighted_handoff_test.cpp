#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "allocation_observation.h"
#include "engine/transfer.h"
#include "field/numeric_ir.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/ir/type_factory.h"
#include "taichi/program/kernel.h"
#include "taichi/runtime/llvm/kernel_launcher.h"

namespace {
using namespace taichi::lang;
using namespace field;
using namespace field::testing;
constexpr int N = 70, H = 140, R = 512, P = 70;
enum Config {
  Parent,
  ChildBegin,
  ChildCount,
  SourceBegin,
  SourceCount,
  ConfigCount
};
enum Diagnostic {
  Visits,
  Publications,
  Notifications,
  ChildReads,
  SourceReads,
  Errors,
  Collisions,
  DCount
};
void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
bool same(engine::RecordHandle a, engine::RecordHandle b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(!allocation_observation::calls,
          "weighted probe reuse allocated C++ metadata");
}
ArgLoadStmt *array(IRBuilder &b, int id, DataType t, int rank = 1) {
  return b.create_ndarray_arg_load({id}, t, rank, 0);
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at) {
  return b.create_global_load(b.create_external_ptr(a, at));
}
void write(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
std::vector<Stmt *> cell(IRBuilder &b, int f, Stmt *i) {
  return {b.get_int32(f), i};
}
void count(IRBuilder &b, ArgLoadStmt *a, int f, Stmt *i) {
  b.create_atomic_add(b.create_external_ptr(a, cell(b, f, i)), b.get_int64(1));
}
std::vector<engine::Param> signature() {
  using engine::Param;
  return {
      Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::i32, 2), Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::i32),    Param::array(PrimitiveType::u64, 2),
      Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i64, 2),
      Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::i32),    Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::u64),    Param::scalar(PrimitiveType::i32),
      Param::scalar(PrimitiveType::i32),   Param::scalar(PrimitiveType::i32)};
}
struct Args {
  ArgLoadStmt *values, *selector, *config, *children, *sources, *ratios,
      *bodies, *diag, *observed, *observed_selector, *own_selector, *active,
      *masks;
  Stmt *target, *op, *repeat;
  explicit Args(IRBuilder &b) {
    values = array(b, 0, PrimitiveType::f64, 2);
    selector = array(b, 1, PrimitiveType::i32);
    config = array(b, 2, PrimitiveType::i32, 2);
    children = array(b, 3, PrimitiveType::i32);
    sources = array(b, 4, PrimitiveType::i32);
    ratios = array(b, 5, PrimitiveType::u64, 2);
    bodies = array(b, 6, PrimitiveType::f64, 2);
    diag = array(b, 7, PrimitiveType::i64, 2);
    observed = array(b, 8, PrimitiveType::f64, 2);
    observed_selector = array(b, 9, PrimitiveType::i32);
    own_selector = array(b, 10, PrimitiveType::i32);
    active = array(b, 11, PrimitiveType::i32);
    masks = array(b, 12, PrimitiveType::u64);
    target = b.create_arg_load({13}, PrimitiveType::i32, false, 0);
    op = b.create_arg_load({14}, PrimitiveType::i32, false, 0);
    repeat = b.create_arg_load({15}, PrimitiveType::i32, false, 0);
  }
};
class NativeKernel {
 public:
  engine::Runtime &r;
  std::unique_ptr<Kernel> kernel;
  const CompiledKernelData *compiled;
  std::unique_ptr<LaunchContextBuilder> context;
  NativeKernel(engine::Runtime &runtime,
               const std::string &name,
               std::unique_ptr<Block> ir)
      : r(runtime) {
    kernel = std::make_unique<Kernel>(r.program(), std::move(ir), name);
    for (const auto &p : signature()) {
      if (p.is_array)
        kernel->insert_ndarray_param(p.dt, p.total_dim);
      else
        kernel->insert_scalar_param(p.dt);
    }
    kernel->finalize_params();
    kernel->finalize_rets();
    auto cfg = r.program().compile_config();
    cfg.fast_math = false;
    compiled = &r.program().compile_kernel(cfg, r.program().get_device_caps(),
                                           *kernel);
    context =
        std::make_unique<LaunchContextBuilder>(kernel->make_launch_context());
  }
  void bind(const std::array<Ndarray *, 13> &a) {
    for (int i = 0; i < 13; ++i)
      context->set_arg_ndarray({i}, *a[i]);
    for (int i = 13; i < 16; ++i)
      context->set_arg_int({i}, 0);
  }
  void launch(int target = -1, int op = 0, int repeat = 1) {
    engine::argument_int(*context, 13, target);
    engine::argument_int(*context, 14, op);
    engine::argument_int(*context, 15, repeat);
    r.program().launch_kernel(*compiled, *context);
    if (!context->prepared)
      dynamic_cast<LLVM::KernelLauncher &>(
          r.program().get_program_impl()->get_kernel_launcher())
          .prepare_kernel(*compiled, *context);
  }
  std::size_t binding_bytes() const {
    const auto &c = *context;
    return c.arg_buffer_size + c.result_buffer_size + c.prepared->device_bytes +
           c.prepared->host_bytes +
           c.array_bindings.capacity() *
               sizeof(LaunchContextBuilder::ArrayBinding) +
           c.prepared->scalars.capacity() *
               sizeof(LaunchContextBuilder::Prepared::Scalar);
  }
  void audit() const;
};
struct Probe {
  engine::Runtime &r;
  field::detail::NativeSet awake;
  std::array<std::unique_ptr<Ndarray>, 13> owned;
  std::array<Ndarray *, 13> arrays{};
  std::vector<double> values = std::vector<double>(8 * N),
                      body = std::vector<double>(4 * P),
                      observed = std::vector<double>(4 * H);
  std::vector<int> selectors = std::vector<int>(N),
                   config = std::vector<int>(ConfigCount * N),
                   children = std::vector<int>(H),
                   sources = std::vector<int>(R),
                   observed_selectors = std::vector<int>(H),
                   own_selectors = std::vector<int>(N),
                   activity = std::vector<int>(N);
  std::vector<std::uint64_t> ratios = std::vector<std::uint64_t>(2 * R, 1),
                             masks =
                                 std::vector<std::uint64_t>(N,
                                                            ~std::uint64_t{});
  std::vector<std::int64_t> diagnostic = std::vector<std::int64_t>(DCount * N);
  std::unique_ptr<NativeKernel> poll, command, inspect;
  explicit Probe(engine::Runtime &runtime) : r(runtime) {
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    awake = field::detail::NativeSet::attach(*root, N);
    r.program().add_snode_tree(std::move(root), false);
    const std::array<DataType, 13> types{
        PrimitiveType::f64, PrimitiveType::i32, PrimitiveType::i32,
        PrimitiveType::i32, PrimitiveType::i32, PrimitiveType::u64,
        PrimitiveType::f64, PrimitiveType::i64, PrimitiveType::f64,
        PrimitiveType::i32, PrimitiveType::i32, PrimitiveType::i32,
        PrimitiveType::u64};
    const std::array<std::vector<int>, 13> shapes{{{8, N},
                                                   {N},
                                                   {ConfigCount, N},
                                                   {H},
                                                   {R},
                                                   {2, R},
                                                   {4, P},
                                                   {DCount, N},
                                                   {4, H},
                                                   {H},
                                                   {N},
                                                   {N},
                                                   {N}}};
    for (int i = 0; i < 13; ++i) {
      owned[i] = std::make_unique<Ndarray>(&r.program(), types[i], shapes[i]);
      arrays[i] = owned[i].get();
    }
    compile();
    for (auto *k : {poll.get(), command.get(), inspect.get()})
      k->bind(arrays);
    std::fill_n(config.begin(), N, -1);
    std::fill_n(config.begin() + SourceCount * N, N, 1);
    body[3 * P] = 1;
    upload_config();
    engine::upload(*arrays[0], values.data(), values.size() * 8);
    command->launch(-1, 0);
    command->launch(-1, 1);
    poll->launch();
    readback();
    command->launch(-1, 0);
    readback();
    poll->audit();
    const char *names[] = {"publication_slots",
                           "selectors",
                           "topology",
                           "children",
                           "source_records",
                           "exact_ratios",
                           "body_snapshot",
                           "diagnostics",
                           "acquired_packets",
                           "acquired_selectors",
                           "own_selector_observations",
                           "activity_inspection",
                           "masks"};
    std::size_t payload = 0;
    for (int i = 0; i < 13; ++i) {
      std::size_t count = 1;
      for (int v : shapes[i])
        count *= v;
      const int width =
          (i == 1 || i == 2 || i == 3 || i == 4 || i == 9 || i == 10 || i == 11)
              ? 4
              : 8;
      payload += count * width;
      std::cout << "probe_array,name=" << names[i] << ",count=" << count
                << ",element_bytes=" << width
                << ",host_copies=1,native_copies=1,bytes=" << 2 * count * width
                << '\n';
    }
    std::cout << "probe_payload,host_bytes=" << payload
              << ",native_bytes=" << payload << ",binding_bytes="
              << poll->binding_bytes() + command->binding_bytes() +
                     inspect->binding_bytes()
              << ",bound_host_bytes=" << 8 * N * 8 + N * 4 + N * 4 * 8
              << ",inspection_bytes=14280,configuration_upload_bytes=15000,"
                 "initial_values_upload_bytes=4480 PASS\n";
  }
  void upload_config() {
    engine::upload(*arrays[2], config.data(), config.size() * 4);
    engine::upload(*arrays[3], children.data(), children.size() * 4);
    engine::upload(*arrays[4], sources.data(), sources.size() * 4);
    engine::upload(*arrays[5], ratios.data(), ratios.size() * 8);
    engine::upload(*arrays[6], body.data(), body.size() * 8);
    engine::upload(*arrays[12], masks.data(), masks.size() * 8);
  }
  void compile() {
    const auto suffix = std::to_string(awake.cells->id);
    const int threads = r.cpu_threads();
    {
      IRBuilder b;
      Args a(b);
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(N), false, threads);
      auto scope = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *reset = b.create_if(b.create_cmp_eq(a.op, b.get_int32(0)));
      {
        auto g = b.get_if_guard(reset, true);
        awake.deactivate(b, i);
        write(b, a.selector, {i}, b.get_int32(0));
        for (int d = 0; d < DCount; ++d)
          write(b, a.diag, cell(b, d, i), b.get_int64(0));
      }
      {
        auto g = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(2))), true);
        write(b, a.diag, cell(b, Visits, i), b.get_int64(0));
      }
      auto g = b.get_if_guard(
          b.create_if(b.create_logical_and(
              b.create_cmp_eq(a.op, b.get_int32(1)),
              b.create_logical_or(b.create_cmp_lt(a.target, b.get_int32(0)),
                                  b.create_cmp_eq(a.target, i)))),
          true);
      auto *repeat =
          b.create_range_for(b.get_int32(0), a.repeat, false, 1, 0, true);
      auto rep = b.get_loop_guard(repeat);
      awake.activate(b, i);
      command = std::make_unique<NativeKernel>(
          r, "weighted_handoff_command_" + suffix, b.extract_ir());
    }
    {
      IRBuilder b;
      Args a(b);
      auto *loop = b.create_struct_for(awake.cells, false, threads, 1);
      auto scope = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      // A target is a deterministic test-control cut. Ordinary polls use -1.
      auto target =
          b.get_if_guard(b.create_if(b.create_logical_or(
                             b.create_cmp_lt(a.target, b.get_int32(0)),
                             b.create_cmp_eq(a.target, row))),
                         true);
      awake.deactivate(b, row);
      count(b, a.diag, Visits, row);
      auto *old_sel = b.create_atomic_or(
          b.create_external_ptr(a.selector, {row}), b.get_int32(0));
      write(b, a.own_selector, {row}, old_sel);
      std::array<Stmt *, 4> old{};
      for (int axis = 0; axis < 4; ++axis)
        old[axis] = read(b, a.values,
                         {b.create_add(b.create_mul(old_sel, b.get_int32(4)),
                                       b.get_int32(axis)),
                          row});
      auto *mass = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(mass, b.get_float64(0));
      std::array<AllocaStmt *, 3> anchor{}, sum{};
      for (int axis = 0; axis < 3; ++axis) {
        anchor[axis] = b.create_local_var(PrimitiveType::f64);
        sum[axis] = b.create_local_var(PrimitiveType::f64);
        b.create_local_store(anchor[axis], b.get_float64(0));
        b.create_local_store(sum[axis], b.get_float64(0));
      }
      auto *parent_branch = b.create_if(b.create_cmp_gt(
          read(b, a.config, cell(b, ChildCount, row)), b.get_int32(0)));
      for (int parent = 0; parent < 2; ++parent) {
        auto branch = b.get_if_guard(parent_branch, parent == 1);
        auto *begin =
            read(b, a.config, cell(b, parent ? ChildBegin : SourceBegin, row));
        auto *length =
            read(b, a.config, cell(b, parent ? ChildCount : SourceCount, row));
        auto *members =
            b.create_range_for(b.get_int32(0), length, false, 1, 0, true);
        auto member = b.get_loop_guard(members);
        auto *index = b.get_loop_index(members);
        auto *edge = b.create_add(begin, index);
        std::array<Stmt *, 4> packet{};
        if (parent) {
          auto *child = read(b, a.children, {edge});
          auto *sel = b.create_atomic_or(
              b.create_external_ptr(a.selector, {child}), b.get_int32(0));
          write(b, a.observed_selector, {edge}, sel);
          for (int axis = 0; axis < 4; ++axis) {
            packet[axis] = read(b, a.values,
                                {b.create_add(b.create_mul(sel, b.get_int32(4)),
                                              b.get_int32(axis)),
                                 child});
            write(b, a.observed, cell(b, axis, edge), packet[axis]);
          }
          count(b, a.diag, ChildReads, row);
        } else {
          auto *source = read(b, a.sources, {edge});
          for (int axis = 0; axis < 4; ++axis)
            packet[axis] = read(b, a.bodies, cell(b, axis, source));
          auto *ratio =
              b.create_div(b.create_cast(read(b, a.ratios, cell(b, 0, edge)),
                                         PrimitiveType::f64),
                           b.create_cast(read(b, a.ratios, cell(b, 1, edge)),
                                         PrimitiveType::f64));
          packet[3] = b.create_mul(packet[3], ratio);
          count(b, a.diag, SourceReads, row);
        }
        for (auto *v : packet) {
          auto bad = b.get_if_guard(b.create_if(b.create_logical_not(
                                        field::detail::supported_real(b, v))),
                                    true);
          count(b, a.diag, Errors, row);
        }
        {
          auto bad = b.get_if_guard(
              b.create_if(b.create_cmp_le(packet[3], b.get_float64(0))), true);
          count(b, a.diag, Errors, row);
        }
        for (int axis = 0; axis < 3; ++axis) {
          {
            auto first = b.get_if_guard(
                b.create_if(b.create_cmp_eq(index, b.get_int32(0))), true);
            b.create_local_store(anchor[axis], packet[axis]);
          }
          b.create_local_store(
              sum[axis],
              b.create_add(
                  b.create_local_load(sum[axis]),
                  b.create_mul(packet[3],
                               b.create_sub(packet[axis], b.create_local_load(
                                                              anchor[axis])))));
        }
        b.create_local_store(
            mass, b.create_add(b.create_local_load(mass), packet[3]));
      }
      auto *m = b.create_local_load(mass);
      std::array<Stmt *, 4> result{};
      Stmt *changed = b.create_cmp_ne(m, old[3]);
      result[3] = m;
      {
        auto bad = b.get_if_guard(
            b.create_if(b.create_cmp_le(m, b.get_float64(0))), true);
        count(b, a.diag, Errors, row);
      }
      for (int axis = 0; axis < 3; ++axis) {
        auto *x = b.create_add(b.create_local_load(anchor[axis]),
                               b.create_div(b.create_local_load(sum[axis]), m));
        result[axis] = b.create_bit_cast(
            b.create_and(b.create_bit_cast(x, PrimitiveType::u64),
                         read(b, a.masks, {row})),
            PrimitiveType::f64);
        changed = b.create_logical_or(changed,
                                      b.create_cmp_ne(result[axis], old[axis]));
        auto bad = b.get_if_guard(b.create_if(b.create_logical_not(
                                      field::detail::supported_real(b, x))),
                                  true);
        count(b, a.diag, Errors, row);
      }
      {
        auto bad = b.get_if_guard(b.create_if(b.create_logical_not(
                                      field::detail::supported_real(b, m))),
                                  true);
        count(b, a.diag, Errors, row);
      }
      auto pub = b.get_if_guard(b.create_if(changed), true);
      auto *slot = b.create_xor(old_sel, b.get_int32(1));
      for (int axis = 0; axis < 4; ++axis)
        write(b, a.values,
              {b.create_add(b.create_mul(slot, b.get_int32(4)),
                            b.get_int32(axis)),
               row},
              result[axis]);
      auto *prior = b.create_atomic_xor(
          b.create_external_ptr(a.selector, {row}), b.get_int32(1));
      {
        auto bad =
            b.get_if_guard(b.create_if(b.create_cmp_ne(prior, old_sel)), true);
        count(b, a.diag, Collisions, row);
      }
      count(b, a.diag, Publications, row);
      auto *parent = read(b, a.config, cell(b, Parent, row));
      auto notify = b.get_if_guard(
          b.create_if(b.create_cmp_ge(parent, b.get_int32(0))), true);
      auto *repeat =
          b.create_range_for(b.get_int32(0), a.repeat, false, 1, 0, true);
      auto rep = b.get_loop_guard(repeat);
      awake.activate(b, parent);
      count(b, a.diag, Notifications, row);
      poll = std::make_unique<NativeKernel>(
          r, "weighted_handoff_poll_" + suffix, b.extract_ir());
    }
    {
      IRBuilder b;
      Args a(b);
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(N), false, threads);
      auto scope = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      write(b, a.active, {row},
            b.create_cast(awake.active(b, row), PrimitiveType::i32));
      inspect = std::make_unique<NativeKernel>(
          r, "weighted_handoff_inspect_" + suffix, b.extract_ir());
    }
  }
  void readback() {
    inspect->launch();
    r.synchronize();
    engine::readback(*arrays[0], values.data(), values.size() * 8);
    engine::readback(*arrays[1], selectors.data(), selectors.size() * 4);
    engine::readback(*arrays[7], diagnostic.data(), diagnostic.size() * 8);
    engine::readback(*arrays[8], observed.data(), observed.size() * 8);
    engine::readback(*arrays[9], observed_selectors.data(),
                     observed_selectors.size() * 4);
    engine::readback(*arrays[10], own_selectors.data(),
                     own_selectors.size() * 4);
    engine::readback(*arrays[11], activity.data(), activity.size() * 4);
  }
};
void NativeKernel::audit() const {
  const auto &module =
      *dynamic_cast<const LLVM::CompiledKernelData &>(*compiled)
           .get_internal_data()
           .compiled_data.module;
  const auto data_arg = [&](const llvm::Value *ptr) -> int {
    const auto *base =
        llvm::dyn_cast<llvm::LoadInst>(llvm::getUnderlyingObject(ptr));
    if (!base || !base->getType()->isPointerTy())
      return -1;
    std::int64_t offset = 0;
    const auto *argument_buffer =
        llvm::dyn_cast<llvm::LoadInst>(llvm::GetPointerBaseWithConstantOffset(
            base->getPointerOperand(), offset, module.getDataLayout()));
    if (!argument_buffer)
      return -1;
    std::int64_t context_offset = 0;
    const auto *context =
        llvm::dyn_cast<llvm::Argument>(llvm::GetPointerBaseWithConstantOffset(
            argument_buffer->getPointerOperand(), context_offset,
            module.getDataLayout()));
    // RuntimeContext's first word is its argument buffer. Runtime SNode and
    // list machinery uses other context members, not these ndarray bases.
    if (!context || context->getArgNo() != 0 ||
        context->getParent() != base->getFunction() || context_offset != 0)
      return -1;
    for (int id = 0; id < 13; ++id)
      if (offset == kernel->args_type->get_element_offset(
                        {id, TypeFactory::DATA_PTR_POS_IN_NDARRAY}))
        return id;
    throw std::runtime_error(
        "external pointer access cannot be attributed to a native array "
        "argument");
  };
  const auto child_dependent = [&](const llvm::Value *value) {
    std::set<const llvm::Value *> seen;
    const auto recurse = [&](const auto &self, const llvm::Value *v) -> bool {
      if (!seen.insert(v).second)
        return false;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v))
        if (data_arg(load->getPointerOperand()) == 3)
          return true;
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          if (self(self, op.get()))
            return true;
      return false;
    };
    return recurse(recurse, value);
  };
  std::vector<const llvm::AtomicRMWInst *> reads, pubs;
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &inst : bb) {
        if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst))
          require(data_arg(load->getPointerOperand()) != 1,
                  "ordinary selector read survived in concurrent kernel");
        if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst))
          require(data_arg(store->getPointerOperand()) != 1,
                  "ordinary selector store survived in concurrent kernel");
        if (const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(&inst))
          if (data_arg(a->getPointerOperand()) == 1) {
            require(a->getOrdering() ==
                        llvm::AtomicOrdering::SequentiallyConsistent,
                    "selector path lost SC ordering");
            const auto *v =
                llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand());
            require(v && v->getType()->isIntegerTy(32),
                    "unexpected selector operand");
            if (a->getOperation() == llvm::AtomicRMWInst::Or && v->isZero())
              reads.push_back(a);
            else if (a->getOperation() == llvm::AtomicRMWInst::Xor &&
                     v->equalsInt(1))
              pubs.push_back(a);
            else
              throw std::runtime_error("unattributed selector operation");
          }
      }
  require(pubs.size() == 1, "publication path missing or duplicated");
  int own = 0, child = 0;
  for (const auto *a : reads) {
    const bool indirect = child_dependent(a->getPointerOperand());
    if (indirect)
      ++child;
    else {
      ++own;
      require(a->getPointerOperand() == pubs[0]->getPointerOperand(),
              "own acquire and publication address differ");
    }
    std::string line;
    llvm::raw_string_ostream os(line);
    a->print(os);
    std::cout << "selector_ir,path="
              << (indirect ? "child_acquire" : "own_acquire") << ",arg=1,"
              << os.str() << '\n';
  }
  require(own == 1 && child > 0, "required own/indirect child path missing");
  std::string line;
  llvm::raw_string_ostream os(line);
  pubs[0]->print(os);
  std::cout << "selector_ir,path=publish,arg=1," << os.str() << '\n';
  std::cout << "selector_audit,own_sites=" << own << ",child_sites=" << child
            << ",publish_sites=1,plain_accesses=0,all_sc=1,selector_abi_offset="
            << kernel->args_type->get_element_offset(
                   {1, TypeFactory::DATA_PTR_POS_IN_NDARRAY})
            << ",child_arg=3 PASS\n";
}
using Packet = std::array<double, 4>;
double quantize(double x, int bits) {
  if (!x)
    return x;
  int exponent;
  std::frexp(std::fabs(static_cast<long double>(x)), &exponent);
  return std::copysign(
      double(std::ldexp(
          std::trunc(std::ldexp(std::fabs(static_cast<long double>(x)),
                                bits - exponent + 1)),
          exponent - 1 - bits)),
      x);
}
Packet fold(const Packet *inputs, int count, int bits) {
  require(count > 0, "empty numerical cohort");
  std::array<long double, 4> sum{};
  for (int i = 0; i < count; ++i) {
    require(inputs[i][3] > 0, "nonpositive input mass");
    for (int a = 0; a < 3; ++a)
      sum[a] += static_cast<long double>(inputs[i][a]) * inputs[i][3];
    sum[3] += inputs[i][3];
  }
  Packet out{};
  for (int a = 0; a < 3; ++a)
    out[a] = quantize(double(sum[a] / sum[3]), bits);
  out[3] = double(sum[3]);
  return out;
}
void numerical(const Packet &actual,
               const Packet &reference,
               int bits,
               int terms) {
  for (int a = 0; a < 4; ++a) {
    if (a < 3 && bits < 52)
      require(actual[a] == reference[a], "wrong centroid publication cell");
    const long double bound = 64 * (terms + 1) *
                              std::numeric_limits<double>::epsilon() *
                              std::max(1., std::fabs(reference[a]));
    require(
        std::fabs(static_cast<long double>(actual[a]) - reference[a]) <= bound,
        "centroid differs from independent weighted reference");
  }
}
Packet selected_packet(const std::vector<double> &v,
                       const std::vector<int> &s,
                       int row) {
  Packet p{};
  for (int a = 0; a < 4; ++a)
    p[a] = v[(s[row] * 4 + a) * N + row];
  return p;
}
struct Graph {
  std::string name;
  std::vector<int> rows, bits, levels;
  std::vector<std::vector<int>> children;
  std::vector<std::vector<std::uint64_t>> coverage;
  std::vector<std::uint64_t> denominators;
  bool repeated{}, aliases{}, equal_positions{};
};
Graph graph(std::string name,
            std::vector<int> rows,
            std::vector<std::vector<int>> children,
            std::vector<std::vector<std::uint64_t>> coverage,
            std::vector<std::uint64_t> denominators = {}) {
  Graph g;
  g.name = std::move(name);
  g.rows = std::move(rows);
  g.children = std::move(children);
  g.coverage = std::move(coverage);
  const int n = g.rows.size(), b = g.coverage[0].size();
  g.bits.assign(n, 52);
  g.levels.assign(n, 0);
  for (int i = 0; i < n; ++i)
    if (!g.children[i].empty()) {
      std::fill(g.coverage[i].begin(), g.coverage[i].end(), 0);
      for (int child : g.children[i]) {
        require(child < i, "fixture must be postorder");
        g.levels[i] = std::max(g.levels[i], g.levels[child] + 2);
        for (int j = 0; j < b; ++j)
          g.coverage[i][j] += g.coverage[child][j];
      }
    }
  g.denominators =
      denominators.empty() ? g.coverage.back() : std::move(denominators);
  g.bits.back() = 4;
  return g;
}
std::vector<Graph> graphs() {
  std::vector<Graph> out;
  for (auto pair : std::vector<std::pair<int, int>>{{3, 4},
                                                    {4, 3},
                                                    {31, 32},
                                                    {32, 31},
                                                    {3, 64},
                                                    {64, 3},
                                                    {1, 69},
                                                    {69, 1}})
    out.push_back(graph("pair_" + std::to_string(pair.first) + "_" +
                            std::to_string(pair.second),
                        {pair.first, pair.second}, {{}, {0}}, {{1}, {0}}));
  out.push_back(graph("two_leaf", {3, 32, 69}, {{}, {}, {0, 1}},
                      {{1, 0}, {0, 1}, {0, 0}}));
  out.push_back(graph("overlap", {64, 4, 31}, {{}, {}, {0, 1}},
                      {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}}));
  auto repeated = graph("repeated_alias", {31, 3, 69}, {{}, {}, {0, 1}},
                        {{2, 0}, {0, 2}, {0, 0}});
  repeated.repeated = true;
  repeated.aliases = true;
  out.push_back(repeated);
  out.push_back(graph("quarters", {4, 64, 69}, {{}, {}, {0, 1}},
                      {{1, 0}, {0, 3}, {0, 0}}, {4, 4}));
  auto thirds = graph("thirds", {32, 4, 69}, {{}, {}, {0, 1}},
                      {{1, 0}, {0, 1}, {0, 0}}, {3, 3});
  thirds.bits.back() = 52;
  out.push_back(thirds);
  auto equal = graph("mass_only", {69, 32, 3}, {{}, {}, {0, 1}},
                     {{1, 0}, {0, 1}, {0, 0}});
  equal.equal_positions = true;
  out.push_back(equal);
  out.push_back(graph("chain_forward", {3, 4, 33, 64, 69},
                      {{}, {0}, {1}, {2}, {3}}, {{1}, {0}, {0}, {0}, {0}}));
  out.push_back(graph("chain_reverse", {69, 64, 33, 4, 3},
                      {{}, {0}, {1}, {2}, {3}}, {{1}, {0}, {0}, {0}, {0}}));
  out.push_back(graph("fan_in", {1, 3, 4, 31, 32, 64, 69},
                      {{}, {}, {}, {}, {}, {}, {0, 1, 2, 3, 4, 5}},
                      {{1, 0, 0, 0, 0, 0},
                       {0, 1, 0, 0, 0, 0},
                       {0, 0, 1, 0, 0, 0},
                       {0, 0, 0, 1, 0, 0},
                       {0, 0, 0, 0, 1, 0},
                       {0, 0, 0, 0, 0, 1},
                       {0, 0, 0, 0, 0, 0}}));
  std::vector<int> rows;
  std::vector<std::vector<int>> kids;
  std::vector<std::vector<std::uint64_t>> counts;
  for (int i = 0; i < 32; ++i) {
    rows.push_back((i * 37 + 13) % N);
    kids.push_back({});
    counts.emplace_back(32, 0);
    counts.back()[i] = 1;
  }
  int begin = 0, width = 32;
  while (width > 1) {
    const int start = kids.size();
    for (int i = 0; i < width; i += 2) {
      rows.push_back((int(rows.size()) * 37 + 13) % N);
      kids.push_back({begin + i, begin + i + 1});
      counts.emplace_back(32, 0);
    }
    begin = start;
    width /= 2;
  }
  out.push_back(graph("permuted_binary", std::move(rows), std::move(kids),
                      std::move(counts)));
  return out;
}
BudgetDemand source_limits() {
  BudgetDemand l;
  l.values = {1048576, P, N, R, 0, 16384, 0, 32, 0};
  l[Budget::CascadeNodes] = N;
  l[Budget::CascadeLinks] = H;
  l[Budget::CascadeReferences] = R;
  l[Budget::CascadeCoverage] = R;
  l[Budget::CascadeLevels] = 16;
  l[Budget::CascadeWork] = 1ull << 40;
  l[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      l, false, NumericalProfile::trial());
  return l;
}
struct Source {
  Graph g;
  Harness h;
  PreparedFixtureAccess::Input in{P, N, 32, 0};
  std::vector<CascadeNode> nodes;
  std::vector<int> children;
  std::vector<CascadeContribution> refs;
  Source(engine::Runtime &r, HostPath path, const Graph &input)
      : g(input),
        h(r,
          PreparedCapacity(source_limits(), NumericalProfile::trial()),
          path) {
    FixtureSource src;
    src.kinds.assign(g.rows.size(), 1);
    src.exact_masses = g.denominators;
    for (std::size_t i = 0; i < g.denominators.size(); ++i) {
      std::array<float, kParticleFieldCount> p{};
      const float x =
          g.equal_positions || g.name == "quarters" ? 1.125f : 1.f + i * .25f;
      p[kPosX] = x;
      p[kPosY] = 2 * x;
      p[kPosZ] = -x;
      p[kMass] = 1;
      src.particles.push_back(p);
    }
    for (std::size_t i = 0; i < g.rows.size(); ++i)
      for (std::size_t j = 0; j < g.denominators.size(); ++j)
        if (g.coverage[i][j]) {
          const auto add = [&](std::uint64_t c) {
            src.edges.push_back({int(j), int(i),
                                 float(double(c) / g.denominators[j]),
                                 src.edges.size(), c});
          };
          if (g.repeated && i == 0 && g.coverage[i][j] > 1) {
            add(1);
            add(g.coverage[i][j] - 1);
          } else
            add(g.coverage[i][j]);
        }
    Parameters p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    no_new([&] { PreparedFixtureAccess::load(h, src, in, p, true); });
    nodes.reserve(N);
    children.reserve(H);
    refs.reserve(R);
    refresh_spec();
  }
  void refresh_spec() {
    nodes.clear();
    children.clear();
    refs.clear();
    const auto &s = h.prepared_storage();
    for (std::size_t i = 0; i < g.rows.size(); ++i) {
      CascadeNode node;
      node.field = in.field_rows[i];
      node.comparison = i + 1;
      node.level = g.levels[i];
      node.retained_fraction_bits = g.bits[i];
      node.child_begin = children.size();
      node.child_count = g.children[i].size();
      for (int child : g.children[i])
        children.push_back(child);
      node.cohort_begin = refs.size();
      for (std::size_t row = 0; row < s.live.edges; ++row)
        if (s.edges[row].second.record == node.field.record) {
          refs.push_back(
              {s.edges[row].first, {s.generation, row, s.edges[row].ordinal}});
          if (g.aliases && i == 0)
            refs.push_back(refs.back());
        }
      node.cohort_count = refs.size() - node.cohort_begin;
      nodes.push_back(node);
    }
  }
  WeightedCascadeSpec spec() const {
    return {nodes.data(),    nodes.size(), children.data(),
            children.size(), refs.data(),  refs.size()};
  }
  void state(int stage, bool cancel, int sign) {
    no_new([&] { h.inspect_prepared(); });
    for (std::size_t i = 0; i < g.denominators.size(); ++i) {
      const int row = in.particle_rows[i].record;
      double x =
          g.equal_positions || g.name == "quarters" ? 1.125 : 1. + i * .25;
      if (stage && !g.equal_positions)
        x += g.name == "quarters" ? (i == 0 ? 3. : 1.) / 16
                                  : (cancel && i % 2 ? -1. : 1.) * 3. / 32;
      if (stage >= 2 && !g.equal_positions && !cancel && i == 0)
        x += 1. / 32;
      h.particle64(kPosX, row) = sign * x;
      h.particle64(kPosY, row) = sign * 2 * x;
      h.particle64(kPosZ, row) = -sign * x;
      h.particle64(kMass, row) = stage == 3 && i == 0 ? 1.5 : 1.;
    }
    no_new([&] { PreparedFixtureAccess::upload_body_fixture64(h); });
  }
  std::uint64_t hash() {
    no_new([&] { h.inspect_prepared(); });
    std::uint64_t v = 1469598103934665603ULL;
    const auto add = [&](const auto &a) {
      const auto *p = reinterpret_cast<const unsigned char *>(a.data());
      for (std::size_t i = 0; i < a.size() * sizeof(a[0]); ++i) {
        v ^= p[i];
        v *= 1099511628211ULL;
      }
    };
    add(h.particles64());
    add(h.groups64());
    add(h.determined64());
    add(h.prepared_storage().motion64());
    add(h.prepared_storage().inspection_state);
    add(h.prepared_storage().inspection_flags);
    add(h.prepared_storage().source_counts);
    add(h.prepared_storage().source_masses);
    add(h.prepared_storage().particle_headers);
    add(h.prepared_storage().field_headers);
    add(h.prepared_storage().edges);
    const std::array<std::uint64_t, 2> state{
        h.prepared_storage().generation, h.prepared_storage().completed_ticks};
    add(state);
    add(h.frame_anchor());
    std::array<unsigned char, sizeof(Demand)> demand{};
    std::memcpy(demand.data(), &h.demand(), demand.size());
    add(demand);
    const std::array<double, 1> universal{h.universal64()};
    add(universal);
    return v;
  }
};
struct Bound {
  Probe &p;
  Source &s;
  bool ready{}, trace{};
  std::uint64_t generation{}, snapshot_tick{}, polls{}, work{},
      readback_bytes{}, snapshots{}, initializations{}, query_bytes{};
  double kernel_us{}, inspection_us{}, first_poll_us{}, initialization_us{};
  std::vector<double> before_values = std::vector<double>(8 * N);
  std::vector<int> before_selectors = std::vector<int>(N);
  std::vector<Packet> reference = std::vector<Packet>(N);
  explicit Bound(Probe &probe, Source &source) : p(probe), s(source) {
  }
  void guard() {
    if (!ready || !s.h.ready() ||
        generation != s.h.prepared_storage().generation ||
        snapshot_tick != s.h.prepared_storage().completed_ticks) {
      ready = false;
      throw std::logic_error("stale/unbound weighted handoff");
    }
  }
  bool bind(const WeightedCascadeSpec &spec) {
    ready = false;
    if (spec.node_count != s.g.rows.size() || spec.node_count > N)
      return false;
    for (std::size_t i = 0; i < spec.node_count; ++i) {
      if (s.g.rows[i] < 0 || s.g.rows[i] >= N)
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (s.g.rows[i] == s.g.rows[j])
          return false;
    }
    for (std::size_t i = 0; i < spec.node_count; ++i) {
      if (!spec.nodes[i].publish)
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (same(spec.nodes[i].field, spec.nodes[j].field))
          return false;
    }
    CascadeBinding result;
    no_new([&] {
      result = s.h.bind_weighted_centroid_cascade(
          s.h.prepared_storage().generation, spec);
    });
    if (result.status != CascadeStatus::Ok)
      return false;
    CascadeResult q;
    no_new([&] {
      q = s.h.query_centroid_cascade(s.h.prepared_storage().generation);
    });
    require(q.status == CascadeStatus::Ok,
            "bound candidate initialization failed");
    const auto &ins = s.h.inspect_centroid_cascade(q);
    std::fill(p.config.begin(), p.config.end(), 0);
    std::fill_n(p.config.begin(), N, -1);
    std::fill(p.sources.begin(), p.sources.end(), 0);
    std::fill(p.children.begin(), p.children.end(), 0);
    std::fill(p.ratios.begin(), p.ratios.end(), 1);
    int nr = 0, nh = 0;
    for (std::size_t i = 0; i < spec.node_count; ++i) {
      const int row = s.g.rows[i];
      const auto &node = ins.nodes[i];
      p.config[ChildBegin * N + row] = nh;
      p.config[ChildCount * N + row] = node.child_count;
      p.masks[row] =
          ~((std::uint64_t{1} << (52 - node.retained_fraction_bits)) - 1);
      for (std::size_t j = 0; j < node.child_count; ++j) {
        const int child = ins.children[node.child_begin + j];
        require(p.config[Parent * N + s.g.rows[child]] < 0,
                "probe requires one parent");
        p.config[Parent * N + s.g.rows[child]] = row;
        p.children[nh++] = s.g.rows[child];
      }
      p.config[SourceBegin * N + row] = nr;
      if (!node.child_count)
        for (std::size_t j = 0; j < node.cohort_count; ++j) {
          const auto at = node.cohort_begin + j;
          const auto source = ins.source_rows[at].row;
          const auto body = ins.cohorts[at].record;
          require(nr < R, "leaf source capacity");
          p.sources[nr] = body;
          p.ratios[nr] = s.h.prepared_storage().source_counts[source];
          p.ratios[R + nr] = s.h.prepared_storage().source_masses[body];
          ++nr;
          ++p.config[SourceCount * N + row];
        }
    }
    generation = s.h.prepared_storage().generation;
    snapshot_tick = s.h.prepared_storage().completed_ticks;
    ready = true;
    snapshot();
    no_new([&] { p.upload_config(); });
    initialize();
    return true;
  }
  void snapshot() {
    guard();
    ++snapshots;
    no_new([&] { s.h.inspect_prepared(); });
    for (int i = 0; i < P; ++i) {
      for (int a = 0; a < 3; ++a)
        p.body[a * P + i] = s.h.particle64(kPosX + a, i);
      p.body[3 * P + i] = s.h.particle64(kMass, i);
    }
    no_new([&] {
      engine::upload(*p.arrays[6], p.body.data(), p.body.size() * 8);
    });
  }
  void initialize() {
    guard();
    const auto start = std::chrono::steady_clock::now();
    ++initializations;
    CascadeResult q;
    no_new([&] { q = s.h.query_centroid_cascade(generation); });
    require(q.status == CascadeStatus::Ok, "candidate initialization rejected");
    const auto &ins = s.h.inspect_centroid_cascade(q);
    query_bytes += q.readback_bytes + 7 * N * 8 + N * 4;
    std::fill(p.values.begin(), p.values.end(), 0);
    for (std::size_t i = 0; i < s.g.rows.size(); ++i)
      for (int a = 0; a < 4; ++a)
        for (int slot = 0; slot < 2; ++slot)
          p.values[(slot * 4 + a) * N + s.g.rows[i]] =
              ins.values[(a < 3 ? a + 3 : 6) * N + i];
    no_new([&] {
      engine::upload(*p.arrays[0], p.values.data(), p.values.size() * 8);
      p.command->launch(-1, 0);
      p.readback();
    });
    initialization_us += std::chrono::duration<double, std::micro>(
                             std::chrono::steady_clock::now() - start)
                             .count();
  }
  void seed(bool all, int repeat) {
    guard();
    no_new([&] {
      for (std::size_t i = 0; i < s.g.rows.size(); ++i)
        if (all || s.g.children[i].empty())
          p.command->launch(s.g.rows[i], 1, repeat);
    });
  }
  Packet leaf(int logical) const {
    std::array<Packet, R> inputs{};
    int n = 0;
    const int row = s.g.rows[logical], begin = p.config[SourceBegin * N + row],
              length = p.config[SourceCount * N + row];
    for (int i = 0; i < length; ++i) {
      const auto source = p.sources[begin + i];
      for (int a = 0; a < 4; ++a)
        inputs[n][a] = p.body[a * P + source];
      const long double exact = static_cast<long double>(inputs[n][3]) *
                                p.ratios[begin + i] / p.ratios[R + begin + i];
      inputs[n][3] = double(exact);
      ++n;
    }
    return fold(inputs.data(), n, s.g.bits[logical]);
  }
  void expected() {
    for (std::size_t i = 0; i < s.g.rows.size(); ++i) {
      if (s.g.children[i].empty())
        reference[i] = leaf(i);
      else {
        std::array<Packet, H> inputs{};
        int n = 0;
        for (int child : s.g.children[i])
          inputs[n++] = reference[child];
        reference[i] = fold(inputs.data(), n, s.g.bits[i]);
      }
    }
  }
  void step(int repeat = 1, int target = -1) {
    guard();
    std::copy(p.values.begin(), p.values.end(), before_values.begin());
    std::copy(p.selectors.begin(), p.selectors.end(), before_selectors.begin());
    const auto time = []() { return std::chrono::steady_clock::now(); };
    auto start = time();
    const auto poll_start = start;
    no_new([&] {
      p.command->launch(-1, 2);
      p.poll->launch(target, 0, repeat);
      p.r.synchronize();
    });
    kernel_us +=
        std::chrono::duration<double, std::micro>(time() - start).count();
    start = time();
    no_new([&] { p.readback(); });
    inspection_us +=
        std::chrono::duration<double, std::micro>(time() - start).count();
    if (!polls)
      first_poll_us =
          std::chrono::duration<double, std::micro>(time() - poll_start)
              .count();
    ++polls;
    readback_bytes += (8 * N + 4 * H) * 8 + (3 * N + H) * 4 + DCount * N * 8;
    for (int row = 0; row < N; ++row)
      require(p.diagnostic[Visits * N + row] <= 1 &&
                  !p.diagnostic[Errors * N + row] &&
                  !p.diagnostic[Collisions * N + row],
              "poll collision/nonfinite/duplicate visit");
    for (std::size_t i = 0; i < s.g.rows.size(); ++i) {
      const int row = s.g.rows[i];
      if (!p.diagnostic[Visits * N + row])
        continue;
      ++work;
      require(p.own_selectors[row] == before_selectors[row],
              "own selector input changed before its sole writer");
      Packet ref{};
      if (s.g.children[i].empty())
        ref = leaf(i);
      else {
        std::array<Packet, H> inputs{};
        const int begin = p.config[ChildBegin * N + row];
        int n = 0;
        for (int child : s.g.children[i]) {
          const int edge = begin + n, cr = s.g.rows[child],
                    sel = p.observed_selectors[edge];
          Packet seen{};
          for (int a = 0; a < 4; ++a)
            seen[a] = p.observed[a * H + edge];
          const bool old =
              sel == before_selectors[cr] &&
              seen == selected_packet(before_values, before_selectors, cr);
          const bool next = sel == p.selectors[cr] &&
                            seen == selected_packet(p.values, p.selectors, cr);
          require(old || next,
                  "child input was torn or not a pre/post-poll publication");
          if (trace) {
            const auto pre =
                           selected_packet(before_values, before_selectors, cr),
                       post = selected_packet(p.values, p.selectors, cr);
            std::cout << "acquired_input,name=" << s.g.name << ",poll=" << polls
                      << ",parent=" << row << ",child=" << cr
                      << ",selector=" << sel << ",old_match=" << old
                      << ",new_match=" << next;
            for (int a = 0; a < 4; ++a)
              std::cout << ",seen" << a << '=' << seen[a] << ",before" << a
                        << '=' << pre[a] << ",after" << a << '=' << post[a];
            std::cout << '\n';
          }
          inputs[n++] = seen;
        }
        ref = fold(inputs.data(), n, s.g.bits[i]);
      }
      numerical(selected_packet(p.values, p.selectors, row), ref, s.g.bits[i],
                s.g.children[i].empty() ? p.config[SourceCount * N + row]
                                        : s.g.children[i].size());
    }
  }
  int settle(int repeat) {
    expected();
    const auto source_hash = s.hash();
    int steps = 0;
    for (; steps < 16; ++steps) {
      step(repeat);
      if (std::none_of(p.activity.begin(), p.activity.end(),
                       [](int v) { return v != 0; }))
        break;
    }
    require(steps < 16, "16-poll numerical convergence limit reached");
    for (std::size_t i = 0; i < s.g.rows.size(); ++i)
      numerical(selected_packet(p.values, p.selectors, s.g.rows[i]),
                reference[i], s.g.bits[i],
                s.g.rows.size() + s.g.denominators.size());
    for (int i = 0; i < 8; ++i) {
      step(repeat);
      require(std::none_of(p.activity.begin(), p.activity.end(),
                           [](int v) { return v != 0; }),
              "quiet observation regained unresolved work");
      for (int row : s.g.rows)
        require(!p.diagnostic[Visits * N + row],
                "quiet observation performed calculation");
    }
    require(s.hash() == source_hash,
            "probe changed source physical/control/history/activity state");
    return steps + 1;
  }
  void candidate_check() {
    guard();
    CascadeResult q;
    no_new([&] { q = s.h.query_centroid_cascade(generation); });
    require(q.status == CascadeStatus::Ok, "frozen reference query failed");
    const auto &ins = s.h.inspect_centroid_cascade(q);
    query_bytes += q.readback_bytes + 7 * N * 8 + N * 4;
    for (std::size_t i = 0; i < s.g.rows.size(); ++i) {
      Packet ref{};
      for (int a = 0; a < 4; ++a)
        ref[a] = ins.values[(a < 3 ? a + 3 : 6) * N + i];
      numerical(selected_packet(p.values, p.selectors, s.g.rows[i]), ref,
                s.g.bits[i], s.g.rows.size() + s.g.denominators.size());
    }
  }
};
void controls(Probe &p, Source &s) {
  for (bool after_consume : {false, true})
    for (bool cancel : {false, true}) {
      s.state(0, cancel, 1);
      Bound b(p, s);
      require(b.bind(s.spec()), "control bind rejected");
      const int root = s.g.rows.back();
      const auto old = selected_packet(p.values, p.selectors, root);
      if (after_consume) {
        no_new([&] { p.command->launch(root, 1); });
        b.step(1, root);
        require(!p.activity[root],
                "control parent did not consume its old input");
      }
      s.state(1, cancel, 1);
      b.snapshot();
      b.seed(false, 64);
      b.step(64, s.g.rows[0]);
      require(p.activity[root],
              "child publication did not leave native parent work");
      b.step(64, s.g.rows[1]);
      require(p.activity[root], "duplicate notification consumed parent work");
      const auto pubs = p.diagnostic[Publications * N + root];
      b.step(64, root);
      if (cancel)
        require(selected_packet(p.values, p.selectors, root) == old &&
                    p.diagnostic[Publications * N + root] == pubs,
                "complete opposing inputs did not cancel before Q");
      else
        require(selected_packet(p.values, p.selectors, root)[0] == 1.1875,
                "collective normalized contributions failed to cross root Q");
      b.settle(64);
      b.candidate_check();
      std::cout << "ordered_control,after_consume=" << after_consume
                << ",cancel=" << cancel
                << ",root_x=" << selected_packet(p.values, p.selectors, root)[0]
                << ",pending=0 PASS\n";
    }
  Bound b(p, s);
  require(b.bind(s.spec()), "rejection base bind");
  const auto reject = [&](const char *name) {
    const auto state = s.hash();
    const auto values = p.values;
    const auto selector = p.selectors;
    require(!b.bind(s.spec()), "invalid hierarchy accepted");
    bool threw = false;
    try {
      b.step();
    } catch (const std::logic_error &) {
      threw = true;
    }
    require(threw && !b.ready && s.h.ready() && s.hash() == state &&
                p.values == values && p.selectors == selector,
            "rejection mutated source/publication or allowed poll");
    std::cout << "binding_rejection,kind=" << name
              << ",source_preserved=1,unbound=1 PASS\n";
  };
  auto field = s.nodes[1].field;
  s.nodes[1].field = s.nodes[0].field;
  reject("aliased_expressed_output");
  s.nodes[1].field = field;
  ++s.nodes[0].field.incarnation;
  reject("stale_field_handle");
  --s.nodes[0].field.incarnation;
  --s.refs[0].source.generation;
  reject("stale_source_row");
  ++s.refs[0].source.generation;
  auto child = s.children.back();
  s.children.back() = s.nodes.size() - 1;
  reject("cycle");
  s.children.back() = child;
  auto count = s.nodes.back().child_count;
  s.nodes.back().child_count = 0;
  reject("missing_root_connectivity");
  s.nodes.back().child_count = count;
  --s.nodes.back().cohort_count;
  reject("incompatible_parent_coverage");
  ++s.nodes.back().cohort_count;
  s.nodes[0].publish = false;
  reject("anonymous_grouping_outside_subset");
  s.nodes[0].publish = true;
  require(b.bind(s.spec()), "valid rebind after rejection failed");
  UpdateCommand edit;
  edit.operation = UpdateOp::ReplaceParticipation;
  edit.generation = s.h.prepared_storage().generation;
  edit.row = 0;
  edit.first = UpdateRef::existing(s.h.prepared_storage().edges[0].first);
  edit.second = UpdateRef::existing(s.h.prepared_storage().edges[0].second);
  edit.ordinal = 10000;
  edit.share = s.h.prepared_storage().edges[0].share;
  edit.exact_count = s.h.prepared_storage().source_counts[0];
  UpdateResult result;
  no_new([&] { result = s.h.update_prepared(edit.generation, &edit, 1); });
  require(result.status == UpdateStatus::Ok, "source edit rejected");
  const auto state = s.hash();
  const auto polls = b.polls;
  bool threw = false;
  try {
    b.step();
  } catch (const std::logic_error &) {
    threw = true;
  }
  require(threw && b.polls == polls && s.hash() == state && s.h.ready(),
          "stale source edit was polled before rebind");
  s.refresh_spec();
  require(b.bind(s.spec()), "source refresh rebind failed");
  b.seed(true, 1);
  b.settle(1);
  b.candidate_check();
  std::cout << "source_edit,stale_poll_rejected=1,rebound=1,source_preserved=1 "
               "PASS\n";
  Parameters64 inert;
  inert.gravity = inert.contact_stiffness = inert.reach_strength = 0;
  no_new([&] { s.h.tick64(inert); });
  const auto after_tick = s.hash();
  const auto before_poll = b.polls;
  threw = false;
  try {
    b.step();
  } catch (const std::logic_error &) {
    threw = true;
  }
  require(threw && b.polls == before_poll && s.hash() == after_tick,
          "source-only tick was coupled to a stale held-input snapshot");
  require(b.bind(s.spec()), "rebind after source-only tick failed");
  std::cout << "source_tick,stale_snapshot_rejected=1,rebound=1 PASS\n";
}
void matrix(Probe &p, Source &s) {
  Bound b(p, s);
  const auto bind_start = std::chrono::steady_clock::now();
  require(b.bind(s.spec()), "valid source hierarchy rejected");
  std::cout << "first_bind,name=" << s.g.name << ",elapsed_us="
            << std::chrono::duration<double, std::micro>(
                   std::chrono::steady_clock::now() - bind_start)
                   .count()
            << '\n';
  prepared_test::Metrics metrics(p.r, PreparedFixtureAccess::nodes(s.h));
  metrics.nodes.insert(
      metrics.nodes.end(),
      {p.awake.cells->parent->parent, p.awake.cells->parent, p.awake.cells});
  const auto before = metrics.sample();
  for (bool all : {false, true})
    for (int repeat : {1, 64})
      for (int cancel = 0; cancel < (s.g.name == "two_leaf" ? 2 : 1);
           ++cancel) {
        b.polls = b.work = b.readback_bytes = b.snapshots = b.initializations =
            b.query_bytes = 0;
        b.kernel_us = b.inspection_us = b.first_poll_us = b.initialization_us =
            0;
        int maximum = 0;
        std::uint64_t pubs = 0, notify = 0;
        const auto start = std::chrono::steady_clock::now();
        for (int round = 0; round < 32; ++round) {
          const int sign = round % 2 ? -1 : 1;
          s.state(0, cancel, sign);
          b.snapshot();
          b.initialize();
          for (int stage = 1; stage <= 3; ++stage) {
            s.state(stage, cancel, sign);
            b.snapshot();
            b.seed(all, repeat);
            b.trace =
                round == 0 && stage == 1 && !all && repeat == 1 && !cancel;
            maximum = std::max(maximum, b.settle(repeat));
            b.trace = false;
            if (round == 0) {
              b.candidate_check();
              const auto actual =
                  selected_packet(p.values, p.selectors, s.g.rows.back());
              const auto ref = b.reference[s.g.rows.size() - 1];
              if (s.g.name == "quarters" && stage == 1)
                require(
                    actual[0] == 1.1875,
                    "unequal normalized contributions did not cross root Q");
              std::cout << "weighted_root,name=" << s.g.name
                        << ",cancel=" << cancel << ",stage=" << stage
                        << ",x=" << actual[0] << ",y=" << actual[1]
                        << ",z=" << actual[2] << ",mass=" << actual[3]
                        << ",reference_x=" << ref[0]
                        << ",reference_mass=" << ref[3] << '\n';
            }
          }
          pubs += std::accumulate(p.diagnostic.begin() + Publications * N,
                                  p.diagnostic.begin() + (Publications + 1) * N,
                                  std::uint64_t{});
          notify += std::accumulate(
              p.diagnostic.begin() + Notifications * N,
              p.diagnostic.begin() + (Notifications + 1) * N, std::uint64_t{});
          if (s.g.equal_positions) {
            for (std::size_t i = 0; i < s.g.rows.size(); ++i) {
              const auto v =
                  selected_packet(p.values, p.selectors, s.g.rows[i]);
              require(v[0] == sign * 1.125,
                      "mass-only fixture changed position");
            }
            require(pubs > 0 && notify > 0,
                    "mass-only publication/notification missing");
          }
        }
        require(
            before.same_backing(metrics.sample()),
            "prepared weighted probe acquired backing/layout/compiler state");
        std::cout << "weighted_case,name=" << s.g.name << ",all_active=" << all
                  << ",repeat=" << repeat << ",cancel=" << cancel
                  << ",rounds=32,inputs=96,max_settle=" << maximum
                  << ",polls=" << b.polls << ",visits=" << b.work
                  << ",publications=" << pubs << ",notifications=" << notify
                  << ",kernel_us=" << b.kernel_us
                  << ",inspection_us=" << b.inspection_us
                  << ",first_poll_us=" << b.first_poll_us
                  << ",initialization_us=" << b.initialization_us
                  << ",snapshots=" << b.snapshots
                  << ",initializations=" << b.initializations
                  << ",source_query_readback_bytes=" << b.query_bytes
                  << ",readback_bytes=" << b.readback_bytes << ",elapsed_us="
                  << std::chrono::duration<double, std::micro>(
                         std::chrono::steady_clock::now() - start)
                         .count()
                  << ",cpp_new=0 PASS\n";
      }
  before.print("source_probe_before");
  metrics.sample().print("source_probe_after");
}
}  // namespace
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings s;
    s.read_environment = false;
    bool direct = false;
    std::string filter;
    int threads = 1;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--graph" && i + 1 < argc)
        filter = argv[++i];
      else if (arg == "--threads" && i + 1 < argc)
        threads = std::stoi(argv[++i]);
      else
        throw std::invalid_argument("unknown weighted handoff option");
    }
    require(threads == 1 || threads == 2, "one/two native workers required");
    s.cpu_threads = threads;
    engine::Runtime r(s);
    const auto probe_start = std::chrono::steady_clock::now();
    Probe p(r);
    require(!r.on_cuda(), "actual CPU backend required");
    std::cout << std::setprecision(17)
              << "actual_backend=cpu,threads=" << threads
              << ",path=" << (direct ? "direct" : "bulk") << '\n';
    std::cout << "probe_setup,elapsed_us="
              << std::chrono::duration<double, std::micro>(
                     std::chrono::steady_clock::now() - probe_start)
                     .count()
              << '\n';
    bool found = false;
    const auto fixtures = graphs();
    const auto graph_bytes = [](const Graph &g) {
      std::size_t size =
          (g.rows.capacity() + g.bits.capacity() + g.levels.capacity()) *
              sizeof(int) +
          g.denominators.capacity() * 8;
      for (const auto &v : g.children)
        size += v.capacity() * sizeof(int);
      for (const auto &v : g.coverage)
        size += v.capacity() * 8;
      return size + g.children.capacity() * sizeof(std::vector<int>) +
             g.coverage.capacity() * sizeof(std::vector<std::uint64_t>);
    };
    std::size_t input_bytes = fixtures.capacity() * sizeof(Graph);
    for (const auto &g : fixtures)
      input_bytes += graph_bytes(g);
    std::cout << "input_fixtures,graphs=" << fixtures.size()
              << ",host_payload_and_headers=" << input_bytes << '\n';
    for (const auto &g : fixtures) {
      if (!filter.empty() && g.name != filter)
        continue;
      found = true;
      const auto source_start = std::chrono::steady_clock::now();
      Source source(r, direct ? HostPath::CpuDirect : HostPath::BulkCopy, g);
      const double setup_us =
          std::chrono::duration<double, std::micro>(
              std::chrono::steady_clock::now() - source_start)
              .count();
      if (g.name == "pair_3_4") {
        for (const auto &e : source.h.prepared_storage().manifest)
          std::cout << "source_manifest,name=" << e.name << ",count=" << e.count
                    << ",element_bytes=" << e.element_bytes
                    << ",copies=" << e.copies << ",bytes=" << e.bytes << '\n';
        for (const auto &e :
             source.h.prepared_storage().capacity.numerical_shapes())
          std::cout << "source_shape,name=" << e.name << ",rows=" << e.rows
                    << ",stride=" << e.stride
                    << ",element_bytes=" << e.element_bytes
                    << ",bytes=" << e.bytes << '\n';
      }
      std::size_t fixture_bytes =
          source.nodes.capacity() * sizeof(CascadeNode) +
          source.children.capacity() * sizeof(int) +
          source.refs.capacity() * sizeof(CascadeContribution);
      fixture_bytes +=
          (source.in.keys.capacity() + source.in.kinds.capacity() +
           source.in.focus.capacity()) *
              sizeof(int) +
          (source.in.particle_rows.capacity() +
           source.in.field_rows.capacity() + source.in.retire.capacity()) *
              sizeof(engine::RecordHandle);
      std::cout
          << "fixture_payload,name=" << g.name << ",setup_us=" << setup_us
          << ",source_tables_bytes=" << fixture_bytes
          << ",source_graph_copy_payload_and_inner_headers="
          << graph_bytes(source.g) << ",source_scratch_bytes="
          << source.h.prepared_storage().capacity.limits()[Budget::ScratchBytes]
          << ",source_bindings=" << source.h.prepared_storage().binding_bytes
          << '\n';
      prepared_test::Metrics first_use(r,
                                       PreparedFixtureAccess::nodes(source.h));
      first_use.nodes.insert(first_use.nodes.end(),
                             {p.awake.cells->parent->parent,
                              p.awake.cells->parent, p.awake.cells});
      const auto prepared = first_use.sample();
      if (g.name == "two_leaf")
        controls(p, source);
      matrix(p, source);
      require(
          prepared.same_backing(first_use.sample()),
          "first bind/control/reuse acquired native backing or compilation");
      prepared.print("first_use_before");
      first_use.sample().print("first_use_after");
    }
    require(found, "unknown graph filter");
    std::cout << "fixed_objects,Probe=" << sizeof(Probe)
              << ",Source=" << sizeof(Source) << ",Bound=" << sizeof(Bound)
              << ",graph_input_table_bytes=" << sizeof(Graph) << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
