#include "field/field.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

#include "engine/packed_view.h"
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/centroid_cascade.h"
#include "field/prepared_storage.h"
#include "field/tick_execution.h"
#include "field/placement.h"

namespace field {
namespace {

using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

using namespace taichi::lang;

void store(IRBuilder &b,
           ArgLoadStmt *array,
           const std::vector<Stmt *> &index,
           Stmt *value) {
  b.create_global_store(b.create_external_ptr(array, index), value);
}

void accumulate(IRBuilder &b,
                ArgLoadStmt *array,
                const std::vector<Stmt *> &index,
                Stmt *value) {
  b.create_atomic_add(b.create_external_ptr(array, index), value);
}

// total_dim is 2 for {field, item} dense arrays and 1 for universal/determined
// (single-axis, indexed directly).
ArgLoadStmt *array_param(IRBuilder &b, int id, DataType dt, int total_dim) {
  return b.create_ndarray_arg_load({id}, dt, total_dim, /*arg_depth=*/0);
}

void validate_parameters(const Parameters64 &p) {
  for (double value :
       {p.gravity, p.dt, p.contact_stiffness, p.reach_strength}) {
    if (!std::isfinite(value) || value < 0.0f) {
      throw std::invalid_argument(
          "field: parameters must be finite and non-negative");
    }
  }
}

std::size_t storage_size(int fields, int count) {
  return std::size_t(fields) * std::size_t(std::max(count, 1));
}

}  // namespace

Harness::Harness(engine::Runtime &runtime,
                 int particle_count,
                 int group_count,
                 int edge_count,
                 int contact_count,
                 HostPath path)
    : Harness(runtime,
              particle_count,
              group_count,
              edge_count,
              contact_count,
              path,
              nullptr) {
}

Harness::Harness(engine::Runtime &runtime,
                 const PreparedCapacity &capacity,
                 HostPath path)
    : Harness(runtime,
              int(capacity.limits()[Budget::Particles]),
              int(capacity.limits()[Budget::Fields]),
              int(capacity.limits()[Budget::Participation]),
              int(capacity.limits()[Budget::Contacts]),
              path,
              &capacity) {
}

Harness::Harness(engine::Runtime &runtime, int particle_count, int group_count,
                 int edge_count, int contact_count, HostPath path,
                 const PreparedCapacity *capacity)
    : runtime_(&runtime),
      profile_(capacity ? capacity->profile() : NumericalProfile{}),
      particles_(particle_count),
      groups_(group_count),
      edges_(edge_count),
      contacts_(contact_count),
      path_(path) {
  const auto allocation_start = Clock::now();
  if (path_ == HostPath::CpuDirect && runtime.on_cuda())
    throw std::invalid_argument("direct host binding requires CPU backend");
  if (particle_count < 0 || group_count < 0 || edge_count < 0 ||
      contact_count < 0) {
    throw std::invalid_argument("field::Harness: counts must be non-negative");
  }
  if (profile_.wide())
    engine::numeric::Precision(engine::numeric::Real::F64,
                               engine::numeric::Real::F64,
                               profile_.body_fraction_bits)
        .require_backend(runtime);
  if (capacity && capacity->limits()[Budget::TickWork] &&
      runtime.arch() != taichi::Arch::x64)
    throw std::invalid_argument(
        "contact-safe candidate ticks currently require x64");
  if (capacity && capacity->limits()[Budget::PlacementWork] &&
      runtime.arch() != taichi::Arch::x64)
    throw std::invalid_argument("certified placement currently requires x64");
  if (capacity) {
    const auto required = detail::PreparedStorage::required_scratch(
        capacity->limits(), runtime.on_cuda(), profile_);
    if (required > capacity->limits()[Budget::ScratchBytes])
      throw CapacityError({Budget::ScratchBytes, required,
                           capacity->limits()[Budget::ScratchBytes]});
    prepared_ = std::make_unique<detail::PreparedStorage>(runtime, *capacity);
    generation_ =
        1;  // Checked binding schema, not a committed object generation.
  }
  // Empty logical arrays still need nonzero allocation/transfer sizes in
  // the RHI. Their real loop counts and host accessor strides stay zero.
  for (auto *values : {&particle_values_, &group_values_, &universal_values_,
                       &determined_values_, &empty_determined_values_})
    values->configure(profile_.wide());
  empty_determined_values_.assign(particles_ ? 0 : 1, 0);
  particle_values_.assign(storage_size(kParticleFieldCount, particles_), 0.0f);
  group_values_.assign(storage_size(kGroupFieldCount, groups_), 0.0f);
  edge_float_data.assign(storage_size(kEdgeFloatFieldCount, edges_), 0.0f);
  edge_int_data.assign(storage_size(kEdgeIntFieldCount, edges_), 0);
  contact_int_data.assign(storage_size(kContactIntFieldCount, contacts_), 0);
  determined_values_.assign(particles_, 0.0f);
  universal_values_.assign(1, 0.0f);

  // Each dense array below is 2-axis {num_fields, num_items} rather than a
  // single flat {num_fields * num_items} axis: a 1-D Ndarray shape is a
  // single i32 axis and so cannot even express more than 2^31 elements,
  // while two axes each far under 2^31 let the (i64-widened) codegen
  // flatten the product beyond that cap. Row-major, so element (f, i) still
  // lives at byte offset f*num_items + i -- exactly the host SoA layout
  // below; only the in-kernel index expression changes.
  Program &program = runtime.program();
  if (path_ == HostPath::BulkCopy) {
    particle_array_ = std::make_unique<Ndarray>(
        &program, real_type(),
        std::vector<int>{kParticleFieldCount, std::max(particles_, 1)});
    group_array_ = std::make_unique<Ndarray>(
        &program, real_type(),
        std::vector<int>{kGroupFieldCount, std::max(groups_, 1)});
    edge_float_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::f32,
        std::vector<int>{kEdgeFloatFieldCount, std::max(edges_, 1)});
    edge_int_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::i32,
        std::vector<int>{kEdgeIntFieldCount, std::max(edges_, 1)});
    contact_int_array_ = std::make_unique<Ndarray>(
        &program, PrimitiveType::i32,
        std::vector<int>{kContactIntFieldCount, std::max(contacts_, 1)});
    determined_array_ = std::make_unique<Ndarray>(
        &program, real_type(), std::vector<int>{std::max(particles_, 1)});
    // The universal mass: one cell, the total mass of the construct with each
    // particle counted exactly once, computed during publication.
    universal_array_ =
        std::make_unique<Ndarray>(&program, real_type(), std::vector<int>{1});
  }
  activity_ = std::make_unique<detail::Activity>(
      runtime, particles_, groups_, edges_, contacts_, prepared_.get());
  timings_.allocation_ms =
      elapsed(allocation_start) - activity_->compilation_ms;
  // These kernels embed Activity's SNode layout; CHI IR cache names below
  // include its id to avoid reusing another study's tree-bound machine code.
  const auto compilation_start = Clock::now();

  auto kernels = build_numerical_kernels(activity_->particle_activity,
                                         activity_->centroids);
  clear_ = std::move(kernels[0]);
  force_ = std::move(kernels[1]);
  contact_ = std::move(kernels[2]);
  integrate_ = std::move(kernels[3]);
  determine_ = std::move(kernels[4]);
  centroid_ = std::move(kernels[5]);
  finalize_ = std::move(kernels[6]);
  timings_.compilation_ms =
      activity_->compilation_ms + elapsed(compilation_start);
  if (prepared_)
    prepare_empty_execution();
}

