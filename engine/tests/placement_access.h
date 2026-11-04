#pragma once
#include "field/placement.h"
#include "field/admission_contacts.h"

namespace field::testing {
struct PlacementAccess {
  static UpdateResult focus(Harness &h, const std::vector<engine::RecordHandle> &fields,
                            std::uint64_t work = UINT64_MAX,
                            std::uint64_t generation = UINT64_MAX) {
    return h.replace_current_focus(generation == UINT64_MAX ? h.prepared_->generation : generation,
                                    fields.data(), fields.size(), work);
  }
  static const engine::CompiledKernel &focus_kernel(Harness &h) { return h.focus_commit_; }
  static void radial(Harness &h, bool enabled) { h.previous_parameters_.reach_strength = enabled; }
  static UpdateResult bootstrap(Harness &h, const std::vector<UpdateCommand> &commands,
                                const std::vector<PlacementIntrinsic> &inputs,
                                const LiveRefreshSpec &hierarchy,
                                const std::vector<UpdateRef> &focus,
                                bool commit = true, std::uint64_t work = UINT64_MAX) {
    return h.stage_current_admission(h.prepared_->generation, commands.data(),
        commands.size(), inputs.data(), inputs.size(), hierarchy, work,
        UINT64_MAX, UINT64_MAX, commit, true, focus.data(), focus.size());
  }
  static std::vector<double> bootstrap_output(Harness &h) {
    std::vector<double> out(h.placement_->attempt_->get_nelement());
    engine::readback(*h.placement_->attempt_, out.data(), out.size() * sizeof(double));
    return out;
  }
  static const engine::CompiledKernel &bootstrap_kernel(Harness &h) {
    return h.placement_->bootstrap_solve_;
  }
  static GeometryResult contacts(Harness &h, std::size_t particles,
                                  std::size_t contacts,
                                  std::uint64_t work = UINT64_MAX) {
    detail::PreparedCounts counts;
    counts.particles = particles;
    counts.contacts = contacts;
    return h.placement_->contacts_->validate(counts, work);
  }
  static auto contact_counts(Harness &h) {
    return h.placement_->contacts_->counts_;
  }
  static void remap_contacts(Harness &h, const std::vector<int> &destinations) {
    engine::upload(*h.prepared_->destinations_array, destinations.data(),
                     destinations.size() * sizeof(int));
    auto &c = *h.placement_->contacts_;
    c.remap(true);
    c.launch(4, int(c.counts_[2]));
    h.runtime_->synchronize();
  }
  static GeometryInspection contact_inspection(Harness &h) {
    auto &c = *h.placement_->contacts_;
    GeometryInspection result;
    const auto copy = [&](int id, std::vector<int> &out) {
      auto &a = *c.arrays_[id];
      out.resize(a.get_nelement());
      engine::readback(a, out.data(), out.size() * sizeof(int));
    };
    copy(c.Rows, result.occupied_rows);
    copy(c.Pairs, result.pairs);
    copy(c.Flags, result.pair_flags);
    copy(c.SourceMap, result.source_to_pair);
    copy(c.Offsets, result.offsets);
    copy(c.Orders, result.orders);
    return result;
  }
  static auto kernels(Harness &h) {
    auto &p = *h.placement_;
    return std::array<const engine::CompiledKernel *, 14>{
        &p.collect_, &p.capture_, &p.solve_, &p.publish_, &p.initialize_,
        &p.contacts_->kernels_[0], &p.contacts_->kernels_[1],
        &p.contacts_->kernels_[2], &p.contacts_->kernels_[3],
        &p.contacts_->kernels_[4], &p.contacts_->remap_, &p.contacts_->publish_,
        &p.bootstrap_solve_, &h.focus_commit_};
  }
  static auto surfaces(Harness &h) {
    auto &p = *h.placement_;
    std::array<taichi::lang::Ndarray *, 33> result{
        p.rows_.get(),  p.stats_.get(),    p.focal_.get(),
        p.keep_.get(),  p.requests_.get(), p.occupancy_.get(),
        p.cells_.get(), p.attempt_.get(),  p.published_.get(),
        p.counts_.get(), h.prepared_->new_rows_array.get()};
    for (int i = 0; i < 15; ++i)
      result[11 + i] = p.contacts_->arrays_[i].get();
    for (int i = 0; i < 6; ++i)
      result[26 + i] = &h.tick_execution_->admission_array(i);
    result[32] = h.activity_->focus.get();
    return result;
  }
  static PlacementResult stage(Harness &h, const UpdateCommand *commands,
                                std::size_t count,
                                const PlacementIntrinsic *inputs,
                                std::size_t bodies) {
    return h.placement_->stage(h.prepared_->generation, commands, count,
                               inputs, bodies, UINT64_MAX, UINT64_MAX);
  }
  static std::vector<double> generated(Harness &h) {
    std::vector<double> rows(h.prepared_->new_rows_array->get_nelement());
    engine::readback(*h.prepared_->new_rows_array, rows.data(),
                     rows.size() * sizeof(double));
    return rows;
  }
  static UpdateResult preflight(Harness &h, const UpdateCommand *commands,
                                std::size_t count,
                                const PlacementIntrinsic *inputs,
                                std::size_t bodies,
                                const LiveRefreshSpec &hierarchy,
                                std::uint64_t work = UINT64_MAX,
                                bool commit = false) {
    return h.stage_current_admission(h.prepared_->generation, commands, count,
                                     inputs, bodies, hierarchy, UINT64_MAX,
                                     UINT64_MAX, work, commit);
  }
  static std::vector<double> derived(Harness &h) {
    std::vector<double> values(h.prepared_->derived_array->get_nelement());
    engine::readback(*h.prepared_->derived_array, values.data(),
                     values.size() * sizeof(double));
    return values;
  }
  static void configure(Harness &h,
                        const std::vector<engine::RecordHandle> &handles,
                        const std::vector<placement_reference::Sphere> &spheres,
                        engine::RecordHandle focal,
                        std::array<double, 3> target) {
    h.inspect_prepared();
    for (std::size_t i = 0; i < spheres.size(); ++i) {
      for (int k = 0; k < 3; ++k)
        h.particle64(kPosX + k, handles[i].record) = spheres[i].centre[k];
      h.particle64(kRadius, handles[i].record) = spheres[i].radius;
    }
    for (int k = 0; k < 3; ++k)
      h.group64(kCenX + k, focal.record) = target[k];
    if (h.path_ == HostPath::BulkCopy) {
      h.particle_values_.upload(*h.particle_array_);
      h.group_values_.upload(*h.group_array_);
    }
    h.runtime_->synchronize();
  }
  static void unused_poison(Harness &h, int row) {
    h.inspect_prepared();
    h.particle64(kPosX, row) = std::numeric_limits<double>::quiet_NaN();
    h.particle64(kRadius, row) = 1000;
    PreparedFixtureAccess::upload_body_fixture64(h);
  }
};
}  // namespace field::testing
