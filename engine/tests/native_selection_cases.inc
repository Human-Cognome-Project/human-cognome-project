// B1 selection qualification. The numerical force/poll kernels are the actual
// C3 builders with candidate bindings. This is not a transactional tick API;
// the small particle/contact driver below isolates native phase selection.
template <std::size_t N>
void retain_selection_modules(
    const std::array<const engine::CompiledKernel *, N> &kernels,
    const std::string &stem,
    const char *explicit_directory = nullptr) {
  const char *dir =
      explicit_directory ? explicit_directory : std::getenv("HCP_B1_IR_DIR");
  if (!dir)
    return;
  std::ofstream metadata(std::string(dir) + "/" + stem + ".configuration.txt");
  require(bool(metadata), "selection IR metadata write");
  for (std::size_t i = 0; i < N; ++i) {
    const auto &kernel =
        engine::testing::CompiledKernelAccess::kernel(*kernels[i]);
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(*kernels[i]))
             .get_internal_data()
             .compiled_data.module;
    std::error_code ec;
    llvm::raw_fd_ostream file(
        std::string(dir) + "/" + stem + "-" + std::to_string(i) + ".ll", ec,
        llvm::sys::fs::OF_Text);
    require(!ec, "selection IR module write");
    module.print(file, nullptr);
    metadata << "mode=" << i << ",kernel=" << kernel.get_name()
             << ",triple=" << module.getTargetTriple()
             << ",layout=" << module.getDataLayoutStr() << '\n';
    for (std::size_t arg = 0; arg < kernel.parameter_list.size(); ++arg) {
      const auto &p = kernel.parameter_list[arg];
      metadata << "argument=" << arg << ",dtype=" << p.get_dtype()->to_string()
               << ",rank=" << p.total_dim;
      if (p.is_array) {
        metadata << ",data_offset="
                 << kernel.args_type->get_element_offset(
                        {int(arg),
                         taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY});
        for (std::size_t axis = 0; axis < p.total_dim; ++axis)
          metadata << ",shape" << axis << "_offset="
                   << kernel.args_type->get_element_offset(
                          {int(arg),
                           taichi::lang::TypeFactory::SHAPE_POS_IN_NDARRAY,
                           int(axis)});
      } else
        metadata << ",scalar_offset="
                 << kernel.args_type->get_element_offset({int(arg)});
      metadata << '\n';
    }
  }
}