template <class T>
void Harness::bind(LaunchContextBuilder &ctx,
                   int argument,
                   const Ndarray *array,
                   std::vector<T> &values,
                   std::vector<int> shape) {
  engine::PackedView<T> view(values, std::move(shape), generation_);
  if (path_ == HostPath::BulkCopy) {
    ctx.set_arg_ndarray({argument}, *array);
  } else {
    std::vector<taichi::int64> dimensions(view.shape().begin(),
                                          view.shape().end());
    ctx.set_arg_external_array_with_shape(
        {argument}, reinterpret_cast<std::uintptr_t>(view.data()), view.bytes(),
        dimensions);
  }
}

void Harness::bind(LaunchContextBuilder &ctx, int argument,
                   const Ndarray *array, detail::RealBuffer &values,
                   std::vector<int> shape) {
  if (values.wide())
    bind(ctx, argument, array, values.doubles(), std::move(shape));
  else
    bind(ctx, argument, array, values.legacy(), std::move(shape));
}
Harness::~Harness() = default;

// Activity launches share the exact same checked payload binding on both host
// paths; keep the template bodies private to this implementation.
template void Harness::bind<float>(LaunchContextBuilder &,
                                   int,
                                   const Ndarray *,
                                   std::vector<float> &,
                                   std::vector<int>);
template void Harness::bind<int>(LaunchContextBuilder &,
                                 int,
                                 const Ndarray *,
                                 std::vector<int> &,
                                 std::vector<int>);

void Harness::set_tick_mode(TickMode mode) {
  runtime_->synchronize();
  mode_ = mode;
  invalidated_ = true;
}

void Harness::set_focus(const std::vector<int> &groups) {
  if (prepared_)
    throw std::logic_error(
        "prepared focal configuration is fixture-only in this slice");
  if (!ready_)
    throw std::logic_error("focus requires seeded field geometry");
  std::set<int> distinct;
  for (int g : groups)
    if (g < 0 || g >= groups_ || !distinct.insert(g).second)
      throw std::invalid_argument(
          "focus requires distinct prepared field slots");
  if (!groups.empty()) {
    download();
    for (int g : groups)
      if (!(group(kCenM, g) > 0))
        throw std::invalid_argument(
            "focal fields require exposed participating mass");
    int anchor_slot = -1;
    for (int e = 0; e < edges_; ++e)
      if (edge_int(kEdgeGroup, e) == groups.front()) {
        anchor_slot = edge_int(kEdgeParticle, e);
        break;
      }
    if (anchor_slot < 0)
      throw std::invalid_argument("focal field has no member");
    FrameAnchor shift{}, next_anchor = frame_anchor_;
    for (int axis = 0; axis < 3; ++axis) {
      shift[axis] = particle(kPosX + axis, anchor_slot);
      next_anchor[axis] += shift[axis];
    }
    if (shift != FrameAnchor{}) {
      auto local = particle_data;
      for (int slot = 0; slot < particles_; ++slot) {
        if (particle(kMass, slot) == 0)
          continue;
        for (int axis = 0; axis < 3; ++axis) {
          const double value =
              double(particle(kPosX + axis, slot)) - shift[axis];
          if (!std::isfinite(value) ||
              std::abs(value) > std::numeric_limits<float>::max())
            throw std::invalid_argument("focus-local extent outside f32 range");
          local[offset(kPosX + axis, particles_, slot)] =
              static_cast<float>(value);
        }
      }
      activity_->set_focus(groups);
      std::copy(local.begin(), local.end(), particle_data.begin());
      publish(previous_parameters_.legacy(), next_anchor);
      return;
    }
  }
  activity_->set_focus(groups);
  invalidated_ = true;
}

