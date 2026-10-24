#pragma once

// Only test code may supply numerical starts to the prepared path. This is
// initial population and new-row update staging, not production initialization.
#include <algorithm>
#include <cmath>
#include <vector>
#include "engine/transfer.h"
#include "field/activity.h"
#include "field/prepared_storage.h"

namespace field::testing {
struct FixtureSource {
  std::vector<std::array<float, kParticleFieldCount>> particles;
  std::vector<int> kinds;
  struct Edge {
    int particle, group;
    float share;
    std::uint64_t ordinal;
  };
  std::vector<Edge> edges;
  std::vector<std::array<int, 2>> contacts;
  std::vector<int> focus;
};
struct PreparedFixtureAccess {
  static void read_contacts(Harness &h) {
    if (h.path_ == HostPath::BulkCopy)
      engine::readback(*h.contact_int_array_, h.contact_int_data.data(),
                       h.contact_int_data.size() * sizeof(int));
  }
  static void read_relationships(Harness &h) {
    read_contacts(h);
    if (h.path_ == HostPath::BulkCopy) {
      engine::readback(*h.edge_int_array_, h.edge_int_data.data(),
                       h.edge_int_data.size() * sizeof(int));
      engine::readback(*h.edge_float_array_, h.edge_float_data.data(),
                       h.edge_float_data.size() * sizeof(float));
    }
  }
  // Deliberate source fault injection for candidate-only rejection tests.
  // Refresh all body values first; do not upload stale observation staging.
  static void inject_position_value(Harness &h, int row, float value) {
    h.inspect_prepared();
    h.particle_data[kPosX * h.particle_capacity() + row] = value;
    if (h.path_ == HostPath::BulkCopy)
      engine::upload(*h.particle_array_, h.particle_data.data(),
                     h.particle_data.size() * sizeof(float));
    h.runtime_->synchronize();
  }
  static void stage_new(Harness &h,
                        int command,
                        const std::array<float, kParticleFieldCount> &row) {
    auto &s = *h.prepared_;
    const auto b =
        std::max(s.capacity.limits()[Budget::Batch], std::uint64_t{1});
    if (command < 0 ||
        std::uint64_t(command) >= s.capacity.limits()[Budget::Batch])
      throw std::invalid_argument("fixture start command out of range");
    float speed2 = 0;
    for (int f = 0; f < kParticleFieldCount; ++f) {
      s.new_rows[f * b + command] = row[f];
      if (f >= kVelX && f <= kVelZ)
        speed2 += row[f] * row[f];
    }
    s.new_rows[11 * b + command] = std::sqrt(speed2);
  }
  static void inject_failure(Harness &h, int point) {
    h.prepared_->failure_point = point;
  }
  static int field_kind(Harness &h, const engine::RecordHandle &handle) {
    return h.prepared_->fields.inspect(handle).kind;
  }
  static std::vector<taichi::lang::SNode *> nodes(Harness &h) {
    auto &s = *h.prepared_;
    std::vector<taichi::lang::SNode *> result;
    for (auto *binding : {&s.particles.bindings(), &s.fields.bindings()}) {
      result.insert(result.end(),
                    {binding->presence->parent, binding->declaration->parent,
                     binding->presence, binding->allocator});
    }
    result.insert(
        result.end(),
        {h.activity_->particle_activity.cells->parent->parent,
         h.activity_->particle_activity.cells->parent,
         h.activity_->particle_activity.cells,
         h.activity_->centroids.cells->parent, h.activity_->centroids.cells});
    return result;
  }
  struct Input {
    std::vector<int> keys, kinds;
    std::vector<engine::RecordHandle> particle_rows, field_rows, retire;
    std::vector<int> focus;
    Input(std::size_t p, std::size_t g, std::size_t b, std::size_t f)
        : keys(b),
          kinds(b),
          particle_rows(p),
          field_rows(g),
          retire(b),
          focus(f) {
    }
  };
  static void load(Harness &h,
                   const FixtureSource &source,
                   Input &input,
                   const Parameters &parameters,
                   bool holes) {
    auto &s = *h.prepared_;
    const auto &limits = s.capacity.limits();
    const auto b = std::size_t(limits[Budget::Batch]);
    if (!b)
      throw std::invalid_argument("fixture requires nonzero batch capacity");
    detail::PreparedCounts demand{source.particles.size(), source.kinds.size(),
                                  source.edges.size(), source.contacts.size(),
                                  source.focus.size()};
    if (s.check(demand, std::min(b, source.particles.size())).exhausted())
      throw std::invalid_argument("fixture exceeds independent budget");
    if (s.populated || s.particles.size() || s.fields.size())
      throw std::logic_error("fixture loading is initial only");
    for (const auto &particle : source.particles) {
      for (float value : particle)
        if (!std::isfinite(value))
          throw std::invalid_argument("nonfinite fixture value");
      if (!(particle[kMass] > 0) || particle[kRadius] < 0)
        throw std::invalid_argument("invalid fixture body");
    }
    for (int kind : source.kinds)
      if (kind < 1 || kind > 255)
        throw std::invalid_argument("invalid fixture field kind");
    for (const auto &edge : source.edges)
      if (edge.particle < 0 ||
          std::size_t(edge.particle) >= source.particles.size() ||
          edge.group < 0 || std::size_t(edge.group) >= source.kinds.size() ||
          !(edge.share > 0) || edge.share > 1)
        throw std::invalid_argument("invalid fixture participation");
    for (const auto &contact : source.contacts)
      if (contact[0] < 0 || contact[1] < 0 ||
          std::size_t(contact[0]) >= source.particles.size() ||
          std::size_t(contact[1]) >= source.particles.size() ||
          contact[0] == contact[1])
        throw std::invalid_argument("invalid fixture contact");
    for (std::size_t i = 0; i < source.focus.size(); ++i) {
      const int group = source.focus[i];
      if (group < 0 || std::size_t(group) >= source.kinds.size() ||
          std::find(source.focus.begin(), source.focus.begin() + i, group) !=
              source.focus.begin() + i ||
          std::none_of(source.edges.begin(), source.edges.end(),
                       [&](const auto &edge) { return edge.group == group; }))
        throw std::invalid_argument("invalid fixture focus");
    }
    try {
      const auto define = [&](engine::PreparedRecordMap &map, std::size_t count,
                              std::vector<engine::RecordHandle> &rows,
                              bool particle) {
        for (std::size_t start = 0; start < count; start += b) {
          const auto n = std::min(b, count - start);
          for (std::size_t i = 0; i < n; ++i) {
            const auto logical = start + i;
            input.keys[i] =
                particle
                    ? int((logical * 104729 + 17) % limits[Budget::Potential])
                    : int(logical);
            input.kinds[i] =
                particle ? 1 : source.kinds[logical % source.kinds.size()];
          }
          if (map.define_batch(input.keys.data(), input.kinds.data(), n,
                               s.results.data()) != engine::RecordStatus::Ok)
            throw std::runtime_error("fixture native definition failed");
          for (std::size_t i = 0; i < n; ++i)
            rows[start + i] = s.results[i].handle;
        }
      };
      // Claim the full native bank then retire a suffix before creating any
      // numerical object. This leaves reusable holes across native record rows.
      const auto pclaims = holes ? std::size_t(limits[Budget::Particles])
                                 : source.particles.size();
      define(s.particles, pclaims, input.particle_rows, true);
      if (holes && !source.particles.empty()) {
        const auto last = std::find_if(
            input.particle_rows.begin(), input.particle_rows.begin() + pclaims,
            [&](const auto &h) {
              return h.record == int(limits[Budget::Particles] - 1);
            });
        if (last == input.particle_rows.begin() + pclaims)
          throw std::runtime_error("last record not claimable");
        std::swap(*last, input.particle_rows[source.particles.size() - 1]);
      }
      for (std::size_t start = source.particles.size(); start < pclaims;
           start += b) {
        const auto n = std::min(b, pclaims - start);
        if (s.particles.release_batch(input.particle_rows.data() + start, n,
                                      s.results.data()) !=
            engine::RecordStatus::Ok)
          throw std::runtime_error("fixture native retirement failed");
      }
      define(s.fields, source.kinds.size(), input.field_rows, false);
      for (std::size_t i = 0; i < source.particles.size(); ++i) {
        const auto ref = input.particle_rows[i];
        s.particle_headers[ref.record] = ref;
        for (int f = 0; f < kParticleFieldCount; ++f)
          h.particle(f, ref.record) = source.particles[i][f];
      }
      for (std::size_t i = 0; i < source.kinds.size(); ++i)
        s.field_headers[input.field_rows[i].record] = input.field_rows[i];
      for (std::size_t i = 0; i < source.edges.size(); ++i) {
        const auto edge = source.edges[i];
        const auto p = input.particle_rows[edge.particle],
                   g = input.field_rows[edge.group];
        s.edges[i] = {p, g, edge.ordinal, edge.share};
        h.edge_int(kEdgeParticle, int(i)) = p.record;
        h.edge_int(kEdgeGroup, int(i)) = g.record;
        h.edge_float(kShare, int(i)) = edge.share;
      }
      for (std::size_t i = 0; i < source.contacts.size(); ++i) {
        const auto a = input.particle_rows[source.contacts[i][0]],
                   b = input.particle_rows[source.contacts[i][1]];
        s.contacts[i] = {a, b, i, 0};
        h.contact_int(kContactA, int(i)) = a.record;
        h.contact_int(kContactB, int(i)) = b.record;
      }
      for (std::size_t i = 0; i < source.focus.size(); ++i) {
        s.focal[i] = input.field_rows[source.focus[i]];
        input.focus[i] = s.focal[i].record;
      }
      input.focus.resize(
          source.focus.size());  // capacity was reserved before measured work
      h.activity_->set_focus(input.focus);
      s.live = demand;
      h.initialize_prepared_fixture(parameters);
    } catch (...) {
      h.ready_ = false;
      throw;
    }
  }
};
}  // namespace field::testing