class SelectionTrial {
 public:
  using Array = taichi::lang::Ndarray;
  using Kernel = engine::CompiledKernel;
  static constexpr std::array<int, 10> mutable_inputs{0,  1,  7,  8,  13,
                                                      14, 15, 16, 28, 29};
  Fixture &f;
  detail::NativeSelectionWorkspace selection;
  std::array<std::unique_ptr<Array>, 30> values;
  std::array<Kernel, 3> kernels;
  Kernel numerical_copy, phases, inspect_bits;
  std::unique_ptr<Array> visits, moving, bits;
  std::array<std::size_t, 30> bytes{};
  const int P, G, N;
  explicit SelectionTrial(Fixture &fixture)
      : f(fixture),
        selection(
            f.runtime,
            LiveHierarchyAccess::particle_set(f.h),
            f.h.particle_capacity(),
            LiveHierarchyAccess::field_set(f.h),
            int(f.h.prepared_storage().capacity.limits()[Budget::Fields])),
        kernels(LiveHierarchyAccess::selection_kernels(f.h, selection)),
        P(f.h.particle_capacity()),
        G(int(f.h.prepared_storage().capacity.limits()[Budget::Fields])),
        N(std::max(P, G)) {
    using namespace taichi::lang;
    for (int input : mutable_inputs) {
      auto *source = LiveHierarchyAccess::input_array(f.h, input);
      auto shape = input == 7   ? std::vector<int>{kParticleFieldCount, P}
                   : input == 8 ? std::vector<int>{kGroupFieldCount, G}
                                : source->shape;
      auto type = input == 7 || input == 8 ? DataType(PrimitiveType::f64)
                                           : source->dtype;
      values[input] =
          std::make_unique<Array>(&f.runtime.program(), type, shape);
      bytes[input] =
          values[input]->get_nelement() * values[input]->get_element_size();
      f.runtime.program().fill_ndarray_fast_u32(values[input].get(), 0);
    }
    visit_before_.resize(bytes[28] / sizeof(std::int64_t));
    visit_after_.resize(visit_before_.size());
    visits = std::make_unique<Array>(&f.runtime.program(), PrimitiveType::i32,
                                     std::vector<int>{P});
    moving = std::make_unique<Array>(&f.runtime.program(), PrimitiveType::i32,
                                     std::vector<int>{P});
    bits = std::make_unique<Array>(&f.runtime.program(), PrimitiveType::i32,
                                   std::vector<int>{4, N});
    const auto suffix = std::to_string(selection.particles().cells->id);
    const auto build_copy = [&](const std::string &name) {
      IRBuilder b;
      std::vector<engine::Param> params;
      auto *enabled = b.create_arg_load({20}, PrimitiveType::i32, false, 0);
      for (int i = 0; i < int(mutable_inputs.size()); ++i) {
        auto &a = *values[mutable_inputs[i]];
        const int rank = a.shape.size();
        auto *from = b.create_ndarray_arg_load({2 * i}, a.dtype, rank, 0);
        auto *to = b.create_ndarray_arg_load({2 * i + 1}, a.dtype, rank, 0);
        params.push_back(engine::Param::array(a.dtype, rank));
        params.push_back(engine::Param::array(a.dtype, rank));
        auto *loop = b.create_range_for(b.get_int32(0),
                                        b.get_int32(int(a.get_nelement())),
                                        false, f.runtime.cpu_threads());
        auto scope = b.get_loop_guard(loop);
        auto allowed = b.get_if_guard(b.create_if(enabled), true);
        auto *index = b.get_loop_index(loop);
        auto at = rank == 1 ? std::vector<Stmt *>{index}
                            : std::vector<Stmt *>{
                                  b.create_div(index, b.get_int32(a.shape[1])),
                                  b.create_mod(index, b.get_int32(a.shape[1]))};
        b.create_global_store(
            b.create_external_ptr(to, at),
            b.create_global_load(b.create_external_ptr(from, at)));
      }
      params.push_back(engine::Param::scalar(PrimitiveType::i32));
      return Kernel(f.runtime, name + suffix, b.extract_ir(), params,
                    engine::MathMode::Strict);
    };
    numerical_copy = build_copy("selection_numeric_stage_");
    publication_copy_ = build_copy("selection_numeric_publish_");
    build_phases(suffix);
    {
      IRBuilder b;
      auto *out = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 2, 0);
      const std::array<detail::NativeSet, 4> sets{
          LiveHierarchyAccess::particle_set(f.h),
          LiveHierarchyAccess::field_set(f.h), selection.particles(),
          selection.centroids()};
      for (int kind = 0; kind < 4; ++kind) {
        auto *loop =
            b.create_range_for(b.get_int32(0), b.get_int32(kind % 2 ? G : P),
                               false, f.runtime.cpu_threads());
        auto scope = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        b.create_global_store(
            b.create_external_ptr(out, {b.get_int32(kind), row}),
            b.create_cast(sets[kind].active(b, row), PrimitiveType::i32));
      }
      inspect_bits =
          Kernel(f.runtime, "selection_inspect_" + suffix, b.extract_ir(),
                 {engine::Param::array(PrimitiveType::i32, 2)});
    }
    stage();
    copy(true, 0);  // Warm publication's actual binding without live writes.
    phase(0);
    run(2, 0, 0);
    run(0, 0, 0);
    run(1, 0, 0);
    membership();
    f.runtime.synchronize();
    for (int i = 0; i < 3; ++i)
      audit(kernels[i], i, nullptr, false);
  }
  void copy(bool publication, int enabled = 1) {
    // Two fixed contexts are needed: reversing bindings is never permitted on
    // a frozen native context. Only the test copy body is shared below.
    auto &kernel = publication ? publish_copy() : numerical_copy;
    auto call = kernel.invocation(true);
    auto &c = call.get();
    if (!c.prepared)
      for (int i = 0; i < int(mutable_inputs.size()); ++i) {
        int input = mutable_inputs[i];
        LiveHierarchyAccess::bind_input(f.h, c, 2 * i + (publication ? 1 : 0),
                                        input);
        c.set_arg_ndarray({2 * i + (publication ? 0 : 1)}, *values[input]);
      }
    engine::argument_int(c, 20, enabled);
    kernel.launch(c);
  }
  void stage() {
    selection.stage();
    copy(false);
    phase(0);
  }
  void publish() {
    copy(true);
    selection.publish();
  }
  void run(int which, double gravity = 0, int target = -1) {
    if (which == 1) {
      f.runtime.synchronize();
      engine::readback(*values[28], visit_before_.data(), bytes[28]);
    }
    auto &kernel = kernels[which];
    auto call = kernel.invocation(true);
    auto &c = call.get();
    if (!c.prepared)
      for (int arg = 0; arg < 30; ++arg) {
        if (arg >= 24 && arg <= 27)
          continue;
        if (values[arg])
          c.set_arg_ndarray({arg}, *values[arg]);
        else
          LiveHierarchyAccess::bind_input(f.h, c, arg, arg);
      }
    engine::argument_int(c, 24, f.nodes.size());
    engine::argument_float(c, 25, gravity);
    engine::argument_int(c, 26, which == 2 ? 0 : target);
    engine::argument_float(c, 27, 0);
    kernel.launch(c);
    if (which == 1) {
      f.runtime.synchronize();
      engine::readback(*values[28], visit_after_.data(), bytes[28]);
      const auto H = visit_before_.size() / 2;
      for (std::size_t n = 0; n < H; ++n) {
        auto visits = visit_after_[n] - visit_before_[n],
             pubs = visit_after_[H + n] - visit_before_[H + n];
        require(visits >= 0 && visits <= 1 && pubs >= 0 && pubs <= visits,
                "candidate poll repeated a comparison or publication");
      }
    }
  }
  std::vector<int> membership() {
    auto call = inspect_bits.invocation(true);
    auto &c = call.get();
    if (!c.prepared)
      c.set_arg_ndarray({0}, *bits);
    inspect_bits.launch(c);
    f.runtime.synchronize();
    std::vector<int> out(4 * N);
    engine::readback(*bits, out.data(), out.size() * 4);
    return out;
  }
  std::vector<int> work() {
    std::vector<int> out(P);
    engine::readback(*visits, out.data(), out.size() * 4);
    return out;
  }
  template <class T>
  std::vector<T> read(int input) {
    std::vector<T> out(bytes[input] / sizeof(T));
    engine::readback(*values[input], out.data(), bytes[input]);
    return out;
  }
  using Snapshot = std::array<std::vector<unsigned char>, 10>;
  Snapshot live() {
    f.runtime.synchronize();
    Snapshot out;
    for (int i = 0; i < 10; ++i) {
      int input = mutable_inputs[i];
      out[i].resize(bytes[input]);
      LiveHierarchyAccess::read_input(f.h, input, out[i].data(), out[i].size());
    }
    return out;
  }
  Snapshot candidate() {
    f.runtime.synchronize();
    Snapshot out;
    for (int i = 0; i < 10; ++i) {
      int input = mutable_inputs[i];
      out[i].resize(bytes[input]);
      engine::readback(*values[input], out[i].data(), out[i].size());
    }
    return out;
  }
  void phase(int op, int row = 0) {
    auto call = phases.invocation(true);
    auto &c = call.get();
    if (!c.prepared) {
      c.set_arg_ndarray({0}, *values[7]);
      c.set_arg_ndarray({1}, *visits);
      c.set_arg_ndarray({2}, *moving);
    }
    engine::argument_int(c, 3, op);
    engine::argument_int(c, 4, row);
    phases.launch(c);
  }
  auto retained_kernels() const {
    const auto masks =
        detail::testing::NativeSelectionAccess::kernels(selection);
    return std::array<const Kernel *, 9>{
        masks[0],        masks[1],           masks[2],
        &kernels[0],     &kernels[1],        &kernels[2],
        &numerical_copy, &publication_copy_, &phases};
  }

 private:
  std::vector<std::int64_t> visit_before_, visit_after_;
  Kernel publication_copy_;
  Kernel &publish_copy() {
    return publication_copy_;
  }
  void build_phases(const std::string &suffix);
};

