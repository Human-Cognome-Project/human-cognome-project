// Private D6 boundary. Only exact full-precision translations are supported.
// Check and publication use the same strict native arithmetic under exclusive
// ownership; the first pass writes only the bounded status scalar.
void Harness::prepare_rebase_execution() {
  using namespace taichi::lang;
  const auto d = PrimitiveType::f64, i = PrimitiveType::i32;
  for (bool commit : {false, true}) {
  IRBuilder b;
  auto *body = array(b, 0, d, 2), *groups = array(b, 1, d, 2),
       *history = array(b, 2, d, 2), *focus = array(b, 3, d),
       *slots = array(b, 4, d, 2), *previous = array(b, 5, d, 2),
       *owners = array(b, 6, i), *status = array(b, 7, i);
  auto *mode = b.create_arg_load({11}, i, false, 0);
  {
    auto enabled = b.get_if_guard(b.create_if(mode), true);
    if (!commit) {
      put(b, status, 0, b.get_int32(0));
    }
    const auto translate = [&](ArgLoadStmt *a, std::vector<Stmt *> at, int axis) {
      auto *x = read(b, a, at);
      auto *delta = b.create_arg_load({8 + axis}, d, false, 0);
      auto *y = b.create_sub(x, delta);
      if (commit) {
        write(b, a, at, y);
        return;
      }
      // Error-free TwoDiff: rounded result plus residual equals x - delta.
      // Round-trip equality alone misses a small delta lost against large x.
      auto *bv = b.create_sub(x, y);
      auto *av = b.create_add(y, bv);
      auto *br = b.create_sub(bv, delta);
      auto *ar = b.create_sub(x, av);
      auto *error = b.create_add(ar, br);
      // The native runtime uses FTZ. Keep every nonzero input's least bit
      // above the subnormal boundary, so a nonzero TwoDiff residual cannot
      // flush into a false exactness certificate. Zero shift is always exact.
      const auto residual_range = [&](Stmt *v) {
        return b.create_logical_or(b.create_cmp_eq(v, b.get_float64(0)),
            b.create_cmp_ge(b.create_abs(v), b.get_float64(0x1p-969)));
      };
      auto *range = b.create_logical_or(b.create_cmp_eq(delta, b.get_float64(0)),
          b.create_logical_and(residual_range(x), residual_range(delta)));
      auto *valid = b.create_logical_and(detail::supported_real(b, x),
          b.create_logical_and(detail::supported_real(b, y),
              b.create_logical_and(range, b.create_cmp_eq(error, b.get_float64(0)))));
      {
        auto invalid = b.get_if_guard(b.create_if(b.create_logical_not(valid)), true);
        b.create_atomic_or(b.create_external_ptr(status, {b.get_int32(0)}), b.get_int32(1));
      }
    };
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(particles_), false,
                                     runtime_->cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto present = b.get_if_guard(b.create_if(
          detail::NativeSet{prepared_->particles.bindings().presence}.active(b, row)), true);
      for (int axis = 0; axis < 3; ++axis)
        translate(body, cell(b, kPosX + axis, row), axis);
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(groups_), false,
                                     runtime_->cpu_threads());
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      auto present = b.get_if_guard(b.create_if(
          detail::NativeSet{prepared_->fields.bindings().presence}.active(b, row)), true);
      for (int axis = 0; axis < 3; ++axis) {
        translate(groups, cell(b, kCenX + axis, row), axis);
        translate(history, cell(b, 3 + axis, row), axis);
      }
      auto *node = read(b, owners, {row});
      auto owned = b.get_if_guard(b.create_if(b.create_cmp_ge(node, b.get_int32(0))), true);
      for (int axis = 0; axis < 3; ++axis) {
        translate(slots, cell(b, axis, node), axis);
        translate(slots, cell(b, 4 + axis, node), axis);
        translate(previous, cell(b, axis, node), axis);
      }
    }
    for (int axis = 0; axis < 3; ++axis) {
      translate(focus, {b.get_int32(axis)}, axis);
      translate(focus, {b.get_int32(8 + axis)}, axis);
    }
  }
  const auto matrix = engine::Param::array(d, 2), vector = engine::Param::array(d),
             ints = engine::Param::array(i), scalar = engine::Param::scalar(d),
             integer = engine::Param::scalar(i);
  (commit ? rebase_ : rebase_check_) = engine::CompiledKernel(*runtime_,
      std::string(commit ? "prepared_rebase_commit_" : "prepared_rebase_check_") +
          std::to_string(prepared_->particles.bindings().presence->id),
      b.extract_ir(), {matrix, matrix, matrix, vector, matrix, matrix, ints, ints,
                       scalar, scalar, scalar, integer}, engine::MathMode::Strict);
  }
  launch_rebase({}, -1);
  launch_rebase({}, 0);
  runtime_->synchronize();
}

