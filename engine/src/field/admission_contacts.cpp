#include "field/admission_contacts.h"

#include <algorithm>
#include <stdexcept>
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/prepared_storage.h"
#include "field/tick_execution.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
using Param = engine::Param;
using Kernel = engine::CompiledKernel;
ArgLoadStmt *array(IRBuilder &b, int id, DataType type, int rank = 1) {
  return b.create_ndarray_arg_load({id}, type, rank, 0);
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index) {
  return b.create_global_load(b.create_external_ptr(a, index));
}
void write(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, index), v);
}
}  // namespace

void AdmissionContacts::append_layout(const BudgetDemand &l,
                                      std::vector<StorageExtent> &out) {
  if (!l[Budget::PlacementWork] || !l[Budget::LiveResponses]) return;
  const auto p = std::max<std::uint64_t>(l[Budget::Particles], 1),
             c = std::max<std::uint64_t>(l[Budget::Contacts], 1);
  const auto add = [&](const char *name, std::uint64_t count, std::size_t bytes) {
    if (count > SIZE_MAX / bytes)
      throw std::overflow_error("admission contact workspace overflow");
    out.push_back({name, std::size_t(count), bytes, 1,
                   std::size_t(count) * bytes});
  };
  add("admission-contact-bodies", kParticleFieldCount * p, sizeof(double));
  add("admission-contact-indexes", 7 * p + 17 * c, sizeof(int));
  add("admission-contact-counters", 32, sizeof(std::int64_t));
  add("admission-contact-control", 1, sizeof(AdmissionContacts));
  add("admission-contact-array-control", ArrayCount,
      sizeof(Ndarray) + 4 * sizeof(int));
}

AdmissionContacts::AdmissionContacts(Harness &h) : h_(h) {
  const int p = std::max(h.particles_, 1), c = std::max(h.contacts_, 1);
  const auto i = PrimitiveType::i32, f = PrimitiveType::f64,
             wide = PrimitiveType::i64;
  const auto add = [&](int id, DataType type, std::vector<int> shape) {
    arrays_[id] = std::make_unique<Ndarray>(&h.runtime_->program(), type, shape);
    h.runtime_->program().fill_ndarray_fast_u32(arrays_[id].get(), 0);
  };
  add(Bodies, f, {kParticleFieldCount, p});
  add(Source, i, {2, c});
  add(Rows, i, {p});
  add(Temporary, i, {4, c});
  add(Pairs, i, {2, c});
  for (int id : {Flags, SourceOrder, GroupEnd, SourceMap, Permutation})
    add(id, i, {c});
  add(Offsets, i, {4, p});
  add(Cursors, i, {2, p});
  add(Orders, i, {2, c});
  add(Stats, wide, {16});
  source_.resize(2 * c);
  {
    IRBuilder b;
    std::vector<Param> params;
    for (int id = 0; id < ArrayCount; ++id) {
      const auto &a = *arrays_[id];
      const int rank = int(a.shape.size());
      auto *out = array(b, id, a.dtype, rank);
      params.push_back(Param::array(a.dtype, rank));
      // Source is supplied by the checked structural ledger, not reset here.
      if (id == Source) continue;
      for (int field = 0; field < (rank == 1 ? 1 : a.shape[0]); ++field) {
        auto *loop = b.create_range_for(b.get_int32(0),
            b.get_int32(a.shape.back()), false, h.runtime_->cpu_threads());
        auto guard = b.get_loop_guard(loop);
        auto *row = b.get_loop_index(loop);
        auto *zero = a.dtype == f ? b.get_float64(0)
                     : a.dtype == wide ? b.get_int64(0) : b.get_int32(0);
        write(b, out, rank == 1 ? std::vector<Stmt *>{row}
                                : std::vector<Stmt *>{b.get_int32(field), row},
              zero);
      }
    }
    kernels_[0] = Kernel(*h.runtime_, "admission_contacts_reset",
                         b.extract_ir(), params, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *live = array(b, 0, f, 2), *starts = array(b, 1, f, 2),
         *state = array(b, 2, i), *bodies = array(b, 3, f, 2),
         *rows = array(b, 4, i), *stats = array(b, 5, wide);
    auto *sources = b.create_arg_load({6}, i, false, 0);
    write(b, stats, {b.get_int32(2)}, b.create_cast(sources, wide));
    write(b, stats, {b.get_int32(9)}, b.get_int64(h.particles_));
    // Structural row states are already checked by update preflight. This
    // bounded copy/compaction includes sleeping survivors and generated rows;
    // it neither traverses potential addresses nor selects numerical work.
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(h.particles_),
                                     false, 1, 0, true);
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop), *status = read(b, state, {row});
    auto present = b.get_if_guard(
        b.create_if(b.create_cmp_gt(status, b.get_int32(0))), true);
    auto *next = read(b, stats, {b.get_int32(0)});
    write(b, rows, {b.create_cast(next, i)}, row);
    write(b, stats, {b.get_int32(0)}, b.create_add(next, b.get_int64(1)));
    auto *survivor = b.create_if(b.create_cmp_eq(status, b.get_int32(1)));
    {
      auto keep = b.get_if_guard(survivor, true);
      for (int k = 0; k < kParticleFieldCount; ++k)
        write(b, bodies, {b.get_int32(k), row},
              read(b, live, {b.get_int32(k), row}));
    }
    {
      auto fresh = b.get_if_guard(survivor, false);
      for (int k = 0; k < kParticleFieldCount; ++k)
        write(b, bodies, {b.get_int32(k), row},
              read(b, starts, {b.get_int32(k),
                               b.create_sub(status, b.get_int32(2))}));
    }
    kernels_[1] = Kernel(*h.runtime_, "admission_contacts_collect",
        b.extract_ir(), {Param::array(f, 2), Param::array(f, 2),
        Param::array(i), Param::array(f, 2), Param::array(i),
        Param::array(wide), Param::scalar(i)}, engine::MathMode::Strict);
  }
  auto geometry = h.build_geometry_kernels(h.activity_->particle_activity);
  for (int k = 1; k < 4; ++k) kernels_[k + 1] = std::move(geometry[k]);
  for (int phase = 0; phase < 5; ++phase) launch(phase, 0);
  h.runtime_->synchronize();
}

