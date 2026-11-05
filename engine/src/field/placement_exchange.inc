// D7 outgoing-state reduction. No live writers, claims or publication.
void Placement::build_exchange_balance() {
  IRBuilder b;
  auto *rows = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
       *requests = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0),
       *state = b.create_ndarray_arg_load({2}, PrimitiveType::f64, 1, 0),
       *report = b.create_ndarray_arg_load({3}, PrimitiveType::f64, 1, 0);
  auto *number = b.create_arg_load({4}, PrimitiveType::i32, false, 0);
  auto *status = b.create_local_var(PrimitiveType::i32);
  b.create_local_store(status, b.get_int32(0));
  placement::Math m(b, status);
  const auto read = [&](ArgLoadStmt *a, int k) {
    return b.create_global_load(b.create_external_ptr(a, {b.get_int32(k)}));
  };
  const auto write = [&](int k, Stmt *v) {
    b.create_global_store(b.create_external_ptr(report, {b.get_int32(k)}), v);
  };
  for (int k = 6; k < 32; ++k) write(k, m.f(0));
  m.invalid(b.create_cmp_ne(read(state, 7), m.f(0)));
  std::array<AllocaStmt *, 10> sum;
  for (auto &v : sum) {
    v = b.create_local_var(PrimitiveType::f64);
    b.create_local_store(v, m.f(0));
  }
  auto *loop = b.create_range_for(b.get_int32(0), number, false, 1, 0, true);
  {
    auto guard = b.get_loop_guard(loop);
    auto *rank = b.get_loop_index(loop);
    auto *command = b.create_cast(b.create_global_load(b.create_external_ptr(requests,
        {b.get_int32(2), rank})), PrimitiveType::i32);
    const auto value = [&](int k) {
      return b.create_global_load(b.create_external_ptr(rows, {b.get_int32(k), command}));
    };
    const auto accumulate = [&](int k, Stmt *v) {
      b.create_local_store(sum[k], b.create_add(b.create_local_load(sum[k]), v));
    };
    auto *mass = m.checked(value(kMass), 0, 53);
    m.invalid(b.create_cmp_le(mass, m.f(0)));
    m.invalid(b.create_cmp_ne(mass, b.create_global_load(b.create_external_ptr(requests,
        {b.get_int32(0), rank}))));
    accumulate(0, mass);
    for (int k = 0; k < 3; ++k) {
      auto *x = m.checked(value(kPosX + k), -256, 128);
      auto *v = m.checked(value(kVelX + k), -768, 288);
      m.invalid(b.create_cmp_ne(v, read(state, 3 + k)));
      accumulate(1 + k, b.create_mul(mass, x));
      accumulate(4 + k, b.create_mul(mass, v));
      accumulate(7 + k, b.create_mul(mass, b.create_abs(v)));
    }
  }
  auto *mass = b.create_local_load(sum[0]);
  m.invalid(b.create_cmp_le(mass, m.f(0)));
  m.invalid(b.create_cmp_ne(mass, read(state, 6)));
  write(6, mass);
  auto *denominator = b.create_select(b.create_cmp_gt(mass, m.f(0)), mass, m.f(1));
  auto *terms = b.create_add(b.create_add(b.create_cast(number, PrimitiveType::f64),
      b.create_cast(b.create_arg_load({5}, PrimitiveType::i32, false, 0), PrimitiveType::f64)), m.f(4));
  auto *factor = b.create_mul(terms, m.f(0x1p-49)); // 16*(P+N+4)*u, u=2^-53.
  for (int k = 0; k < 3; ++k) {
    auto *centre = b.create_div(b.create_local_load(sum[1 + k]), denominator);
    auto *offset = b.create_sub(centre, read(state, k));
    auto *momentum = b.create_local_load(sum[4 + k]);
    auto *old = read(report, k);
    auto *difference = b.create_sub(momentum, old);
    auto *scale = b.create_add(b.create_add(read(report, 3 + k),
        b.create_local_load(sum[7 + k])), b.create_add(b.create_abs(old), b.create_abs(momentum)));
    auto *tolerance = b.create_mul(factor, scale);
    for (auto *v : std::array<Stmt *, 5>{centre, offset, momentum, difference, tolerance})
      m.invalid(b.create_logical_not(m.supported(v, -900, 400)));
    m.invalid(b.create_cmp_gt(b.create_abs(difference), tolerance));
    write(7 + k, centre);
    write(10 + k, offset);
    write(13 + k, momentum);
    write(16 + k, difference);
    write(19 + k, tolerance);
  }
  write(22, b.create_cast(b.create_local_load(status), PrimitiveType::f64));
  exchange_balance_kernel_ = engine::CompiledKernel(*h_.runtime_, "placement_exchange_balance",
      b.extract_ir(), {Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::f64, 2),
                       Param::array(PrimitiveType::f64), Param::array(PrimitiveType::f64),
                       Param::scalar(PrimitiveType::i32), Param::scalar(PrimitiveType::i32)},
      engine::MathMode::Strict);
}