void SelectionTrial::build_phases(const std::string &suffix) {
  using namespace taichi::lang;
  IRBuilder b;
  auto *bodies = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0);
  auto *work = b.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0);
  auto *motion = b.create_ndarray_arg_load({2}, PrimitiveType::i32, 1, 0);
  auto *op = b.create_arg_load({3}, PrimitiveType::i32, false, 0);
  auto *row = b.create_arg_load({4}, PrimitiveType::i32, false, 0);
  const int mover = f.in.particle_rows[0].record,
            peer = f.in.particle_rows[32].record,
            chain = f.in.particle_rows[64].record;
  const auto particles = selection.particles(),
             centroids = selection.centroids();
  const auto read = [&](auto *a, std::vector<Stmt *> at) {
    return b.create_global_load(b.create_external_ptr(a, at));
  };
  const auto put = [&](auto *a, std::vector<Stmt *> at, Stmt *v) {
    b.create_global_store(b.create_external_ptr(a, at), v);
  };
  {
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(P), false,
                                    f.runtime.cpu_threads());
    auto scope = b.get_loop_guard(loop);
    auto yes =
        b.get_if_guard(b.create_if(b.create_cmp_eq(op, b.get_int32(0))), true);
    put(work, {b.get_loop_index(loop)}, b.get_int32(0));
    put(motion, {b.get_loop_index(loop)}, b.get_int32(0));
  }
  // Variant 3 is an intentional unsafe control: peer activation inside the
  // calculation traversal can admit a sleeper into calculation early.
  for (int variant : {1, 3}) {
    auto *loop = b.create_struct_for(
        particles.cells, false, variant == 3 ? 1 : f.runtime.cpu_threads(), 1);
    auto scope = b.get_loop_guard(loop);
    auto yes = b.get_if_guard(
        b.create_if(b.create_cmp_eq(op, b.get_int32(variant))), true);
    auto *p = b.get_loop_index(loop);
    b.create_atomic_add(b.create_external_ptr(work, {p}), b.get_int32(1));
    const auto at = std::vector<Stmt *>{b.get_int32(kPosX), p};
    put(bodies, at, b.create_add(read(bodies, at), b.get_float64(1)));
    auto *moving = b.create_cmp_eq(p, b.get_int32(mover));
    put(motion, {p}, b.create_cast(moving, PrimitiveType::i32));
    {
      auto fixed =
          b.get_if_guard(b.create_if(b.create_logical_not(moving)), true);
      particles.deactivate(b, p);
    }
    centroids.activate(
        b, b.create_select(moving, b.get_int32(f.nodes[0].field.record),
                           b.get_int32(f.nodes[1].field.record)));
    if (variant == 3) {
      auto touch = b.get_if_guard(b.create_if(moving), true);
      particles.activate(b, b.get_int32(f.in.particle_rows[1].record));
    }
  }
  {
    auto *loop =
        b.create_struct_for(particles.cells, false, f.runtime.cpu_threads());
    auto scope = b.get_loop_guard(loop);
    auto *p = b.get_loop_index(loop);
    auto moving = b.get_if_guard(
        b.create_if(b.create_logical_and(b.create_cmp_eq(op, b.get_int32(2)),
                                         read(motion, {p}))),
        true);
    particles.activate(b,
                       b.create_select(b.create_cmp_eq(p, b.get_int32(mover)),
                                       b.get_int32(peer), b.get_int32(chain)));
  }
  {
    auto late =
        b.get_if_guard(b.create_if(b.create_cmp_eq(op, b.get_int32(4))), true);
    centroids.deactivate(b, row);
  }
  {
    auto notifier =
        b.get_if_guard(b.create_if(b.create_cmp_eq(op, b.get_int32(5))), true);
    centroids.deactivate(b, b.get_int32(f.nodes[2].field.record));
    centroids.activate(b, b.get_int32(f.nodes[3].field.record));
  }
  {
    auto wrong =
        b.get_if_guard(b.create_if(b.create_cmp_eq(op, b.get_int32(6))), true);
    LiveHierarchyAccess::particle_set(f.h).activate(b, b.get_int32(peer));
  }
  phases = Kernel(f.runtime, "selection_phase_" + suffix, b.extract_ir(),
                  {engine::Param::array(PrimitiveType::f64, 2),
                   engine::Param::array(PrimitiveType::i32),
                   engine::Param::array(PrimitiveType::i32),
                   engine::Param::scalar(PrimitiveType::i32),
                   engine::Param::scalar(PrimitiveType::i32)},
                  engine::MathMode::Strict);
}

