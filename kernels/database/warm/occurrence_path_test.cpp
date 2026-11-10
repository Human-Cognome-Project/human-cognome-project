#include "pg.h"
#include <iostream>

int main() {
  using namespace hcp::resources;
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  warm::OccurrenceId attached{17, warm::OccurrencePath{SharedReservationAllocator<std::uint32_t>(domain)}};
  attached.path={0,7,UINT32_MAX};
  const auto bytes=domain->result().live_bytes;
  const warm::OccurrenceId copy=attached;
  warm::OccurrenceId moved=std::move(attached);
  if (copy.path.get_allocator().domain || moved.path.get_allocator().domain != domain ||
      domain->result().live_bytes != bytes || !(copy == moved) || copy < moved || moved < copy)
    return 1;
  warm::detail::Params params;
  warm::detail::append(params,copy);
  if (params.size()!=2 || params[0]!="17" || params[1]!="{0,7,4294967295}" ||
      warm::detail::path_array(warm::OccurrencePath{})!="{}" ||
      warm::detail::path_array(std::vector<std::uint32_t>{0,7,UINT32_MAX})!=params[1]) return 2;
  // Construct a libpq result locally: the production reader must decode the
  // same ordinals without a running server or any persistent writes.
  warm::detail::Result result(PQmakeEmptyPGresult(nullptr,PGRES_TUPLES_OK),PQclear);
  char entry_name[]="entry", path_name[]="path";
  PGresAttDesc attrs[2]{};
  attrs[0].name=entry_name; attrs[1].name=path_name;
  char entry[]="17", path[]="0,7,4294967295";
  if (!result || !PQsetResultAttrs(result.get(),2,attrs) ||
      !PQsetvalue(result.get(),0,0,entry,2) || !PQsetvalue(result.get(),0,1,path,14)) return 3;
  const auto decoded=warm::detail::occurrence_id(result,0,0);
  if (!(decoded==copy) || decoded.path.get_allocator().domain ||
      !(warm::OccurrenceId{17,{0,6}} < decoded) || !(decoded < warm::OccurrenceId{18,{}})) return 4;
  moved=warm::OccurrenceId{};
  if (domain->result().live_bytes) return 5;
  std::cout << "occurrence_path_test: PASS\n";
}
