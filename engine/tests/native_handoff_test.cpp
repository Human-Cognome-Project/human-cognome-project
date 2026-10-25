#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "allocation_observation.h"
#include "engine/transfer.h"
#include "field/native_set.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "prepared_metrics.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/program/kernel.h"
#include "taichi/runtime/llvm/kernel_launcher.h"

namespace {
using namespace taichi::lang;
// C2a diagnostic packets (t,2t,3t,4t) expose incomplete XYZ/mass-shaped reads.
// This probe applies no centroid/force equation and changes no live engine
// path.
constexpr int N = 70, Links = 2 * N;
enum Diagnostic {
  Visits,
  Publications,
  Propagations,
  Reads,
  Torn,
  Collisions,
  DCount
};
enum Command { Reset, Seed, Activate, Consume, Piece, Observe, ClearVisits };
enum Poll { CalculateOnly = 0, ConsumeFirst = 5 };
void require(bool ok, const char *why) {
  if (!ok) throw std::runtime_error(why);
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
          "probe reuse allocated linked C++ metadata");
}
ArgLoadStmt *array(IRBuilder &b, int id, DataType t, int rank = 1) {
  return b.create_ndarray_arg_load({id}, t, rank, 0);
}
Stmt *scalar(IRBuilder &b, int id, DataType t) {
  return b.create_arg_load({id}, t, false, 0);
}
Stmt *load(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at) {
  return b.create_global_load(b.create_external_ptr(a, at));
}
void store(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> at, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
std::vector<Stmt *> cell(IRBuilder &b, int f, Stmt *row) {
  return {b.get_int32(f), row};
}
void count(IRBuilder &b, ArgLoadStmt *d, int f, Stmt *row) {
  b.create_atomic_add(b.create_external_ptr(d, cell(b, f, row)),
                      b.get_int64(1));
}
Stmt *selected(IRBuilder &b, ArgLoadStmt *s, Stmt *row) {
  // Use the native RMW's returned value as an acquire read, not an ordinary
  // load. The compiled-LLVM audit below requires this to survive optimization.
  return b.create_atomic_or(b.create_external_ptr(s, {row}), b.get_int32(0));
}
std::array<Stmt *, 4> read_tuple(IRBuilder &b, ArgLoadStmt *v, ArgLoadStmt *s,
                                 Stmt *row, ArgLoadStmt *d) {
  auto *bank = selected(b, s, row);
  std::array<Stmt *, 4> out;
  for (int a = 0; a < 4; ++a)
    out[a] =
        load(b, v,
             {b.create_add(b.create_mul(bank, b.get_int32(4)), b.get_int32(a)),
              row});
  count(b, d, Reads, row);
  Stmt *bad = b.get_int32(0);
  for (int a = 1; a < 4; ++a)
    bad = b.create_logical_or(
        bad,
        b.create_cmp_ne(out[a], b.create_mul(out[0], b.get_float64(a + 1))));
  auto guard = b.get_if_guard(b.create_if(bad), true);
  count(b, d, Torn, row);
  return out;
}
struct Args {
  ArgLoadStmt *values, *selector, *topology, *children, *desired, *diagnostic,
      *changed, *observed, *active;
  Stmt *op, *row, *value, *piece, *repeat;
  explicit Args(IRBuilder &b) {
    values = array(b, 0, PrimitiveType::f64, 2);
    selector = array(b, 1, PrimitiveType::i32);
    topology = array(b, 2, PrimitiveType::i32, 2);
    children = array(b, 3, PrimitiveType::i32);
    desired = array(b, 4, PrimitiveType::f64);
    diagnostic = array(b, 5, PrimitiveType::i64, 2);
    changed = array(b, 6, PrimitiveType::i32);
    observed = array(b, 7, PrimitiveType::f64, 2);
    active = array(b, 8, PrimitiveType::i32);
    op = scalar(b, 9, PrimitiveType::i32);
    row = scalar(b, 10, PrimitiveType::i32);
    value = scalar(b, 11, PrimitiveType::f64);
    piece = scalar(b, 12, PrimitiveType::i32);
    repeat = scalar(b, 13, PrimitiveType::i32);
  }
};
std::vector<engine::Param> parameters() {
  using engine::Param;
  return {
      Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::i32, 2), Param::array(PrimitiveType::i32),
      Param::array(PrimitiveType::f64),    Param::array(PrimitiveType::i64, 2),
      Param::array(PrimitiveType::i32),    Param::array(PrimitiveType::f64, 2),
      Param::array(PrimitiveType::i32),    Param::scalar(PrimitiveType::i32),
      Param::scalar(PrimitiveType::i32),   Param::scalar(PrimitiveType::f64),
      Param::scalar(PrimitiveType::i32),   Param::scalar(PrimitiveType::i32)};
}
// Test-only use of the same native compile/prepare/launch facilities. Keeping
// the compiled module accessible lets this investigation audit its actual
// synchronization instructions without extending the production wrapper API.
class ProbeKernel {
 public:
  engine::Runtime &runtime;
  std::unique_ptr<Kernel> kernel;
  const CompiledKernelData *compiled{};
  std::unique_ptr<LaunchContextBuilder> context;
  ProbeKernel(engine::Runtime &r, const std::string &name,
              std::unique_ptr<Block> ir)
      : runtime(r) {
    kernel = std::make_unique<Kernel>(r.program(), std::move(ir), name);
    for (const auto &p : parameters()) {
      if (p.is_array)
        kernel->insert_ndarray_param(p.dt, p.total_dim);
      else
        kernel->insert_scalar_param(p.dt);
    }
    kernel->finalize_params();
    kernel->finalize_rets();
    auto config = r.program().compile_config();
    config.fast_math = false;
    compiled = &r.program().compile_kernel(
        config, r.program().get_device_caps(), *kernel);
    context =
        std::make_unique<LaunchContextBuilder>(kernel->make_launch_context());
  }
  void bind(const std::array<Ndarray *, 9> &arrays) {
    for (int i = 0; i < 9; ++i) context->set_arg_ndarray({i}, *arrays[i]);
    for (int i : {9, 10, 12, 13}) context->set_arg_int({i}, 0);
    context->set_arg_float({11}, 0.);
  }
  void launch(int op = 0, int row = 0, double value = 0, int piece = 0,
              int repeat = 1) {
    engine::argument_int(*context, 9, op);
    engine::argument_int(*context, 10, row);
    engine::argument_float(*context, 11, value);
    engine::argument_int(*context, 12, piece);
    engine::argument_int(*context, 13, repeat);
    runtime.program().launch_kernel(*compiled, *context);
    if (!context->prepared)
      dynamic_cast<LLVM::KernelLauncher &>(
          runtime.program().get_program_impl()->get_kernel_launcher())
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
  void audit() const {
    const auto &data = dynamic_cast<const LLVM::CompiledKernelData &>(*compiled)
                           .get_internal_data();
    int acquire = 0, publish = 0;
    bool all_sc = true;
    for (const auto &fn : *data.compiled_data.module) {
      for (const auto &block : fn)
        for (const auto &instruction : block)
          if (const auto *a =
                  llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction)) {
            const auto *v =
                llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand());
            if (!v || !v->getType()->isIntegerTy(32)) continue;
            const bool read =
                a->getOperation() == llvm::AtomicRMWInst::Or && v->isZero();
            const bool flip = a->getOperation() == llvm::AtomicRMWInst::Xor &&
                              v->equalsInt(1);
            if (read || flip) {
              acquire += read;
              publish += flip;
              all_sc &= a->getOrdering() ==
                        llvm::AtomicOrdering::SequentiallyConsistent;
              std::string line;
              llvm::raw_string_ostream os(line);
              a->print(os);
              std::cout << "native_ir," << fn.getName().str() << ',' << os.str()
                        << '\n';
            }
          }
    }
    require(acquire >= 2 && publish >= 1 && all_sc,
            "native selector atomics were demoted or lost their SC ordering");
    std::cout << "atomic_audit,acquire_sites=" << acquire
              << ",publication_sites=" << publish << ",all_seq_cst=1 PASS\n";
  }
};
struct Graph {
  std::string name;
  std::vector<int> nodes, leaves;
  std::vector<std::pair<int, int>> edges;
};
class Probe {
 public:
  engine::Runtime &r;
  field::detail::NativeSet set;
  std::unique_ptr<Ndarray> values, selector, topology, children, desired,
      diagnostic, changed, observed, active;
  std::vector<double> host_values = std::vector<double>(8 * N),
                      host_observed = std::vector<double>(4 * N);
  std::vector<int> host_selector = std::vector<int>(N),
                   host_active = std::vector<int>(N),
                   host_topology = std::vector<int>(3 * N),
                   host_children = std::vector<int>(Links);
  std::vector<std::int64_t> host_diagnostic =
      std::vector<std::int64_t>(DCount * N);
  std::unique_ptr<ProbeKernel> command, poll, propagate, inspect;
  explicit Probe(engine::Runtime &runtime) : r(runtime) {
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    set = field::detail::NativeSet::attach(*root, N);
    r.program().add_snode_tree(std::move(root), false);
    const auto make = [&](DataType t, std::vector<int> shape) {
      return std::make_unique<Ndarray>(&r.program(), t, shape);
    };
    values = make(PrimitiveType::f64, {8, N});
    selector = make(PrimitiveType::i32, {N});
    topology = make(PrimitiveType::i32, {3, N});
    children = make(PrimitiveType::i32, {Links});
    desired = make(PrimitiveType::f64, {N});
    diagnostic = make(PrimitiveType::i64, {DCount, N});
    changed = make(PrimitiveType::i32, {N});
    observed = make(PrimitiveType::f64, {4, N});
    active = make(PrimitiveType::i32, {N});
    configure({"warm", {}, {}, {}});
    compile();
    const std::array<Ndarray *, 9> arrays{
        values.get(),   selector.get(), topology.get(),
        children.get(), desired.get(),  diagnostic.get(),
        changed.get(),  observed.get(), active.get()};
    for (auto *k : {command.get(), poll.get(), propagate.get(), inspect.get()})
      k->bind(arrays);
    poll->audit();
    command->launch(Reset);
    for (int i = 0; i < N; ++i) command->launch(Seed, i, 2.);
    poll->launch(ConsumeFirst);
    propagate->launch();
    inspect->launch();
    r.synchronize();
    command->launch(Reset);
    readback();
  }
  void configure(const Graph &graph) {
    std::fill(host_topology.begin(), host_topology.end(), 0);
    std::fill_n(host_topology.begin(), N, -1);
    std::fill(host_children.begin(), host_children.end(), 0);
    int k = 0;
    for (int parent = 0; parent < N; ++parent) {
      host_topology[N + parent] = k;
      for (const auto &e : graph.edges)
        if (e.second == parent) {
          require(k < Links, "probe adjacency overflow");
          host_children[k++] = e.first;
          host_topology[e.first] = parent;
          ++host_topology[2 * N + parent];
        }
    }
    engine::upload(*topology, host_topology.data(),
                   host_topology.size() * sizeof(int));
    engine::upload(*children, host_children.data(),
                   host_children.size() * sizeof(int));
  }
  void compile() {
    const int threads = r.cpu_threads();
    const auto name = std::to_string(set.cells->id);
    {
      IRBuilder b;
      Args a(b);
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Reset))), true);
        auto *loop =
            b.create_range_for(b.get_int32(0), b.get_int32(N), false, threads);
        auto body = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        set.deactivate(b, row);
        for (int bank = 0; bank < 2; ++bank)
          for (int axis = 0; axis < 4; ++axis)
            store(b, a.values, cell(b, bank * 4 + axis, row),
                  b.get_float64(axis + 1));
        store(b, a.selector, {row}, b.get_int32(0));
        store(b, a.desired, {row}, b.get_float64(1));
        store(b, a.changed, {row}, b.get_int32(0));
        for (int f = 0; f < DCount; ++f)
          store(b, a.diagnostic, cell(b, f, row), b.get_int64(0));
        for (int f = 0; f < 4; ++f)
          store(b, a.observed, cell(b, f, row), b.get_float64(0));
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(ClearVisits))), true);
        auto *loop =
            b.create_range_for(b.get_int32(0), b.get_int32(N), false, threads);
        auto body = b.get_loop_guard(loop);
        store(b, a.diagnostic, cell(b, Visits, b.get_loop_index(loop)),
              b.get_int64(0));
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Seed))), true);
        store(b, a.desired, {a.row}, a.value);
        set.activate(b, a.row);
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Activate))), true);
        auto *loop =
            b.create_range_for(b.get_int32(0), a.repeat, false, threads);
        auto body = b.get_loop_guard(loop);
        set.activate(b, a.row);
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Consume))), true);
        set.deactivate(b, a.row);
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Piece))), true);
        store(b, a.values, {a.piece, a.row},
              b.create_mul(a.value,
                           b.create_cast(b.create_add(a.piece, b.get_int32(1)),
                                         PrimitiveType::f64)));
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_eq(a.op, b.get_int32(Observe))), true);
        const auto tuple =
            read_tuple(b, a.values, a.selector, a.row, a.diagnostic);
        for (int f = 0; f < 4; ++f)
          store(b, a.observed, cell(b, f, a.row), tuple[f]);
      }
      command = std::make_unique<ProbeKernel>(r, "handoff_command_" + name,
                                              b.extract_ir());
    }
    {
      IRBuilder b;
      Args a(b);
      // One native traversal visits each record at most once per poll. Consume
      // before acquiring inputs. Write the other numerical publication slot,
      // release its selector, then notify the parent. Both slots remain intact
      // until this dispatch's readers finish; only the following poll may reuse
      // an old slot. The selector addresses data, never runnable work.
      auto *loop = b.create_struct_for(set.cells, false, threads, 1);
      auto body = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_and(a.op, b.get_int32(1))), true);
        set.deactivate(b, row);
      }
      count(b, a.diagnostic, Visits, row);
      store(b, a.changed, {row}, b.get_int32(0));
      auto *old_selector = selected(b, a.selector, row);
      auto *old =
          load(b, a.values, {b.create_mul(old_selector, b.get_int32(4)), row});
      auto *tag = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(tag, load(b, a.desired, {row}));
      auto *length = load(b, a.topology, cell(b, 2, row));
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_gt(length, b.get_int32(0))), true);
        b.create_local_store(tag, b.get_float64(1));
        auto *members =
            b.create_range_for(b.get_int32(0), length, false, 1, 0, true);
        auto member = b.get_loop_guard(members);
        auto *child = load(b, a.children,
                           {b.create_add(load(b, a.topology, cell(b, 1, row)),
                                         b.get_loop_index(members))});
        const auto tuple =
            read_tuple(b, a.values, a.selector, child, a.diagnostic);
        for (int f = 0; f < 4; ++f)
          store(b, a.observed, cell(b, f, row), tuple[f]);
        b.create_local_store(tag,
                             b.create_max(b.create_local_load(tag), tuple[0]));
      }
      {
        auto guard = b.get_if_guard(
            b.create_if(b.create_cmp_ne(old, b.create_local_load(tag))), true);
        auto *next = b.create_xor(old_selector, b.get_int32(1));
        for (int f = 0; f < 4; ++f)
          store(
              b, a.values,
              {b.create_add(b.create_mul(next, b.get_int32(4)), b.get_int32(f)),
               row},
              b.create_mul(b.create_local_load(tag), b.get_float64(f + 1)));
        auto *prior = b.create_atomic_xor(
            b.create_external_ptr(a.selector, {row}), b.get_int32(1));
        {
          auto bad = b.get_if_guard(
              b.create_if(b.create_cmp_ne(prior, old_selector)), true);
          count(b, a.diagnostic, Collisions, row);
        }
        count(b, a.diagnostic, Publications, row);
        store(b, a.changed, {row}, b.get_int32(1));
        auto *parent = load(b, a.topology, cell(b, 0, row));
        auto notify =
            b.get_if_guard(b.create_if(b.create_logical_and(
                               b.create_and(a.op, b.get_int32(4)),
                               b.create_cmp_ge(parent, b.get_int32(0)))),
                           true);
        auto *repeat =
            b.create_range_for(b.get_int32(0), a.repeat, false, 1, 0, true);
        auto repeated = b.get_loop_guard(repeat);
        set.activate(b, parent);
        count(b, a.diagnostic, Propagations, row);
      }
      poll = std::make_unique<ProbeKernel>(r, "handoff_poll_" + name,
                                           b.extract_ir());
    }
    {
      IRBuilder b;
      Args a(b);
      auto *loop = b.create_struct_for(set.cells, false, 1, 1);
      auto body = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      set.deactivate(b, row);
      count(b, a.diagnostic, Propagations, row);
      auto *parent = load(b, a.topology, cell(b, 0, row));
      auto guard = b.get_if_guard(b.create_if(b.create_logical_and(
                                      load(b, a.changed, {row}),
                                      b.create_cmp_ge(parent, b.get_int32(0)))),
                                  true);
      set.activate(b, parent);
      propagate = std::make_unique<ProbeKernel>(
          r, "handoff_unsafe_propagate_" + name, b.extract_ir());
    }
    {
      IRBuilder b;
      Args a(b);
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(N), false, threads);
      auto body = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      store(b, a.active, {row},
            b.create_cast(set.active(b, row), PrimitiveType::i32));
      inspect = std::make_unique<ProbeKernel>(r, "handoff_inspect_" + name,
                                              b.extract_ir());
    }
  }
  void readback() {
    inspect->launch();
    r.synchronize();
    engine::readback(*values, host_values.data(),
                     host_values.size() * sizeof(double));
    engine::readback(*selector, host_selector.data(),
                     host_selector.size() * sizeof(int));
    engine::readback(*diagnostic, host_diagnostic.data(),
                     host_diagnostic.size() * sizeof(std::int64_t));
    engine::readback(*observed, host_observed.data(),
                     host_observed.size() * sizeof(double));
    engine::readback(*active, host_active.data(),
                     host_active.size() * sizeof(int));
  }
  double value(int row, int axis = 0) const {
    return host_values[(host_selector[row] * 4 + axis) * N + row];
  }
  void complete(int row, double tag) {
    for (int a = 0; a < 4; ++a) command->launch(Piece, row, tag, a);
  }
  std::int64_t total(int f) const {
    return std::accumulate(host_diagnostic.begin() + f * N,
                           host_diagnostic.begin() + (f + 1) * N,
                           std::int64_t(0));
  }
  int awake() const {
    return std::accumulate(host_active.begin(), host_active.end(), 0);
  }
  void check_coherence() const {
    require(!total(Torn) && !total(Collisions),
            "publication was torn or had multiple writers");
    for (int i = 0; i < N; ++i)
      for (int a = 1; a < 4; ++a)
        require(value(i, a) == value(i) * double(a + 1),
                "published tuple is incomplete");
  }
};