std::unique_ptr<Fixture> selection_fixture(engine::Runtime &r,
                                           HostPath path,
                                           bool permute = false,
                                           int bits = 52) {
  const auto letter = letter_data(r);
  std::vector<std::uint64_t> denominators(70);
  std::vector<double> mass(70), positions(70), radii(70);
  std::vector<std::vector<std::uint64_t>> counts(
      4, std::vector<std::uint64_t>(70));
  for (int i = 0; i < 70; ++i) {
    denominators[i] = letter.denominators[i % letter.denominators.size()];
    mass[i] = letter.masses[i % letter.masses.size()];
    positions[i] = 4 * i;
    radii[i] = letter.radii[i % letter.radii.size()];
    counts[i ? 1 : 0][i] = denominators[i];
  }
  auto out = std::make_unique<Fixture>(
      r, path, std::vector<std::vector<int>>{{}, {}, {0, 1}, {2}}, counts,
      denominators, mass, positions, bits, bits == 52 ? 52 : 6, false, false,
      false, permute, 1024, radii);
  out->install();
  return out;
}
bool committed_membership_equal(const SelectionTrial &t,
                                const std::vector<int> &a,
                                const std::vector<int> &b) {
  return std::equal(a.begin(), a.begin() + 2 * t.N, b.begin());
}
void selection_isolation(engine::Runtime &r, HostPath path) {
  auto fixture = selection_fixture(r, path);
  auto &f = *fixture;
  f.control->set(0);
  f.control->set(2, f.in.particle_rows[0].record);
  f.control->set(1, f.nodes[0].field.record);
  r.synchronize();
  const auto original = state_hash(f);
  SelectionTrial t(f);
  require(state_hash(f) == original, "selection setup wrote committed state");
  const auto live = t.live();
  const auto entry = t.membership();
  t.stage();
  require(t.candidate() == live,
          "native candidate copy lost packet/history/selector input");
  t.phase(1);
  t.phase(2);
  r.synchronize();
  const auto work = t.work(), contact = t.membership();
  const int peer = f.in.particle_rows[32].record,
            chain = f.in.particle_rows[64].record;
  require(work[0] == 1 && work[peer] == 0 && work[chain] == 0,
          "candidate contact wake entered this particle calculation");
  require(contact[2 * t.N + peer] == 1 && contact[2 * t.N + chain] == 0,
          "candidate propagation ran through a newly touched sleeper");
  const auto visits = t.read<std::int64_t>(28);
  t.run(1);
  r.synchronize();
  const auto after = t.read<std::int64_t>(28);
  for (int i = 0; i < 70; ++i)
    require(after[i] - visits[i] >= 0 && after[i] - visits[i] <= 1,
            "candidate native poll visited a comparison twice");
  if (r.cpu_threads() == 1)
    require(after[2] - visits[2] == 1 && after[3] - visits[3] == 1,
            "native same-poll centroid progress was frozen");
  require(committed_membership_equal(t, entry, t.membership()) &&
              t.live() == live && state_hash(f) == original,
          "candidate calculation changed committed activity or "
          "numerical/control state");
  t.selection.abandon();
  r.synchronize();
  const auto abandoned = t.membership();
  require(std::all_of(abandoned.begin() + 2 * t.N, abandoned.end(),
                      [](int x) { return x == 0; }) &&
              committed_membership_equal(t, entry, abandoned) &&
              state_hash(f) == original,
          "abandon modified committed selection or retained candidate bits");
  t.stage();
  require(t.candidate() == live &&
              committed_membership_equal(t, entry, t.membership()),
          "retry inherited rejected candidate numerical/history/bit effects");
  t.phase(1);
  t.phase(2);
  t.run(1);
  r.synchronize();
  const auto candidate = t.candidate();
  const auto bits = t.membership();
  t.publish();
  r.synchronize();
  require(t.live() == candidate,
          "publication did not deliver the complete candidate surfaces");
  const auto published = t.membership();
  require(std::equal(published.begin(), published.begin() + 2 * t.N,
                     bits.begin() + 2 * t.N),
          "publication did not deliver native final membership");
  std::cout
      << "selection_isolation,contact_peer=32,chain=64,committed_rejection="
         "exact,"
         "native_same_poll=preserved,at_most_one=1,publish=complete PASS\n";
}

void selection_controls(engine::Runtime &r, HostPath path) {
  auto fixture = selection_fixture(r, path);
  auto &f = *fixture;
  const auto setup = [&] {
    f.control->set(0);
    f.control->set(2, f.in.particle_rows[0].record);
    f.control->set(1, f.nodes[0].field.record);
    f.control->set(1, f.nodes[2].field.record);
    r.synchronize();
  };
  setup();
  SelectionTrial t(f);
  const auto original = state_hash(f);
  int negatives = 0;
  const auto expected_failure = [&](const char *name, auto action, auto check) {
    setup();
    t.stage();
    action();
    r.synchronize();
    bool failed = false;
    try {
      check();
    } catch (const std::runtime_error &) {
      failed = true;
    }
    require(failed, "selection unsafe control was accepted");
    ++negatives;
    std::cout << "selection_negative,control=" << name << " rejected PASS\n";
  };
  expected_failure(
      "peer_inside_particle_calculation", [&] { t.phase(3); },
      [&] {
        require(t.work()[f.in.particle_rows[1].record] == 0,
                "sleeping peer calculated early");
      });
  const auto late = [&] {
    t.phase(1);
    t.run(1, 0, 2);
    t.run(1, 0, 0);
    r.synchronize();
    require(t.membership()[3 * t.N + f.nodes[2].field.record] == 1,
            "control setup lost late parent input");
  };
  setup();
  t.stage();
  late();
  require(state_hash(f) == original,
          "late notification affected committed state");
  auto before = t.read<std::int64_t>(28);
  t.run(1, 0, 2);
  r.synchronize();
  require(t.read<std::int64_t>(28)[2] - before[2] == 1,
          "valid retry did not fully consume late parent input");
  expected_failure(
      "consume_after_notification",
      [&] {
        late();
        t.phase(4, f.nodes[2].field.record);
      },
      [&] {
        require(t.membership()[3 * t.N + f.nodes[2].field.record] == 1,
                "late notification consumed");
      });
  expected_failure(
      "propagation_only_centroid",
      [&] {
        t.phase(1);
        t.run(1, 0, 0);
        t.phase(5);
        t.run(1, 0, 3);
      },
      [&] {
        const auto visited = t.read<std::int64_t>(28);
        require(visited[2] > 0,
                "notifier consumed a centroid without full calculation");
      });
  if (r.cpu_threads() == 1)
    expected_failure(
        "frozen_centroid_work",
        [&] {
          t.phase(4, f.nodes[2].field.record);
          t.phase(1);
          t.run(1, 0, 0);
        },
        [&] {
          const auto visited = t.read<std::int64_t>(28);
          require(visited[2] > 0 && visited[3] > 0,
                  "new centroid omitted from full native poll");
        });
  expected_failure(
      "committed_target", [&] { t.phase(6); },
      [&] {
        require(state_hash(f) == original,
                "candidate activation changed committed bits");
      });
  // Reset only this deliberately faulty test fixture, never an operational
  // rejection/rollback path. Production stage/abandon needed no source restore.
  setup();
  require(state_hash(f) == original, "unsafe-control fixture cleanup");
  t.selection.abandon();
  r.synchronize();
  std::cout << "selection_controls,expected_rejections=" << negatives
            << ",late_pending=preserved,valid_late_calculation=1 PASS\n";
}

