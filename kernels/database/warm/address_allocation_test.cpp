#include "pg.h"
#include <iostream>

int main() {
  using namespace hcp::resources;
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> watch=domain;
  const auto parsed=codec::decode_token_id("00.9Z.Az.zz");
  if (!parsed) return 1;
  warm::ObjectKey attached{warm::Uuid(),codec::Address{parsed->begin(),parsed->end(),
      SharedReservationAllocator<codec::AddressElement>(domain)}};
  const auto bytes=domain->result().live_bytes;
  const warm::ObjectKey copy=attached;
  warm::ObjectKey moved=std::move(attached);
  if (copy.address.get_allocator().domain || moved.address.get_allocator().domain!=domain ||
      domain->result().live_bytes!=bytes || bytes!=4*sizeof(codec::AddressElement) ||
      !(copy==moved) || copy<moved || moved<copy) return 2;
  const auto params=warm::detail::params(copy);
  if (params.size()!=2 || params[0]!="00000000-0000-0000-0000-000000000000" ||
      params[1]!="{00,9Z,Az,zz}" || warm::address_text(copy.address)!="00.9Z.Az.zz") return 3;
  const codec::Address context(copy.address.begin(),copy.address.begin()+2);
  const auto delta=codec::compute_delta(context,copy.address);
  if (!delta || codec::encode_token_id(*delta)!="Az.zz" ||
      codec::reconstruct_address(context,*delta)!=copy.address) return 4;
  const auto partial=codec::decode_token_id("00.A*");
  if (!partial || !codec::is_valid_address(*partial) || codec::decode_token_id("A*.00") ||
      codec::decode_token_id("00..Az") || codec::is_valid_address(codec::Address{{62,0,false}})) return 5;
  bool rejected_empty=false, rejected_partial=false;
  try { warm::address_array(codec::Address{}); } catch (const warm::Error &) { rejected_empty=true; }
  try { warm::address_array(*partial); } catch (const warm::Error &) { rejected_partial=true; }
  if (!rejected_empty || !rejected_partial || codec::encode_token_id(codec::Address{})!="") return 6;
  warm::detail::Result result(PQmakeEmptyPGresult(nullptr,PGRES_TUPLES_OK),PQclear);
  char scope_name[]="scope", address_name[]="address";
  PGresAttDesc attrs[2]{}; attrs[0].name=scope_name; attrs[1].name=address_name;
  char scope[]="00000000-0000-0000-0000-000000000000", address[]="00.9Z.Az.zz";
  if (!result || !PQsetResultAttrs(result.get(),2,attrs) ||
      !PQsetvalue(result.get(),0,0,scope,sizeof(scope)-1) ||
      !PQsetvalue(result.get(),0,1,address,sizeof(address)-1)) return 7;
  const auto decoded=warm::detail::object(result,0,0);
  if (!(decoded==copy) || decoded.address.get_allocator().domain ||
      !(warm::ObjectKey{warm::Uuid(),*codec::decode_token_id("00.9Y")} < decoded) ||
      !(decoded < warm::ObjectKey{warm::Uuid(),*codec::decode_token_id("00.A0")})) return 8;
  domain.reset();
  if (watch.expired() || warm::address_array(moved.address)!="{00,9Z,Az,zz}") return 9;
  moved=warm::ObjectKey{};
  if (!watch.expired() || warm::address_text(copy.address)!="00.9Z.Az.zz") return 10;
  std::cout << "address_allocation_test: PASS\n";
}
