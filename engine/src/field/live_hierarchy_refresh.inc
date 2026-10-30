// Private implementation fragment of live_hierarchy.cpp. All numerical work
// stays in prepared native kernels; host tables describe source/ownership only.
namespace {
enum RefreshPlan {
  PriorSlot,
  Repair,
  LogicalOrder,
  Present,
  PacketChanged,
  PlanRows
};
struct RefreshReject {
  UpdateStatus status;
  BudgetFailure failure{};
};
bool same_comparison(const CascadeNode &a, const CascadeNode &b) {
  return same_handle(a.field, b.field) && a.comparison == b.comparison &&
         a.level == b.level;
}
bool within(std::uint64_t first, std::uint64_t length, std::uint64_t size) {
  return first <= size && length <= size - first;
}
struct RefreshArgs {
  ArgLoadStmt *live, *selector, *history, *values, *selected, *previous, *cfg,
      *children, *sources, *ratios, *masks, *plan, *bodies, *starts,
      *validation, *derived, *status, *groups, *delta, *flags, *dest, *visits;
  Stmt *count;
  explicit RefreshArgs(IRBuilder &b) {
    const auto d = PrimitiveType::f64, i = PrimitiveType::i32,
               u = PrimitiveType::u64;
    live = array(b, 0, d, 2);
    selector = array(b, 1, i);
    history = array(b, 2, d, 2);
    values = array(b, 3, d, 2);
    selected = array(b, 4, i);
    previous = array(b, 5, d, 2);
    cfg = array(b, 6, i, 2);
    children = array(b, 7, i);
    sources = array(b, 8, i, 2);
    ratios = array(b, 9, u, 2);
    masks = array(b, 10, u);
    plan = array(b, 11, i, 2);
    bodies = array(b, 12, d, 2);
    starts = array(b, 13, d, 2);
    validation = array(b, 14, i);
    derived = array(b, 15, d);
    status = array(b, 16, i);
    groups = array(b, 17, d, 2);
    delta = array(b, 18, d, 2);
    flags = array(b, 19, i, 2);
    dest = array(b, 20, i);
    visits = array(b, 21, PrimitiveType::i64, 2);
    count = b.create_arg_load({22}, i, false, 0);
  }
};
std::vector<engine::Param> refresh_signature() {
  using engine::Param;
  const auto d = PrimitiveType::f64, i = PrimitiveType::i32,
             u = PrimitiveType::u64;
  return {Param::array(d, 2), Param::array(i),
          Param::array(d, 2), Param::array(d, 2),
          Param::array(i),    Param::array(d, 2),
          Param::array(i, 2), Param::array(i),
          Param::array(i, 2), Param::array(u, 2),
          Param::array(u),    Param::array(i, 2),
          Param::array(d, 2), Param::array(d, 2),
          Param::array(i),    Param::array(d),
          Param::array(i),    Param::array(d, 2),
          Param::array(d, 2), Param::array(i, 2),
          Param::array(i),    Param::array(PrimitiveType::i64, 2),
          Param::scalar(i)};
}
void refresh_finite(IRBuilder &b, const RefreshArgs &a, Stmt *v) {
  auto bad = b.get_if_guard(
      b.create_if(b.create_logical_not(supported_real(b, v))), true);
  b.create_atomic_or(b.create_external_ptr(a.status, {b.get_int32(0)}),
                     b.get_int32(1));
}
}  // namespace

