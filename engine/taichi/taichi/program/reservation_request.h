#pragma once
#include <cstdint>

namespace taichi::lang {
enum class ReservationStatus : int { Unavailable, Absent, Present, Invalid };
enum class ReservationKind : int {
  NodeManager, Ambient, AmbientMetadata, ListObject, ListPage, ListChunk
};
enum class ReservationList : int { Elements, Free, Recycled, Data };
constexpr std::uint64_t reservation_max_chunks = 128 * 1024;
constexpr std::uint64_t reservation_chunks_per_page = 512;
constexpr std::uint64_t reservation_max_pages =
    reservation_max_chunks / reservation_chunks_per_page;

// Synchronous native-owner request view, NOT a Device allocation token or a
// lifetime lease. Pool backing identity/rounded extents remain unresolved.
struct ReservationRequest {
  ReservationStatus status{ReservationStatus::Unavailable};
  std::uint64_t runtime_instance{0};
  std::uintptr_t address{0};
  std::uint64_t requested_bytes{0}, alignment{0};
  bool backing_identity_complete{false}, rounded_extent_available{false};
  std::uint64_t extent_bytes{0};
};
struct ReservationLimits {
  bool available{false};
  std::uint64_t node_capacity{0}, list_pages{0}, list_chunks{0};
  // One bounded lookup per query; enumeration is the caller's metered work.
  std::uint64_t query_work_units{0};
  // Added pointer fields, subset of enclosing runtime/executor object sizes.
  // AmbientMetadata exposes the separate table request once per runtime.
  std::uint64_t incremental_owner_bytes{0};
};
}  // namespace taichi::lang
