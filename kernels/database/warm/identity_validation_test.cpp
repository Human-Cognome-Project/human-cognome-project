#include "values.h"
#include "../../../engine/tests/allocation_observation.h"
#include <iostream>

namespace {
void check(bool ok) { if (!ok) throw std::runtime_error("identity validation check failed"); }
template<class F> void no_new(F f, bool expected) {
  allocation_observation::calls=0;
  allocation_observation::enabled=true;
  const bool result=f();
  allocation_observation::enabled=false;
  check(result==expected && !allocation_observation::calls);
}
void nil_checks() {
  const warm::Uuid nil;
  no_new([&]{return nil.nil();},true);
  const std::string zero(nil.str());
  for (std::size_t i=0;i<zero.size();++i) if (zero[i]!='-') {
    auto text=zero; text[i]='A';
    const warm::Uuid value(text);
    no_new([&]{return value.nil();},value==nil);
  }
  using namespace hcp::resources;
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{37,37});
  const warm::Uuid attached(domain);
  no_new([&]{return attached.nil();},true);
  check(domain->result().live_bytes==37 && domain->result().charged_work==37);
}
void containment_checks() {
  using namespace warm;
  std::vector<ObjectKey> keys;
  for (const auto scope : {"00000000-0000-0000-0000-000000000000",
                          "ABCDEF01-2345-6789-ABCD-EF0123456789"})
    for (int n : {1,2,5,128}) for (int code : {0,9,10,35,36,61,62,3843})
      keys.push_back({Uuid(scope),codec::Address(n,codec::make_full_element(code))});
  // A shared first couplet with a different second element exercises near misses.
  auto near=keys.front(); near.address.push_back(codec::make_full_element(1));
  keys.push_back(std::move(near));
  for (const auto &prefix : keys) for (const auto &object : keys) {
    const auto p=key(prefix), o=key(object);
    const bool expected=p==o || (o.size()>p.size() && o.compare(0,p.size(),p)==0 && o[p.size()]=='.');
    no_new([&]{return contains(prefix,object);},expected);
  }
  const std::vector<codec::Address> invalid={ {}, {{62,0,false}}, {{0,62,false}},
      {{0,0,true}}, {{0,0,true},{0,0,false}}, {{0,0,false},{0,0,true}} };
  for (const auto &address : invalid) for (const auto &valid : keys) {
    const ObjectKey bad{Uuid{},address};
    for (bool reverse : {false,true}) {
      bool rejected=false;
      try { (void)(reverse ? contains(valid,bad) : contains(bad,valid)); }
      catch (const Error &e) {
        rejected=std::string_view(e.what())=="invalid, empty or wildcard object address";
      }
      check(rejected);
    }
  }
}
}
int main(int argc,char **argv) {
  if (argc==1 || std::string_view(argv[1])=="nil") nil_checks();
  if (argc==1 || std::string_view(argv[1])=="contains") containment_checks();
  std::cout << "identity_validation_test: nil, containment rendering oracle, invalid operands, zero allocations PASS\n";
  return 0;
}