void LiveHierarchy::Impl::prepare_refresh_execution() {
  refresh = std::make_unique<Refresh>(*this);
  auto &r = *refresh;
  const std::array<DataType, 9> types{
      PrimitiveType::f64, PrimitiveType::i32, PrimitiveType::f64,
      PrimitiveType::i32, PrimitiveType::i32, PrimitiveType::i32,
      PrimitiveType::u64, PrimitiveType::u64, PrimitiveType::i32};
  const std::array<std::vector<int>, 9> shapes{{{8, H},
                                                {H},
                                                {4, H},
                                                {Columns, H},
                                                {L},
                                                {2, S},
                                                {2, S},
                                                {H},
                                                {PlanRows, H}}};
  for (int i = 0; i < 9; ++i) {
    r.arrays[i] =
        std::make_unique<Ndarray>(&h.runtime_->program(), types[i], shapes[i]);
    h.runtime_->program().fill_ndarray_fast_u32(r.arrays[i].get(), 0);
  }
  {
    IRBuilder b;
    RefreshArgs a(b);
    write(b, a.status, {b.get_int32(0)}, b.get_int32(0));
    // One edit-time ordered pass. Unchanged child packets are copied, never
    // solved from bodies to hide their pending native publication lag.
    auto *loop = b.create_range_for(b.get_int32(0), a.count, false, 1, 0, true);
    auto scope = b.get_loop_guard(loop);
    auto *node = read(b, a.plan, cell(b, LogicalOrder, b.get_loop_index(loop)));
    auto *prior = read(b, a.plan, cell(b, PriorSlot, node));
    auto *exists = b.create_cmp_ge(prior, b.get_int32(0));
    auto *sel = b.create_local_var(PrimitiveType::i32);
    b.create_local_store(sel, b.get_int32(0));
    {
      auto old = b.get_if_guard(b.create_if(exists), true);
      b.create_local_store(sel, read(b, a.selector, {prior}));
    }
    write(b, a.selected, {node}, b.create_local_load(sel));
    write(b, a.plan, cell(b, PacketChanged, node), b.get_int32(0));
    for (int k = 0; k < 8; ++k) {
      auto *v = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(v, b.get_float64(0));
      {
        auto old = b.get_if_guard(b.create_if(exists), true);
        b.create_local_store(v, read(b, a.live, cell(b, k, prior)));
      }
      write(b, a.values, cell(b, k, node), b.create_local_load(v));
    }
    for (int k = 0; k < 4; ++k) {
      auto *v = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(v, b.get_float64(0));
      {
        auto old = b.get_if_guard(b.create_if(exists), true);
        b.create_local_store(v, read(b, a.history, cell(b, k, prior)));
      }
      write(b, a.previous, cell(b, k, node), b.create_local_load(v));
    }
    {
      auto repair = b.get_if_guard(
          b.create_if(read(b, a.plan, cell(b, Repair, node))), true);
      auto *mass = b.create_local_var(PrimitiveType::f64);
      b.create_local_store(mass, b.get_float64(0));
      std::array<AllocaStmt *, 3> anchor{}, sum{};
      for (int k = 0; k < 3; ++k) {
        anchor[k] = b.create_local_var(PrimitiveType::f64);
        sum[k] = b.create_local_var(PrimitiveType::f64);
        b.create_local_store(anchor[k], b.get_float64(0));
        b.create_local_store(sum[k], b.get_float64(0));
      }
      auto *branch = b.create_if(read(b, a.cfg, cell(b, ChildCount, node)));
      for (int parent = 0; parent < 2; ++parent) {
        auto mode = b.get_if_guard(branch, parent == 1);
        auto *begin =
            read(b, a.cfg, cell(b, parent ? ChildBegin : SourceBegin, node));
        auto *length =
            read(b, a.cfg, cell(b, parent ? ChildCount : SourceCount, node));
        auto *members =
            b.create_range_for(b.get_int32(0), length, false, 1, 0, true);
        auto member = b.get_loop_guard(members);
        auto *index = b.get_loop_index(members);
        auto *ref = b.create_add(begin, index);
        std::array<Stmt *, 4> value{};
        if (parent) {
          auto *child = read(b, a.children, {ref});
          auto *slot = read(b, a.selected, {child});
          for (int k = 0; k < 4; ++k)
            value[k] = read(b, a.values,
                            {b.create_add(b.create_mul(slot, b.get_int32(4)),
                                          b.get_int32(k)),
                             child});
        } else {
          auto *body = read(b, a.sources, cell(b, 0, ref));
          for (int k = 0; k < 4; ++k) {
            const int field = k == 3 ? kMass : kPosX + k;
            auto *v = b.create_local_var(PrimitiveType::f64);
            auto *state = read(b, a.validation, {body});
            auto *fresh = b.create_if(b.create_cmp_gt(state, b.get_int32(1)));
            {
              auto yes = b.get_if_guard(fresh, true);
              Stmt *x =
                  read(b, a.starts,
                       cell(b, field, b.create_sub(state, b.get_int32(2))));
              if (k < 3)
                x = publish_position(b, x, h.profile_.body_fraction_bits);
              b.create_local_store(v, x);
            }
            {
              auto no = b.get_if_guard(fresh, false);
              b.create_local_store(v, read(b, a.bodies, cell(b, field, body)));
            }
            value[k] = b.create_local_load(v);
          }
          value[3] = b.create_mul(
              value[3],
              b.create_div(b.create_cast(read(b, a.ratios, cell(b, 0, ref)),
                                         PrimitiveType::f64),
                           b.create_cast(read(b, a.ratios, cell(b, 1, ref)),
                                         PrimitiveType::f64)));
        }
        for (auto *v : value)
          refresh_finite(b, a, v);
        {
          auto bad = b.get_if_guard(
              b.create_if(b.create_cmp_le(value[3], b.get_float64(0))), true);
          b.create_atomic_or(b.create_external_ptr(a.status, {b.get_int32(0)}),
                             b.get_int32(1));
        }
        for (int k = 0; k < 3; ++k) {
          {
            auto first = b.get_if_guard(
                b.create_if(b.create_cmp_eq(index, b.get_int32(0))), true);
            b.create_local_store(anchor[k], value[k]);
          }
          b.create_local_store(
              sum[k],
              b.create_add(
                  b.create_local_load(sum[k]),
                  b.create_mul(
                      value[3],
                      b.create_sub(value[k], b.create_local_load(anchor[k])))));
        }
        b.create_local_store(mass,
                             b.create_add(b.create_local_load(mass), value[3]));
      }
      std::array<Stmt *, 4> next{};
      next[3] = b.create_local_load(mass);
      for (int k = 0; k < 3; ++k) {
        auto *trial =
            b.create_add(b.create_local_load(anchor[k]),
                         b.create_div(b.create_local_load(sum[k]), next[3]));
        refresh_finite(b, a, trial);
        next[k] = b.create_bit_cast(
            b.create_and(b.create_bit_cast(trial, PrimitiveType::u64),
                         read(b, a.masks, {node})),
            PrimitiveType::f64);
      }
      Stmt *different = b.create_logical_not(exists);
      for (int k = 0; k < 4; ++k) {
        refresh_finite(b, a, next[k]);
        different = b.create_logical_or(
            different,
            b.create_cmp_ne(
                next[k],
                read(b, a.values,
                     {b.create_add(b.create_mul(b.create_local_load(sel),
                                                b.get_int32(4)),
                                   b.get_int32(k)),
                      node})));
      }
      auto changed = b.get_if_guard(b.create_if(different), true);
      write(b, a.plan, cell(b, PacketChanged, node), b.get_int32(1));
      for (int k = 0; k < 4; ++k) {
        write(b, a.previous, cell(b, k, node),
              b.create_select(
                  exists,
                  read(b, a.values,
                       {b.create_add(b.create_mul(b.create_local_load(sel),
                                                  b.get_int32(4)),
                                     b.get_int32(k)),
                        node}),
                  next[k]));
        for (int slot = 0; slot < 2; ++slot)
          write(b, a.values, cell(b, slot * 4 + k, node), next[k]);
      }
    }
    auto *field = read(b, a.cfg, cell(b, Field, node));
    for (int k = 0; k < 4; ++k)
      write(b, a.derived,
            {b.create_add(field, b.get_int32(k * std::max(G, 1)))},
            read(b, a.values,
                 {b.create_add(
                      b.create_mul(b.create_local_load(sel), b.get_int32(4)),
                      b.get_int32(k)),
                  node}));
    r.candidate = engine::CompiledKernel(
        *h.runtime_,
        "live_refresh_candidate_" +
            std::to_string(h.activity_->centroids.cells->id),
        b.extract_ir(), refresh_signature(), engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    RefreshArgs a(b);
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(H), false,
                                    h.runtime_->cpu_threads());
    auto scope = b.get_loop_guard(loop);
    auto *node = b.get_loop_index(loop);
    auto *present = b.create_if(read(b, a.plan, cell(b, Present, node)));
    {
      auto yes = b.get_if_guard(present, true);
      auto *field = read(
          b, a.dest,
          {b.create_add(b.get_int32(P), read(b, a.cfg, cell(b, Field, node)))});
      auto *fresh = b.create_cmp_lt(read(b, a.plan, cell(b, PriorSlot, node)),
                                    b.get_int32(0));
      {
        auto changed = b.get_if_guard(
            b.create_if(read(b, a.plan, cell(b, PacketChanged, node))), true);
        for (int k = 0; k < 8; ++k)
          write(b, a.live, cell(b, k, node),
                read(b, a.values, cell(b, k, node)));
        write(b, a.selector, {node}, read(b, a.selected, {node}));
        auto *sel = read(b, a.selected, {node});
        for (int k = 0; k < 4; ++k) {
          write(b, a.history, cell(b, k, node),
                read(b, a.previous, cell(b, k, node)));
          write(b, a.groups, cell(b, k, field),
                read(b, a.values,
                     {b.create_add(b.create_mul(sel, b.get_int32(4)),
                                   b.get_int32(k)),
                      node}));
          write(b, a.groups, cell(b, k + 4, field), b.get_float64(0));
          if (k < 3) {
            write(b, a.delta, cell(b, k, field), b.get_float64(0));
            write(b, a.delta, cell(b, k + 3, field),
                  read(b, a.previous, cell(b, k, node)));
          }
        }
        for (int k = 0; k < 2; ++k)
          write(b, a.flags, cell(b, k, field), b.get_int32(0));
      }
      {
        auto repair = b.get_if_guard(
            b.create_if(read(b, a.plan, cell(b, Repair, node))), true);
        h.activity_->centroids.activate(b, field);
      }
      {
        auto init = b.get_if_guard(b.create_if(fresh), true);
        for (int k = 0; k < 2; ++k)
          write(b, a.visits, cell(b, k, node), b.get_int64(0));
      }
    }
    {
      auto no = b.get_if_guard(present, false);
      for (int k = 0; k < 8; ++k)
        write(b, a.live, cell(b, k, node), b.get_float64(0));
      for (int k = 0; k < 4; ++k)
        write(b, a.history, cell(b, k, node), b.get_float64(0));
      write(b, a.selector, {node}, b.get_int32(0));
      for (int k = 0; k < 2; ++k)
        write(b, a.visits, cell(b, k, node), b.get_int64(0));
    }
    r.commit = engine::CompiledKernel(
        *h.runtime_,
        "live_refresh_commit_" +
            std::to_string(h.activity_->centroids.cells->id),
        b.extract_ir(), refresh_signature(), engine::MathMode::Strict);
  }
  launch_refresh(false);
  launch_refresh(true);
  h.runtime_->synchronize();
}

