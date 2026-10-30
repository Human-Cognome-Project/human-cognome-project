// Included by live_hierarchy_test.cpp, reusing its native fixtures and
// independent numerical references rather than constructing a second engine.
int refresh_column(hierarchy_audit::Analyzer &a,
                   const llvm::Value *pointer,
                   int arg) {
  auto index = a.data_index(pointer, arg, 8);
  auto term =
      index.terms.find(hierarchy_audit::Monomial{std::uint64_t(arg + 1)});
  return term == index.terms.end() ? 0 : int(term->second);
}
void refresh_ir_contract(const engine::CompiledKernel &compiled,
                         const llvm::Module &module,
                         int mode) {
  const auto &kernel = engine::testing::CompiledKernelAccess::kernel(compiled);
  hierarchy_audit::Analyzer a(kernel, module, 3 + mode);
  std::set<int> packet, history;
  bool selector = false;
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &instruction : bb) {
        if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction)) {
          const int arg = a.data_arg(store->getPointerOperand());
          if (mode == 0)
            require(arg < 0 || arg == 3 || arg == 4 || arg == 5 || arg == 11 ||
                        arg == 15 || arg == 16,
                    "refresh candidate writes committed state/input");
          else
            require(
                arg < 0 || arg == 0 || arg == 1 || arg == 2 || arg == 17 ||
                    arg == 18 || arg == 19 || arg == 21,
                "refresh commit writes physical survivors or source inputs");
          if (mode == 0 && arg == 3)
            packet.insert(refresh_column(a, store->getPointerOperand(), 3));
          if (mode == 1) {
            const auto *load =
                llvm::dyn_cast<llvm::LoadInst>(store->getValueOperand());
            if (!load)
              continue;  // Zeroing retired slots is not repaired packet
                         // delivery.
            const int source = a.data_arg(load->getPointerOperand());
            if (arg == 0 && source == 3) {
              int dest = refresh_column(a, store->getPointerOperand(), 0);
              require(dest == refresh_column(a, load->getPointerOperand(), 3),
                      "refresh commit mixes packet words");
              packet.insert(dest);
            }
            if (arg == 2 && source == 5) {
              int dest = refresh_column(a, store->getPointerOperand(), 2);
              require(dest == refresh_column(a, load->getPointerOperand(), 5),
                      "refresh commit mixes history words");
              history.insert(dest);
            }
            if (arg == 1 && source == 4)
              selector = true;
          }
        }
        if (mode == 1)
          if (auto *load = llvm::dyn_cast<llvm::LoadInst>(&instruction))
            require(a.data_arg(load->getPointerOperand()) != 13,
                    "refresh commit replays fixture starts");
      }
  require(packet == std::set<int>({0, 1, 2, 3, 4, 5, 6, 7}),
          "refresh IR lacks a complete packet");
  if (mode == 1)
    require(history == std::set<int>({0, 1, 2, 3}) && selector,
            "refresh IR lacks complete history/selector delivery");
}
void refresh_compiler_controls(Fixture &f) {
  int mode = 0, negatives = 0, positives = 0;
  for (auto *compiled : LiveHierarchyAccess::refresh_kernels(f.h)) {
    const auto &kernel =
        engine::testing::CompiledKernelAccess::kernel(*compiled);
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(*compiled))
             .get_internal_data()
             .compiled_data.module;
    refresh_ir_contract(*compiled, module, mode);
    const auto negative = [&](auto mutate) {
      auto copy = llvm::CloneModule(module);
      mutate(*copy);
      require(!llvm::verifyModule(*copy, &llvm::errs()),
              "invalid refresh negative-control IR");
      bool rejected = false;
      try {
        refresh_ir_contract(*compiled, *copy, mode);
      } catch (const std::exception &) {
        rejected = true;
      }
      require(rejected, "unsafe refresh compiler control was accepted");
      ++negatives;
    };
    negative([&](auto &copy) {
      hierarchy_audit::Analyzer a(kernel, copy, 3 + mode);
      std::vector<llvm::Instruction *> erase;
      for (auto &fn : copy)
        for (auto &bb : fn)
          for (auto &inst : bb)
            if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst)) {
              int arg = a.data_arg(store->getPointerOperand());
              if (arg == (mode ? 0 : 3) &&
                  refresh_column(a, store->getPointerOperand(), arg) == 7)
                erase.push_back(store);
            }
      require(!erase.empty(), "refresh packet control has no target");
      for (auto *inst : erase)
        inst->eraseFromParent();
    });
    if (!mode)
      negative([&](auto &copy) {
        hierarchy_audit::Analyzer a(kernel, copy, 3);
        llvm::LoadInst *target = nullptr;
        for (auto &fn : copy)
          for (auto &bb : fn)
            for (auto &inst : bb)
              if (auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst))
                if (a.data_arg(load->getPointerOperand()) == 12 &&
                    load->getType()->isDoubleTy())
                  target = load;
        require(target, "refresh candidate control has no body read");
        new llvm::StoreInst(llvm::ConstantFP::get(target->getType(), 0),
                            target->getPointerOperand(), target);
      });
    else
      negative([&](auto &copy) {
        hierarchy_audit::Analyzer a(kernel, copy, 4);
        std::vector<llvm::Instruction *> erase;
        for (auto &fn : copy)
          for (auto &bb : fn)
            for (auto &inst : bb)
              if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst))
                if (a.data_arg(store->getPointerOperand()) == 1)
                  erase.push_back(store);
        require(!erase.empty(), "refresh selector control has no target");
        for (auto *inst : erase)
          inst->eraseFromParent();
      });
    auto safe = llvm::CloneModule(module);
    hierarchy_audit::Analyzer a(kernel, *safe, 3 + mode);
    llvm::StoreInst *target = nullptr;
    for (auto &fn : *safe)
      for (auto &bb : fn)
        for (auto &inst : bb)
          if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst))
            if (a.data_arg(store->getPointerOperand()) == (mode ? 0 : 3))
              target = store;
    require(target, "refresh safe compiler control has no target");
    auto *clone = target->clone();
    clone->insertAfter(target);
    require(!llvm::verifyModule(*safe, &llvm::errs()),
            "invalid safe refresh IR");
    refresh_ir_contract(*compiled, *safe, mode);
    ++positives;
    ++mode;
  }
  std::cout << "refresh_compiler_audit,modules=2,expected_rejections="
            << negatives << ",safe_duplicates=" << positives
            << ",candidate_committed_writes=0,commit_body_writes=0 PASS\n";
}
struct RefreshRequest {
  std::vector<LiveRefreshNode> nodes;
  std::vector<int> children;
  std::vector<std::vector<LiveRefreshContribution>> cohorts;
  std::vector<LiveRefreshContribution> refs;
  explicit RefreshRequest(Harness &h) {
    const auto &v = h.inspect_live_hierarchy();
    const auto &s = h.prepared_storage();
    for (std::size_t logical = 0; logical < v.node_count; ++logical) {
      int slot = v.order[logical];
      LiveRefreshNode node;
      static_cast<CascadeNode &>(node) = v.nodes[slot];
      nodes.push_back(node);
      cohorts.emplace_back();
      for (std::size_t row = 0; row < s.live.edges; ++row)
        if (s.edges[row].second.record == node.field.record)
          cohorts.back().push_back({UpdateRef::existing(s.edges[row].first),
                                    {s.generation, row, s.edges[row].ordinal},
                                    -1});
    }
    // The fixture's original tree order is reconstructed through owner
    // identities kept in node spans. Child identities are exposed below by
    // the private test-only descriptor accessor.
    const auto links = LiveHierarchyAccess::children(h);
    for (std::size_t logical = 0; logical < nodes.size(); ++logical) {
      auto &n = nodes[logical];
      const auto begin = n.child_begin;
      n.child_begin = children.size();
      for (std::size_t j = 0; j < n.child_count; ++j) {
        int slot = links[begin + j];
        auto i =
            std::find(v.order.begin(), v.order.begin() + v.node_count, slot);
        require(i != v.order.begin() + v.node_count,
                "missing child slot identity");
        children.push_back(int(i - v.order.begin()));
      }
    }
  }
  LiveRefreshSpec spec() {
    refs.clear();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      nodes[i].cohort_begin = refs.size();
      nodes[i].cohort_count = cohorts[i].size();
      refs.insert(refs.end(), cohorts[i].begin(), cohorts[i].end());
    }
    return {nodes.data(),    nodes.size(), children.data(),
            children.size(), refs.data(),  refs.size()};
  }
  void replaced(std::uint64_t row, int command) {
    for (auto &cohort : cohorts)
      for (auto &ref : cohort)
        if (ref.source_command < 0 && ref.source.row == row)
          ref.source_command = command;
  }
};
UpdateResult refresh_apply(Fixture &f,
                           RefreshRequest &request,
                           const std::vector<UpdateCommand> &commands = {},
                           std::uint64_t work = UINT64_MAX,
                           std::uint64_t expected = UINT64_MAX) {
  const auto spec = request.spec();
  UpdateResult result;
  const auto generation =
      expected == UINT64_MAX ? f.h.prepared_storage().generation : expected;
  no_new([&] {
    result = f.h.refresh_live_hierarchy(generation, commands.data(),
                                        commands.size(), spec, work);
  });
  return result;
}
void refresh_ok(const UpdateResult &result) {
  if (result.status != UpdateStatus::Ok) {
    std::cerr << "refresh_status=" << int(result.status)
              << ",budget=" << int(result.failure.budget)
              << ",required=" << result.failure.required
              << ",capacity=" << result.failure.capacity << '\n';
    throw std::runtime_error("combined refresh did not commit");
  }
}
void refresh_survivors(Fixture &f,
                       const std::vector<double> &bodies,
                       const std::vector<double> &motion,
                       const std::vector<double> &speed,
                       const std::vector<engine::RecordHandle> &handles) {
  f.h.inspect_prepared();
  const int P = f.h.particle_capacity();
  for (const auto &h : handles) {
    for (int k = 0; k < kParticleFieldCount; ++k)
      require(f.h.particle64(k, h.record) == bodies[k * P + h.record],
              "refresh rewrote survivor physical state");
    for (int k = 0; k < 3; ++k)
      require(f.h.prepared_storage().motion64()[k * P + h.record] ==
                  motion[k * P + h.record],
              "refresh rewrote survivor displacement");
    require(f.h.determined64()[h.record] == speed[h.record],
            "refresh rewrote survivor cached speed");
  }
}
void refresh_basics(engine::Runtime &runtime, HostPath path) {
  Fixture f(runtime, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, .25}, 52, 52, false, false, false,
            false, 1024, {.25, .125, .25});
  f.install();
  refresh_compiler_controls(f);
  const auto hash = state_hash(f);
  const auto generation = f.h.prepared_storage().generation;
  RefreshRequest same(f.h);
  require(refresh_apply(f, same).status == UpdateStatus::NoChange &&
              state_hash(f) == hash,
          "empty/identical refresh changed committed state");
  auto aliases = same;
  aliases.cohorts[0].push_back(aliases.cohorts[0].front());
  require(refresh_apply(f, aliases).status == UpdateStatus::NoChange &&
              state_hash(f) == hash,
          "repeated source-row alias changed physical coverage");
  // Reorder serialized siblings while preserving declared child identity order.
  RefreshRequest permuted(f.h);
  std::swap(permuted.nodes[0], permuted.nodes[1]);
  std::swap(permuted.cohorts[0], permuted.cohorts[1]);
  permuted.children = {1, 0};
  require(refresh_apply(f, permuted).status == UpdateStatus::NoChange &&
              state_hash(f) == hash,
          "serialization-only reorder changed arithmetic or state");
  auto changed = same;
  changed.nodes[0].retained_fraction_bits = 4;
  f.h.inspect_prepared();
  auto before = f.h.particles64();
  auto motion = f.h.prepared_storage().motion64(), speed = f.h.determined64();
  auto handles = std::vector<engine::RecordHandle>(
      f.in.particle_rows.begin(), f.in.particle_rows.begin() + 3);
  refresh_ok(refresh_apply(f, changed));
  require(f.h.prepared_storage().generation == generation + 1,
          "hierarchy-only refresh did not advance once");
  refresh_survivors(f, before, motion, speed, handles);
  UpdateCommand old;
  old.operation = UpdateOp::ReplaceFocus;
  require(f.h.update_prepared(generation + 1, &old, 1).status ==
              UpdateStatus::Unsupported,
          "ordinary installed update guard was removed");
  RefreshRequest contact_request(f.h);
  UpdateCommand contact;
  contact.operation = UpdateOp::InsertContact;
  contact.first = UpdateRef::existing(f.in.particle_rows[0]);
  contact.second = UpdateRef::existing(f.in.particle_rows[2]);
  f.control->set(0);
  runtime.synchronize();
  refresh_ok(refresh_apply(f, contact_request, {contact}));
  f.h.inspect_prepared();
  for (int logical : {0, 2})
    require(f.h.prepared_storage()
                    .inspection_flags[f.in.particle_rows[logical].record] &
                2,
            "contact edit did not invalidate both native endpoint sides");
  auto p = Parameters64{};
  p.dt = .125;
  p.gravity = .125;
  p.contact_stiffness = 2;
  p.reach_strength = 0;
  auto expected = physical_reference(f, p, true);
  before = f.h.particles64();
  no_new([&] { f.h.tick64(p); });
  f.h.inspect_prepared();
  verify_physical(f, expected, before, p);
  std::cout << "refresh_basics,no_op=1,permutation=1,hierarchy_only=1,"
               "survivors=exact,combined_integration=1 PASS\n";
}
void refresh_dirty_and_counts(engine::Runtime &runtime, HostPath path) {
  const auto C = std::uint64_t{1} << 25, D = std::uint64_t{1} << 30;
  Fixture f(runtime, path, {{}, {}, {0, 1}}, {{C, C, 0}, {0, 0, C}, {0, 0, 0}},
            {D, D, D}, {1, 1, 2}, {0, 2, 5});
  f.install();
  f.h.inspect_prepared();
  // First publish once so the surviving node has selector 1, unequal slots
  // and nontrivial old history. A subsequent body edit is deliberately left
  // pending while an unrelated subtree is refreshed.
  f.h.particle64(kPosX, f.in.particle_rows[0].record) = 4;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  f.control->set(1, f.nodes[0].field.record);
  LiveHierarchyAccess::poll(f.h, 0, 0);
  f.h.inspect_prepared();
  f.h.particle64(kPosX, f.in.particle_rows[0].record) = 8;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  f.control->set(1, f.nodes[0].field.record);
  runtime.synchronize();
  const auto before = f.h.inspect_live_hierarchy();
  require(before.selectors[0] == 1 &&
              before.slots[0] != before.slots[4 * before.selectors.size()] &&
              before.previous[0] != selected(before, 0)[0],
          "dirty fixture lacks distinguishing selector/slot/history state");
  RefreshRequest request(f.h);
  // Change only the other leaf's precision, repairing the root from the
  // deliberately lagged left packet rather than from its changed body.
  request.nodes[1].retained_fraction_bits = 8;
  refresh_ok(refresh_apply(f, request));
  const auto after = f.h.inspect_live_hierarchy();
  for (int k = 0; k < 8; ++k)
    require(after.slots[k * after.selectors.size()] ==
                before.slots[k * before.selectors.size()],
            "unaffected dirty slot was reseeded");
  for (int k = 0; k < 4; ++k)
    require(after.previous[k * after.selectors.size()] ==
                before.previous[k * before.selectors.size()],
            "dirty history was overwritten");
  require(after.selectors[0] == before.selectors[0], "dirty selector changed");
  f.h.inspect_prepared();
  require(f.h.prepared_storage().inspection_flags[f.h.particle_capacity() +
                                                  f.nodes[0].field.record] &
              2,
          "dirty pending native bit was consumed by refresh");
  const auto expected_parent = fold({selected(before, 0), selected(before, 1)},
                                    f.nodes[2].retained_fraction_bits);
  near(selected(after, 2)[0], expected_parent[0],
       "root used an unpublished body input");
  LiveHierarchyAccess::poll(f.h, 0, 0);
  LiveHierarchyAccess::poll(f.h, 0, 2);
  require(selected(f.h.inspect_live_hierarchy(), 0)[0] == 5,
          "dirty input was lost after refresh");
  // Count-only change has exactly the same f32 share. Both leaf and root
  // provenance must change, and recipients must be invalidated independently.
  RefreshRequest counted(f.h);
  std::vector<UpdateCommand> commands;
  const auto &s = f.h.prepared_storage();
  const auto body = f.in.particle_rows[0];
  for (std::size_t row = 0; row < s.live.edges; ++row)
    if (s.edges[row].first.record == body.record) {
      const auto &edge = s.edges[row];
      UpdateCommand cmd;
      cmd.operation = UpdateOp::ReplaceParticipation;
      cmd.row = row;
      cmd.generation = s.generation;
      cmd.first = UpdateRef::existing(edge.first);
      cmd.second = UpdateRef::existing(edge.second);
      cmd.ordinal = edge.ordinal;
      cmd.share = edge.share;
      cmd.exact_count = C + 1;
      require(cmd.share == float(double(C + 1) / D),
              "count witness is distinguishable by f32 share");
      counted.replaced(row, int(commands.size()));
      commands.push_back(cmd);
    }
  f.control->set(0);
  runtime.synchronize();
  refresh_ok(refresh_apply(f, counted, commands));
  f.h.inspect_prepared();
  const auto packet = selected(f.h.inspect_live_hierarchy(), 0);
  const long double w = static_cast<long double>(C + 1) / D,
                    other = static_cast<long double>(C) / D;
  near(packet[0], double((8 * w + 2 * other) / (w + other)),
       "count repair ignored exact source ratios");
  require(packet[3] == double(C + 1) / D + double(C) / D,
          "count-only mass repair lost u64 provenance");
  require(f.h.prepared_storage().inspection_flags[body.record] & 2,
          "count-only input did not wake recipient");
  std::cout
      << "refresh_dirty_counts,selector=1,unequal_slots=1,unaffected_slots="
         "exact,history=exact,pending="
         "preserved,root_uses_publications=1,f32_share_unchanged=1 PASS\n";
}
void refresh_rejections(engine::Runtime &runtime, HostPath path) {
  Fixture f(runtime, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  f.install();
  const auto baseline = state_hash(f);
  int rejections = 0;
  const auto reject = [&](RefreshRequest &request,
                          const std::vector<UpdateCommand> &commands = {},
                          std::uint64_t work = UINT64_MAX,
                          std::uint64_t generation = UINT64_MAX) {
    auto result = refresh_apply(f, request, commands, work, generation);
    require(result.status != UpdateStatus::Ok &&
                result.status != UpdateStatus::NoChange && f.h.ready(),
            "bad refresh did not reject with healthy execution");
    require(state_hash(f) == baseline,
            "ordinary refresh rejection changed committed state");
    ++rejections;
    return result;
  };
  {
    RefreshRequest q(f.h);
    reject(q, {}, UINT64_MAX, f.h.prepared_storage().generation + 1);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].field.context++;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].field.incarnation++;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[0][0].occurrence.handle.context++;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[0][0].source.generation++;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[0][0].source.ordinal++;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[0][0].source.row = 10000;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].field_command = 0;
    q.nodes[0].field = {};
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].comparison = 0;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].comparison = q.nodes[1].comparison;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].retained_fraction_bits = 53;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.nodes[0].publish = false;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.children[0] = 2;
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[2].pop_back();
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    q.cohorts[0].pop_back();
    reject(q);
  }
  {
    RefreshRequest q(f.h);
    auto result = reject(q, {}, 0);
    require(result.failure.budget == Budget::CascadeWork,
            "wrong refresh work exhaustion budget");
  }
  {
    RefreshRequest q(f.h);
    reject(q, {},
           f.h.prepared_storage().capacity.limits()[Budget::CascadeWork] + 1);
  }
  {
    RefreshRequest q(f.h);
    std::vector<UpdateCommand> commands(71);
    auto result = reject(q, commands);
    require(result.failure.budget == Budget::Batch,
            "wrong refresh batch exhaustion budget");
  }
  {
    RefreshRequest q(f.h);
    UpdateCommand cmd;
    cmd.operation = UpdateOp::ReplaceFocus;
    reject(q, {cmd});
  }
  {
    RefreshRequest q(f.h);
    UpdateCommand cmd;
    cmd.operation = UpdateOp::RemoveParticipation;
    cmd.row = q.cohorts[0][0].source.row;
    cmd.generation = f.h.prepared_storage().generation;
    reject(q, {cmd});
  }
  {
    RefreshRequest q(f.h);
    UpdateCommand cmd;
    cmd.operation = UpdateOp::InsertContact;
    cmd.first = UpdateRef::existing(f.in.particle_rows[0]);
    cmd.second = UpdateRef::existing(f.in.particle_rows[1]);
    cmd.position = f.h.prepared_storage().live.contacts;
    q.cohorts[0][0].source_command = 0;
    reject(q, {cmd});
  }
  // Same-generation views remain usable after rejection, but not after a real
  // commit. Keep a candidate result and native geometry result across both.
  const auto generation = f.h.prepared_storage().generation;
  auto bound = f.h.bind_weighted_centroid_cascade(generation, f.spec());
  require(bound.status == CascadeStatus::Ok, "candidate witness bind failed");
  auto candidate = f.h.query_centroid_cascade(generation);
  auto geometry = f.h.query_prepared_geometry(generation);
  RefreshRequest valid(f.h);
  valid.nodes[0].retained_fraction_bits = 8;
  PreparedFixtureAccess::inject_failure(f.h, 1);
  require(refresh_apply(f, valid).status == UpdateStatus::InvalidInput &&
              f.h.ready(),
          "pre-claim injection poisoned execution");
  PreparedFixtureAccess::inject_failure(f.h, 0);
  require(state_hash(f) == baseline,
          "pre-claim failure altered committed state");
  f.h.inspect_centroid_cascade(candidate);
  f.h.inspect_prepared_geometry(geometry);
  refresh_ok(refresh_apply(f, valid));
  int stale = 0;
  try {
    f.h.inspect_centroid_cascade(candidate);
  } catch (const std::exception &) {
    ++stale;
  }
  try {
    f.h.inspect_prepared_geometry(geometry);
  } catch (const std::exception &) {
    ++stale;
  }
  require(stale == 2, "old query result survived committed refresh");
  std::cout
      << "refresh_rejections,healthy=" << rejections
      << ",preclaim_failure=healthy,valid_retry=1,stale_query_views=2 PASS\n";
}
RefreshRequest refresh_intermediate(Fixture &f,
                                    std::vector<UpdateCommand> &commands) {
  RefreshRequest q(f.h);
  commands.resize(1);
  commands[0].operation = UpdateOp::DefineField;
  commands[0].address = 63;
  LiveRefreshNode extra;
  extra.field_command = 0;
  extra.comparison = 999;
  extra.level = q.nodes[2].level;
  extra.child_begin = 0;
  extra.child_count = 2;
  std::vector<LiveRefreshContribution> sources;
  const auto &s = f.h.prepared_storage();
  for (const auto &ref : q.cohorts[2]) {
    UpdateCommand cmd;
    cmd.operation = UpdateOp::InsertParticipation;
    cmd.first = ref.occurrence;
    cmd.second = UpdateRef::pending(0);
    cmd.ordinal = 100 + commands.size();
    cmd.share = 1;
    cmd.exact_count = s.source_counts[ref.source.row];
    cmd.position = s.live.edges + commands.size() - 1;
    sources.push_back({ref.occurrence, {}, int(commands.size())});
    commands.push_back(cmd);
  }
  q.nodes.insert(q.nodes.begin() + 2, extra);
  q.cohorts.insert(q.cohorts.begin() + 2, sources);
  q.children = {0, 1, 2};
  q.nodes[3].child_begin = 2;
  q.nodes[3].child_count = 1;
  return q;
}
void refresh_field_lifecycle(engine::Runtime &runtime, HostPath path) {
  Fixture f(runtime, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  f.install();
  std::vector<UpdateCommand> commands;
  auto q = refresh_intermediate(f, commands);
  const auto before = f.h.inspect_live_hierarchy();
  refresh_ok(refresh_apply(f, q, commands));
  const auto field = f.h.prepared_storage().resolved[0];
  const auto installed = f.h.inspect_live_hierarchy();
  require(installed.node_count == 4 && installed.order[2] == 3 &&
              installed.order[3] == 2,
          "new field displaced a surviving packet slot");
  for (int node : {0, 1, 2})
    for (int k = 0; k < 8; ++k)
      require(
          installed.slots[k * installed.selectors.size() + node] ==
              before.slots[k * before.selectors.size() + node],
          "equal surviving packet changed while inserting intermediate field");
  near(selected(installed, 3)[0], 3, "new intermediate field repair");
  Parameters64 p;
  p.dt = 0;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  no_new([&] { f.h.tick64(p); });
  RefreshRequest remove(f.h);
  commands.clear();
  const auto &current = f.h.prepared_storage();
  for (std::size_t row = 0; row < current.live.edges; ++row)
    if (current.edges[row].second.record == field.record) {
      UpdateCommand cmd;
      cmd.operation = UpdateOp::RemoveParticipation;
      cmd.row = row;
      cmd.generation = current.generation;
      commands.push_back(cmd);
    }
  UpdateCommand retire;
  retire.operation = UpdateOp::RetireField;
  retire.first = UpdateRef::existing(field);
  commands.push_back(retire);
  remove.nodes.erase(remove.nodes.begin() + 2);
  remove.cohorts.erase(remove.cohorts.begin() + 2);
  remove.children = {0, 1};
  remove.nodes[2].child_begin = 0;
  remove.nodes[2].child_count = 2;
  refresh_ok(refresh_apply(f, remove, commands));
  const auto retired = f.h.inspect_live_hierarchy();
  require(retired.node_count == 3 && retired.nodes[3].field.record < 0,
          "retired comparison identity survived");
  for (int k = 0; k < 8; ++k)
    require(retired.slots[k * retired.selectors.size() + 3] == 0,
            "retired packet not cleared");
  no_new([&] { f.h.tick64(p); });
  std::cout << "refresh_field_lifecycle,pending_owner=1,three_levels=1,stable_"
               "slots=1,same_level=1,retired_packet_cleared=1 PASS\n";
}
void refresh_occurrence_reuse(engine::Runtime &runtime, HostPath path) {
  const auto letter = letter_data(runtime);
  const auto D = letter.denominators[0];
  const auto mass = letter.masses[0];
  Fixture f(runtime, path, {{}, {}, {0, 1}}, {{D, D, 0}, {0, 0, D}, {0, 0, 0}},
            {D, D, D}, {mass, mass, mass}, {0, 2, 5}, 8, 6, false, false, false,
            false, 1024, {letter.radii[0], letter.radii[0], letter.radii[0]});
  f.install();
  refresh_compiler_controls(f);
  auto survivors = std::vector<engine::RecordHandle>(
      f.in.particle_rows.begin(), f.in.particle_rows.begin() + 3);
  Parameters64 p;
  p.dt = .125;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  f.h.inspect_prepared();
  f.h.particle64(kVelX, survivors[0].record) = .25;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  f.control->set(2, survivors[0].record);
  runtime.synchronize();
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(f.h));
  const auto backing = metrics.sample();
  engine::RecordHandle old;
  for (int round = 0; round < 32; ++round) {
    no_new([&] { f.h.tick64(p); });
    f.h.inspect_prepared();
    auto bodies = f.h.particles64(), motion = f.h.prepared_storage().motion64(),
         speed = f.h.determined64();
    const auto ticks = f.h.prepared_storage().completed_ticks;
    RefreshRequest add(f.h);
    const auto &s = f.h.prepared_storage();
    std::vector<UpdateCommand> commands(1);
    commands[0].operation = UpdateOp::DefineParticle;
    commands[0].address = 1000 + round;
    commands[0].exact_mass = D;
    std::array<double, kParticleFieldCount> values{};
    values[kMass] = mass;
    values[kRadius] = letter.radii[0];
    values[kPosX] = 3 + 1. / 1024;
    values[kVelX] = .125;
    PreparedFixtureAccess::stage_new(f.h, 0, values);
    for (int node : {0, 2}) {
      UpdateCommand cmd;
      cmd.operation = UpdateOp::InsertParticipation;
      cmd.first = UpdateRef::pending(0);
      cmd.second = UpdateRef::existing(add.nodes[node].field);
      cmd.ordinal = 12345;
      cmd.share = 1;
      cmd.exact_count = D;
      cmd.position = s.live.edges + commands.size() - 1;
      add.cohorts[node].push_back(
          {UpdateRef::pending(0), {}, int(commands.size())});
      commands.push_back(cmd);
    }
    UpdateCommand contact;
    contact.operation = UpdateOp::InsertContact;
    contact.first = UpdateRef::pending(0);
    contact.second = UpdateRef::existing(survivors[0]);
    contact.position = s.live.contacts;
    commands.push_back(contact);
    refresh_ok(refresh_apply(f, add, commands));
    const auto added = f.h.prepared_storage().resolved[0];
    f.h.inspect_prepared();
    require(
        f.h.particle64(kPosX, added.record) ==
            quantize(values[kPosX], f.h.numerical_profile().body_fraction_bits),
        "new-row projection disagrees with current body precision");
    require(f.h.prepared_storage().completed_ticks == ticks,
            "structural refresh advanced simulation time");
    refresh_survivors(f, bodies, motion, speed, survivors);
    if (old.context)
      require(!detail::same_handle(old, added),
              "recycled native handle retained incarnation");
    no_new([&] { f.h.tick64(p); });
    f.h.inspect_prepared();
    bodies = f.h.particles64();
    motion = f.h.prepared_storage().motion64();
    speed = f.h.determined64();
    RefreshRequest remove(f.h);
    commands.clear();
    for (auto &cohort : remove.cohorts)
      cohort.erase(std::remove_if(cohort.begin(), cohort.end(),
                                  [&](auto &ref) {
                                    return ref.occurrence.handle.record ==
                                           added.record;
                                  }),
                   cohort.end());
    const auto &live = f.h.prepared_storage();
    for (std::size_t row = 0; row < live.live.edges; ++row)
      if (live.edges[row].first.record == added.record) {
        UpdateCommand cmd;
        cmd.operation = UpdateOp::RemoveParticipation;
        cmd.row = row;
        cmd.generation = live.generation;
        commands.push_back(cmd);
      }
    UpdateCommand drop_contact;
    drop_contact.operation = UpdateOp::RemoveContact;
    drop_contact.row = 0;
    drop_contact.generation = live.generation;
    commands.push_back(drop_contact);
    UpdateCommand retire;
    retire.operation = UpdateOp::RetireParticle;
    retire.first = UpdateRef::existing(added);
    commands.push_back(retire);
    refresh_ok(refresh_apply(f, remove, commands));
    refresh_survivors(f, bodies, motion, speed, survivors);
    old = added;
    require(f.h.present_particles() == 3,
            "retirement changed another occurrence");
  }
  const auto after = metrics.sample();
  backing.print("refresh_reuse_before");
  after.print("refresh_reuse_after");
  require(
      backing.same_backing(after),
      "within-capacity refresh allocated/rebuilt/recompiled native backing");
  std::cout << "refresh_occurrences,rounds=32,commits=64,contact_edits=64,"
               "survivors=exact,fixed_backing=1 PASS\n";
}
void refresh_late_failures(engine::Runtime &runtime, HostPath path) {
  for (int point = 2; point <= 5; ++point) {
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}}, {1, 1, 1}, {1, 1, 2},
              {0, 2, 5});
    f.install();
    std::vector<UpdateCommand> commands;
    auto request = refresh_intermediate(f, commands);
    PreparedFixtureAccess::inject_failure(f.h, point);
    bool failed = false;
    try {
      refresh_apply(f, request, commands);
    } catch (const std::exception &) {
      failed = true;
    }
    require(failed && !f.h.ready(),
            "late refresh failure left a healthy partial generation");
    auto p = Parameters64{};
    int refused = 0;
    try {
      f.h.tick64(p);
    } catch (const std::exception &) {
      ++refused;
    }
    try {
      f.h.inspect_prepared();
    } catch (const std::exception &) {
      ++refused;
    }
    try {
      f.h.inspect_live_hierarchy();
    } catch (const std::exception &) {
      ++refused;
    }
    require(f.h.refresh_live_hierarchy(0, nullptr, 0, request.spec()).status ==
                    UpdateStatus::Unready &&
                refused == 3,
            "unready refresh execution accepted work/reporting");
  }
  std::cout << "refresh_late_failure,points=4,unready=4,teardown=clean PASS\n";
}
void refresh_budget_and_numerics(engine::Runtime &runtime, HostPath path) {
  int witnessed = 0;
  for (auto item : {std::pair{Budget::CascadeNodes, 3ULL},
                    {Budget::CascadeLinks, 2ULL},
                    {Budget::CascadeReferences, 6ULL},
                    {Budget::CascadeCoverage, 6ULL},
                    {Budget::LiveResponses, 6ULL},
                    {Budget::References, 60ULL},
                    {Budget::Participation, 6ULL},
                    {Budget::Fields, 3ULL}}) {
    auto capacity = limits(1024, runtime.on_cuda());
    capacity[item.first] = item.second;
    capacity[Budget::Focus] =
        std::min(capacity[Budget::Focus], capacity[Budget::Fields]);
    capacity[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
        capacity, runtime.on_cuda(), NumericalProfile::trial());
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}}, {1, 1, 1}, {1, 1, 2},
              {0, 2, 5}, 52, 52, false, false, false, false, 1024, {},
              &capacity);
    f.install();
    const auto baseline = state_hash(f);
    std::vector<UpdateCommand> commands;
    auto request = refresh_intermediate(f, commands);
    const auto result = refresh_apply(f, request, commands);
    require(result.status == UpdateStatus::CapacityExhausted &&
                result.failure.budget == item.first && f.h.ready() &&
                state_hash(f) == baseline,
            "independent refresh reservation failed incorrectly");
    std::cout << "refresh_budget,budget=" << budget_name(item.first)
              << ",required=" << result.failure.required
              << ",capacity=" << result.failure.capacity << ",healthy=1 PASS\n";
    ++witnessed;
  }
  {
    auto capacity = limits(1024, runtime.on_cuda());
    capacity[Budget::Particles] = 3;
    capacity[Budget::Contacts] = 0;
    capacity[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
        capacity, runtime.on_cuda(), NumericalProfile::trial());
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}}, {1, 1, 1}, {1, 1, 2},
              {0, 2, 5}, 52, 52, false, false, false, false, 1024, {},
              &capacity);
    f.install();
    const auto baseline = state_hash(f);
    RefreshRequest request(f.h);
    UpdateCommand retire, define;
    retire.operation = UpdateOp::RetireParticle;
    retire.first = UpdateRef::existing(f.in.particle_rows[0]);
    define.operation = UpdateOp::DefineParticle;
    define.address = 333;
    define.exact_mass = 1;
    auto result = refresh_apply(f, request, {retire, define});
    require(result.status == UpdateStatus::CapacityExhausted &&
                result.failure.budget == Budget::Particles &&
                state_hash(f) == baseline,
            "same-batch retirement funded a native claim");
    ++witnessed;
    UpdateCommand contact;
    contact.operation = UpdateOp::InsertContact;
    contact.first = UpdateRef::existing(f.in.particle_rows[0]);
    contact.second = UpdateRef::existing(f.in.particle_rows[1]);
    result = refresh_apply(f, request, {contact});
    require(result.status == UpdateStatus::CapacityExhausted &&
                result.failure.budget == Budget::Contacts &&
                state_hash(f) == baseline,
            "contact reservation changed committed state");
    ++witnessed;
  }
  {
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}}, {1, 1, 1}, {1, 1, 2},
              {0, 2, 5});
    f.install();
    RefreshRequest add(f.h);
    std::vector<UpdateCommand> commands(1);
    commands[0].operation = UpdateOp::DefineParticle;
    commands[0].address = 333;
    commands[0].exact_mass = 1;
    for (int node : {0, 2}) {
      UpdateCommand edge;
      edge.operation = UpdateOp::InsertParticipation;
      edge.first = UpdateRef::pending(0);
      edge.second = UpdateRef::existing(add.nodes[node].field);
      edge.share = 1;
      edge.exact_count = 1;
      edge.position = f.h.prepared_storage().live.edges + commands.size() - 1;
      edge.ordinal = 999;
      add.cohorts[node].push_back(
          {UpdateRef::pending(0), {}, int(commands.size())});
      commands.push_back(edge);
    }
    const auto baseline = state_hash(f);
    std::array<double, kParticleFieldCount> input{};
    input[kMass] = input[kPosX] = std::numeric_limits<double>::max() / 4;
    input[kRadius] = .125;
    PreparedFixtureAccess::stage_new(f.h, 0, input);
    require(refresh_apply(f, add, commands).status ==
                    UpdateStatus::InvalidNumerics &&
                f.h.ready() && state_hash(f) == baseline,
            "native numerical repair rejection mutated live state");
    input[kMass] = 3;
    input[kPosX] = 4;
    PreparedFixtureAccess::stage_new(f.h, 0, input);
    refresh_ok(refresh_apply(f, add, commands));
    near(selected(f.h.inspect_live_hierarchy(), 0)[3], 5,
         "new physical mass did not reach participating mass");
    const auto added = f.h.prepared_storage().resolved[0];
    f.in.particle_rows[3] = added;
    f.denominators.push_back(1);
    f.coverage[0].push_back(1);
    f.coverage[1].push_back(0);
    f.coverage[2].push_back(1);
    f.refs.clear();
    const auto &current = f.h.prepared_storage();
    for (auto &node : f.nodes) {
      node.cohort_begin = f.refs.size();
      for (std::size_t row = 0; row < current.live.edges; ++row)
        if (current.edges[row].second.record == node.field.record)
          f.refs.push_back(
              {current.edges[row].first,
               {current.generation, row, current.edges[row].ordinal}});
      node.cohort_count = f.refs.size() - node.cohort_begin;
    }
    f.h.inspect_prepared();
    Parameters64 p;
    p.dt = .015625;
    p.gravity = .125;
    p.contact_stiffness = p.reach_strength = 0;
    const auto before = f.h.particles64(),
               expected = physical_reference(f, p, true);
    no_new([&] { f.h.tick64(p); });
    f.h.inspect_prepared();
    verify_physical(f, expected, before, p);
  }
  std::cout << "refresh_limits,independent_exhaustions=" << witnessed
            << ",old_plus_new_claims=1,native_invalid_numerics=healthy,valid_"
               "mass_retry=1 PASS\n";
}
#ifdef HCP_HAS_VIEWPORT
void refresh_capture(engine::Runtime &runtime, HostPath path) {
  const auto data = letter_data(runtime);
  const auto &D = data.denominators;
  Fixture f(runtime, path, {{}, {}, {0, 1}},
            {{D[0], D[1], 0}, {0, 0, D[2]}, {0, 0, 0}}, D, data.masses,
            data.positions, 52, 52, false, false, false, false, 1024,
            data.radii);
  f.install();
  assembly::ActiveReport mapping;
  mapping.handle = {warm::new_uuid(), data.origins.front().occurrence.study,
                    warm::new_uuid()};
  mapping.owner = warm::new_uuid();
  mapping.instance = warm::new_uuid();
  mapping.origins = data.origins;
  mapping.prepared = data.mapping;
  mapping.prepared.capacity = f.h.particle_capacity();
  mapping.prepared.groups.assign(f.h.field_capacity(), {});
  mapping.prepared.focal_groups.clear();
  for (std::size_t i = 0; i < mapping.prepared.particles.size(); ++i)
    mapping.prepared.particles[i].occurrence.slot =
        f.in.particle_rows[i].record;
  auto binding = viewport::bind(f.h, mapping);
  const auto initial = viewport::observe(f.h, binding);
  const auto baseline = state_hash(f);
  RefreshRequest bad(f.h);
  bad.nodes[0].level = -1;
  require(refresh_apply(f, bad).status == UpdateStatus::InvalidInput,
          "capture rejection witness accepted");
  require(viewport::observe(f.h, binding).prepared_generation ==
                  initial.prepared_generation &&
              state_hash(f) == baseline,
          "rejected refresh invalidated old observation");
  std::vector<UpdateCommand> commands;
  auto add = refresh_intermediate(f, commands);
  refresh_ok(refresh_apply(f, add, commands));
  bool stale = false;
  try {
    viewport::observe(f.h, binding);
  } catch (const std::exception &) {
    stale = true;
  }
  require(stale, "old capture binding survived a committed generation");
  // Retire the old top field, leaving the new root at stable slot 3 while the
  // logical node count is only three. This exposes a prefix-based adapter bug.
  RefreshRequest remove(f.h);
  const auto old_root = remove.nodes.back().field;
  commands.clear();
  const auto &s = f.h.prepared_storage();
  for (std::size_t row = 0; row < s.live.edges; ++row)
    if (s.edges[row].second.record == old_root.record) {
      UpdateCommand cmd;
      cmd.operation = UpdateOp::RemoveParticipation;
      cmd.row = row;
      cmd.generation = s.generation;
      commands.push_back(cmd);
    }
  UpdateCommand retire;
  retire.operation = UpdateOp::RetireField;
  retire.first = UpdateRef::existing(old_root);
  commands.push_back(retire);
  remove.nodes.pop_back();
  remove.cohorts.pop_back();
  remove.children = {0, 1};
  refresh_ok(refresh_apply(f, remove, commands));
  const auto &v = f.h.inspect_live_hierarchy();
  require(v.node_count == 3 && v.order[2] == 3,
          "capture fixture did not retain a slot hole");
  const auto before = state_hash(f);
  auto current_binding = viewport::bind(f.h, mapping);
  const auto current = viewport::observe(f.h, current_binding);
  require(current.prepared_generation == f.h.prepared_storage().generation &&
              current.tick == initial.tick && current.comparisons.size() == 3 &&
              current.comparisons.back().comparison == 999 &&
              current.report.particles64 == f.h.particles64() &&
              state_hash(f) == before,
          "capture mixed source generations/packet slots or changed native "
          "execution");
  std::cout
      << "refresh_capture,stale_binding_rejected=1,rejection_binding_preserved="
         "1,stable_slot_hole=1,same_execution=1,observation_neutral=1 PASS\n";
}
#endif
void refresh_u64_and_overlap(engine::Runtime &runtime, HostPath path) {
  {
    const auto C = (std::uint64_t{1} << 59) + 1,
               D = (std::uint64_t{1} << 60) + 3;
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{C, C, 0}, {0, 0, C}, {0, 0, 0}}, {D, D, D}, {1, 1, 2},
              {-1, 1, 4});
    f.install();
    f.control->set(0);
    runtime.synchronize();
    const auto before = f.h.inspect_live_hierarchy();
    RefreshRequest request(f.h);
    std::vector<UpdateCommand> commands;
    const auto &s = f.h.prepared_storage();
    for (std::size_t row = 0; row < s.live.edges; ++row)
      if (s.edges[row].first.record == f.in.particle_rows[0].record) {
        const auto &e = s.edges[row];
        UpdateCommand c;
        c.operation = UpdateOp::ReplaceParticipation;
        c.row = row;
        c.generation = s.generation;
        c.first = UpdateRef::existing(e.first);
        c.second = UpdateRef::existing(e.second);
        c.share = e.share;
        c.exact_count = C + 1;
        c.ordinal = e.ordinal;
        request.replaced(row, int(commands.size()));
        commands.push_back(c);
      }
    require(double(C) == double(C + 1),
            "u64 witness is distinguishable by f64");
    refresh_ok(refresh_apply(f, request, commands));
    f.h.inspect_prepared();
    const auto &after = f.h.inspect_live_hierarchy();
    require(after.slots == before.slots &&
                after.selectors == before.selectors &&
                after.previous == before.previous,
            "numerically equal u64 metadata edit rewrote packets/history");
    for (const auto &c : commands)
      require(f.h.prepared_storage().source_counts[c.row] == C + 1,
              "u64 count was rounded in source provenance");
    require(
        f.h.prepared_storage().inspection_flags[f.in.particle_rows[0].record] &
            2,
        "equal-packet u64 edit lost invalidation");
    RefreshRequest ordinal(f.h);
    UpdateCommand edit;
    const auto row = ordinal.cohorts[0][1].source.row;
    const auto &edge = f.h.prepared_storage().edges[row];
    edit.operation = UpdateOp::ReplaceParticipation;
    edit.row = row;
    edit.generation = f.h.prepared_storage().generation;
    edit.first = UpdateRef::existing(edge.first);
    edit.second = UpdateRef::existing(edge.second);
    edit.ordinal = ordinal.cohorts[0][0].source.ordinal;
    edit.share = edge.share;
    edit.exact_count = f.h.prepared_storage().source_counts[row];
    ordinal.replaced(row, 0);
    refresh_ok(refresh_apply(f, ordinal, {edit}));
    const auto &distinct = f.h.inspect_live_hierarchy();
    require(
        distinct.nodes[0].cohort_count == 2 && distinct.slots == before.slots,
        "distinct source rows with equal ordinals coalesced");
  }
  {
    Fixture f(runtime, path, {{}, {}, {0, 1}},
              {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}}, {1, 2, 1}, {1, 2, 1},
              {0, 2, 4});
    f.install();
    RefreshRequest request(f.h);
    const auto &s = f.h.prepared_storage();
    const auto body = f.in.particle_rows[1];
    std::vector<UpdateCommand> commands;
    for (std::size_t row = 0; row < s.live.edges; ++row)
      if (s.edges[row].first.record == body.record) {
        const auto &e = s.edges[row];
        UpdateCommand c;
        c.row = row;
        c.generation = s.generation;
        if (e.second.record == f.nodes[0].field.record) {
          c.operation = UpdateOp::RemoveParticipation;
          commands.push_back(c);
          auto &cohort = request.cohorts[0];
          cohort.erase(
              std::remove_if(cohort.begin(), cohort.end(),
                             [&](auto &r) { return r.source.row == row; }),
              cohort.end());
        } else if (e.second.record == f.nodes[2].field.record) {
          c.operation = UpdateOp::ReplaceParticipation;
          c.first = UpdateRef::existing(e.first);
          c.second = UpdateRef::existing(e.second);
          c.ordinal = e.ordinal;
          c.exact_count = 1;
          c.share = .5;
          request.replaced(row, int(commands.size()));
          commands.push_back(c);
        }
      }
    refresh_ok(refresh_apply(f, request, commands));
    f.coverage[0][1] = 0;
    f.coverage[2][1] = 1;
    // Rebind source references in the independent fixture model after ledger
    // compaction; expected coverage and hierarchy are explicitly changed above.
    f.refs.clear();
    const auto &current = f.h.prepared_storage();
    for (auto &n : f.nodes) {
      n.cohort_begin = f.refs.size();
      for (std::size_t row = 0; row < current.live.edges; ++row)
        if (current.edges[row].second.record == n.field.record)
          f.refs.push_back(
              {current.edges[row].first,
               {current.generation, row, current.edges[row].ordinal}});
      n.cohort_count = f.refs.size() - n.cohort_begin;
    }
    f.h.inspect_prepared();
    near(f.h.universal64(), 4, "coverage edit changed universal mass");
    near(selected(f.h.inspect_live_hierarchy(), 2)[3], 3,
         "partial root coverage used physical universal mass");
    Parameters64 p;
    p.dt = .03125;
    p.gravity = .25;
    p.contact_stiffness = p.reach_strength = 0;
    auto before = f.h.particles64(), expected = physical_reference(f, p, true);
    no_new([&] { f.h.tick64(p); });
    f.h.inspect_prepared();
    verify_physical(f, expected, before, p);
  }
  std::cout << "refresh_u64_overlap,above_2pow53=1,equal_packet_metadata_wake="
               "1,compatible_overlap=1,changed_root_coverage=1,compaction=1,"
               "combined_integration=1 PASS\n";
}