void AdmissionContacts::launch(int phase, int sources) {
  auto call = kernels_[phase].invocation(true);
  auto &ctx = call.get();
  auto &s = *h_.prepared_;
  const auto bind = [&](int arg, int id) {
    ctx.set_arg_ndarray({arg}, *arrays_[id]);
  };
  if (!ctx.prepared) {
    switch (phase) {
      case 0:
        for (int id = 0; id < ArrayCount; ++id) bind(id, id);
        break;
      case 1:
        h_.bind(ctx, 0, h_.particle_array_.get(), h_.particle_values_,
                 {kParticleFieldCount, std::max(h_.particles_, 1)});
        ctx.set_arg_ndarray({1}, *s.new_rows_array);
        ctx.set_arg_ndarray({2}, *s.validation_array);
        bind(3, Bodies); bind(4, Rows); bind(5, Stats);
        break;
      case 2:
        bind(0, Source); bind(1, SourceOrder); bind(2, GroupEnd);
        break;
      case 3:
        bind(0, Bodies); bind(1, Source); bind(2, Rows);
        bind(3, SourceOrder); bind(4, GroupEnd); bind(5, Temporary);
        bind(6, SourceMap); bind(7, Stats);
        break;
      case 4:
        bind(0, Temporary); bind(1, Pairs); bind(2, Flags);
        bind(3, Permutation); bind(4, SourceMap); bind(5, Offsets);
        bind(6, Cursors); bind(7, Orders); bind(8, Stats);
        break;
    }
  }
  if (phase) engine::argument_int(ctx, phase == 1 ? 6 : phase == 2 ? 3 :
                                         phase == 3 ? 8 : 9, sources);
  kernels_[phase].launch(ctx);
}

GeometryResult AdmissionContacts::validate(const PreparedCounts &candidate,
                                            std::uint64_t work) {
  GeometryResult q;
  upload_bytes_ = 0;
  const auto &s = *h_.prepared_;
  const auto &limits = s.capacity.limits();
  work = std::min(work, limits[Budget::GeometryWork]);
  const auto exhaust = [&](Budget budget, std::uint64_t required,
                            std::uint64_t capacity) {
    q.status = GeometryStatus::CapacityExhausted;
    q.failure = {budget, required, capacity};
    return q;
  };
  q.present = candidate.particles;
  q.source_contacts = candidate.contacts;
  q.pair_tests = q.present * (q.present ? q.present - 1 : 0) / 2;
  q.source_order_tests = q.source_contacts * q.source_contacts;
  // Charge fixed clearing/copy/compaction separately from pair comparisons.
  for (const auto &a : arrays_) q.required_work += a->get_nelement();
  q.required_work += std::uint64_t(h_.particles_) * (kParticleFieldCount + 1);
  q.required_work += q.pair_tests + q.source_order_tests;
  if (q.required_work > work)
    return exhaust(Budget::GeometryWork, q.required_work, work);
  const int stride = std::max(h_.contacts_, 1);
  std::fill(source_.begin(), source_.end(), 0);
  for (std::size_t row = 0; row < candidate.contacts; ++row) {
    source_[row] = s.prospective_contacts[row].first.record;
    source_[stride + row] = s.prospective_contacts[row].second.record;
  }
  engine::upload(*arrays_[Source], source_.data(), source_.size() * sizeof(int));
  upload_bytes_ = source_.size() * sizeof(int);
  launch(0, candidate.contacts);
  launch(1, candidate.contacts);
  launch(2, candidate.contacts);
  launch(3, candidate.contacts);
  h_.runtime_->synchronize();
  ++q.synchronizations;
  engine::readback(*arrays_[Stats], counts_.data(), sizeof(counts_));
  q.readback_bytes += sizeof(counts_);
  q.contacts = counts_[3];
  q.derived = counts_[4];
  q.compaction_slots = counts_[9];
  if (counts_[5] || std::uint64_t(counts_[0]) != q.present) {
    q.status = GeometryStatus::InvalidNumerics;
    return q;
  }
  if (q.contacts > limits[Budget::Contacts])
    return exhaust(Budget::Contacts, q.contacts, limits[Budget::Contacts]);
  const auto old_contacts = h_.tick_execution_
      ? h_.tick_execution_->installed_contact_count() : 0;
  const auto references = h_.live_hierarchy_->refresh_reference_peak() +
                          2 * (old_contacts + q.contacts);
  if (references > limits[Budget::References])
    return exhaust(Budget::References, references, limits[Budget::References]);
  q.union_order_tests = q.contacts * q.contacts;
  q.required_work += q.union_order_tests;
  if (q.required_work > work)
    return exhaust(Budget::GeometryWork, q.required_work, work);
  launch(4, candidate.contacts);
  h_.runtime_->synchronize();
  ++q.synchronizations;
  q.status = GeometryStatus::Ok;
  return q;
}

std::size_t AdmissionContacts::binding_bytes() const {
  std::size_t bytes = 0;
  for (const auto &k : kernels_) bytes += k.prepared_bytes();
  return bytes;
}
}  // namespace field::detail