void Placement::balance_exchange(int bodies) {
  auto call = exchange_balance_kernel_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *h_.prepared_->new_rows_array);
    c.set_arg_ndarray({1}, *requests_);
    c.set_arg_ndarray({2}, *exchange_state_);
    c.set_arg_ndarray({3}, *exchange_balance_);
  }
  engine::argument_int(c, 4, bodies);
  engine::argument_int(c, 5, h_.particles_);
  exchange_balance_kernel_.launch(c);
}

void Placement::build_exchange() {
  IRBuilder b;
  auto *body = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
       *selected = b.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0),
       *output = b.create_ndarray_arg_load({2}, PrimitiveType::f64, 1, 0),
       *balance = b.create_ndarray_arg_load({4}, PrimitiveType::f64, 1, 0);
  auto *source = b.create_ndarray_arg_load({5}, PrimitiveType::u64, 1, 0);
  auto *status = b.create_local_var(PrimitiveType::i32);
  b.create_local_store(status, b.get_int32(0));
  placement::Math m(b, status);
  std::array<AllocaStmt *, 7> sum;
  std::array<AllocaStmt *, 3> magnitude;
  for (auto &v : magnitude) {
    v = b.create_local_var(PrimitiveType::f64);
    b.create_local_store(v, m.f(0));
  }
  for (auto &v : sum) {
    v = b.create_local_var(PrimitiveType::f64);
    b.create_local_store(v, m.f(0));
  }
  // Canonical physical-record order, independent of caller handle order.
  auto *loop = b.create_range_for(b.get_int32(0),
      b.create_arg_load({3}, PrimitiveType::i32, false, 0), false, 1, 0, true);
  {
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    auto chosen = b.get_if_guard(b.create_if(b.create_cmp_ne(
        b.create_global_load(b.create_external_ptr(selected, {row})), b.get_int32(0))), true);
    const auto value = [&](int k) {
      return b.create_global_load(b.create_external_ptr(body, {b.get_int32(k), row}));
    };
    auto *mass = m.checked(value(kMass), -128, 128);
    m.invalid(b.create_cmp_le(mass, m.f(0)));
    {
      auto reconcile = b.get_if_guard(b.create_if(
          b.create_arg_load({6}, PrimitiveType::i32, false, 0)), true);
      auto *expected = b.create_global_load(b.create_external_ptr(source, {row}));
      m.invalid(b.create_cmp_ne(mass, b.create_cast(expected, PrimitiveType::f64)));
    }
    b.create_local_store(sum[6], b.create_add(b.create_local_load(sum[6]), mass));
    for (int k = 0; k < 6; ++k) {
      auto *x = m.checked(value(k < 3 ? kPosX + k : kVelX + k - 3), -256, 128);
      b.create_local_store(sum[k], b.create_add(b.create_local_load(sum[k]), b.create_mul(mass, x)));
      if (k >= 3)
        b.create_local_store(magnitude[k - 3], b.create_add(b.create_local_load(magnitude[k - 3]),
            b.create_mul(mass, b.create_abs(x))));
    }
  }
  auto *mass = b.create_local_load(sum[6]);
  m.invalid(b.create_cmp_le(mass, m.f(0)));
  auto *denominator = b.create_select(b.create_cmp_gt(mass, m.f(0)), mass, m.f(1));
  for (int k = 0; k < 7; ++k) {
    Stmt *v = k == 6 ? static_cast<Stmt *>(mass) : b.create_div(b.create_local_load(sum[k]), denominator);
    // Bounded operands keep products/sums/division out of the FTZ domain.
    // Cancellation may produce small normal results; retain all f64 bits.
    m.invalid(b.create_logical_not(m.supported(v, -768, 288)));
    b.create_global_store(b.create_external_ptr(output, {b.get_int32(k)}), v);
  }
  b.create_global_store(b.create_external_ptr(output, {b.get_int32(7)}),
      b.create_cast(b.create_local_load(status), PrimitiveType::f64));
  for (int k = 0; k < 3; ++k) {
    b.create_global_store(b.create_external_ptr(balance, {b.get_int32(k)}), b.create_local_load(sum[3 + k]));
    b.create_global_store(b.create_external_ptr(balance, {b.get_int32(3 + k)}), b.create_local_load(magnitude[k]));
  }
  exchange_reduce_ = engine::CompiledKernel(*h_.runtime_, "placement_exchange_state",
      b.extract_ir(), {Param::array(PrimitiveType::f64, 2), Param::array(PrimitiveType::i32),
                       Param::array(PrimitiveType::f64), Param::scalar(PrimitiveType::i32),
                       Param::array(PrimitiveType::f64), Param::array(PrimitiveType::u64),
                       Param::scalar(PrimitiveType::i32)},
      engine::MathMode::Strict);
  IRBuilder target;
  auto *state = target.create_ndarray_arg_load({0}, PrimitiveType::f64, 1, 0),
       *focal = target.create_ndarray_arg_load({1}, PrimitiveType::f64, 1, 0);
  for (int k = 0; k < 4; ++k)
    target.create_global_store(target.create_external_ptr(focal, {target.get_int32(k)}),
        target.create_global_load(target.create_external_ptr(state,
            {target.get_int32(k == 3 ? 6 : k)})));
  exchange_target_ = engine::CompiledKernel(*h_.runtime_, "placement_exchange_target",
      target.extract_ir(), {Param::array(PrimitiveType::f64), Param::array(PrimitiveType::f64)},
      engine::MathMode::Strict);
}

