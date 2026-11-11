#pragma once
#include "assembly/exchange_coordinator.h"
#include "assembly/adoption_input.h"

namespace assembly {
namespace testing { struct ExchangeOwnerAccess; }
namespace detail {
enum class PreparationPhase { Staging, Capture, Commands, Complete };
struct OwnerPreparationResult {
  PreparationStatus status{PreparationStatus::Ok};
  PreparationPhase phase{PreparationPhase::Staging};
  PreparationResult staging, capture, commands;
};
// Setup of the spare projection and both retained exposure pools only.
// Initial projection copying is covered by AdoptionInputBudget; catalogue,
// declarations, maps and other constructor work remain separate.
enum class ProjectionSetupPhase { Admission, SpareInspection, SpareCopy, CommittedParents, SpareParents, Complete };
struct ProjectionSetupResult {
  ProjectionSetupPhase phase{ProjectionSetupPhase::Admission};
  PreparationResult work;
};
// Host retained-capacity charge. Shared pointees are counted once; allocator
// control blocks, setup peak and borrowed execution are separate surfaces.
struct OwnerStorage {
  ProjectionStorage owner, projections, maps, binding, coordinator, declarations, memberships;
  std::uint64_t shared_allocations{};
  ProjectionStorage total() const {
    auto out = owner;
    for (const auto *part : {&projections, &maps, &binding, &coordinator,
                            &declarations, &memberships}) out.add(*part);
    return out;
  }
};
// Private owner of logical exposure and committed mappings on one borrowed
// prepared execution. The harness outlives this owner; all calls are exclusive.
class ExchangeOwner {
 private:
  friend struct assembly::testing::ExchangeOwnerAccess;
  static AdoptionInputResult input_admission(const LetterProjection &,
      const std::vector<OccurrenceBinding> &, const std::vector<ExchangeFieldBinding> &,
      AdoptionInputBudget);
  static ReservedUnique<ExchangeOwner> adopt(field::Harness &, const LetterProjection &,
      const std::vector<OccurrenceBinding> &, const std::vector<ExchangeFieldBinding> &,
      AdoptionInputBudget, AdoptionInputResult &, ReservationBudget, ReservationResult &,
      std::uint64_t projection_setup_work, ProjectionSetupResult &);
  ExchangeOwner(field::Harness &, const LetterProjection &, const std::vector<OccurrenceBinding> &,
                const std::vector<ExchangeFieldBinding> &, const std::shared_ptr<ReservationDomain> &,
                PreparationWork &, ProjectionSetupResult &);
  OwnerPreparationResult prepare(const OccurrenceKey &, warm::Exposure,
               const std::vector<ExchangeIncoming> &, const std::vector<ExchangeNewField> &,
               std::uint64_t staging_work, std::uint64_t capture_work, std::uint64_t command_work);
  HierarchySealResult seal(const field::LiveRefreshSpec &, HierarchySealBudget);
  void prepare_projection(const OccurrenceKey &, warm::Exposure, PreparationWork &);
  std::shared_ptr<const LetterExchangeContract> declaration(const OccurrenceKey &, warm::Exposure, PreparationWork &) const;
  ProjectionStorage projection_storage() const;
  OwnerStorage storage() const;
  field::UpdateResult execute(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);
  field::Harness &h_;
  // Declared before the buffers: destroyed after every attached allocator.
  std::shared_ptr<ReservationDomain> reservations_;
  ReservedUnique<LetterProjection> projection_, prospective_;
  ExchangeMap<OccurrenceBinding> occurrences_;
  ExchangeMap<ExchangeFieldBinding> fields_;
  ReservedUnique<ExchangeCoordinator> plan_;
  std::vector<std::shared_ptr<const LetterExchangeContract>,
              SharedReservationAllocator<std::shared_ptr<const LetterExchangeContract>>> declarations_;
  std::shared_ptr<const MembershipFields> memberships_;
  std::uint64_t generation_{};
  std::array<double,8> outgoing_report_{};
  std::array<double,32> balance_report_{};
  bool completed_{};
};
}
}