void LiveHierarchy::Impl::launch_refresh(bool commit) {
  auto &r = *refresh;
  auto &s = *h.prepared_;
  auto &kernel = commit ? r.commit : r.candidate;
  auto call = kernel.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *arrays[0]);
    c.set_arg_ndarray({1}, *arrays[1]);
    c.set_arg_ndarray({2}, *arrays[13]);
    for (int i = 0; i < 9; ++i)
      c.set_arg_ndarray({i + 3}, *r.arrays[i]);
    h.bind(c, 12, h.particle_array_.get(), h.particle_values_,
           {kParticleFieldCount, std::max(P, 1)});
    c.set_arg_ndarray({13}, *s.new_rows_array);
    c.set_arg_ndarray({14}, *s.validation_array);
    c.set_arg_ndarray({15}, *s.derived_array);
    c.set_arg_ndarray({16}, *s.candidate_status_array);
    h.bind(c, 17, h.group_array_.get(), h.group_values_,
           {kGroupFieldCount, std::max(G, 1)});
    c.set_arg_ndarray({18}, *h.activity_->group_delta);
    c.set_arg_ndarray({19}, *h.activity_->group_flags);
    c.set_arg_ndarray({20}, *s.destinations_array);
    c.set_arg_ndarray({21}, *owned[19]);
  }
  engine::argument_int(c, 22, r.count);
  kernel.launch(c);
}