void Harness::validate_staging() const {
  if (particle_values_.size() !=
          storage_size(kParticleFieldCount, particles_) ||
      group_values_.size() != storage_size(kGroupFieldCount, groups_) ||
      edge_float_data.size() != storage_size(kEdgeFloatFieldCount, edges_) ||
      edge_int_data.size() != storage_size(kEdgeIntFieldCount, edges_) ||
      contact_int_data.size() !=
          storage_size(kContactIntFieldCount, contacts_)) {
    throw std::invalid_argument(
        "field::Harness::upload: staging size mismatch");
  }
  for (const auto *data : {&particle_values_, &group_values_}) {
    for (std::size_t i = 0; i < data->size(); ++i) {
      const double value = (*data)[i];
      if (!std::isfinite(value) ||
          (profile_.wide() && !NumericalProfile::supported_real(value))) {
        throw std::invalid_argument(
            "field::Harness::upload: non-finite staging");
      }
    }
  }
  for (float value : edge_float_data)
    if (!std::isfinite(value))
      throw std::invalid_argument("nonfinite source share");
  for (int i = 0; i < particles_; ++i) {
    const double mass = particle_values_[offset(kMass, particles_, i)];
    const double radius = particle_values_[offset(kRadius, particles_, i)];
    if (prepared_ &&
        ((prepared_->particle_headers[i].record >= 0) != (mass > 0)))
      throw std::invalid_argument(
          "fixture mass and native particle presence disagree");
    if (mass < 0.0f || radius < 0.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: negative mass or radius");
    }
    if (mass == 0.0f) {
      for (int f = 0; f < kParticleFieldCount; ++f) {
        if (particle_values_[offset(f, particles_, i)] != 0.0f) {
          throw std::invalid_argument(
              "field::Harness::upload: free slot must be zero");
        }
      }
    }
  }
  // The kernels address device memory directly with these indices; an
  // out-of-range one would read or write past the array, not fail.
  for (int e = 0; e < (prepared_ ? int(prepared_->live.edges) : edges_); ++e) {
    const int p = edge_int_data[offset(kEdgeParticle, edges_, e)];
    const int g = edge_int_data[offset(kEdgeGroup, edges_, e)];
    if (p < 0 || p >= particles_ || g < 0 || g >= groups_) {
      throw std::out_of_range("field::Harness::upload: edge " +
                              std::to_string(e) +
                              " names a particle or group out of range");
    }
    if (prepared_ && (prepared_->particle_headers[p].record < 0 ||
                      prepared_->field_headers[g].record < 0))
      throw std::invalid_argument(
          "fixture participation names absent endpoint");
    const float share = edge_float_data[offset(kShare, edges_, e)];
    if (particle_values_[offset(kMass, particles_, p)] == 0.0f ||
        share <= 0.0f || share > 1.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: invalid participation");
    }
  }
  for (int k = 0; k < (prepared_ ? int(prepared_->live.contacts) : contacts_);
       ++k) {
    const int a = contact_int_data[offset(kContactA, contacts_, k)];
    const int b = contact_int_data[offset(kContactB, contacts_, k)];
    if (a < 0 || a >= particles_ || b < 0 || b >= particles_) {
      throw std::out_of_range("field::Harness::upload: contact " +
                              std::to_string(k) +
                              " names a particle out of range");
    }
    if (a == b || particle_values_[offset(kMass, particles_, a)] == 0.0f ||
        particle_values_[offset(kMass, particles_, b)] == 0.0f) {
      throw std::invalid_argument(
          "field::Harness::upload: invalid contact endpoints");
    }
  }
}

void Harness::invalidate() {
  ready_ = false;
  uploaded_ = false;
  runtime_->synchronize();
  invalidated_ = true;
  demand_ = {};
}

void Harness::publish(const Parameters &p) {
  publish(p, frame_anchor_);
}

void Harness::publish(const Parameters &p, const FrameAnchor &anchor) {
  if (prepared_)
    throw std::logic_error(
        "legacy publication cannot reset a prepared execution");
  invalidate();
  validate_parameters(p);
  for (double coordinate : anchor)
    if (!std::isfinite(coordinate) ||
        std::abs(coordinate) > std::numeric_limits<float>::max())
      throw std::invalid_argument("invalid frame reference");
  frame_anchor_ = anchor;
  upload();
  seed_centroids(p);
}

void Harness::upload() {
  if (prepared_)
    throw std::logic_error("legacy upload cannot reset a prepared execution");
  invalidate();
  validate_staging();
  activity_->index(edge_int_data, contact_int_data);
  exposed_ = 0;
  for (int p = 0; p < particles_; ++p)
    if (particle(kMass, p) > 0)
      ++exposed_;
  std::vector<bool> live_groups(groups_, false);
  for (int e = 0; e < edges_; ++e)
    if (particle(kMass, edge_int(kEdgeParticle, e)) * edge_float(kShare, e) > 0)
      live_groups[edge_int(kEdgeGroup, e)] = true;
  exposed_fields_ = std::count(live_groups.begin(), live_groups.end(), true);
  ++generation_;
  const auto start = Clock::now();
  // Validate each complete typed shape before any copy; synchronous transfer
  // completion and exclusive harness ownership keep borrowed vectors stable.
  engine::PackedView<float> particles(
      particle_data, {kParticleFieldCount, std::max(particles_, 1)},
      generation_);
  engine::PackedView<float> groups(
      group_data, {kGroupFieldCount, std::max(groups_, 1)}, generation_);
  engine::PackedView<float> edge_f(edge_float_data,
                                   {kEdgeFloatFieldCount, std::max(edges_, 1)},
                                   generation_);
  engine::PackedView<int> edge_i(
      edge_int_data, {kEdgeIntFieldCount, std::max(edges_, 1)}, generation_);
  engine::PackedView<int> contacts(
      contact_int_data, {kContactIntFieldCount, std::max(contacts_, 1)},
      generation_);
  if (path_ == HostPath::BulkCopy) {
    engine::upload(*particle_array_, particle_data.data(),
                   particle_data.size() * sizeof(float));
    engine::upload(*group_array_, group_data.data(),
                   group_data.size() * sizeof(float));
    engine::upload(*edge_float_array_, edge_float_data.data(),
                   edge_float_data.size() * sizeof(float));
    engine::upload(*edge_int_array_, edge_int_data.data(),
                   edge_int_data.size() * sizeof(int));
    engine::upload(*contact_int_array_, contact_int_data.data(),
                   contact_int_data.size() * sizeof(int));
    runtime_->synchronize();
    timings_.upload_bytes += particles.bytes() + groups.bytes() +
                             edge_f.bytes() + edge_i.bytes() + contacts.bytes();
  }
  timings_.upload_ms += elapsed(start);
  uploaded_ = true;
}

