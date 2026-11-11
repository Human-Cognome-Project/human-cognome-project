#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../codec/codec.h"
#include "../../../common/reservation_allocator.h"

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

using UuidText = std::basic_string<char,std::char_traits<char>,
    hcp::resources::SharedReservationAllocator<char>>;
class Uuid {
 public:
  Uuid();  // Shared scope / nil.
  explicit Uuid(std::string_view value);
  Uuid(const Uuid &other, const std::shared_ptr<hcp::resources::ReservationDomain> &domain);
  const UuidText &str() const { return value_; }
  bool nil() const;
  friend bool operator==(const Uuid &a, const Uuid &b) {
    return a.value_ == b.value_;
  }
  friend bool operator!=(const Uuid &a, const Uuid &b) { return !(a == b); }
  friend bool operator<(const Uuid &a, const Uuid &b) {
    return a.value_ < b.value_;
  }

 private:
  UuidText value_;
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

// Recipe table backing may attach to an explicit owner allocation domain.
// Ordinary copies detach through the engine-independent allocator.
template<class T> using RecipeTable = hcp::resources::SharedReservedVector<T>;
struct Parent {
  ToolRef target;
  std::uint32_t ordinal{0};
  std::optional<std::uint64_t> stored_mass;
  std::uint64_t contribution{0};
};
using RecipeText = std::basic_string<char,std::char_traits<char>,
    hcp::resources::SharedReservationAllocator<char>>;
// Inline characters are already part of the string/row object. Heap requests
// include the terminator. Detect inline backing by location, not a fixed SSO size.
inline std::size_t recipe_text_backing(const RecipeText &text) {
  const auto data = reinterpret_cast<std::uintptr_t>(text.data());
  const auto object = reinterpret_cast<std::uintptr_t>(&text);
  return data >= object && data-object < sizeof(text) ? 0 : text.capacity()+1;
}
struct Tool {
  ToolRef ref;
  Uuid epoch;
  RecipeText recipe;
  std::optional<RecipeText> notation;
  std::optional<std::uint64_t> mass;
  RecipeTable<Parent> parents;
  RecipeTable<ToolRef> member_of;
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
  RecipeTable<Sphere> spheres;
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
using OccurrencePath = hcp::resources::SharedReservedVector<std::uint32_t>;
struct OccurrenceId {
  std::uint64_t entry{0};
  OccurrencePath path;
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
  RecipeTable<OccurrenceId> parts;
};
struct Contact {
  OccurrenceId a, b;
};
struct Bundle {
  BundleRef ref;
  Uuid owner, creator;
  int recipe_version{1};
  RecipeTable<Tool> tools;
  RecipeTable<Geometry> geometry;
  RecipeTable<Node> nodes;
  RecipeTable<Occurrence> occurrences;
  RecipeTable<Selection> selections;
  RecipeTable<Contact> contacts;
  // Versioned alternatives: v1 uses occurrences, v2 uses these structural rows.
  // Mixing versions is rejected, rather than discarding supplied initial state.
  RecipeTable<StructuralOccurrence> structural_occurrences;
  RecipeTable<FocalField> focal_fields;
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