std::array<const engine::CompiledKernel *, 2> LiveHierarchy::refresh_kernels()
    const {
  return {&impl_->refresh->candidate, &impl_->refresh->commit};
}
const std::vector<int> &LiveHierarchy::child_links() const {
  return impl_->children;
}

UpdateStatus LiveHierarchy::prepare_refresh(const LiveRefreshSpec &spec,
                                            const PreparedCounts &candidate,
                                            std::size_t commands,
                                            bool changed,
                                            UpdateResult &out,
                                            std::uint64_t work_limit) {
  auto &a = *impl_;
  auto &r = *a.refresh;
  auto &s = *a.h.prepared_;
  const auto &limits = s.capacity.limits();
  const auto cap = limits[Budget::CascadeWork];
  const auto budget = work_limit == UINT64_MAX ? cap : work_limit;
  const auto reject = [&](UpdateStatus status) -> void {
    throw RefreshReject{status};
  };
  const auto exhaust = [&](Budget kind, std::uint64_t required,
                           std::uint64_t capacity) -> void {
    throw RefreshReject{UpdateStatus::CapacityExhausted,
                        {kind, required, capacity}};
  };
  std::uint64_t work = out.structural_visits;
  const auto charge = [&](std::uint64_t n = 1) {
    if (work > budget || n > budget - work)
      exhaust(Budget::CascadeWork, work + n, budget);
    work += n;
  };
  try {
    if (budget > cap)
      exhaust(Budget::CascadeWork, budget, cap);
    charge(0);
    for (auto item :
         {std::pair{Budget::CascadeNodes, std::uint64_t(spec.node_count)},
          {Budget::CascadeLinks, std::uint64_t(spec.child_count)},
          {Budget::CascadeReferences, std::uint64_t(spec.contribution_count)}})
      if (item.second > limits[item.first])
        exhaust(item.first, item.second, limits[item.first]);
    if (!spec.node_count || spec.node_count != candidate.fields ||
        !spec.nodes || (spec.child_count && !spec.children) ||
        (spec.contribution_count && !spec.contributions))
      reject(UpdateStatus::InvalidInput);
    charge(r.cfg.size() + r.plan.size() + r.owners.size() + r.leaf.size() +
           r.rank.size());
    std::fill(r.cfg.begin(), r.cfg.end(), 0);
    std::fill_n(r.cfg.begin(), a.H, -1);
    std::fill(r.plan.begin(), r.plan.end(), 0);
    std::fill_n(r.plan.begin(), a.H, -1);
    std::fill(r.owners.begin(), r.owners.end(), -1);
    std::fill(r.leaf.begin(), r.leaf.end(), 0);
    std::fill(r.rank.begin(), r.rank.end(), 0);
    const auto ref = [&](UpdateRef input, bool particle) {
      const auto &headers =
          particle ? s.prospective_particles : s.prospective_fields;
      engine::RecordHandle result = input.handle;
      if (input.command >= 0) {
        if (!same_handle(input.handle, engine::RecordHandle{}) ||
            std::size_t(input.command) >= commands ||
            s.commands[input.command].operation !=
                (particle ? UpdateOp::DefineParticle : UpdateOp::DefineField))
          reject(UpdateStatus::InvalidInput);
        result = s.resolved[input.command];
      } else if (!result.context)
        reject(UpdateStatus::Stale);
      if (result.record < 0 || std::size_t(result.record) >= headers.size() ||
          !same_handle(result, headers[result.record]))
        reject(UpdateStatus::Stale);
      return result;
    };
    // Reserve every surviving packet location before assigning new identities.
    for (std::size_t i = 0; i < spec.node_count; ++i) {
      charge();
      auto node = static_cast<CascadeNode>(spec.nodes[i]);
      node.field = ref({node.field, spec.nodes[i].field_command}, false);
      if (!node.publish)
        reject(UpdateStatus::Unsupported);
      if (!node.comparison || node.level < 0 || !node.cohort_count ||
          !within(node.cohort_begin, node.cohort_count,
                  spec.contribution_count) ||
          !within(node.child_begin, node.child_count, spec.child_count))
        reject(UpdateStatus::InvalidInput);
      if (node.calculation != engine::numeric::Real::F64 ||
          node.storage != engine::numeric::Real::F64 ||
          node.retained_fraction_bits < 0 || node.retained_fraction_bits > 52)
        reject(UpdateStatus::Unsupported);
      for (std::size_t j = 0; j < i; ++j) {
        charge();
        if (r.proposed[j].comparison == node.comparison ||
            same_handle(r.proposed[j].field, node.field))
          reject(UpdateStatus::InvalidInput);
      }
      r.proposed[i] = node;
      int slot = -1;
      for (std::size_t j = 0; j < a.inspection.node_count; ++j) {
        charge();
        int old = a.inspection.order[j];
        if (same_comparison(node, a.inspection.nodes[old])) {
          slot = old;
          break;
        }
      }
      r.plan[LogicalOrder * a.H + i] = slot;
      if (slot >= 0) {
        r.plan[Present * a.H + slot] = 1;
        r.plan[PriorSlot * a.H + slot] = slot;
      }
    }
    for (std::size_t i = 0; i < spec.node_count; ++i) {
      int &slot = r.plan[LogicalOrder * a.H + i];
      if (slot < 0) {
        slot = 0;
        while (slot < a.H && r.plan[Present * a.H + slot]) {
          charge();
          ++slot;
        }
        if (slot == a.H)
          reject(UpdateStatus::InvalidInput);
        r.plan[Present * a.H + slot] = 1;
        changed = true;
      }
    }
    const CascadeSourceView view{
        s.prospective_edges, s.prospective_source_counts,
        s.prospective_source_masses, candidate.edges, s.generation};
    std::size_t ns = 0, nl = 0, nc = 0;
    const auto config = [&](int column, int node) -> int & {
      return r.cfg[column * a.H + node];
    };
    for (std::size_t logical = 0; logical < spec.node_count; ++logical) {
      charge();
      int slot = r.plan[LogicalOrder * a.H + logical];
      auto node = r.proposed[logical];
      const auto source_begin = ns, cover_begin = nc;
      int last = -1;
      for (std::size_t j = 0; j < node.cohort_count; ++j) {
        charge();
        const auto &input = spec.contributions[node.cohort_begin + j];
        auto occurrence = ref(input.occurrence, true);
        int origin;
        if (input.source_command >= 0) {
          if (std::size_t(input.source_command) >= commands ||
              (s.commands[input.source_command].operation !=
                   UpdateOp::InsertParticipation &&
               s.commands[input.source_command].operation !=
                   UpdateOp::ReplaceParticipation))
            reject(UpdateStatus::InvalidInput);
          origin = -input.source_command - 1;
        } else {
          if (input.source.generation != s.generation ||
              input.source.row >= s.live.edges ||
              input.source.ordinal != s.edges[input.source.row].ordinal)
            reject(UpdateStatus::Stale);
          origin = int(input.source.row);
        }
        std::size_t row = 0;
        for (; row < candidate.edges; ++row) {
          charge();
          if (s.refresh_origins[row] == origin)
            break;
        }
        if (row == candidate.edges)
          reject(UpdateStatus::Stale);
        CascadeSourceRow source{s.generation, row,
                                s.prospective_edges[row].ordinal};
        std::uint64_t c, d;
        if (view.contribution(occurrence, node.field, source, c, d) !=
            CascadeStatus::Ok)
          reject(UpdateStatus::InvalidInput);
        bool duplicate = false;
        for (auto k = source_begin; k < ns; ++k) {
          charge();
          duplicate |= r.source_headers[k].row == row;
        }
        if (duplicate)
          continue;
        if (int(row) <= last)
          reject(UpdateStatus::InvalidInput);
        last = int(row);
        r.sources[ns] = occurrence.record;
        r.sources[a.S + ns] = int(row);
        r.ratios[ns] = c;
        r.ratios[a.S + ns] = d;
        r.source_headers[ns++] = source;
        auto k = cover_begin;
        for (; k < nc; ++k) {
          charge();
          if (same_handle(r.exact[k].occurrence, occurrence))
            break;
        }
        if (k == nc) {
          if (nc == limits[Budget::CascadeCoverage])
            exhaust(Budget::CascadeCoverage, nc + 1,
                    limits[Budget::CascadeCoverage]);
          r.exact[nc] = {occurrence, 0, d};
          r.coverage_sum[nc++] = 0;
        }
        if (!add_coverage(r.exact[k], c, d))
          reject(UpdateStatus::InvalidInput);
        r.coverage[k] = occurrence.record;
      }
      std::size_t full = 0;
      for (std::size_t e = 0; e < candidate.edges; ++e) {
        charge();
        full += same_handle(s.prospective_edges[e].second, node.field);
      }
      if (full != ns - source_begin)
        reject(UpdateStatus::Unsupported);
      const auto child_begin = nl;
      for (std::size_t j = 0; j < node.child_count; ++j) {
        charge();
        int input = spec.children[node.child_begin + j];
        if (input < 0 || std::size_t(input) >= logical)
          reject(UpdateStatus::InvalidInput);
        bool duplicate = false;
        for (auto k = child_begin; k < nl; ++k) {
          charge();
          duplicate |= r.children[k] == r.plan[LogicalOrder * a.H + input];
        }
        if (duplicate)
          continue;
        int child = r.plan[LogicalOrder * a.H + input];
        if (r.proposed[input].level > node.level || config(Parent, child) >= 0)
          reject(UpdateStatus::InvalidInput);
        config(Parent, child) = slot;
        r.children[nl++] = child;
        r.rank[slot] = std::max(r.rank[slot], r.rank[child] + 1);
        for (int k = config(CoverBegin, child);
             k < config(CoverBegin, child) + config(CoverCount, child); ++k) {
          auto p = cover_begin;
          for (; p < nc; ++p) {
            charge();
            if (same_handle(r.exact[p].occurrence, r.exact[k].occurrence))
              break;
          }
          if (p == nc ||
              !account_child(r.exact[p], r.exact[k], r.coverage_sum[p]))
            reject(UpdateStatus::InvalidInput);
        }
      }
      if (nl != child_begin)
        for (auto k = cover_begin; k < nc; ++k) {
          charge();
          if (r.coverage_sum[k] != r.exact[k].count)
            reject(UpdateStatus::InvalidInput);
        }
      if (std::uint64_t(r.rank[slot] + 1) > limits[Budget::CascadeLevels])
        exhaust(Budget::CascadeLevels, r.rank[slot] + 1,
                limits[Budget::CascadeLevels]);
      node.child_begin = child_begin;
      node.child_count = nl - child_begin;
      node.cohort_begin = source_begin;
      node.cohort_count = ns - source_begin;
      r.nodes[slot] = node;
      config(ChildBegin, slot) = int(child_begin);
      config(ChildCount, slot) = int(node.child_count);
      config(SourceBegin, slot) = int(source_begin);
      config(SourceCount, slot) = int(node.cohort_count);
      config(Field, slot) = node.field.record;
      config(CoverBegin, slot) = int(cover_begin);
      config(CoverCount, slot) = int(nc - cover_begin);
      config(MultipleBodies, slot) = nc - cover_begin > 1;
      r.owners[node.field.record] = slot;
      r.leaf[node.field.record] = node.child_count == 0;
      r.masks[slot] = engine::numeric::Precision(node.calculation, node.storage,
                                                 node.retained_fraction_bits)
                          .fraction_mask();
      bool repair = r.plan[PriorSlot * a.H + slot] < 0;
      if (!repair) {
        repair = a.masks[slot] != r.masks[slot] ||
                 a.config(SourceCount, slot) != config(SourceCount, slot) ||
                 a.config(ChildCount, slot) != config(ChildCount, slot);
        for (std::size_t j = 0; !repair && j < node.cohort_count; ++j) {
          charge();
          auto old = a.config(SourceBegin, slot) + j, next = source_begin + j;
          repair =
              !same_handle(s.particle_headers[a.sources[old]],
                           s.prospective_particles[r.sources[next]]) ||
              a.ratios[old] != r.ratios[next] ||
              a.ratios[a.S + old] != r.ratios[a.S + next] ||
              a.source_headers[old].ordinal != r.source_headers[next].ordinal;
        }
        for (std::size_t j = 0; !repair && j < node.child_count; ++j) {
          charge();
          int child = r.children[child_begin + j];
          repair = a.children[a.config(ChildBegin, slot) + j] != child ||
                   r.plan[PriorSlot * a.H + child] < 0 ||
                   r.plan[Repair * a.H + child];
        }
      }
      r.plan[Repair * a.H + slot] = repair;
      changed |= repair;
    }
    for (std::size_t logical = 0; logical < spec.node_count; ++logical) {
      charge();
      int slot = r.plan[LogicalOrder * a.H + logical],
          parent = config(Parent, slot);
      if ((parent < 0) != (logical + 1 == spec.node_count))
        reject(UpdateStatus::InvalidInput);
      config(Complement, slot) = parent >= 0 && config(ChildCount, parent) > 1;
      if (r.plan[PriorSlot * a.H + slot] >= 0)
        changed |= a.config(Parent, slot) != parent ||
                   a.config(Complement, slot) != config(Complement, slot);
    }
    for (int field = 0; field < a.G; ++field) {
      charge();
      if (s.prospective_fields[field].record >= 0 && r.owners[field] < 0)
        reject(UpdateStatus::InvalidInput);
    }
    // Stable packet-slot order preserves the producer summation order across
    // serialization-only permutations. Local ordered rows precede that node's
    // inherited edge, exactly as in C3-1 installation.
    std::size_t nr = 0;
    for (int body = 0; body < a.P; ++body) {
      charge();
      r.offsets[body] = int(nr);
      bool local = false;
      const auto add = [&](int kind, int slot, int parent, std::uint64_t c,
                           std::uint64_t d) {
        charge();
        if (nr == limits[Budget::LiveResponses])
          exhaust(Budget::LiveResponses, nr + 1, limits[Budget::LiveResponses]);
        r.influences[nr] = body;
        r.influences[a.R + nr] = slot;
        r.influences[2 * a.R + nr] = parent;
        r.influences[3 * a.R + nr] = kind;
        r.weights[nr] = c;
        r.weights[a.R + nr++] = d;
      };
      for (int slot = 0; slot < a.H; ++slot) {
        charge();
        if (!r.plan[Present * a.H + slot])
          continue;
        if (!config(ChildCount, slot))
          for (int j = config(SourceBegin, slot);
               j < config(SourceBegin, slot) + config(SourceCount, slot); ++j) {
            charge();
            if (r.sources[j] == body) {
              local = true;
              add(0, slot, slot, r.ratios[j], r.ratios[a.S + j]);
            }
          }
        if (config(Parent, slot) >= 0)
          for (int j = config(CoverBegin, slot);
               j < config(CoverBegin, slot) + config(CoverCount, slot); ++j) {
            charge();
            if (r.coverage[j] == body)
              add(1, slot, config(Parent, slot), r.exact[j].count,
                  r.exact[j].denominator);
          }
      }
      r.offsets[a.P + body] = int(nr);
      if (s.prospective_particles[body].record >= 0 && !local)
        reject(UpdateStatus::InvalidInput);
      bool different =
          a.offsets[a.P + body] - a.offsets[body] != int(nr) - r.offsets[body];
      for (int j = r.offsets[body]; !different && j < int(nr); ++j) {
        charge();
        int old = a.offsets[body] + j - r.offsets[body];
        for (int k = 1; k < 4; ++k)
          different |= a.influences[k * a.R + old] != r.influences[k * a.R + j];
        different |= a.weights[old] != r.weights[j] ||
                     a.weights[a.R + old] != r.weights[a.R + j];
      }
      if (different) {
        changed = true;
        s.validation[a.P + a.G + body] = 1;
      }
    }
    for (int slot = 0; slot < a.H; ++slot) {
      charge();
      bool present = r.plan[Present * a.H + slot],
           repair = r.plan[Repair * a.H + slot];
      bool old = a.inspection.nodes[slot].field.record >= 0 &&
                 a.inspection.nodes[slot].field.record < a.G &&
                 a.owners[a.inspection.nodes[slot].field.record] == slot;
      bool retired = old && (!present || r.plan[PriorSlot * a.H + slot] < 0);
      if (retired)
        changed = true;
      if (repair || retired) {
        if (old)
          for (int j = a.config(CoverBegin, slot);
               j < a.config(CoverBegin, slot) + a.config(CoverCount, slot);
               ++j) {
            charge();
            s.validation[a.P + a.G + a.coverage[j]] = 1;
          }
        if (present)
          for (int j = config(CoverBegin, slot);
               j < config(CoverBegin, slot) + config(CoverCount, slot); ++j) {
            charge();
            s.validation[a.P + a.G + r.coverage[j]] = 1;
          }
      }
      if (present && repair)
        out.repair_visits +=
            1 + (config(ChildCount, slot) ? config(ChildCount, slot)
                                          : config(SourceCount, slot));
    }
    // Both structural generations and both hierarchy descriptors coexist.
    std::uint64_t old_refs = a.responses + a.inspection.node_count;
    for (std::size_t j = 0; j < a.inspection.node_count; ++j) {
      charge();
      int slot = a.inspection.order[j];
      old_refs += a.config(ChildCount, slot) + a.config(SourceCount, slot) +
                  a.config(CoverCount, slot);
    }
    std::uint64_t claims = 0;
    for (std::size_t j = 0; j < commands; ++j) {
      charge();
      claims += s.commands[j].operation == UpdateOp::DefineParticle ||
                s.commands[j].operation == UpdateOp::DefineField;
    }
    const auto peak = s.live.particles + s.live.fields + claims +
                      2 * std::max(s.live.edges, candidate.edges) +
                      2 * std::max(s.live.contacts, candidate.contacts) +
                      s.live.focus + old_refs + spec.node_count + ns + nl + nc +
                      nr;
    if (peak > limits[Budget::References])
      exhaust(Budget::References, peak, limits[Budget::References]);
    // Charge complete native candidate/copy/focus/U/commit/invalidation loops
    // before dispatch, including unchanged snapshots and unused slot cleanup.
    charge(24 * spec.node_count + 32 * a.H + 8 * out.repair_visits +
           4 * std::uint64_t(a.P + a.G) + candidate.focus * 8);
    r.count = spec.node_count;
    r.responses = nr;
    out.hierarchy_nodes = r.count;
    out.responses = nr;
    out.structural_visits = work;
    return changed ? UpdateStatus::Ok : UpdateStatus::NoChange;
  } catch (const RefreshReject &e) {
    out.failure = e.failure;
    out.structural_visits = work;
    return e.status;
  }
}

