#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace taichi::lang {
class SNode;
}

// Host input values only. Including these does not load the engine or libpq.
namespace assembly {
struct Point {
  double x{0}, y{0}, z{0};
};
struct Sphere {
  Point centre;
  double radius{0};
};
enum class ScaleRole {
  Root,
  PrimaryStep,
  SecondaryCompression,
  NamingEquivalent
};
struct StudyNode {
  const taichi::lang::SNode *layout{nullptr};
  int parent{-1};
  ScaleRole role{ScaleRole::Root};
};
struct ScaleBinding {
  StudyNode node;
  double units{1};
};
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
  std::string id, definition;
  int slot{-1}, study_node{-1};
  Point position;
  // Node-local spatial units per tick-time unit, converted with position.
  Point velocity;
};
enum class Exposure { Aggregate, Parts };
struct Selection {
  std::string aggregate;
  std::vector<std::string> parts;
  Exposure exposure{Exposure::Aggregate};
};
struct Contact {
  std::string a, b;
};
// Explicit field names, encoded losslessly from ObjectKey by typed callers.
// Group anchors need no live Definition, geometry, slot or naming mass.
struct Membership {
  std::string occurrence, group;
};
struct StudyInput {
  int capacity{0};
  std::vector<StudyNode> nodes;
  std::vector<Definition> definitions;
  std::vector<Occurrence> occurrences;
  std::vector<Selection> selections;
  std::vector<Contact> contacts;
  std::vector<Membership> memberships;
};
enum class GroupKind { SiblingDefinition, ParentValue, Membership };
}  // namespace assembly
