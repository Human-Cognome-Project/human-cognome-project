#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "assembly/study_input.h"
#include "field/field.h"

// Native preparation for one fixed study. No database access, pool lifecycle,
// general recomposition protocol or analyst policy is implemented here.
namespace assembly {

// The caller supplies the centroid and component layout. This does not solve
// packing or move the centroid to find a minimum enclosing sphere.
double enclosing_radius(Point centroid, const std::vector<Sphere> &components);

struct PreparedParticle {
  Occurrence occurrence;
  std::uint64_t exact_mass{0};
  Point position, velocity;
  float mass{0}, radius{0};
};
struct Group { GroupKind kind; std::string value; };
struct Participation {
  std::string occurrence;
  int slot{-1}, group{-1};
  std::string parent_value;
  std::uint32_t ordinal{0};
  std::uint64_t exact_count{0};
  float share{0};
};
struct ContactSlots { int a, b; };
struct PreparedStudy {
  int capacity{0};
  std::vector<ScaleBinding> scales;
  std::vector<PreparedParticle> particles;
  std::vector<Group> groups;
  std::vector<Participation> participation;
  std::vector<ContactSlots> contacts;
  std::vector<int> focal_groups;
};

// Recomputes derived geometry/scale on publication. An unchanged prepared
// study is reused by tick; no recursive geometry walk enters the hot path.
PreparedStudy prepare(const StudyInput &input);

// Counts are fixed for this instance. A supported refresh restages equal-sized
// relationships/state and reseeds centroids. A count change needs another
// fixed study, not implicit general recomposition or transfer rollback.
class FixedStudy {
 public:
  FixedStudy(engine::Runtime &runtime, const StudyInput &input,
             const field::Parameters &parameters,
             field::HostPath path = field::HostPath::BulkCopy);
  void publish(const StudyInput &input, const field::Parameters &parameters);
  void tick(const field::Parameters &parameters) { fields_->tick(parameters); }
  void download() { fields_->download(); }
  void set_tick_mode(field::TickMode mode) { fields_->set_tick_mode(mode); }
  const field::Demand &demand() const { return fields_->demand(); }
  bool ready() const { return fields_->ready(); }
  const std::string &error() const { return error_; }
  const PreparedStudy &prepared() const { return prepared_; }
  const field::Harness &fields() const { return *fields_; }

 private:
  void stage(const PreparedStudy &target, const field::Parameters &parameters);
  PreparedStudy prepared_;
  std::unique_ptr<field::Harness> fields_;
  std::string error_;
};

}  // namespace assembly