void selection_dirty_force(engine::Runtime &r, HostPath path) {
  auto fixture = selection_fixture(r, path, false, 8);
  auto &f = *fixture;
  f.h.inspect_prepared();
  const auto &v = f.h.inspect_live_hierarchy();
  auto slots = v.slots, history = v.previous;
  auto selectors = v.selectors;
  // A valid selected publication with a different inactive packet/history.
  // The source body's unconsumed edit makes that selected publication lag.
  selectors[1] = 1;
  for (int k = 0; k < 4; ++k) {
    slots[(4 + k) * 70 + 1] = slots[k * 70 + 1];
    slots[k * 70 + 1] = k == 3 ? slots[(4 + k) * 70 + 1] : -16 * (k + 1);
    history[k * 70 + 1] = k == 3 ? slots[(4 + k) * 70 + 1] : -32 * (k + 1);
  }
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 0), slots.data(),
                 slots.size() * 8);
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 1), selectors.data(),
                 selectors.size() * 4);
  engine::upload(*LiveHierarchyAccess::input_array(f.h, 13), history.data(),
                 history.size() * 8);
  f.h.particle64(kPosX, f.in.particle_rows[69].record) += 1;
  PreparedFixtureAccess::upload_body_fixture64(f.h);
  f.control->set(0);
  for (const auto &body : f.in.particle_rows)
    if (body.context)
      f.control->set(2, body.record);
  f.control->set(1, f.nodes[1].field.record);
  r.synchronize();
  const auto hash = state_hash(f);
  SelectionTrial t(f);
  const auto live = t.live();
  t.stage();
  require(t.candidate() == live, "stage reset dirty slots/selector/history");
  auto p = Parameters64{};
  p.gravity = .125;
  p.dt = p.contact_stiffness = p.reach_strength = 0;
  const auto expected = physical_reference(f, p, true);
  t.run(2);
  t.run(0, p.gravity);
  r.synchronize();
  const auto actual = t.read<double>(7);
  for (int i = 0; i < 70; ++i)
    for (int k = 0; k < 3; ++k)
      near(actual[(kForceX + k) * 70 + i], expected[(kForceX + k) * 70 + i],
           "candidate force reader differs from independent combined-force "
           "reference");
  require(t.live() == live && state_hash(f) == hash,
          "candidate force changed committed body/publication state");
  t.selection.abandon();
  t.stage();
  require(t.candidate() == live && state_hash(f) == hash,
          "dirty candidate rejection lost history or pending input");
  std::cout
      << "selection_dirty_force,selector=1,unequal_slots=1,distinct_history=1,"
         "pending_lag=1,force=independent_reference,body_bits=8,field_bits=6 "
         "PASS\n";
}

void selection_reuse(engine::Runtime &r, HostPath path) {
  auto fixture = selection_fixture(r, path, true);
  auto &f = *fixture;
  f.control->set(0);
  f.control->set(2, f.in.particle_rows[0].record);
  f.control->set(1, f.nodes[0].field.record);
  r.synchronize();
  SelectionTrial t(f);
  auto nodes = PreparedFixtureAccess::nodes(f.h);
  for (auto set : {t.selection.particles(), t.selection.centroids()})
    for (auto *node : {set.cells->parent->parent, set.cells->parent, set.cells})
      if (std::find(nodes.begin(), nodes.end(), node) == nodes.end())
        nodes.push_back(node);
  prepared_test::Metrics metrics(r, nodes);
  const auto before = metrics.sample();
  for (int round = 0; round < 32; ++round) {
    const auto hash = state_hash(f);
    const auto entry = t.membership();
    no_new([&] {
      t.stage();
      t.phase(1);
      t.phase(2);
      t.run(2);
      t.run(0);
      t.run(1);
      t.selection.abandon();
      r.synchronize();
    });
    require(state_hash(f) == hash &&
                committed_membership_equal(t, entry, t.membership()),
            "reuse rejection leaked numerical or selection effects");
    no_new([&] {
      t.stage();
      t.phase(1);
      t.phase(2);
      t.run(2);
      t.run(0);
      t.run(1);
      t.publish();
      r.synchronize();
    });
    // Held-input ordinary polls discharge any native record-order lag. No
    // extra body calculation, solve-by-depth or scheduler is introduced.
    for (int poll = 0; poll < 8; ++poll) {
      no_new([&] {
        t.stage();
        t.run(2);
        t.run(1);
        t.publish();
        r.synchronize();
      });
      const auto membership = t.membership();
      if (std::none_of(membership.begin() + t.N, membership.begin() + 2 * t.N,
                       [](int x) { return x; }))
        break;
      require(poll != 7, "candidate native poll stranded pending input");
    }
    f.h.inspect_prepared();
    hierarchy_reference(f);
  }
  const auto after = metrics.sample();
  before.print("selection_reuse_before");
  after.print("selection_reuse_after");
  require(before.same_backing(after),
          "selection reuse changed prepared native backing/layout/compiler "
          "counters");
  std::cout
      << "selection_reuse,rounds=32,rejections=32,retained_letter_intrinsics=1,"
         "permuted_native_order=1,cpp_new_observed=0,binding_bytes="
      << t.selection.binding_bytes() << " PASS\n";
}

