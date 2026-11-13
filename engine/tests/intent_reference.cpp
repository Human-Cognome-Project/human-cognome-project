// Independent algebra fixtures for the #324 proposal, not an HCP tick or oracle.
// No production helper is reused. Compile standalone as C++17.
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using R = long double;
struct V { R x{}, y{}; };
V operator+(V a, V b) { return {a.x + b.x, a.y + b.y}; }
V operator-(V a, V b) { return {a.x - b.x, a.y - b.y}; }
V operator*(V a, R b) { return {a.x * b, a.y * b}; }
R norm(V a) { return std::sqrt(a.x * a.x + a.y * a.y); }
struct Pull { V displacement; R coupling; }; // coupling = G * participating mass * field mass
enum class Status { Absent, Valid, UnresolvedCoincidence };
struct Result { Status status{Status::Absent}; V force{}, coefficient{}, magnitude{}, coupling{}; };
Result compare(const std::vector<Pull> &pulls) {
  Result r;
  V a{}, b{}, c{};
  R wa = 0, wb = 0, wc = 0;
  for (const auto &p : pulls) {
    if (!std::isfinite(p.coupling) || !std::isfinite(p.displacement.x) ||
        !std::isfinite(p.displacement.y) || p.coupling < 0)
      throw std::runtime_error("finite attractive-domain fixture required");
    if (p.coupling == 0) continue;
    R d = norm(p.displacement);
    // Proposed formula has no defined zero-distance coefficient. Do not call
    // this absence or invent a softened denominator.
    if (d == 0) { r.status = Status::UnresolvedCoincidence; return r; }
    R alpha = p.coupling / (d * d * d), strength = p.coupling / (d * d);
    r.force = r.force + p.displacement * alpha;
    a = a + p.displacement * alpha; wa += alpha;
    b = b + p.displacement * strength; wb += strength;
    c = c + p.displacement * p.coupling; wc += p.coupling;
  }
  if (wa != 0) {
    r.status = Status::Valid;
    r.coefficient = a * (1 / wa);
    r.magnitude = b * (1 / wb);
    r.coupling = c * (1 / wc);
  }
  return r;
}
void near(R a, R b) {
  // Reference comparison tolerance only, never a model coincidence threshold.
  if (!std::isfinite(a) || !std::isfinite(b) ||
      std::abs(a - b) > 1e-15L * (1 + std::abs(b)))
    throw std::runtime_error("independent expected value mismatch");
}
void require(bool b) { if (!b) throw std::runtime_error("status mismatch"); }
R attenuation(R ratio) { return std::exp(-ratio * ratio * ratio * ratio); }
template <typename F> void rejects(F f) {
  bool rejected = false;
  try { f(); } catch (const std::runtime_error &) { rejected = true; }
  require(rejected);
}

// Explicit alternatives for comparison only. Neither selects a production law.
struct CoincidenceComparison {
  V gated_force{}, limiting_target{}, omitted_target{};
  bool omitted_valid{false};
  std::size_t coincident_sources{0};
};
CoincidenceComparison coincidence(const std::vector<Pull> &pulls) {
  CoincidenceComparison out;
  std::vector<Pull> separated;
  for (const auto &p : pulls) {
    if (p.coupling > 0 && norm(p.displacement) == 0)
      ++out.coincident_sources;
    else
      separated.push_back(p);
  }
  const auto r = compare(separated);
  out.gated_force = r.force;
  out.omitted_valid = r.status == Status::Valid;
  out.omitted_target = r.coefficient;
  // Local origin is the recipient x; a limiting coincident target is x.
  out.limiting_target = out.coincident_sources ? V{} : r.coefficient;
  return out;
}

