#include "values.h"
#include <iomanip>
#include <iostream>
#include <sstream>

int main() {
  using namespace hcp::resources;
  using warm::RecipeText;
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> watch=domain;
  RecipeText snapshot;
  {
    RecipeText empty{SharedReservationAllocator<char>(domain)};
    RecipeText short_text("q\"\\",SharedReservationAllocator<char>(domain));
    RecipeText long_text(131,'x',SharedReservationAllocator<char>(domain));
    if (warm::recipe_text_backing(empty) || warm::recipe_text_backing(short_text) ||
        warm::recipe_text_backing(long_text)!=long_text.capacity()+1 ||
        domain->result().live_bytes!=long_text.capacity()+1) return 1;
    RecipeText detached=long_text;
    if (detached.get_allocator().domain || detached!=long_text) return 2;
    snapshot=detached;
    RecipeText moved=std::move(long_text);
    if (moved.get_allocator().domain!=domain || moved!=snapshot) return 3;
    warm::Tool tool;
    tool.ref.object.address=*codec::decode_token_id("00.00");
    tool.recipe=short_text;
    const std::string prefix=warm::key(tool.ref.object)+" "+tool.epoch.str()+" ";
    for (int state=0; state<3; ++state) {
      if (!state) tool.notation.reset();
      else tool.notation=state==1 ? RecipeText{} : moved;
      std::ostringstream expected;
      expected<<prefix<<std::quoted(std::string(short_text))<<' '<<bool(state)<<' '
        <<std::quoted(state==2 ? std::string(snapshot) : std::string{})<<" 0 0\n";
      if (warm::tool_body(tool)!=expected.str()) return 4;
    }
    domain.reset();
    if (watch.expired()) return 5;
  }
  if (!watch.expired() || snapshot!=RecipeText(131,'x')) return 6;
  for (bool bytes : {true,false}) {
    auto denied=std::make_shared<ReservationDomain>(ReservationBudget{
        bytes ? 131u : UINT64_MAX, bytes ? UINT64_MAX : 131u});
    bool stopped=false;
    try { RecipeText value(131,'x',SharedReservationAllocator<char>(denied)); }
    catch (const ReservationStop &) { stopped=true; }
    if (!stopped || denied->result().live_bytes || denied->result().charged_work ||
        (bytes ? denied->result().required_bytes : denied->result().required_work)!=132) return 7;
  }
  auto exact=std::make_shared<ReservationDomain>(ReservationBudget{132,132});
  { RecipeText value(131,'x',SharedReservationAllocator<char>(exact));
    if (value!=snapshot || exact->result().live_bytes!=132) return 8; }
  if (exact->result().live_bytes) return 9;
  std::cout<<"recipe_text_test PASS\n";
}