void Harness::download() {
  if (prepared_) {
    if (!ready_)
      throw std::logic_error(
          "unready prepared execution cannot report current values");
    try {
      validate_prepared_bindings();
    } catch (...) {
      ready_ = false;
      throw;
    }
  }
  const auto start = Clock::now();
  runtime_->synchronize();
  if (path_ == HostPath::BulkCopy) {
    engine::readback(*particle_array_, particle_values_.data(),
                     particle_values_.bytes());
    engine::readback(*group_array_, group_values_.data(),
                     group_values_.bytes());
    if (particles_ > 0) {
      engine::readback(*determined_array_, determined_values_.data(),
                       determined_values_.bytes());
    }
    timings_.download_bytes +=
        particle_values_.bytes() + group_values_.bytes() +
        (particles_ > 0 ? determined_values_.bytes() : 0);
  }
  timings_.report_ms += elapsed(start);
}

bool Harness::uploaded_payload_matches() {
  if (!uploaded_ || ready_)
    throw std::logic_error("payload diagnostic requires unseeded upload");
  runtime_->synchronize();
  if (path_ == HostPath::CpuDirect)
    return true;  // Bound storage is the source.
  const auto matches = [](Ndarray &array, const auto &source) {
    using T = typename std::decay_t<decltype(source)>::value_type;
    std::vector<T> received(source.size());
    engine::readback(array, received.data(), received.size() * sizeof(T));
    return !std::memcmp(source.data(), received.data(),
                        received.size() * sizeof(T));
  };
  return matches(*particle_array_, particle_data) &&
         matches(*group_array_, group_data) &&
         matches(*edge_float_array_, edge_float_data) &&
         matches(*edge_int_array_, edge_int_data) &&
         matches(*contact_int_array_, contact_int_data);
}

void Harness::launch_clear(bool seed, detail::TickExecution *target) {
  auto &kernel = target ? target->kernel(detail::TickKernel::Clear) : clear_;
  auto call = kernel.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 1, group_array_.get(), group_values_,
         {kGroupFieldCount, std::max(groups_, 1)});
  }
  engine::argument_int(ctx, 2, particles_);
  engine::argument_int(ctx, 3, groups_);
  if (!ctx.prepared) {
    bind(ctx, 4, universal_array_.get(), universal_values_, {1});
  }
  engine::argument_int(ctx, 5, seed);
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *activity_->particle_flags);
  }
  engine::argument_int(ctx, 7, seed ? particles_ : 0);
  if (target && !ctx.prepared)
    target->redirect(detail::TickKernel::Clear, ctx);
  kernel.launch(ctx);
}