void selection_capacity(engine::Runtime &r) {
  using namespace taichi::lang;
  for (const auto sizes :
       {std::array<int, 2>{0, 0}, std::array<int, 2>{70, 70},
        std::array<int, 2>{4096, 70}, std::array<int, 2>{70, 4096}}) {
    const int size = std::max(sizes[0], sizes[1]);
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    const std::array<detail::NativeSet, 2> live{
        detail::NativeSet::attach(*root, sizes[0]),
        detail::NativeSet::attach(*root, sizes[1])};
    r.program().add_snode_tree(std::move(root), false);
    const auto suffix = std::to_string(live[0].cells->id);
    {
      IRBuilder b;
      for (int type = 0; type < 2; ++type) {
        auto *loop = b.create_range_for(
            b.get_int32(0), b.get_int32(sizes[type]), false, r.cpu_threads());
        auto scope = b.get_loop_guard(loop);
        live[type].activate(b, b.get_loop_index(loop));
      }
      for (const auto &s : live) {
        auto *loop = b.create_struct_for(s.cells, false, r.cpu_threads());
        auto scope = b.get_loop_guard(loop);
        s.deactivate(b, b.get_loop_index(loop));
      }
      engine::CompiledKernel warm(r, "selection_live_provision_" + suffix,
                                  b.extract_ir(), {});
      auto c = warm.make_context();
      warm.launch(c);
      r.synchronize();
    }
    const auto issued = r.issued_snode_ids();
    for (int bad = 0; bad < 3; ++bad) {
      bool refused = false;
      try {
        detail::NativeSelectionWorkspace invalid(r, live[0],
                                                 bad == 0   ? -1
                                                 : bad == 1 ? 2 * sizes[0] + 65
                                                            : sizes[0],
                                                 bad == 2 ? live[0] : live[1],
                                                 sizes[1]);
      } catch (const std::invalid_argument &) {
        refused = true;
      }
      require(refused && r.issued_snode_ids() == issued,
              "invalid native selection setup allocated a layout");
    }
    detail::NativeSelectionWorkspace candidate(r, live[0], sizes[0], live[1],
                                               sizes[1]);
    const auto mask_kernels =
        detail::testing::NativeSelectionAccess::kernels(candidate);
    const std::array<detail::NativeSet, 2> scratch_sets{candidate.particles(),
                                                        candidate.centroids()};
    for (int mode = 0; mode < 3; ++mode) {
      const auto &module =
          *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
               engine::testing::CompiledKernelAccess::compiled(
                   *mask_kernels[mode]))
               .get_internal_data()
               .compiled_data.module;
      selection_audit::Analyzer(mode == 2 ? live : scratch_sets)
          .run(module, mode);
    }
    const std::array<detail::NativeSet, 4> sets{
        live[0], live[1], candidate.particles(), candidate.centroids()};
    const int stride = std::max(size, 1);
    Ndarray flags(&r.program(), PrimitiveType::i32, {4, stride}),
        counts(&r.program(), PrimitiveType::i32, {8});
    std::vector<int> flag_values(4 * stride);
    std::array<int, 8> count_values{};
    engine::CompiledKernel command, inspect;
    {
      IRBuilder b;
      auto *op = b.create_arg_load({0}, PrimitiveType::i32, false, 0),
           *kind = b.create_arg_load({1}, PrimitiveType::i32, false, 0),
           *row = b.create_arg_load({2}, PrimitiveType::i32, false, 0);
      for (int i = 0; i < 4; ++i) {
        {
          auto *loop =
              b.create_struct_for(sets[i].cells, false, r.cpu_threads());
          auto scope = b.get_loop_guard(loop);
          auto zero =
              b.get_if_guard(b.create_if(b.create_logical_and(
                                 b.create_cmp_eq(op, b.get_int32(0)),
                                 b.create_cmp_eq(kind, b.get_int32(i)))),
                             true);
          sets[i].deactivate(b, b.get_loop_index(loop));
        }
        auto which = b.get_if_guard(
            b.create_if(b.create_cmp_eq(kind, b.get_int32(i))), true);
        {
          auto yes = b.get_if_guard(
              b.create_if(b.create_cmp_eq(op, b.get_int32(1))), true);
          sets[i].activate(b, row);
        }
        {
          auto yes = b.get_if_guard(
              b.create_if(b.create_cmp_eq(op, b.get_int32(2))), true);
          sets[i].deactivate(b, row);
        }
      }
      command = engine::CompiledKernel(
          r, "selection_capacity_command_" + suffix, b.extract_ir(),
          {engine::Param::scalar(PrimitiveType::i32),
           engine::Param::scalar(PrimitiveType::i32),
           engine::Param::scalar(PrimitiveType::i32)});
    }
    {
      IRBuilder b;
      auto *bits = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 2, 0),
           *out = b.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0);
      for (int i = 0; i < 8; ++i)
        b.create_global_store(b.create_external_ptr(out, {b.get_int32(i)}),
                              b.get_int32(0));
      for (int i = 0; i < 4; ++i) {
        {
          auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(stride),
                                          false, r.cpu_threads());
          auto scope = b.get_loop_guard(loop);
          b.create_global_store(
              b.create_external_ptr(bits,
                                    {b.get_int32(i), b.get_loop_index(loop)}),
              b.get_int32(0));
        }
        auto *loop = b.create_struct_for(sets[i].cells, false, r.cpu_threads());
        auto scope = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        b.create_atomic_add(b.create_external_ptr(out, {b.get_int32(i)}),
                            b.get_int32(1));
        auto *branch =
            b.create_if(b.create_cmp_lt(row, b.get_int32(sizes[i % 2])));
        {
          auto valid = b.get_if_guard(branch, true);
          b.create_global_store(
              b.create_external_ptr(bits, {b.get_int32(i), row}),
              b.get_int32(1));
        }
        {
          auto padding = b.get_if_guard(branch, false);
          b.create_atomic_add(b.create_external_ptr(out, {b.get_int32(i + 4)}),
                              b.get_int32(1));
        }
      }
      inspect = engine::CompiledKernel(
          r, "selection_capacity_inspect_" + suffix, b.extract_ir(),
          {engine::Param::array(PrimitiveType::i32, 2),
           engine::Param::array(PrimitiveType::i32)});
    }
    const auto set = [&](int op, int kind, int row = 0) {
      auto call = command.invocation(true);
      auto &c = call.get();
      engine::argument_int(c, 0, op);
      engine::argument_int(c, 1, kind);
      engine::argument_int(c, 2, row);
      command.launch(c);
    };
    const auto read = [&] {
      auto call = inspect.invocation(true);
      auto &c = call.get();
      if (!c.prepared) {
        c.set_arg_ndarray({0}, flags);
        c.set_arg_ndarray({1}, counts);
      }
      inspect.launch(c);
      r.synchronize();
      engine::readback(flags, flag_values.data(), flag_values.size() * 4);
      engine::readback(counts, count_values.data(), sizeof(count_values));
    };
    set(0, 0);
    read();
    std::array<std::vector<int>, 2> rows;
    for (int type = 0; type < 2; ++type)
      if (sizes[type]) {
        rows[type] = {0, 31, 32, sizes[type] - 1};
        std::sort(rows[type].begin(), rows[type].end());
        rows[type].erase(std::unique(rows[type].begin(), rows[type].end()),
                         rows[type].end());
      }
    const auto seed = [&] {
      for (int type = 0; type < 2; ++type) {
        set(0, type);
        for (int row : rows[type])
          set(1, type, row);
      }
    };
    const auto check = [&](bool scratch, bool changed = false) {
      read();
      for (int kind = 0; kind < 4; ++kind)
        for (int row = 0; row < stride; ++row) {
          const auto &source = rows[kind % 2];
          const int bound = sizes[kind % 2];
          bool expected =
              (kind < 2 || scratch) &&
              std::find(source.begin(), source.end(), row) != source.end();
          if (changed && bound && row == bound - 1)
            expected = false;
          if (changed && bound && row == 1)
            expected = true;
          require(flag_values[kind * stride + row] == int(expected),
                  "selection copied/reused wrong native bit");
        }
      for (int i = 4; i < 8; ++i)
        require(count_values[i] == 0, "selection activated sparse padding");
    };
    seed();
    candidate.stage();
    read();
    candidate.abandon();
    candidate.stage();
    candidate.publish();
    read();
    std::vector<SNode *> nodes;
    for (auto s : sets)
      for (auto *node : {s.cells->parent->parent, s.cells->parent, s.cells})
        if (std::find(nodes.begin(), nodes.end(), node) == nodes.end())
          nodes.push_back(node);
    prepared_test::Metrics metrics(r, nodes);
    const auto before = metrics.sample();
    for (int round = 0; round < 32; ++round) {
      no_new([&] {
        seed();
        candidate.stage();
        r.synchronize();
      });
      check(true);
      no_new([&] {
        candidate.abandon();
        r.synchronize();
      });
      check(false);
      no_new([&] {
        candidate.stage();
        for (int type = 2; type < 4; ++type)
          if (sizes[type % 2]) {
            set(2, type, sizes[type % 2] - 1);
            set(1, type, 1);
          }
        candidate.publish();
        r.synchronize();
      });
      check(true, true);
    }
    const auto after = metrics.sample();
    before.print("selection_capacity_before");
    after.print("selection_capacity_after");
    require(before.same_backing(after),
            "native selection capacity reuse allocated backing or compiled");
    retain_selection_modules(
        detail::testing::NativeSelectionAccess::kernels(candidate),
        "capacity-" + std::to_string(sizes[0]) + "-" +
            std::to_string(sizes[1]));
    std::cout
        << "selection_capacity,particles=" << sizes[0]
        << ",centroids=" << sizes[1] << ",rounds=32,last_record=" << size - 1
        << ",setup_rejections=3,padding=0,cpp_new_observed=0,binding_bytes="
        << candidate.binding_bytes() << " PASS\n";
  }
}

