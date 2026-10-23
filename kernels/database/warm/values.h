#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "../codec/codec.h"

// Engine-independent, supplied recipe values. No projection engine, live slots,
// SNode pointers or device state are part of the Stage A interface.
namespace warm {

struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct Conflict : Error {
  using Error::Error;
};
struct Stale : Error {
  using Error::Error;
};
struct Unauthorized : Error {
  using Error::Error;
};
struct UnboundRoot : Error {
  using Error::Error;
};

class Uuid {
 public:
  Uuid();  // Shared scope / nil.
  explicit Uuid(std::string value);
  const std::string &str() const { return value_; }
  bool nil() const;
  friend bool operator==(const Uuid &a, const Uuid &b) {
    return a.value_ == b.value_;
  }
  friend bool operator!=(const Uuid &a, const Uuid &b) { return !(a == b); }
  friend bool operator<(const Uuid &a, const Uuid &b) {
    return a.value_ < b.value_;
  }

 private:
  std::string value_;
};
Uuid new_uuid();

struct ObjectKey {
  Uuid scope;
  codec::Address address;
};
std::string address_text(const codec::Address &address);
std::string address_array(const codec::Address &address);
std::string key(const ObjectKey &object);
bool operator==(const ObjectKey &a, const ObjectKey &b);
bool operator<(const ObjectKey &a, const ObjectKey &b);
struct ToolRef {
  ObjectKey object;
  Uuid revision;
};
bool operator==(const ToolRef &a, const ToolRef &b);
struct BundleRef {
  Uuid selection, revision;
};

enum class BindingKind { Shared, Personal, Temporary };
struct Binding {
  ObjectKey prefix;
  BindingKind kind{BindingKind::Shared};
  Uuid epoch, policy;
  std::string source_handle;
  std::optional<std::string> snapshot_hash;
  std::vector<Uuid> writers;
};
enum class WriterRole { Reader, Manager, Temporary };
// Created by the trusted embedding/configuration owner, once per connection.
// Neither prepare requests nor acquire/release calls can select a role/actor.
struct Context {
  Uuid actor, instance;
  WriterRole role{WriterRole::Reader};
};

struct Parent {
  ToolRef target;
  std::uint32_t ordinal{0};
  std::optional<std::uint64_t> stored_mass;
  std::uint64_t contribution{0};
};
struct Tool {
  ToolRef ref;
  Uuid epoch;
  std::string recipe;
  std::optional<std::string> notation;
  std::optional<std::uint64_t> mass;
  std::vector<Parent> parents;
  std::vector<ToolRef> member_of;
  Uuid author;
};
struct Point {
  double x{0}, y{0}, z{0};
};
struct Sphere {
  Point centre;
  double radius{0};
};
struct Geometry {
  ObjectKey object;
  Point centroid;
  std::vector<Sphere> spheres;
};
enum class ScaleRole {
  Root,
  PrimaryStep,
  SecondaryCompression,
  NamingEquivalent
};
struct Node {
  int parent{-1};
  ScaleRole role{ScaleRole::Root};
};
struct OccurrenceId {
  std::uint64_t entry{0};
  std::vector<std::uint32_t> path;
};
bool operator==(const OccurrenceId &a, const OccurrenceId &b);
bool operator<(const OccurrenceId &a, const OccurrenceId &b);
struct Occurrence {
  OccurrenceId id;
  ObjectKey definition;
  int node{0};
  Point position, velocity;  // Immutable initial inputs, in node-local units.
};
// Recipe v2: structural identity only, including inactive alternatives. No
// execution slot, world position, velocity or recovery hint can be stored here.
struct StructuralOccurrence {
  OccurrenceId id;
  ObjectKey definition;
  int node{0};
};
enum class FieldKind { SiblingDefinition, ParentValue, Membership };
struct FocalField {
  FieldKind kind;
  ObjectKey object;
};
enum class Exposure { Aggregate, Parts };
struct Selection {
  std::uint64_t entry{0};
  Exposure exposure{Exposure::Aggregate};
  std::vector<OccurrenceId> parts;
};
struct Contact {
  OccurrenceId a, b;
};
struct Bundle {
  BundleRef ref;
  Uuid owner, creator;
  int recipe_version{1};
  std::vector<Tool> tools;
  std::vector<Geometry> geometry;
  std::vector<Node> nodes;
  std::vector<Occurrence> occurrences;
  std::vector<Selection> selections;
  std::vector<Contact> contacts;
  // Versioned alternatives: v1 uses occurrences, v2 uses these structural rows.
  // Mixing versions is rejected, rather than discarding supplied initial state.
  std::vector<StructuralOccurrence> structural_occurrences;
  std::vector<FocalField> focal_fields;
};

// Validate a supplied normalized recipe, rather than derive one (Stage B).
void validate(const Bundle &bundle);
// Explicit legacy-to-operational adapter. Original v1 rows remain unchanged;
// caller supplies a distinct publication identity and explicit focal selection.
// Legacy starts are discarded, never treated as restart hints.
Bundle structural_from_v1(const Bundle &legacy, BundleRef publication,
                          std::vector<FocalField> focal_fields);
std::string tool_body(
    const Tool &tool);  // Canonical exact immutable-body comparison.
void validate_directory(const std::vector<Binding> &bindings);
bool contains(const ObjectKey &prefix, const ObjectKey &object);

}  // namespace warm