void Harness::launch_rebase(const FrameAnchor &delta, int mode) {
  auto &kernel = (mode == 1 || mode == -1) ? rebase_check_ : rebase_;
  auto call = kernel.invocation(true);
  auto &ctx = call.get();
  if (!ctx.prepared) {
    live_hierarchy_->bind_input(ctx, 0, 7);
    live_hierarchy_->bind_input(ctx, 1, 8);
    ctx.set_arg_ndarray({2}, *activity_->group_delta);
    ctx.set_arg_ndarray({3}, *activity_->focus);
    live_hierarchy_->bind_input(ctx, 4, 0);
    live_hierarchy_->bind_input(ctx, 5, 13);
    live_hierarchy_->bind_input(ctx, 6, 18);
    ctx.set_arg_ndarray({7}, *prepared_->candidate_status_array);
  }
  for (int axis = 0; axis < 3; ++axis)
    engine::argument_float(ctx, 8 + axis, delta[axis]);
  engine::argument_int(ctx, 11, mode > 0);
  kernel.launch(ctx);
}

UpdateResult Harness::rebase_current(std::uint64_t expected,
                                     const FrameAnchor &delta, std::uint64_t work) {
  UpdateResult out;
  const auto reject = [&](UpdateStatus status) { out.status = status; return out; };
  if (!prepared_ || !ready_) return reject(UpdateStatus::Unready);
  auto &s = *prepared_;
  out.generation = s.generation;
  if (expected != s.generation) return reject(UpdateStatus::Stale);
  if (!profile_.wide() || profile_.body_fraction_bits != 52 ||
      profile_.field_fraction_bits != 52 || !tick_execution_ || !live_hierarchy_installed())
    return reject(UpdateStatus::Unsupported);
  FrameAnchor anchor;
  bool changed = false;
  for (int axis = 0; axis < 3; ++axis) {
    const double a = frame_anchor_[axis], d = delta[axis], sum = a + d;
    const double v = sum - a, error = (a - (sum - v)) + (d - v);
    const bool range = d == 0 || ((a == 0 || std::abs(a) >= 0x1p-969) && std::abs(d) >= 0x1p-969);
    if (!NumericalProfile::supported_real(d) || !NumericalProfile::supported_real(sum) || !range || error != 0)
      return reject(UpdateStatus::InvalidNumerics);
    anchor[axis] = sum;
    changed |= d != 0;
  }
  if (!changed) return reject(UpdateStatus::NoChange);
  const auto &limits = s.capacity.limits();
  if (work == UINT64_MAX) work = limits[Budget::CascadeWork];
  const auto need = 64 * (limits[Budget::Particles] + limits[Budget::Fields] + 1) +
                    4 * live_hierarchy_->reference_count();
  if (work > limits[Budget::CascadeWork] || need > work) {
    out.failure = {Budget::CascadeWork, work > limits[Budget::CascadeWork] ? work : need,
                   work > limits[Budget::CascadeWork] ? limits[Budget::CascadeWork] : work};
    return reject(UpdateStatus::CapacityExhausted);
  }
  out.structural_visits = need;
  try {
    validate_prepared_bindings();
    live_hierarchy_->validate();
    // Descriptor-only qualification: no numerical host copy is an authority.
    if (!live_hierarchy_->full_precision_rebase())
      return reject(UpdateStatus::Unsupported);
    launch_rebase(delta, 1);
    runtime_->synchronize();
    ++out.synchronizations;
    engine::readback(*s.candidate_status_array, s.candidate_status.data(), sizeof(s.candidate_status));
    out.readback_bytes = sizeof(s.candidate_status);
    if (s.candidate_status[0]) return reject(UpdateStatus::InvalidNumerics);
    if (s.failure_point == 410) return reject(UpdateStatus::InvalidInput);
    if (s.generation == UINT64_MAX) { ready_ = false; return reject(UpdateStatus::Unready); }
    launch_rebase(delta, 2);
    runtime_->synchronize();
    ++out.synchronizations;
    if (s.failure_point == 411) throw std::runtime_error("injected failure after rebase writes");
    frame_anchor_ = anchor;
    activity_->set_frame_anchor(anchor);
    if (s.failure_point == 412) throw std::runtime_error("injected failure after rebase anchor");
    out.generation = ++s.generation;
    live_hierarchy_->retag_generation(s.generation);
    s.geometry_valid = false;
    return reject(UpdateStatus::Ok);
  } catch (...) {
    ready_ = false;
    throw;
  }
}
