#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "engine/runtime.h"

namespace engine {

enum class RecordStatus {
  Ok,
  NotFound,
  AlreadyDefined,
  CapacityExhausted,
  InvalidInput,
  Stale,
  WrongContext,
  Unready
};

struct RecordHandle {
  std::uint64_t context{0}, incarnation{0};
  int address{-1}, record{-1};
};
struct RecordResult {
  RecordStatus status{RecordStatus::InvalidInput};
  RecordHandle handle;
  int kind{0};
  std::uint64_t lookup_entries{0};
};

// Native metadata for trusted kernels, scoped to this map/runtime. Read-only
// use: mutation goes through the map boundary. Presence is not an awake bit.
struct RecordBindings {
  taichi::lang::SNode *presence{}, *address{}, *incarnation{}, *declaration{};
  taichi::lang::SNode *allocator{};  // Diagnostics for the native NodeManager.
  int potential_capacity{0}, record_capacity{0};
};

// Lightweight potential bytes + independently bounded native record references.
// Owns no physics state or scheduling policy. Calls require exclusive ownership
// of an engine boundary; they synchronize, and may not overlap another call or
// a kernel reading these bindings. Runtime must outlive this object/its
// kernels. Layout construction/destruction are resource operations, never
// stream updates.
class PreparedRecordMap {
 public:
  PreparedRecordMap(Runtime &runtime,
                    int potential_capacity,
                    int record_capacity);
  ~PreparedRecordMap();
  PreparedRecordMap(const PreparedRecordMap &) = delete;
  PreparedRecordMap &operator=(const PreparedRecordMap &) = delete;

  RecordResult define(int address, int kind);
  RecordResult lookup(int address);
  RecordResult inspect(const RecordHandle &handle);
  RecordResult set_kind(const RecordHandle &handle, int kind);
  RecordResult release(const RecordHandle &handle);
  const RecordBindings &bindings() const;
  std::size_t size() const;
  std::uint64_t generation() const;
  bool ready() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace engine
