#pragma once
#include "assembly/membership_fields.h"
#include "assembly/preparation_work.h"

namespace assembly::testing {
struct MembershipFieldsAccess {
  static auto catalogue(const LetterProjection &p) { return MembershipFields::collect(p,true); }
  static auto bounded(const LetterProjection &p,bool alternatives,
      const std::shared_ptr<detail::ReservationDomain> &domain,detail::PreparationWork &work) {
    return MembershipFields::collect(p,alternatives,domain,&work);
  }
  static const auto &rows(const MembershipFields &f) { return f.contributions_; }
  static const auto &targets(const MembershipFields &f) { return f.targets_; }
  template<class Row> static const auto &target(const MembershipFields &f,const Row &row) { return f.target(row); }
  static void index(MembershipFields &f,std::uint32_t index) { f.contributions_.front().target_index=index; }
  static auto retain(MembershipFields &f,const warm::ToolRef &pin,detail::PreparationWork &work) {
    return f.retain_target(pin,work);
  }
  static auto copy(const MembershipFields &f,const std::shared_ptr<detail::ReservationDomain> &domain) {
    return MembershipFields(f,domain);
  }
  static const auto &recipe(const MembershipFields &f) { return f.recipe_; }
  static void kind(MembershipFields &f,GroupKind kind) { f.contributions_.front().kind=kind; }
  static void study(MembershipFields &f,const warm::Uuid &study) { f.study_=study; }
};
}