bool LiveHierarchy::stage_refresh(UpdateResult &out) {
  auto &a = *impl_;
  auto &r = *a.refresh;
  auto &s = *a.h.prepared_;
  const auto upload = [&](int index, const auto &values) {
    auto bytes = values.size() * sizeof(values[0]);
    engine::upload(*r.arrays[index], values.data(), bytes);
    out.upload_bytes += bytes;
  };
  upload(3, r.cfg);
  upload(4, r.children);
  upload(5, r.sources);
  upload(6, r.ratios);
  upload(7, r.masks);
  upload(8, r.plan);
  a.launch_refresh(false);
  a.h.runtime_->synchronize();
  ++out.synchronizations;
  engine::readback(*s.candidate_status_array, s.candidate_status.data(),
                   sizeof(s.candidate_status));
  out.readback_bytes += sizeof(s.candidate_status);
  return s.candidate_status[0] == 0;
}

void LiveHierarchy::commit_refresh(UpdateResult &out) {
  auto &a = *impl_;
  auto &r = *a.refresh;
  auto &s = *a.h.prepared_;
  // Claims have succeeded. Convert structural scratch names to actual native
  // records without reading/writing survivors' numerical host staging.
  for (std::size_t logical = 0; logical < r.count; ++logical) {
    int slot = r.plan[LogicalOrder * a.H + logical];
    r.cfg[Field * a.H + slot] = s.destinations[a.P + r.cfg[Field * a.H + slot]];
  }
  for (std::size_t logical = 0; logical < r.count; ++logical) {
    int slot = r.plan[LogicalOrder * a.H + logical];
    for (int j = r.cfg[SourceBegin * a.H + slot];
         j < r.cfg[SourceBegin * a.H + slot] + r.cfg[SourceCount * a.H + slot];
         ++j)
      r.sources[j] = s.destinations[r.sources[j]];
    for (int j = r.cfg[CoverBegin * a.H + slot];
         j < r.cfg[CoverBegin * a.H + slot] + r.cfg[CoverCount * a.H + slot];
         ++j)
      r.coverage[j] = s.destinations[r.coverage[j]];
  }
  // Prospective bodies are indexed by scratch name. Rebuild body CSR and leaf/
  // owner maps in actual record order with reserved temporary source buffers.
  std::fill(r.owners.begin(), r.owners.end(), -1);
  std::fill(r.leaf.begin(), r.leaf.end(), 0);
  for (std::size_t logical = 0; logical < r.count; ++logical) {
    int slot = r.plan[LogicalOrder * a.H + logical],
        field = r.cfg[Field * a.H + slot];
    r.owners[field] = slot;
    r.leaf[field] = r.cfg[ChildCount * a.H + slot] == 0;
  }
  // The response list remains grouped in scratch body order; offsets remap
  // each span independently, preserving its exact summation order.
  std::fill(a.offsets.begin(), a.offsets.end(), 0);
  for (int body = 0; body < a.P; ++body) {
    int target = s.destinations[body];
    if (s.validation[body] <= 0)
      continue;
    a.offsets[target] = r.offsets[body];
    a.offsets[a.P + target] = r.offsets[a.P + body];
  }
  for (std::size_t j = 0; j < r.responses; ++j)
    r.influences[j] = s.destinations[r.influences[j]];
  const auto publish = [&](int index, const auto &values) {
    auto bytes = values.size() * sizeof(values[0]);
    engine::upload(*a.arrays[index], values.data(), bytes);
    out.upload_bytes += bytes;
  };
  publish(2, r.cfg);
  publish(3, r.children);
  publish(4, r.sources);
  publish(5, r.ratios);
  publish(6, r.masks);
  publish(9, r.coverage);
  publish(10, r.influences);
  publish(11, r.weights);
  publish(12, a.offsets);
  publish(17, r.leaf);
  publish(18, r.owners);
  a.launch_refresh(true);
  a.h.runtime_->synchronize();
  ++out.synchronizations;
}