void Harness::launch_force(const Parameters64 &p,
                           detail::TickExecution *target) {
  auto &kernel = target ? target->kernel(detail::TickKernel::Force) : force_;
  auto call = kernel.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, edge_int_array_.get(), edge_int_data,
         {kEdgeIntFieldCount, std::max(edges_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 1, edge_float_array_.get(), edge_float_data,
         {kEdgeFloatFieldCount, std::max(edges_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 2, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 3, group_array_.get(), group_values_,
         {kGroupFieldCount, std::max(groups_, 1)});
  }
  engine::argument_int(ctx, 4, particles_);
  engine::argument_int(ctx, 5, groups_);
  engine::argument_int(ctx, 6, edges_);
  engine::argument_float(ctx, 7, p.gravity);
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({8}, *activity_->particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({9}, *activity_->counters);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({10}, *activity_->particle_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({11}, *activity_->particle_edges);
  }
  if (profile_.wide() && !ctx.prepared)
    ctx.set_arg_ndarray({12}, *prepared_->live_ratios_array);
  if (target && !ctx.prepared)
    target->redirect(detail::TickKernel::Force, ctx);
  kernel.launch(ctx);
}

void Harness::launch_contact(const Parameters64 &p,
                             detail::TickExecution *target) {
  auto &kernel =
      target ? target->kernel(detail::TickKernel::Contact) : contact_;
  auto call = kernel.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, contact_int_array_.get(), contact_int_data,
         {kContactIntFieldCount, std::max(contacts_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 1, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
  }
  engine::argument_int(ctx, 2, contacts_);
  engine::argument_float(ctx, 3, p.contact_stiffness);
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({4}, *activity_->particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *activity_->counters);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *activity_->contact_a_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({7}, *activity_->contact_a_edges);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({8}, *activity_->contact_b_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({9}, *activity_->contact_b_edges);
  }
  engine::argument_int(ctx, 10, particles_);
  if (target && !ctx.prepared)
    target->redirect(detail::TickKernel::Contact, ctx);
  kernel.launch(ctx);
}

void Harness::launch_integrate(const Parameters64 &p,
                               detail::TickExecution *target) {
  auto &kernel =
      target ? target->kernel(detail::TickKernel::Integrate) : integrate_;
  auto call = kernel.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
  }
  engine::argument_int(ctx, 1, particles_);
  engine::argument_float(ctx, 2, p.dt);
  if (!ctx.prepared) {
    bind(ctx, 3, universal_array_.get(), universal_values_, {1});
  }
  engine::argument_float(ctx, 4, p.reach_strength);
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *activity_->particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *activity_->particle_delta);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({7}, *activity_->focus);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({8}, *activity_->counters);
  }
  if (!ctx.prepared) {
    bind(ctx, 9, edge_int_array_.get(), edge_int_data,
         {kEdgeIntFieldCount, std::max(edges_, 1)});
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({10}, *activity_->particle_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({11}, *activity_->particle_edges);
  }
  if (profile_.wide() && !ctx.prepared)
    bind(ctx, 12, determined_array_.get(),
         particles_ ? determined_values_ : empty_determined_values_,
         {std::max(particles_, 1)});
  if (live_hierarchy_) {
    if (!ctx.prepared)
      ctx.set_arg_ndarray({13}, live_hierarchy_->leaf_fields());
    engine::argument_int(ctx, 14, live_hierarchy_->installed());
  }
  if (target && !ctx.prepared)
    target->redirect(detail::TickKernel::Integrate, ctx);
  kernel.launch(ctx);
}

void Harness::launch_determine(detail::TickExecution *target) {
  auto &kernel =
      target ? target->kernel(detail::TickKernel::Determine) : determine_;
  auto call = kernel.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    bind(ctx, 0, particle_array_.get(), particle_values_,
         {kParticleFieldCount, std::max(particles_, 1)});
  }
  if (!ctx.prepared) {
    bind(ctx, 1, determined_array_.get(),
         particles_ ? determined_values_ : empty_determined_values_,
         {std::max(particles_, 1)});
  }
  engine::argument_int(ctx, 2, particles_);
  if (profile_.wide() && !ctx.prepared)
    ctx.set_arg_ndarray({3}, *activity_->counters);
  if (target && !ctx.prepared)
    target->redirect(detail::TickKernel::Determine, ctx);
  kernel.launch(ctx);
}

void Harness::launch_centroid(const Parameters64 &,
                              bool seed,
                              detail::TickExecution *target) {
  {
    auto &kernel =
        target ? target->kernel(detail::TickKernel::Centroid) : centroid_;
    auto call = kernel.invocation(bool(prepared_));
    auto &ctx = call.get();
    if (!ctx.prepared) {
      bind(ctx, 0, edge_int_array_.get(), edge_int_data,
           {kEdgeIntFieldCount, std::max(edges_, 1)});
    }
    if (!ctx.prepared) {
      bind(ctx, 1, edge_float_array_.get(), edge_float_data,
           {kEdgeFloatFieldCount, std::max(edges_, 1)});
    }
    if (!ctx.prepared) {
      bind(ctx, 2, particle_array_.get(), particle_values_,
           {kParticleFieldCount, std::max(particles_, 1)});
    }
    if (!ctx.prepared) {
      bind(ctx, 3, group_array_.get(), group_values_,
           {kGroupFieldCount, std::max(groups_, 1)});
    }
    engine::argument_int(ctx, 4, particles_);
    engine::argument_int(ctx, 5, groups_);
    engine::argument_int(ctx, 6, edges_);
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({7}, profile_.wide() ? *activity_->group_delta
                                               : *activity_->group_flags);
    }
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({8}, *activity_->counters);
    }
    engine::argument_int(ctx, 9, seed);
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({10}, *activity_->group_offsets);
    }
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({11}, *activity_->group_edges);
    }
    if (profile_.wide() && !ctx.prepared)
      ctx.set_arg_ndarray({12}, *prepared_->live_ratios_array);
    if (target && !ctx.prepared)
      target->redirect(detail::TickKernel::Centroid, ctx);
    kernel.launch(ctx);
  }
  {
    auto &kernel =
        target ? target->kernel(detail::TickKernel::Finalize) : finalize_;
    auto call = kernel.invocation(bool(prepared_));
    auto &ctx = call.get();
    if (!ctx.prepared) {
      bind(ctx, 0, group_array_.get(), group_values_,
           {kGroupFieldCount, std::max(groups_, 1)});
    }
    engine::argument_int(ctx, 1, groups_);
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({2}, *activity_->group_flags);
    }
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({3}, *activity_->group_delta);
    }
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({4}, *activity_->counters);
    }
    if (target && !ctx.prepared)
      target->redirect(detail::TickKernel::Finalize, ctx);
    kernel.launch(ctx);
  }
}

void Harness::seed_centroids(const Parameters &p) {
  if (prepared_)
    throw std::logic_error("legacy seed cannot reset a prepared execution");
  const auto start = Clock::now();
  ready_ = false;
  if (!uploaded_) {
    throw std::logic_error(
        "field::Harness: upload required before centroid seeding");
  }
  validate_parameters(p);
  activity_->set_frame_anchor(frame_anchor_);
  activity_->reset();
  launch_clear(true);
  launch_centroid(p, true);
  launch_determine();
  activity_->clear();
  runtime_->synchronize();
  ready_ = true;
  previous_parameters_ = p;
  invalidated_ = true;
  timings_.seed_ms += elapsed(start);
}

const detail::PreparedStorage &Harness::prepared_storage() const {
  if (!prepared_)
    throw std::logic_error("not a prepared execution");
  return *prepared_;
}
std::size_t Harness::present_particles() const {
  return prepared_ ? prepared_->live.particles : exposed_;
}
std::size_t Harness::present_fields() const {
  return prepared_ ? prepared_->live.fields : exposed_fields_;
}

