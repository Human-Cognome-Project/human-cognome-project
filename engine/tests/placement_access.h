#pragma once
#include "field/placement.h"

namespace field::testing {
struct PlacementAccess {
  static auto kernels(Harness &h) {
    auto &p = *h.placement_;
    return std::array<const engine::CompiledKernel *, 4>{
        &p.collect_, &p.capture_, &p.solve_, &p.publish_};
  }
  static auto surfaces(Harness &h) {
    auto &p = *h.placement_;
    return std::array<taichi::lang::Ndarray *, 10>{
        p.rows_.get(),  p.stats_.get(),    p.focal_.get(),
        p.keep_.get(),  p.requests_.get(), p.occupancy_.get(),
        p.cells_.get(), p.attempt_.get(),  p.published_.get(),
        p.counts_.get()};
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