void Placement::target_exchange() {
  auto call = exchange_target_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    c.set_arg_ndarray({0}, *exchange_state_);
    c.set_arg_ndarray({1}, *focal_);
  }
  exchange_target_.launch(c);
}

PlacementResult Placement::stage_exchange(std::uint64_t expected,
    const UpdateCommand *commands, std::size_t command_count,
    const PlacementIntrinsic *inputs, std::size_t body_count,
    const engine::RecordHandle *outgoing, std::size_t outgoing_count,
    std::uint64_t work, std::uint64_t cells) {
  if (!outgoing || !outgoing_count) {
    PlacementResult result;
    result.status = PlacementStatus::InvalidInput;
    result.generation = h_.prepared_->generation;
    return result;
  }
  return attempt(expected, commands, command_count, inputs, body_count, work, cells,
                 false, false, nullptr, 0, outgoing, outgoing_count);
}

void Placement::reduce_exchange(bool reconcile) {
  auto call = exchange_reduce_.invocation(true);
  auto &c = call.get();
  if (!c.prepared) {
    h_.bind(c, 0, h_.particle_array_.get(), h_.particle_values_,
            {kParticleFieldCount, std::max(h_.particles_, 1)});
    c.set_arg_ndarray({1}, *keep_);
    c.set_arg_ndarray({2}, *exchange_state_);
    c.set_arg_ndarray({4}, *exchange_balance_);
    c.set_arg_ndarray({5}, *exchange_masses_);
  }
  engine::argument_int(c, 3, h_.particles_);
  engine::argument_int(c, 6, reconcile);
  exchange_reduce_.launch(c);
}

UpdateResult Placement::stage_exchange_state(std::uint64_t expected,
    const engine::RecordHandle *outgoing, std::size_t count, std::uint64_t work, bool reconcile) {
  auto &h = h_;
  auto &s = *h.prepared_;
  UpdateResult out;
  out.generation = s.generation;
  const auto reject = [&](UpdateStatus status) { out.status = status; return out; };
  const auto &l = s.capacity.limits();
  if (!h.ready_) return reject(UpdateStatus::Unready);
  if (expected != s.generation) return reject(UpdateStatus::Stale);
  if (std::fegetround() != FE_TONEAREST) return reject(UpdateStatus::Unsupported);
  if (count > l[Budget::Particles]) {
    out.failure = {Budget::Particles, count, l[Budget::Particles]};
    return reject(UpdateStatus::CapacityExhausted);
  }
  if (!count || !outgoing) return reject(UpdateStatus::InvalidInput);
  if (work == UINT64_MAX) work = l[Budget::PlacementWork];
  const auto required = 64 * (l[Budget::Particles] + 1);
  if (work > l[Budget::PlacementWork] || work < required) {
    out.failure = {Budget::PlacementWork, work > l[Budget::PlacementWork] ? work : required,
                   std::min(work, l[Budget::PlacementWork])};
    return reject(UpdateStatus::CapacityExhausted);
  }
  out.structural_visits = required;
  std::fill(keep_values_.begin(), keep_values_.end(), 0);
  std::uint64_t total = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const auto &v = outgoing[i];
    if (v.record < 0 || v.record >= h.particles_ || !v.context)
      return reject(UpdateStatus::Stale);
    const auto &stored = s.particle_headers[v.record];
    if (v.context != stored.context || v.incarnation != stored.incarnation ||
        v.address != stored.address || v.record != stored.record)
      return reject(UpdateStatus::Stale);
    if (keep_values_[v.record]) return reject(UpdateStatus::InvalidInput);
    keep_values_[v.record] = 1;
    if (reconcile) {
      const auto mass = s.source_masses[v.record];
      if (!mass || mass > (UINT64_C(1) << 53) || total > (UINT64_C(1) << 53) - mass)
        return reject(UpdateStatus::Unsupported);
      total += mass;
    }
  }
  try {
    h.validate_prepared_bindings();
    engine::upload(*keep_, keep_values_.data(), keep_values_.size() * sizeof(int));
    out.upload_bytes = keep_values_.size() * sizeof(int);
    if (reconcile) {
      engine::upload(*exchange_masses_, s.source_masses.data(), s.source_masses.size() * sizeof(std::uint64_t));
      out.upload_bytes += s.source_masses.size() * sizeof(std::uint64_t);
    }
    reduce_exchange(reconcile);
    h.runtime_->synchronize();
    ++out.synchronizations;
    // Diagnostic status only; numerical scratch is never a host authority.
    std::array<double, 8> result{};
    engine::readback(*exchange_state_, result.data(), sizeof(result));
    out.readback_bytes = sizeof(result);
    return reject(result[7] == 0 ? UpdateStatus::Ok : UpdateStatus::InvalidNumerics);
  } catch (...) {
    h.ready_ = false;
    throw;
  }
}
