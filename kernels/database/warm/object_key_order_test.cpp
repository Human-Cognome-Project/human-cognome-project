#include "values.h"
#include "../../../engine/tests/allocation_observation.h"
#include <iostream>

namespace {
void check(bool ok) { if (!ok) throw std::runtime_error("ObjectKey ordering check failed"); }
void pair(const warm::ObjectKey &a, const warm::ObjectKey &b) {
  const bool expected = warm::key(a) < warm::key(b);
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const bool actual = a < b;
  allocation_observation::enabled = false;
  check(actual == expected && !allocation_observation::calls);
}
}
int main() {
  using namespace warm;
  ObjectKey a{Uuid{}, {codec::make_full_element(0)}}, b=a;
  // Every alphabet transition in either couplet position, including 9/A/Z/a.
  for (int i=0; i<codec::kCoupletSpace; ++i) {
    a.address[0]=codec::make_full_element(i);
    pair(a,a);
    if (i+1<codec::kCoupletSpace) {
      b.address[0]=codec::make_full_element(i+1);
      pair(a,b); pair(b,a);
    }
  }
  std::vector<ObjectKey> keys;
  for (const auto scope : {"00000000-0000-0000-0000-000000000000",
                          "ABCDEF01-2345-6789-ABCD-EF0123456789",
                          "ffffffff-ffff-ffff-ffff-ffffffffffff"})
    for (int n : {1,2,5,128})
      for (int code : {0,9,10,35,36,61,62,3843}) {
        ObjectKey k{Uuid(scope),codec::Address(n,codec::make_full_element(code))};
        keys.push_back(std::move(k));
      }
  for (const auto &x : keys) for (const auto &y : keys) {
    pair(x,y);
    check((!(x<y) && !(y<x)) == (x==y));
  }
  // Corrupt either operand, including when scope alone decides ordering.
  const std::vector<codec::Address> invalid={ {}, {{62,0,false}},
      {{0,62,false}}, {{0,0,true}}, {{0,0,true},{0,0,false}},
      {{0,0,false},{0,0,true}} };
  for (const auto &address : invalid) for (const auto &valid : keys) {
    ObjectKey bad{Uuid{},address};
    for (bool reverse : {false,true}) {
      bool rejected=false;
      try { (void)(reverse ? valid<bad : bad<valid); }
      catch (const Error &e) {
        rejected=std::string_view(e.what())=="invalid, empty or wildcard object address";
      }
      check(rejected);
    }
  }
  std::cout << "object_key_order_test: rendering oracle, alphabet, scopes, prefixes, long addresses, invalid operands, zero allocations PASS\n";
}
