#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "field/prepared_capacity.h"

namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <typename Exception, typename F>
void rejects(F action) {
  try {
    action();
  } catch (const Exception &) {
    return;
  }
  throw std::runtime_error("invalid capacity was accepted");
}
}  // namespace
int main() {
  try {
    using namespace field;
    BudgetDemand limits;
    limits.values = {1048576, 70, 9, 280, 140, 1024, 9, 32, 65536};
    const PreparedCapacity prepared(limits);
    require(!prepared.check(limits).exhausted(), "exact limits rejected");
    for (std::size_t i = 0; i < limits.values.size(); ++i) {
      BudgetDemand demand;
      demand.values[i] = limits.values[i] + 1;
      const auto failure = prepared.check(demand);
      require(failure.exhausted() &&
                  static_cast<std::size_t>(failure.budget) == i &&
                  failure.required == demand.values[i] &&
                  failure.capacity == limits.values[i],
              "independent exhausted budget misreported");
      require(std::string(budget_name(failure.budget)) != "none",
              "budget not named");
      require(prepared.limits().values == limits.values,
              "check changed limits");
    }
    BudgetDemand live;
    live.values = {100, 3, 2, 5, 1, 4, 1, 2, 64};
    require(!prepared.check(live).exhausted(), "small demand rejected");
    const auto shapes = prepared.numerical_shapes();
    require(shapes[0].rows == 11 && shapes[0].stride == 70 &&
                shapes[0].bytes == 3080,
            "particle stride depends on live count");
    require(shapes[1].stride == 9 && shapes[2].stride == 280 &&
                shapes[4].stride == 140,
            "independent capacities collapsed");
    std::size_t sum = 0;
    for (const auto &shape : shapes)
      sum += shape.bytes;
    require(sum == prepared.numerical_bytes(), "numerical byte sum incorrect");
    BudgetDemand zero;
    zero[Budget::Potential] = 1;
    const PreparedCapacity empty(zero);
    require(empty.numerical_shapes()[0].stride == 1,
            "empty native shape lacks sentinel");
    live = {};
    live[Budget::Particles] = 1;
    require(empty.check(live).budget == Budget::Particles,
            "sentinel admitted logical row");
    rejects<std::invalid_argument>([] { PreparedCapacity invalid({}); });
    auto invalid = limits;
    invalid[Budget::Particles] = limits[Budget::Potential] + 1;
    rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    invalid = limits;
    invalid[Budget::Focus] = 10;
    rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    invalid = limits;
    invalid[Budget::Participation] = (std::uint64_t{1} << 30) + 1;
    rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    invalid = limits;
    invalid[Budget::GeometryWork] = std::uint64_t{1} << 63;
    rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    invalid[Budget::GeometryWork] = std::numeric_limits<std::int64_t>::max();
    require(PreparedCapacity(invalid).limits()[Budget::GeometryWork] ==
                invalid[Budget::GeometryWork],
            "wide work limit narrowed to logical axis");
    for (const auto b :
         {Budget::CascadeNodes, Budget::CascadeLinks, Budget::CascadeReferences,
          Budget::CascadeLevels, Budget::CascadeCoverage}) {
      invalid = limits;
      invalid[b] = (std::uint64_t{1} << 30) + 1;
      rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    }
    invalid = limits;
    invalid[Budget::CascadeWork] = std::uint64_t{1} << 63;
    rejects<std::invalid_argument>([&] { PreparedCapacity bad(invalid); });
    invalid[Budget::CascadeWork] = std::numeric_limits<std::int64_t>::max();
    require(PreparedCapacity(invalid).limits()[Budget::CascadeWork] ==
                invalid[Budget::CascadeWork],
            "wide cascade work limit narrowed");
    invalid = limits;
    invalid[Budget::ScratchBytes] = std::numeric_limits<std::size_t>::max();
    rejects<std::overflow_error>([&] { PreparedCapacity bad(invalid); });
    auto large = limits;
    large[Budget::Potential] = large[Budget::Particles] = std::uint64_t{1}
                                                          << 30;
    require(PreparedCapacity(large).numerical_shapes()[0].bytes ==
                (std::uint64_t{44} << 30),
            "large byte product narrowed");
    std::cout << "prepared_capacity: independent limits, fixed strides, zero "
                 "limits and overflow PASS\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
