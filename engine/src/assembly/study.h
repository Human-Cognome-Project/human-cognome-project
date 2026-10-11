#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "field/field.h"

// Native preparation for one fixed study. No database access, pool lifecycle,
// general recomposition protocol or analyst policy is implemented here.
namespace assembly {

struct Point { double x{0}, y{0}, z{0}; };
struct Sphere { Point centre; double radius{0}; };

// The caller supplies the centroid and component layout. This does not solve
// packing or move the centroid to find a minimum enclosing sphere.
double enclosing_radius(Point centroid, const std::vector<Sphere> &components);

enum class ScaleRole { Root, PrimaryStep, SecondaryCompression, NamingEquivalent };
struct StudyNode {
  const taichi::lang::SNode *layout{nullptr};
  int parent{-1};
  ScaleRole role{ScaleRole::Root};
};
struct ScaleBinding { StudyNode node; double units{1}; };

struct ParentOccurrence {
  std::string value;
  std::uint32_t ordinal{0};
  std::uint64_t count{0};
};
struct Definition {
  std::string id;
  std::uint64_t mass{0};
  Point centroid;
  std::vector<Sphere> components;
  std::vector<ParentOccurrence> parents;
};
struct Occurrence {
  std::string id;
  std::string definition;
  int slot{-1};
  int study_node{-1};
  Point position;
  // In this node's spatial units per tick-time unit, converted with position.
  // No time conversion or cross-focus trajectory equivalence is claimed.
  Point velocity;
};
enum class Exposure { Aggregate, Parts };
struct Selection {
  std::string aggregate;
  std::vector<std::string> parts;
  Exposure exposure{Exposure::Aggregate};
};
struct Contact { std::string a, b; };
struct StudyInput {
  int capacity{0};
  std::vector<StudyNode> nodes;
  std::vector<Definition> definitions;
  std::vector<Occurrence> occurrences;
  std::vector<Selection> selections;
  std::vector<Contact> contacts;
};

struct PreparedParticle {
  Occurrence occurrence;
  std::uint64_t exact_mass{0};
  Point position, velocity;
  float mass{0}, radius{0};
};
enum class GroupKind { SiblingDefinition, ParentValue };
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
             const field::Parameters &parameters);
  void publish(const StudyInput &input, const field::Parameters &parameters);
  void tick(const field::Parameters &parameters) { fields_->tick(parameters); }
  void download() { fields_->download(); }
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