void selection_compiler_controls(engine::Runtime &r, HostPath path) {
  auto fixture = selection_fixture(r, path);
  SelectionTrial t(*fixture);
  audit_negative(t.kernels[0], 0);
  audit_negative(t.kernels[1], 1);
  const auto originals =
      detail::testing::NativeSelectionAccess::committed(t.selection);
  const std::array<detail::NativeSet, 2> candidates{t.selection.particles(),
                                                    t.selection.centroids()};
  const auto kernels =
      detail::testing::NativeSelectionAccess::kernels(t.selection);
  int negatives = 0, positives = 0;
  for (int mode = 0; mode < 3; ++mode) {
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(*kernels[mode]))
             .get_internal_data()
             .compiled_data.module;
    const auto allowed = mode == 2 ? originals : candidates;
    const auto forbidden = mode == 2 ? candidates : originals;
    const auto report = selection_audit::Analyzer(allowed).run(module, mode);
    const auto expected = [&](const char *name, auto mutate) {
      auto clone = llvm::CloneModule(module);
      selection_audit::Analyzer a(allowed);
      auto facts = a.run(*clone, mode);
      mutate(*clone, a, facts);
      require(!llvm::verifyModule(*clone, &llvm::errs()),
              "invalid native selection LLVM control");
      bool failed = false;
      try {
        selection_audit::Analyzer(allowed).run(*clone, mode);
      } catch (const std::runtime_error &) {
        failed = true;
      }
      require(failed, "native selection audit accepted unsafe target/order");
      ++negatives;
      std::cout << "selection_compiler_negative,mode=" << mode
                << ",control=" << name << " rejected PASS\n";
    };
    expected("wrong_tree", [&](auto &, auto &a, const auto &facts) {
      auto *lookup = a.lookup(
          const_cast<llvm::Value *>(facts.masks.front()->getPointerOperand()),
          selection_audit::Roots);
      require(lookup, "native root lookup control missing");
      auto *old = llvm::cast<llvm::ConstantInt>(
          lookup->getOperand(lookup->getNumOperands() - 1));
      lookup->setOperand(
          lookup->getNumOperands() - 1,
          llvm::ConstantInt::get(old->getType(),
                                 forbidden[0].cells->get_snode_tree_id()));
    });
    expected("wrong_ambient_node", [&](auto &, auto &a, const auto &facts) {
      auto *lookup = a.lookup(
          const_cast<llvm::Value *>(facts.masks.front()->getPointerOperand()),
          selection_audit::Ambient);
      require(lookup, "native ambient lookup control missing");
      auto *old = llvm::cast<llvm::ConstantInt>(
          lookup->getOperand(lookup->getNumOperands() - 1));
      lookup->setOperand(lookup->getNumOperands() - 1,
                         llvm::ConstantInt::get(
                             old->getType(), forbidden[0].cells->parent->id));
    });
    expected("weak_mask_order", [](auto &, auto &, const auto &facts) {
      const_cast<llvm::AtomicRMWInst *>(facts.masks.front())
          ->setOrdering(llvm::AtomicOrdering::Monotonic);
    });
    expected("plain_mask_write", [](auto &, auto &, const auto &facts) {
      auto *atomic = const_cast<llvm::AtomicRMWInst *>(facts.masks.front());
      llvm::IRBuilder<> b(atomic);
      auto *old = b.CreateLoad(atomic->getType(), atomic->getPointerOperand());
      old->setVolatile(true);
      auto *value = atomic->getOperation() == llvm::AtomicRMWInst::And
                        ? b.CreateAnd(old, atomic->getValOperand())
                        : b.CreateOr(old, atomic->getValOperand());
      b.CreateStore(value, atomic->getPointerOperand())->setVolatile(true);
      atomic->replaceAllUsesWith(old);
      atomic->eraseFromParent();
    });
    auto safe = llvm::CloneModule(module);
    const auto facts = selection_audit::Analyzer(allowed).run(*safe, mode);
    auto *first = const_cast<llvm::AtomicRMWInst *>(facts.masks.front());
    llvm::IRBuilder<> b(first);
    b.CreateAtomicRMW(llvm::AtomicRMWInst::Or, first->getPointerOperand(),
                      llvm::ConstantInt::get(first->getType(), 0),
                      llvm::MaybeAlign(4),
                      llvm::AtomicOrdering::SequentiallyConsistent);
    require(!llvm::verifyModule(*safe, &llvm::errs()),
            "invalid safe native mask control");
    selection_audit::Analyzer(allowed).run(*safe, mode);
    ++positives;
    std::cout << "selection_mask_audit,mode=" << mode
              << ",clear_sites=" << report.clears
              << ",activation_sites=" << report.activations
              << ",root_target=owned,all_seq_cst=1,safe_duplicate=1 PASS\n";
  }
  std::cout << "selection_compiler_controls,candidate_force_poll=qualified,"
               "existing_semantic_controls=preserved,mask_negative_controls="
            << negatives << ",mask_safe_controls=" << positives << " PASS\n";
}
void collect_selection_ir(engine::Runtime &r, HostPath path, bool no_unroll) {
  const char *dir = std::getenv("HCP_B1_IR_DIR");
  require(dir && *dir, "selection collector requires HCP_B1_IR_DIR");
  auto fixture = selection_fixture(r, path);
  SelectionTrial t(*fixture);
  retain_selection_modules(t.retained_kernels(), "candidate");
  std::ofstream meta(std::string(dir) + "/configuration.txt");
  meta << "actual_backend=x64\nworkers=" << r.cpu_threads()
       << "\nsource_path=" << (path == HostPath::BulkCopy ? "bulk" : "direct")
       << "\nllvm_unroll=" << (no_unroll ? "disabled-test-control" : "default")
       << "\nllvm_optimizer_mcpu=" << llvm::sys::getHostCPUName().str()
       << "\nmath=strict\n";
  for (auto set : {LiveHierarchyAccess::particle_set(fixture->h),
                   LiveHierarchyAccess::field_set(fixture->h),
                   t.selection.particles(), t.selection.centroids()})
    meta << "native_set=" << set.cells->id
         << ",pointer=" << set.cells->parent->id
         << ",root=" << set.cells->parent->parent->id
         << ",tree=" << set.cells->get_snode_tree_id()
         << ",cells_per_container=" << set.cells->num_cells_per_container
         << ",pointer_cells=" << set.cells->parent->num_cells_per_container
         << '\n';
  selection_capacity(r);
  std::cout << "selection_compiler_collection,modules=21,capacity_profiles=4,"
               "body_calculation=test_driver,contact_safe_tick=unbuilt PASS\n";
}
