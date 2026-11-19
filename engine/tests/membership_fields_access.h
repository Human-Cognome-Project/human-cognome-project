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
  static void kind(MembershipFields &f,GroupKind kind) { f.contributions_.front().kind=kind; }
  static void study(MembershipFields &f,const warm::Uuid &study) { f.study_=study; }
};
}