void controls(Probe &p) {
  p.configure({"same-word", {3, 4}, {3}, {{3, 4}}});
  no_new([&] {
    p.command->launch(Reset);
    p.command->launch(Seed, 3, 2);
    p.poll->launch(CalculateOnly);
    p.propagate->launch();
    p.readback();
  });
  require(p.value(3) == 2 && p.value(4) == 1 && !p.awake() &&
              p.host_diagnostic[Visits * N + 4] == 0 &&
              p.host_diagnostic[Propagations * N + 4] == 1,
          "negative control did not expose propagation-only notification "
          "consumption");
  std::cout
      << "negative_control,propagation_only,child=3,parent=4,parent_"
         "calculations=0,parent_propagation_visits=1,awake=0,stranded=1 PASS\n";
  no_new([&] {
    p.command->launch(Reset);
    p.command->launch(Activate, 4);
    p.poll->launch(CalculateOnly);
    p.complete(3, 2);
    p.command->launch(Activate, 4);
    p.command->launch(Consume, 4);
    p.readback();
  });
  require(p.value(4) == 1 && p.value(3) == 2 && !p.awake(),
          "negative control did not expose late consumption of already-active "
          "parent");
  std::cout << "negative_control,already_active_parent,read=1,child_"
               "publication=2,late_consume=1,awake=0,stranded=1 PASS\n";
  no_new([&] {
    p.command->launch(Reset);
    p.command->launch(Piece, 3, 2, 0);
    p.command->launch(Observe, 3);
    p.readback();
  });
  require(p.total(Torn) == 1 && p.host_observed[3] == 2 &&
              p.host_observed[N + 3] == 2,
          "negative control did not expose partial in-place publication");
  std::cout << "negative_control,inplace_partial,tuple=2/2/3/4,complete_old=1/"
               "2/3/4,complete_new=2/4/6/8,torn=1 PASS\n";
  // Notifications on either side of the consume/read cut, with complete
  // publication. These controlled dispatches are evidence, not a runtime
  // host/per-depth scheduler.
  for (bool before : {true, false}) {
    no_new([&] {
      p.command->launch(Reset);
      p.command->launch(Activate, 4);
      if (before) {
        p.complete(3, 2);
        p.command->launch(Activate, 4);
        p.poll->launch(ConsumeFirst);
      } else {
        p.poll->launch(ConsumeFirst);
        p.complete(3, 2);
        p.command->launch(Activate, 4);
      }
      p.readback();
    });
    require(before ? (p.value(4) == 2 && !p.awake())
                   : (p.value(4) == 1 && p.host_active[4]),
            "complete notification neither evaluated nor left pending");
    if (!before) {
      no_new([&] {
        p.poll->launch(ConsumeFirst);
        p.readback();
      });
      require(p.value(4) == 2 && !p.awake(),
              "pending publication was stranded");
    }
    p.check_coherence();
    std::cout << "ordered_handoff,publication_" << (before ? "before" : "after")
              << "_consume,delivered=1,quiet_value=2 PASS\n";
  }
  no_new([&] {
    p.command->launch(Reset);
    p.command->launch(Activate, 69, 0, 0, 4096);
    p.readback();
  });
  require(p.awake() == 1 && p.host_active[69],
          "duplicate activations did not collapse at final valid row");
  std::cout << "native_duplicates,notifications=4096,awake=1,record=69 PASS\n";
}
std::vector<Graph> graphs() {
  std::vector<Graph> out;
  for (auto e : std::vector<std::pair<int, int>>{{3, 4},
                                                 {4, 3},
                                                 {31, 32},
                                                 {32, 31},
                                                 {3, 64},
                                                 {64, 3},
                                                 {1, 69},
                                                 {69, 1}})
    out.push_back(
        {"pair-" + std::to_string(e.first) + "-" + std::to_string(e.second),
         {e.first, e.second},
         {e.first},
         {e}});
  out.push_back({"fan-in",
                 {0, 1, 31, 32, 64, 69, 33},
                 {0, 1, 31, 32, 64, 69},
                 {{0, 33}, {1, 33}, {31, 33}, {32, 33}, {64, 33}, {69, 33}}});
  for (const auto &nodes :
       std::vector<std::vector<int>>{{3, 4, 33, 64, 69}, {69, 64, 33, 4, 3}}) {
    Graph g{
        "chain-" + std::to_string(nodes.front()), nodes, {nodes.front()}, {}};
    for (int i = 0; i < 4; ++i) g.edges.push_back({nodes[i], nodes[i + 1]});
    out.push_back(g);
  }
  Graph tree{"permuted-tree", {}, {}, {}};
  const auto row = [](int logical) { return (logical * 37 + 13) % N; };
  for (int i = 0; i < 63; ++i) {
    tree.nodes.push_back(row(i));
    if (i >= 31) tree.leaves.push_back(row(i));
    if (i) tree.edges.push_back({row(i), row((i - 1) / 2)});
  }
  out.push_back(tree);
  return out;
}
void matrix(Probe &p, const std::vector<Graph> &topology) {
  for (const auto &g : topology)
    for (bool initially_all : {false, true})
      for (int repeat : {1, 64}) {
        p.configure(g);
        std::uint64_t max_polls = 0, recovery_max = 0, sum_polls = 0, work = 0,
                      pub = 0, notifies = 0, reads = 0, poll_calls = 0;
        double kernel_us = 0, inspection_us = 0, first_poll_us = 0;
        const auto step = [&](int repeats) {
          p.command->launch(ClearVisits);
          const auto t = std::chrono::steady_clock::now();
          p.poll->launch(ConsumeFirst, 0, 0, 0, repeats);
          p.r.synchronize();
          const auto time = std::chrono::duration<double, std::micro>(
                                std::chrono::steady_clock::now() - t)
                                .count();
          kernel_us += time;
          if (!poll_calls) first_poll_us = time;
          ++poll_calls;
          const auto obs = std::chrono::steady_clock::now();
          p.readback();
          inspection_us += std::chrono::duration<double, std::micro>(
                               std::chrono::steady_clock::now() - obs)
                               .count();
          work += p.total(Visits);
        };
        const auto start = std::chrono::steady_clock::now();
        for (int round = 0; round < 32; ++round) {
          no_new([&] {
            p.command->launch(Reset);
            if (initially_all)
              for (int node : g.nodes) p.command->launch(Activate, node);
            for (int leaf : g.leaves) p.command->launch(Seed, leaf, 2);
          });
          int cycles = 0;
          for (; cycles < 16; ++cycles) {
            no_new([&] { step(repeat); });
            require(
                *std::max_element(p.host_diagnostic.begin(),
                                  p.host_diagnostic.begin() + N) <= 1,
                "native traversal evaluated a record twice within one poll");
            p.check_coherence();
            if (!p.awake()) break;
          }
          require(cycles < 16, "bounded native handoff failed to become quiet");
          for (int node : g.nodes)
            require(p.value(node) == 2,
                    "consequential publication stranded behind quiet mask");
          for (int node = 0; node < N; ++node)
            if (std::find(g.nodes.begin(), g.nodes.end(), node) ==
                g.nodes.end())
              require(p.value(node) == 1,
                      "handoff changed an untouched numerical record");
          max_polls = std::max(max_polls, std::uint64_t(cycles + 1));
          sum_polls += cycles + 1;
          no_new([&] { step(1); });
          require(!p.total(Visits) && !p.awake(),
                  "quiet native state did not persist");
          // Reuse both numerical publication slots: perturb the same leaves
          // again without reset, while all other numerical state is retained.
          no_new([&] {
            for (int leaf : g.leaves) p.command->launch(Seed, leaf, 3);
          });
          for (cycles = 0; cycles < 16; ++cycles) {
            no_new([&] { step(repeat); });
            p.check_coherence();
            require(*std::max_element(p.host_diagnostic.begin(),
                                      p.host_diagnostic.begin() + N) <= 1,
                    "recovery evaluated a record twice");
            if (!p.awake()) break;
          }
          require(cycles < 16, "second publication failed to resolve");
          for (int node : g.nodes)
            require(p.value(node) == 3,
                    "reused publication slot lost a later input");
          recovery_max = std::max(recovery_max, std::uint64_t(cycles + 1));
          pub += p.total(Publications);
          notifies += p.total(Propagations);
          reads += p.total(Reads);
        }
        const auto us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - start)
                            .count();
        std::cout << "handoff_case," << g.name
                  << ",initial=" << (initially_all ? "all" : "leaves")
                  << ",repeats=" << repeat
                  << ",rounds=32,max_polls=" << max_polls
                  << ",recovery_max_polls=" << recovery_max
                  << ",sum_polls=" << sum_polls << ",body_visits=" << work
                  << ",publications=" << pub << ",notifications=" << notifies
                  << ",input_reads=" << reads
                  << ",torn=0,lost=0,elapsed_us=" << us
                  << ",first_poll_us=" << first_poll_us
                  << ",poll_calls=" << poll_calls << ",kernel_us=" << kernel_us
                  << ",inspection_us=" << inspection_us
                  << ",inspection_readback_bytes=" << poll_calls * 10640
                  << ",configuration_upload_bytes=1400,cpp_new=0 PASS\n";
      }
}
}  // namespace
int main(int argc, char **argv) {
  try {
    int threads = 1;
    if (argc == 3 && std::string(argv[1]) == "--threads")
      threads = std::stoi(argv[2]);
    else if (argc != 1)
      throw std::invalid_argument("expected --threads 1|2");
    require(threads == 1 || threads == 2, "unsupported worker count");
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = threads;
    engine::Runtime runtime(settings);
    require(!runtime.on_cuda(), "C2a probe requires actual CPU backend");
    std::cout << std::setprecision(17)
              << "actual_backend=cpu,threads=" << threads << ",capacity=" << N
              << ",native_activity_sets=1\n";
    const auto setup = std::chrono::steady_clock::now();
    Probe probe(runtime);
    const auto topology = graphs();
    const auto setup_us = std::chrono::duration<double, std::micro>(
                              std::chrono::steady_clock::now() - setup)
                              .count();
    prepared_test::Metrics metrics(
        runtime, {probe.set.cells->parent->parent, probe.set.cells->parent,
                  probe.set.cells});
    const auto before = metrics.sample();
    controls(probe);
    matrix(probe, topology);
    const auto after = metrics.sample();
    require(before.same_backing(after),
            "C2a first use/reuse changed known backing/layout/compiler state");
    before.print("after_setup");
    after.print("after_handoff_cycles");
    const auto binding =
        probe.command->binding_bytes() + probe.poll->binding_bytes() +
        probe.propagate->binding_bytes() + probe.inspect->binding_bytes();
    std::cout << "manifest,publication-values," << 8 * N << ",8,2,"
              << 16 * N * 8 << '\n';
    std::cout << "manifest,publication-selector," << N << ",4,2," << N * 8
              << '\n';
    std::cout << "manifest,topology," << 3 * N << ",4,2," << 6 * N * 4 << '\n';
    std::cout << "manifest,child-adjacency," << Links << ",4,2," << Links * 8
              << '\n';
    std::cout << "manifest,leaf-input," << N << ",8,1," << N * 8 << '\n';
    std::cout << "manifest,diagnostics," << DCount * N << ",8,2,"
              << DCount * N * 16 << '\n';
    std::cout << "manifest,negative-control-changed," << N << ",4,1," << N * 4
              << '\n';
    std::cout << "manifest,observed-inputs," << 4 * N << ",8,2," << 8 * N * 8
              << '\n';
    std::cout << "manifest,activity-inspection," << N << ",4,2," << N * 8
              << '\n';
    std::cout << "manifest,prepared-bindings," << binding << ",1,1," << binding
              << '\n';
    std::cout << "native_handoff,threads=" << threads
              << ",cases=" << topology.size() * 4
              << ",rounds_per_case=32,second_publication_per_round=1,setup_us="
              << setup_us << ",binding_bytes=" << binding
              << ",production_changes=0 PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
