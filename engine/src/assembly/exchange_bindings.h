#pragma once

#include "assembly/exchange_contract.h"
#include "assembly/capture_work.h"
#include "assembly/exchange_map.h"
#include "assembly/membership_fields.h"
#include <memory>
#include "field/field.h"
#include "field/prepared_storage.h"

namespace assembly {
namespace testing { struct ExchangeBindingAccess; struct ExchangeCoordinatorAccess; }
namespace detail {
class ExchangeCoordinator;
class ExchangeOwner;
struct OccurrenceBinding {
  OccurrenceKey occurrence;
  engine::RecordHandle handle;
};
struct ExchangeFieldBinding {
  FieldKey field;
  engine::RecordHandle handle;
};
inline auto &key_buffer(OccurrenceBinding &v) { return v.occurrence.source.path; }
inline const auto &key_buffer(const OccurrenceBinding &v) { return v.occurrence.source.path; }
inline auto &key_buffer(ExchangeFieldBinding &v) { return v.field.object.address; }
inline const auto &key_buffer(const ExchangeFieldBinding &v) { return v.field.object.address; }
inline void set_key(OccurrenceBinding &v, const OccurrenceKey &k) { v.occurrence = k; }
inline void set_key(ExchangeFieldBinding &v, const FieldKey &k) { v.field = k; }
// Setup-only reservation from the pinned universe, including inactive alternatives.
inline std::size_t exchange_map_capacity(std::size_t live, std::size_t extra) {
  if (extra > SIZE_MAX-live) throw warm::Error("exchange map reservation overflow");
  return live+extra;
}
inline void reserve_exchange_maps(const LetterProjection &p, std::size_t particles,
    std::size_t fields, std::size_t batch, ExchangeMap<OccurrenceBinding> &rows,
    ExchangeMap<ExchangeFieldBinding> &groups,
    const ExchangeMap<ExchangeFieldBinding> *trusted = nullptr) {
  std::size_t path = 0, address = 0;
  for (const auto &o : p.occurrences()) path = std::max(path,o.key.source.path.size());
  for (const auto &t : p.recipe().tools) address = std::max(address,t.ref.object.address.size());
  for (const auto &g : groups) address = std::max(address,g.field.object.address.size());
  if (trusted) for (const auto &g : *trusted)
    address = std::max(address,g.field.object.address.size());
  // Candidate maps must describe over-capacity requests so ordinary native
  // preflight can return CapacityExhausted without changing committed state.
  rows.reserve(exchange_map_capacity(particles,batch),path);
  groups.reserve(exchange_map_capacity(fields,std::min(fields,batch)),address);
}
// Private preparation boundary. Initial identity bindings must come from the
// trusted owner's committed admission results, never an exchange request.
// Harness lifetime and exclusive ownership encompass this generation-bound view.
class ExchangeBindings {
 private:
  friend class ExchangeCoordinator;
  friend class ExchangeOwner;
  friend struct assembly::testing::ExchangeBindingAccess;
  friend struct assembly::testing::ExchangeCoordinatorAccess;
  explicit ExchangeBindings(field::Harness &);
  CaptureResult prepare(const LetterProjection &, std::shared_ptr<const LetterExchangeContract>,
               std::shared_ptr<const MembershipFields>,
               const ExchangeMap<OccurrenceBinding> &, const ExchangeMap<ExchangeFieldBinding> &,
               std::uint64_t capture_work);
  void stage(std::shared_ptr<const LetterExchangeContract>, std::shared_ptr<const MembershipFields>,
             const ExchangeMap<OccurrenceBinding> &, const ExchangeMap<ExchangeFieldBinding> &);
  CaptureResult capture(const LetterProjection &, std::uint64_t capture_work);
  ExchangeBindings(field::Harness &, const LetterProjection &, LetterExchangeContract,
                   std::vector<OccurrenceBinding>, std::vector<ExchangeFieldBinding>);
  ExchangeBindings(field::Harness &, const LetterProjection &,
                   std::shared_ptr<const LetterExchangeContract>,
                   std::shared_ptr<const MembershipFields>,
                   std::vector<OccurrenceBinding>, std::vector<ExchangeFieldBinding>);
  field::UpdateResult validate(std::uint64_t generation, std::uint64_t work) const;
  struct Edge {
    std::size_t row;
    field::detail::RelationshipHeader header;
    std::uint64_t count;
  };
  field::Harness &h_;
  std::shared_ptr<const LetterExchangeContract> contract_;
  std::shared_ptr<const MembershipFields> memberships_;
  ExchangeMap<OccurrenceBinding> occurrences_;
  ExchangeMap<ExchangeFieldBinding> fields_;
  std::vector<std::uint64_t> masses_;
  std::vector<engine::RecordHandle> outgoing_;
  std::vector<Edge> edges_;
  std::vector<bool> outgoing_rows_, edge_rows_;
  std::uint64_t generation_{}, work_{};
  std::size_t source_edges_{};
  bool valid_{};
};
}  // namespace detail
}  // namespace assembly