void LiveHierarchy::finish_refresh(std::uint64_t generation) {
  auto &a = *impl_;
  auto &r = *a.refresh;
  auto &s = *a.h.prepared_;
  a.cfg.swap(r.cfg);
  a.children.swap(r.children);
  a.sources.swap(r.sources);
  a.ratios.swap(r.ratios);
  a.masks.swap(r.masks);
  a.coverage.swap(r.coverage);
  a.influences.swap(r.influences);
  a.weights.swap(r.weights);
  a.leaf.swap(r.leaf);
  a.owners.swap(r.owners);
  std::fill(a.inspection.fields.begin(), a.inspection.fields.end(), -1);
  std::fill(a.inspection.nodes.begin(), a.inspection.nodes.end(),
            CascadeNode{});
  for (std::size_t logical = 0; logical < r.count; ++logical) {
    int slot = r.plan[LogicalOrder * a.H + logical],
        field = a.config(Field, slot);
    a.inspection.order[logical] = slot;
    a.inspection.nodes[slot] = r.nodes[slot];
    a.inspection.nodes[slot].field = s.field_headers[field];
    a.inspection.fields[slot] = field;
    for (int j = a.config(SourceBegin, slot);
         j < a.config(SourceBegin, slot) + a.config(SourceCount, slot); ++j)
      r.source_headers[j].generation = generation;
  }
  a.source_headers.swap(r.source_headers);
  a.responses = r.responses;
  a.inspection.node_count = r.count;
  a.nodes = r.count;
  a.generation = generation;
  s.geometry_valid = false;
}
