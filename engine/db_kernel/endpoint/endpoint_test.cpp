// Unit tests for endpoint::Registry. Standalone: standard library only,
// no test framework, no DB. Prints one line per check; the process exits
// non-zero if any check failed.
#include <cstdio>
#include <stdexcept>
#include <string>

#include "endpoint.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s\n", what.c_str());
    ++g_failures;
  }
}

void test_register_and_resolve() {
  endpoint::Registry registry(4, 4);
  box::Box b;
  endpoint::EndpointId id = registry.register_standing(2, &b);
  check(id.slot == 2, "register_standing returns the requested slot");
  check(id.generation == endpoint::Registry::kStandingGeneration,
        "register_standing's returned id carries the standing-generation sentinel");
  check(registry.resolve(id) == &b, "resolve on a standing id returns the registered box");
}

void test_allocate_recycle_round_trip() {
  endpoint::Registry registry(2, 2);
  endpoint::EndpointId a = registry.allocate();
  check(registry.is_ephemeral_slot(a.slot), "allocate() draws a slot from the ephemeral range");
  check(registry.resolve(a) != nullptr, "a freshly allocated endpoint resolves");

  registry.recycle(a);
  endpoint::EndpointId b = registry.allocate();
  check(b.slot == a.slot, "recycle returns the slot to the freelist for the next allocate()");
}

void test_stale_generation_resolve_fails() {
  endpoint::Registry registry(1, 1);
  endpoint::EndpointId a = registry.allocate();
  registry.recycle(a);
  check(registry.resolve(a) == nullptr,
        "resolving the pre-recycle id fails -- the one real local correctness point: a "
        "late/duplicate ack is dropped, never misdelivered");
}

void test_recycle_bumps_generation() {
  endpoint::Registry registry(1, 1);
  endpoint::EndpointId a = registry.allocate();
  registry.recycle(a);
  endpoint::EndpointId b = registry.allocate();

  check(b.generation == a.generation + 1, "recycle bumps the slot's generation by exactly one");
  check(registry.resolve(a) == nullptr, "the pre-recycle id no longer resolves after recycle");
  check(registry.resolve(b) != nullptr, "the post-recycle id resolves");
}

void test_allocate_does_not_itself_bump() {
  endpoint::Registry registry(1, 1);
  endpoint::EndpointId a = registry.allocate();
  check(registry.resolve(a) != nullptr,
        "an id fresh from allocate() resolves immediately -- allocate() never bumps generation "
        "(F2 -- recycle is the ONE bump-site)");
  uint32_t generation_before_recycle = a.generation;
  registry.recycle(a);
  endpoint::EndpointId b = registry.allocate();  // the bump already happened at recycle, not here
  check(b.generation == generation_before_recycle + 1,
        "the generation seen after re-allocating reflects exactly one bump (from recycle), "
        "not a second bump from this allocate() call");
}

void test_standing_resolves_regardless_of_generation() {
  endpoint::Registry registry(2, 2);
  box::Box b;
  endpoint::EndpointId id = registry.register_standing(0, &b);
  endpoint::EndpointId wrong_generation{id.slot, id.generation + 12345};
  check(registry.resolve(wrong_generation) == &b,
        "a standing slot resolves regardless of the generation carried in the id (F7)");
}

void test_allocate_throws_when_exhausted() {
  endpoint::Registry registry(1, 2);
  registry.allocate();
  registry.allocate();  // pool of 2 now fully drawn

  bool threw = false;
  try {
    registry.allocate();
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "allocate() throws once the ephemeral pool is exhausted -- the honest capacity "
               "ceiling, not silently reused or UB");
}

void test_recycle_rejects_non_ephemeral_slot() {
  endpoint::Registry registry(2, 2);
  box::Box b;
  endpoint::EndpointId standing_id = registry.register_standing(0, &b);

  bool threw_standing = false;
  try {
    registry.recycle(standing_id);
  } catch (const std::invalid_argument &) {
    threw_standing = true;
  }
  check(threw_standing,
        "recycle() rejects a standing slot -- invalid_argument, never silently accepted");

  bool threw_out_of_range = false;
  try {
    registry.recycle(endpoint::EndpointId{999, 0});  // outside both ranges
  } catch (const std::invalid_argument &) {
    threw_out_of_range = true;
  }
  check(threw_out_of_range, "recycle() rejects a slot outside both ranges -- invalid_argument");
}

void test_recycle_rejects_double_recycle() {
  endpoint::Registry registry(1, 1);
  endpoint::EndpointId a = registry.allocate();
  registry.recycle(a);  // first recycle: fine, bumps the generation

  bool threw = false;
  try {
    registry.recycle(a);  // same (now-stale) id recycled a second time
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  check(threw, "double-recycling the same id throws invalid_argument -- its generation was "
               "already bumped past what `a` carries");
}

void test_standing_and_ephemeral_ranges_disjoint() {
  endpoint::Registry registry(3, 3);
  for (uint32_t slot = 0; slot < 3; ++slot) {
    check(registry.is_standing_slot(slot) && !registry.is_ephemeral_slot(slot),
          "slot " + std::to_string(slot) + " is standing-only");
  }
  for (uint32_t slot = 3; slot < 6; ++slot) {
    check(!registry.is_standing_slot(slot) && registry.is_ephemeral_slot(slot),
          "slot " + std::to_string(slot) + " is ephemeral-only");
  }

  box::Box b;
  bool threw = false;
  try {
    registry.register_standing(3, &b);  // slot 3 is in the ephemeral range
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  check(threw,
        "register_standing rejects a slot outside the standing range -- allocate() can never "
        "hand out a slot register_standing could also claim (F3)");
}

}  // namespace

int main() {
  test_register_and_resolve();
  test_allocate_recycle_round_trip();
  test_stale_generation_resolve_fails();
  test_recycle_bumps_generation();
  test_allocate_does_not_itself_bump();
  test_allocate_throws_when_exhausted();
  test_recycle_rejects_non_ephemeral_slot();
  test_recycle_rejects_double_recycle();
  test_standing_resolves_regardless_of_generation();
  test_standing_and_ephemeral_ranges_disjoint();

  if (g_failures == 0) {
    std::printf("PASS endpoint_test\n");
    return 0;
  }
  std::printf("FAIL endpoint_test (%d failed)\n", g_failures);
  return 1;
}
