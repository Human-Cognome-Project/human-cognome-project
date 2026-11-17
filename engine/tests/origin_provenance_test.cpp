#include "prepared_owner_fixture.h"
#include "origin_reference.h"
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include <iostream>

namespace {
using namespace prepared_owner_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok) throw std::runtime_error(label);
  ++checks;
}
template<class F> void rejects(F action, const char *label) {
  bool refused = false;
  try { action(); } catch (const warm::Error &) { refused = true; }
  check(refused,label);
}
struct Cost {
  std::size_t rows{}, capacity{}, element_bytes{}, calls{}, requested{}, retained{}, peak{}, cleared{};
};
template<class F> Cost measure(F producer) {
  Cost out;
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try {
    {
      auto rows = producer();
      allocation_observation::enabled = false;
      out = {rows.size(),rows.capacity(),sizeof(rows[0]),allocation_observation::calls.load(),
             allocation_observation::bytes.load(),allocation_lifetime::counts.live,allocation_lifetime::counts.peak,0};
      rows.clear();
      out.cleared = allocation_lifetime::counts.live;
    }
    const auto released = allocation_lifetime::end();
    check(!released.live && !released.blocks && released.acquired == released.released &&
          released.acquired == out.calls && out.cleared == out.capacity*out.element_bytes,
          "producer rows release nested requests, then vector backing, completely");
  } catch (...) {
    allocation_observation::enabled = false;
    if (allocation_lifetime::active) allocation_lifetime::end();
    throw;
  }
  return out;
}
void compare(bool local) {
  Input fixture(true);
  const auto owner = fixture.source->owner();
  auto supplied = origin_reference::local_input(fixture.source->instance(),owner);
  auto source = ActiveAssembly::assemble(fixture.source->shared_recipe(),owner,local,
      local ? std::optional<LocalInput>(supplied) : std::nullopt);
  const auto seeds = source.origins();
  const auto legacy = origin_reference::produce(seeds);
  const auto before = measure([&] { return origin_reference::produce(seeds); });
  const auto after = measure([&] { return source.origins(); });
  check(before.rows == (local ? 5U : 4U) && after.rows == before.rows && before.capacity == after.capacity,
        "all hidden alternatives and repeated occurrences retain their order and count");
  check(sizeof(Origin) < sizeof(origin_reference::LegacyOrigin) && after.calls < before.calls &&
        after.requested < before.requested && after.retained < before.retained && after.peak < before.peak,
        "actual changed producer reduces calls, cumulative requests, retained bytes and scoped peak");
  for (std::size_t i=0; i<seeds.size(); ++i) {
    const auto converted = origin_reference::checked(legacy[i]);
    check(converted.occurrence == seeds[i].occurrence && converted.definition() == seeds[i].definition() &&
          converted.shared == seeds[i].shared && origin_reference::same_local(converted.local,seeds[i].local),
          "one full pin preserves exact occurrence, shared revision or local owner/version");
    const auto *key = seeds[i].shared ? &seeds[i].shared->object : &seeds[i].local->object;
    allocation_observation::calls = 0;
    allocation_observation::enabled = true;
    const auto *borrowed = &seeds[i].definition();
    allocation_observation::enabled = false;
    check(borrowed == key && !allocation_observation::calls,
          "checked definition accessor borrows the already owned pin without allocation");
  }
  const auto print = [&](const char *phase, const Cost &v) {
    std::cout << "origin_producer,local=" << local << ",phase=" << phase << ",rows=" << v.rows
              << ",calls=" << v.calls << ",requested=" << v.requested << ",retained=" << v.retained
              << ",peak=" << v.peak << ",element_bytes=" << v.element_bytes << ",capacity=" << v.capacity << '\n';
  };
  print("legacy",before); print("current",after);
}
void provenance() {
  Input fixture;
  auto shared = fixture.source->origins().front();
  const auto supplied = origin_reference::local_input(fixture.source->instance(),fixture.source->owner());
  Origin local{{fixture.source->study(),{42,{}}},{},supplied.values.front().ref};
  rejects([&] { auto bad = shared; bad.shared.reset(); bad.definition(); },"no provenance pin refused");
  rejects([&] { auto bad = shared; bad.local = local.local; bad.definition(); },"dual provenance pins refused");
  rejects([&] { auto bad = shared; bad.shared->revision = warm::Uuid(); bad.definition(); },"missing shared revision refused");
  rejects([&] { auto bad = shared; const auto moved = std::move(bad.shared->revision); (void)moved; bad.definition(); },"moved-out shared revision refused");
  rejects([&] { auto bad = shared; bad.shared->object.scope = warm::new_uuid(); bad.definition(); },"private scope cannot masquerade as a shared pin");
  rejects([&] { auto bad = local; bad.local->version = warm::Uuid(); bad.definition(); },"missing local version refused");
  rejects([&] { auto bad = local; const auto moved = std::move(bad.local->version); (void)moved; bad.definition(); },"moved-out local version refused");
  rejects([&] { auto bad = local; bad.local->owner = warm::Uuid(); bad.definition(); },"missing local owner refused");
  rejects([&] { auto bad = local; bad.local->object.scope = warm::Uuid(); bad.definition(); },"missing local instance scope refused");
  rejects([&] { auto bad = shared; bad.shared->object.address.pop_back(); bad.definition(); },"partial definition address refused");
  rejects([&] { auto bad = shared; bad.shared->object.address[0].partial = true; bad.definition(); },"partial address element refused");
  rejects([&] { auto bad = shared; bad.shared->object.address[0].first = 255; bad.definition(); },"invalid address element refused");
  auto old = origin_reference::produce({shared}).front();
  old.definition = supplied.values.front().ref.object;
  rejects([&] { origin_reference::checked(old); },"reference legacy boundary rejects a mismatched duplicate");
  std::vector<Origin> detached;
  std::vector<origin_reference::LegacyOrigin> expected;
  {
    auto source = ActiveAssembly::assemble(fixture.source->shared_recipe(),fixture.source->owner(),true,supplied);
    detached = source.origins(); expected = origin_reference::produce(detached);
  }
  for (std::size_t i=0; i<detached.size(); ++i)
    check(detached[i].definition() == expected[i].definition && detached[i].shared == expected[i].shared &&
          origin_reference::same_local(detached[i].local,expected[i].local),
          "detached origins retain their own exact pins after source assembly teardown");
}
}
int main() {
  try {
    provenance(); compare(false); compare(true);
    std::cout << "origin_provenance,checks=" << checks << " PASS\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