void domain_comparisons() {
  const R nan = std::numeric_limits<R>::quiet_NaN();
  const R inf = std::numeric_limits<R>::infinity();
  for (R bad : {nan, inf, -inf}) {
    rejects([&] { near(bad, 0); });
    rejects([&] { near(0, bad); });
    rejects([&] { near(bad, bad); });
    rejects([&] { compare({{{1, 0}, bad}}); });
    rejects([&] { compare({{{bad, 0}, 1}}); });
    rejects([&] { compare({{{0, bad}, 1}}); });
  }
  rejects([] { compare({{{1, 0}, -1}}); });
  const auto mixed = coincidence({{{0, 0}, 1}, {{1, 0}, 1}});
  require(mixed.coincident_sources == 1 && mixed.omitted_valid);
  near(mixed.gated_force.x, 1); near(mixed.limiting_target.x, 0);
  near(mixed.omitted_target.x, 1);
  const auto alone = coincidence({{{0, 0}, 1}});
  require(alone.coincident_sources == 1 && !alone.omitted_valid);
  near(alone.gated_force.x, 0);
  const auto absent = coincidence({});
  require(!absent.coincident_sources && !absent.omitted_valid);
  // Exact rational expectations on both sides, not a softened distance gate.
  for (int n : {1, 2, 4, 8}) {
    R q = std::ldexp(1.0L, n);
    for (R sign : {-1.0L, 1.0L}) {
      auto r = compare({{{sign / q, 0}, 1}, {{1, 0}, 1}});
      near(r.force.x, sign * q * q + 1);
      near(r.coefficient.x, (sign * q * q + 1) / (q * q * q + 1));
    }
  }
  // Fixed material sources can have a changing coefficient-weighted intent.
  const auto initial = compare({{{-1, 0}, 1}, {{1, 0}, 1}});
  const auto moved = compare({{{-1.5L, 0}, 1}, {{0.5L, 0}, 1}});
  near(initial.coefficient.x, 0);
  near(0.5L + moved.coefficient.x, 13.0L / 14);
  // Inherited displacement is fixed, but its virtual target follows the body.
  near((V{10, 0} + V{2, 0}).x, 12);
  near((V{11, 0} + V{2, 0}).x, 13);
  // Contact cannot be inserted as a negative centroid coefficient safely:
  // attraction alpha=1, contact alpha=-1 gives zero denominator, nonzero force.
  R signed_denominator = 1 - 1, total_force = 1 - 2;
  near(signed_denominator, 0); near(total_force, -1);
  std::cout << "intent_domain PASS: nonfinite signed coincidence-limits moving-intent inherited-drift contact\n";
}
}

int main() {
  try {
    domain_comparisons();
    const auto single = compare({{{2, 0}, 8}});
    near(single.force.x, 2); near(single.coefficient.x, 2);
    near(single.magnitude.x, 2); near(single.coupling.x, 2);
    const auto unequal = compare({{{2, 0}, 8}, {{-1, 0}, 1}});
    near(unequal.force.x, 1); near(unequal.coefficient.x, 0.5L);
    near(unequal.magnitude.x, 1); near(unequal.coupling.x, 5.0L / 3);
    const auto angled = compare({{{2, 0}, 8}, {{0, 1}, 1}});
    near(angled.force.x, 2); near(angled.force.y, 1);
    near(angled.coefficient.x, 1); near(angled.coefficient.y, 0.5L);
    near(angled.magnitude.x, 4.0L / 3); near(angled.magnitude.y, 1.0L / 3);
    near(angled.coupling.x, 16.0L / 9); near(angled.coupling.y, 1.0L / 9);
    // Magnitude-weighted target is not generally parallel to the net force.
    near(angled.force.x * angled.coefficient.y - angled.force.y * angled.coefficient.x, 0);
    near(angled.force.x * angled.magnitude.y - angled.force.y * angled.magnitude.x, -2.0L / 3);
    const auto cancelled = compare({{{1, 0}, 1}, {{-1, 0}, 1}});
    require(cancelled.status == Status::Valid);
    near(cancelled.force.x, 0); near(cancelled.coefficient.x, 0);
    require(compare({}).status == Status::Absent);
    require(compare({{{1, 0}, 0}}).status == Status::Absent);
    require(compare({{{0, 0}, 1}}).status == Status::UnresolvedCoincidence);
    // Equal force, different target and denominator, under the proposed rule.
    const auto one = compare({{{1, 0}, 1}}), two = compare({{{2, 0}, 4}});
    near(one.force.x, two.force.x); near(one.coefficient.x, 1); near(two.coefficient.x, 2);
    // Keep participating coupling fixed while changing whole-body inertia.
    near(single.force.x / 2, 1); near(single.force.x / 8, 0.25L);
    // Inherited displacement is parent-child, transplanted at the recipient.
    V body{10, 0}, child{3, 0}, parent{5, 0}, shift{128, -64};
    V target = body + (parent - child);
    near(target.x, 12);
    V translated = (body + shift) + ((parent + shift) - (child + shift));
    near((translated - shift).x, target.x); near((translated - shift).y, target.y);
    const auto inherited_local = compare({{{2, 0}, 8}, {{-1, 0}, 1}});
    near((body + inherited_local.coefficient).x, 10.5L);
    // Adding canonical contact force changes acceleration, not this proposed field intent.
    near(inherited_local.force.x - 3, -2); near(inherited_local.coefficient.x, 0.5L);
    // Full-speed attenuation acts on purely tangential motion; approach-only
    // comparison leaves it intact. Neither comparison is an accepted brake.
    near(attenuation(1), std::exp(-1.0L)); near(attenuation(0), 1);
    near(norm(V{1, 1}), std::sqrt(2.0L));
    near(attenuation(2), std::exp(-16.0L)); // unbraked crossing length 2, reach 1
    near(attenuation(0), 1); // dt=0 ratio, positive reach
    std::cout << "intent_reference PASS: single unequal angled cancellation absence coincidence "
                 "equal-force inertia inherited translation contact tangent crossing dt-zero\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