void Harness::validate_prepared_bindings() const {
  if (!prepared_)
    return;
  const std::array<const void *, 7> addresses{
      particle_values_.data(), group_values_.data(),
      edge_float_data.data(),  edge_int_data.data(),
      contact_int_data.data(), determined_values_.data(),
      universal_values_.data()};
  const std::array<std::size_t, 7> sizes{
      particle_values_.size(), group_values_.size(),
      edge_float_data.size(),  edge_int_data.size(),
      contact_int_data.size(), determined_values_.size(),
      universal_values_.size()};
  if (profile_.wide() && (!particle_data.empty() || !group_data.empty() ||
                          !universal_data.empty()))
    throw std::logic_error("inactive f32 view modified in trial execution");
  if (addresses != prepared_host_addresses_ || sizes != prepared_host_sizes_)
    throw std::logic_error("prepared host buffers changed identity or shape");
}

void Harness::prepare_empty_execution() {
  auto &s = *prepared_;
  prepared_host_addresses_ = {
      particle_values_.data(), group_values_.data(),
      edge_float_data.data(),  edge_int_data.data(),
      contact_int_data.data(), determined_values_.data(),
      universal_values_.data()};
  prepared_host_sizes_ = {particle_values_.size(), group_values_.size(),
                          edge_float_data.size(),  edge_int_data.size(),
                          contact_int_data.size(), determined_values_.size(),
                          universal_values_.size()};
  activity_->provision();
  activity_->index(edge_int_data, contact_int_data);
  if (path_ == HostPath::BulkCopy) {
    for (auto *array :
         {particle_array_.get(), group_array_.get(), edge_float_array_.get(),
          edge_int_array_.get(), contact_int_array_.get(),
          determined_array_.get(), universal_array_.get()})
      runtime_->program().fill_ndarray_fast_u32(array, 0);
  }
  activity_->reset();
  launch_clear(true);
  launch_centroid({}, true);
  launch_determine();
  activity_->clear();

  IRBuilder b;
  auto *flags = array_param(b, 0, PrimitiveType::i32, 1);
  auto *counts = array_param(b, 1, PrimitiveType::i64, 1);
  for (int i = 0; i < 4; ++i)
    store(b, counts, {b.get_int32(i)}, b.get_int64(0));
  for (int type = 0; type < 2; ++type) {
    auto *presence =
        type ? s.fields.bindings().presence : s.particles.bindings().presence;
    const auto awake =
        type ? activity_->centroids : activity_->particle_activity;
    const int n = type ? groups_ : particles_, offset = type ? particles_ : 0;
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(n), false,
                                    runtime_->cpu_threads());
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    auto *present = detail::NativeSet{presence}.active(b, row);
    auto *active = awake.active(b, row);
    store(b, flags, {b.create_add(row, b.get_int32(offset))},
          b.create_add(b.create_cast(present, PrimitiveType::i32),
                       b.create_mul(b.create_cast(active, PrimitiveType::i32),
                                    b.get_int32(2))));
    accumulate(b, counts, {b.get_int32(type * 2)},
               b.create_cast(present, PrimitiveType::i64));
    accumulate(b, counts, {b.get_int32(type * 2 + 1)},
               b.create_cast(active, PrimitiveType::i64));
  }
  inspection_ = engine::CompiledKernel(
      *runtime_,
      "prepared_inspection_" +
          std::to_string(s.particles.bindings().presence->id),
      b.extract_ir(),
      {engine::Param::array(PrimitiveType::i32),
       engine::Param::array(PrimitiveType::i64)});
  prepare_update_execution();
  prepare_geometry_execution();
  if (s.capacity.limits()[Budget::LiveResponses])
    live_hierarchy_ = std::make_unique<detail::LiveHierarchy>(*this);
  ready_ = true;
  uploaded_ = true;
  invalidated_ = false;
  tick_values({});  // Actual empty dispatch for every numerical family, with
                    // stable bindings.
  inspect_prepared();
  s.completed_ticks = 0;  // Setup warming is not a published model tick.
  if (s.capacity.limits()[Budget::GeometryWork]) {
    const auto empty = query_prepared_geometry(0);
    if (empty.status != GeometryStatus::Ok || empty.present || empty.contacts)
      throw std::runtime_error("geometry preparation did not finish empty");
    s.geometry_valid = false;
  }
  if (s.capacity.limits()[Budget::CascadeNodes])
    centroid_cascade_ = std::make_unique<detail::CentroidCascade>(*this);
  if (s.capacity.limits()[Budget::TickWork]) {
    tick_execution_.reset(new detail::TickExecution(*this));
    const auto required = detail::PreparedStorage::required_scratch(
        s.capacity.limits(), runtime_->on_cuda(), profile_);
    const auto native = tick_execution_->native_bytes();
    if (native > s.capacity.limits()[Budget::ScratchBytes] - required)
      throw CapacityError({Budget::ScratchBytes, required + native,
                           s.capacity.limits()[Budget::ScratchBytes]});
    s.manifest.push_back({"tick-native-metadata-measured", std::size_t(native),
                          1, 1, std::size_t(native)});
  }
  if (s.capacity.limits()[Budget::PlacementWork]) {
    placement_.reset(new detail::Placement(*this));
    const auto required = detail::PreparedStorage::required_scratch(
        s.capacity.limits(), runtime_->on_cuda(), profile_);
    const auto native = placement_->native_bytes();
    const auto tick_native =
        tick_execution_ ? tick_execution_->native_bytes() : 0;
    if (native >
        s.capacity.limits()[Budget::ScratchBytes] - required - tick_native)
      throw CapacityError({Budget::ScratchBytes,
                           required + tick_native + native,
                           s.capacity.limits()[Budget::ScratchBytes]});
    s.manifest.push_back({"placement-native-metadata-measured",
                          std::size_t(native), 1, 1, std::size_t(native)});
  }
  if (live_hierarchy_ && tick_execution_)
    prepare_rebase_execution();
  s.binding_bytes =
      activity_->prepared_binding_bytes() + s.particles.prepared_bytes() +
      s.fields.prepared_bytes() + inspection_.prepared_bytes() +
      update_candidate_.prepared_bytes() + update_commit_.prepared_bytes() +
      focus_commit_.prepared_bytes() + rebase_.prepared_bytes() + rebase_check_.prepared_bytes();
  for (const auto *kernel : {&clear_, &force_, &contact_, &integrate_,
                             &determine_, &centroid_, &finalize_})
    s.binding_bytes += kernel->prepared_bytes();
  if (s.capacity.limits()[Budget::GeometryWork])
    for (const auto *kernel : {&geometry_collect_, &geometry_source_order_,
                               &geometry_discover_, &geometry_finalize_})
      s.binding_bytes += kernel->prepared_bytes();
  if (centroid_cascade_)
    s.binding_bytes += centroid_cascade_->binding_bytes();
  if (live_hierarchy_)
    s.binding_bytes += live_hierarchy_->binding_bytes();
  if (tick_execution_)
    s.binding_bytes += tick_execution_->binding_bytes();
  if (placement_)
    s.binding_bytes += placement_->binding_bytes();
  const auto reserve = detail::PreparedStorage::binding_reserve_bytes(
      runtime_->on_cuda(), s.capacity.limits()[Budget::GeometryWork] != 0,
      s.capacity.limits()[Budget::CascadeNodes] != 0,
      s.capacity.limits()[Budget::CascadeCoverage] != 0, profile_.wide(),
      s.capacity.limits()[Budget::LiveResponses] != 0,
      s.capacity.limits()[Budget::TickWork] != 0,
      s.capacity.limits()[Budget::PlacementWork] != 0);
  if (s.binding_bytes > reserve)
    throw std::runtime_error(
        "prepared binding signature exceeds declared reservation");
  s.manifest.push_back(
      {"prepared-binding-payload", s.binding_bytes, 1, 1, s.binding_bytes});
  s.manifest.push_back({"unused-binding-reserve", reserve - s.binding_bytes, 1,
                        1, reserve - s.binding_bytes});
  if (s.inspection_counts != std::array<std::int64_t, 4>{} ||
      universal_values_[0] != 0)
    throw std::runtime_error("prepared execution did not finish empty");
}

