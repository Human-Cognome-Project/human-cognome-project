// Independent algebra fixtures for the #324 proposal, not an HCP tick or oracle.
// No production helper is reused. Compile standalone as C++17.
#include <cmath>
#include <iostream>
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
    if (p.coupling < 0) throw std::runtime_error("attractive-domain fixture required");
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
  if (std::abs(a - b) > 1e-15L * (1 + std::abs(b)))
    throw std::runtime_error("independent expected value mismatch");
}
void require(bool b) { if (!b) throw std::runtime_error("status mismatch"); }
R attenuation(R ratio) { return std::exp(-ratio * ratio * ratio * ratio); }
}

int main() {
  try {
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
