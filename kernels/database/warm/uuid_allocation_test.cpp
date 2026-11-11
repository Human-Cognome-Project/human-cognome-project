#include "pg.h"
#include <iostream>

int main() {
  using namespace hcp::resources;
  using warm::Uuid;
  const Uuid normalized("ABCDEF01-2345-6789-ABCD-EF0123456789"), nil;
  if (normalized.str()!="abcdef01-2345-6789-abcd-ef0123456789" || normalized.nil() ||
      !nil.nil() || !(nil<normalized) || normalized<nil ||
      normalized!=Uuid("abcdef01-2345-6789-abcd-ef0123456789")) return 1;
  for (const auto bad : {"", "abcdef01_2345-6789-abcd-ef0123456789",
       "gbcdef01-2345-6789-abcd-ef0123456789", "abcdef01-2345-6789-abcd-ef01234567890"}) {
    bool rejected=false;
    try { Uuid value(bad); } catch (const warm::Error &) { rejected=true; }
    if (!rejected) return 2;
  }
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{74,74});
  std::weak_ptr<ReservationDomain> watch=domain;
  std::optional<Uuid> snapshot, moved;
  {
    Uuid attached(normalized,domain), attached_nil(domain);
    if (domain->result().live_bytes!=74 || attached.str().capacity()+1!=37 ||
        attached.str().get_allocator().domain!=domain || !attached_nil.nil()) return 3;
    snapshot.emplace(attached);
    moved.emplace(std::move(attached));
    if (snapshot->str().get_allocator().domain || *snapshot!=normalized ||
        moved->str().get_allocator().domain!=domain) return 4;
    const auto params=warm::detail::params(warm::BundleRef{*moved,attached_nil});
    if (params.size()!=2 || params[0]!=std::string(normalized.str()) ||
        params[1]!="00000000-0000-0000-0000-000000000000") return 5;
    domain.reset();
  }
  if (watch.expired() || *moved!=normalized) return 6;
  moved.reset();
  if (!watch.expired() || *snapshot!=normalized) return 7;
  for (bool bytes : {true,false}) {
    auto denied=std::make_shared<ReservationDomain>(ReservationBudget{
        bytes ? 36u : UINT64_MAX, bytes ? UINT64_MAX : 36u});
    bool stopped=false;
    try { Uuid value(normalized,denied); } catch (const ReservationStop &) { stopped=true; }
    bool nil_stopped=false;
    try { Uuid value(denied); } catch (const ReservationStop &) { nil_stopped=true; }
    if (!nil_stopped) return 11;
    if (!stopped || denied->result().live_bytes || denied->result().charged_work ||
        (bytes ? denied->result().required_bytes : denied->result().required_work)!=37) return 8;
  }
  auto exact=std::make_shared<ReservationDomain>(ReservationBudget{37,37});
  { Uuid value(exact); if (!value.nil() || exact->result().live_bytes!=37) return 9; }
  if (exact->result().live_bytes) return 10;
  std::cout<<"uuid_allocation_test PASS\n";
}