void Harness::inspect_prepared() {
  if (!prepared_ || !ready_)
    throw std::logic_error("prepared execution is not ready");
  try {
    validate_prepared_bindings();
    auto &s = *prepared_;
    auto call = inspection_.invocation(true);
    auto &ctx = call.get();
    if (!ctx.prepared) {
      ctx.set_arg_ndarray({0}, *s.inspection_flags_array);
      ctx.set_arg_ndarray({1}, *s.inspection_counts_array);
    }
    inspection_.launch(ctx);
    runtime_->synchronize();
    engine::readback(*s.inspection_flags_array, s.inspection_flags.data(),
                     s.inspection_flags.size() * sizeof(int));
    engine::readback(*s.inspection_counts_array, s.inspection_counts.data(),
                     sizeof(s.inspection_counts));
    engine::readback(*activity_->particle_flags, s.inspection_state.data(),
                     2 * std::size_t(std::max(particles_, 1)) * sizeof(int));
    engine::readback(*activity_->group_flags,
                     s.inspection_state.data() + 2 * std::max(particles_, 1),
                     2 * std::size_t(std::max(groups_, 1)) * sizeof(int));
    engine::readback(
        *activity_->particle_delta, s.inspection_motion_values.data(),
        3 * std::size_t(std::max(particles_, 1)) * profile_.bytes());
    engine::readback(
        *activity_->group_delta,
        s.inspection_motion_values.data_at(3 * std::max(particles_, 1)),
        6 * std::size_t(std::max(groups_, 1)) * profile_.bytes());
    engine::readback(*activity_->focus,
                     profile_.wide()
                         ? static_cast<void *>(s.inspection_focus64.data())
                         : s.inspection_focus.data(),
                     11 * profile_.bytes());
    download();
    if (path_ == HostPath::BulkCopy)
      engine::readback(*universal_array_, universal_values_.data(),
                       profile_.bytes());
    s.inspected_generation = s.generation;
  } catch (...) {
    ready_ = false;
    throw;
  }
}

