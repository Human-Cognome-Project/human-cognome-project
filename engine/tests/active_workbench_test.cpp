#include "active_workbench_checks.h"
#include "membership_fixture.h"

int main(int argc, char **argv) {
  try {
    const bool cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    membership_fixture::Fixture f;
    auto a = f.request(f.english, 0, warm::Exposure::Aggregate);
    auto b = f.request(f.english, 1, warm::Exposure::Parts);
    a.aggregate = {{-4, 0, 0}, {0.25, 0, 0}};
    b.bytes.at(0).initial = {{4, 0, 0}, {-0.5, 0, 0}};
    f.base.contacts = {{{0, {}}, {1, {0, 0}}}};
    active_checks::run(assembly::normalize_letters(f.base, {a, b}), cuda);
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << active_checks::checks << " active workbench checks\n";
  return harness::report("active_workbench_test");
}