void Harness::initialize_prepared_fixture(const Parameters64 &p) {
  if (!prepared_ || !ready_ || prepared_->populated)
    throw std::logic_error(
        "fixture initialization requires an empty prepared execution");
  auto &s = *prepared_;
  const auto failure = s.check(s.live, 0);
  if (failure.exhausted())
    throw std::invalid_argument("prepared fixture capacity exhausted");
  if (s.live.particles != s.particles.size() ||
      s.live.fields != s.fields.size())
    throw std::invalid_argument("fixture counts and native presence disagree");
  try {
    validate_prepared_bindings();
    validate_staging();
    if (profile_.wide()) {
      std::uint64_t visits = 0;
      if (!s.prepare_ratios(false, s.live.edges, visits))
        throw std::invalid_argument("invalid trial source accounting");
      engine::upload(*s.live_ratios_array, s.live_ratios.data(),
                     s.live_ratios.size() * sizeof(std::uint64_t));
    }
    exposed_ = s.live.particles;
    std::fill_n(s.validation.data(), groups_, 0);
    for (std::size_t i = 0; i < s.live.edges; ++i)
      s.validation[edge_int(kEdgeGroup, int(i))] = 1;
    exposed_fields_ =
        std::count(s.validation.begin(), s.validation.begin() + groups_, 1);
    activity_->index(edge_int_data, contact_int_data);
    if (path_ == HostPath::BulkCopy) {
      engine::upload(*particle_array_, particle_values_.data(),
                     particle_values_.bytes());
      engine::upload(*group_array_, group_values_.data(),
                     group_values_.bytes());
      engine::upload(*edge_float_array_, edge_float_data.data(),
                     edge_float_data.size() * sizeof(float));
      engine::upload(*edge_int_array_, edge_int_data.data(),
                     edge_int_data.size() * sizeof(int));
      engine::upload(*contact_int_array_, contact_int_data.data(),
                     contact_int_data.size() * sizeof(int));
    }
    activity_->reset();
    launch_clear(true);
    launch_centroid(p, true);
    launch_determine();
    activity_->clear();
    runtime_->synchronize();
    if (profile_.wide() &&
        activity_->report(exposed_, exposed_fields_).numerical_errors)
      throw std::runtime_error("trial seed numerical invariant failed");
    if (profile_.wide()) {
      double mass;
      if (path_ == HostPath::BulkCopy)
        engine::readback(*universal_array_, &mass, sizeof(mass));
      else
        mass = universal_values_[0];
      if (!NumericalProfile::supported_real(mass))
        throw std::runtime_error(
            "trial seed universal mass outside finite domain");
    }
    previous_parameters_ = p;
    invalidated_ = true;
    s.populated = true;
    ++s.generation;
  } catch (...) {
    ready_ = false;
    throw;
  }
}

void Harness::validate_tick_parameters(const Parameters64 &p) const {
  validate_parameters(p);
  if (profile_.wide())
    for (double x : {p.gravity, p.dt, p.contact_stiffness, p.reach_strength})
      if (!NumericalProfile::supported_real(x))
        throw std::invalid_argument(
            "trial parameter outside supported finite/FTZ domain");
}

void Harness::tick(const Parameters &p) {
  require_legacy();
  tick_values(p);
}
void Harness::tick64(const Parameters64 &p) {
  require_wide();
  tick_values(p);
}
void Harness::tick_values(const Parameters64 &p) {
  if (!ready_) {
    throw std::logic_error(
        "field::Harness: successful publication required before tick");
  }
  if (tick_execution_)
    throw std::logic_error("contact-safe execution requires tick_prepared");
  validate_tick_parameters(p);
  if (prepared_ &&
      prepared_->completed_ticks == std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("completed tick observation counter exhausted");
  if (live_hierarchy_)
    live_hierarchy_->validate();
  try {
    validate_prepared_bindings();
    const auto start = std::chrono::steady_clock::now();
    const bool parameters_changed =
        p.gravity != previous_parameters_.gravity ||
        p.dt != previous_parameters_.dt ||
        p.contact_stiffness != previous_parameters_.contact_stiffness ||
        p.reach_strength != previous_parameters_.reach_strength;
    activity_->begin(*this, mode_ == TickMode::AllLoaded,
                     invalidated_ || parameters_changed, p.reach_strength != 0);
    launch_clear(false);
    if (live_hierarchy_installed())
      live_hierarchy_->force(p.gravity);
    else
      launch_force(p);
    launch_contact(p);
    launch_integrate(p);
    if (!profile_.wide())
      launch_determine();
    activity_->touch_contacts(*this, p.contact_stiffness);
    if (live_hierarchy_installed())
      live_hierarchy_->poll(p.gravity, p.contact_stiffness);
    else {
      activity_->mark(*this, mode_ == TickMode::AllLoaded);
      launch_centroid(p, false);
      activity_->touch(*this, p.gravity);
    }
    runtime_->synchronize();
    demand_ = activity_->report(exposed_, exposed_fields_);
    if (live_hierarchy_installed())
      demand_.force_skipped =
          live_hierarchy_->response_count() - demand_.force_requests;
    if (demand_.numerical_errors)
      throw std::runtime_error("trial numerical/source invariant failed");
    demand_.frame_anchor = frame_anchor_;
    demand_.tick_ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - start)
                          .count();
    previous_parameters_ = p;
    invalidated_ = false;
    timings_.kernel_ms += elapsed(start);
    if (prepared_)
      ++prepared_->completed_ticks;
  } catch (...) {
    ready_ = false;
    throw;
  }
}

bool Harness::live_hierarchy_installed() const {
  return live_hierarchy_ && live_hierarchy_->installed();
}
LiveHierarchyResult Harness::install_live_hierarchy(
    std::uint64_t generation,
    const WeightedCascadeSpec &spec,
    std::uint64_t work) {
  if (!live_hierarchy_)
    return {};
  validate_prepared_bindings();
  return live_hierarchy_->install(generation, spec, work);
}
const LiveHierarchyInspection &Harness::inspect_live_hierarchy() {
  if (!ready_ || !live_hierarchy_installed())
    throw std::logic_error("no ready live hierarchy");
  live_hierarchy_->validate();
  try {
    return live_hierarchy_->inspect();
  } catch (...) {
    ready_ = false;
    throw;
  }
}

const LiveResponseInspection &Harness::inspect_live_responses(std::uint64_t work) {
  if (!ready_ || !live_hierarchy_installed())
    throw std::logic_error("no ready live hierarchy");
  live_hierarchy_->validate();
  try {
    return live_hierarchy_->inspect_responses(work);
  } catch (...) {
    ready_ = false;
    throw;
  }
}

}  // namespace field
