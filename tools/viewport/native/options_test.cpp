#include <iostream>

#include "capture_options.h"

namespace {
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  if (!ok) throw std::runtime_error(label);
}
using Args = std::vector<std::string>;
bool rejected(const Args &a) {
  try { viewport::parse_options(a); } catch (const viewport::UsageError &) { return true; }
  return false;
}
}
int main() {
  try {
    const auto d = viewport::parse_options({"out"});
    check(d.output == "out" && !d.cuda && d.ticks == 8 && d.dt == 0.1 && d.cpu_threads == 1, "defaults reproduce the fixed capture");
    const auto o = viewport::parse_options({"out", "--backend", "cuda", "--ticks", "200", "--dt=0.25", "--cpu-threads", "4"});
    check(o.cuda && o.ticks == 200 && o.dt == 0.25 && o.cpu_threads == 4, "all options");
    check(!viewport::parse_options({"out", "--backend=cpu"}).cuda, "explicit cpu");
    check(viewport::parse_options({"out", "--ticks", "18446744073709551615"}).ticks == UINT64_MAX, "large ticks");
    check(rejected({}) && rejected({"--ticks", "3"}) && rejected({""}), "output directory required first");
    check(rejected({"out", "extra"}) && rejected({"out", "--nope", "1"}) && rejected({"out", "-x"}), "unknown or positional");
    check(rejected({"out", "--backend", "gpu"}) && rejected({"out", "--backend", "CUDA"}) && rejected({"out", "--backend"}), "backend values");
    for (const char *bad : {"0", "-1", "+3", "1.5", "", " 2", "3x", "abc", "18446744073709551616"})
      check(rejected({"out", "--ticks", bad}), "invalid ticks");
    for (const char *bad : {"0", "-0.1", "nan", "inf", "-inf", "", "0.1x", "x", "1e-60", "1e60", "1e400", " 0.1", "+0.1", "0x1p-3", "1e-40", "0.1 ", "3.5e38", "e5", ".", "NaN"})
      check(rejected({"out", "--dt", bad}), "invalid dt");
    check(viewport::parse_options({"out", "--dt", "1e-3"}).dt == 1e-3 && viewport::parse_options({"out", "--dt", ".5"}).dt == 0.5 && viewport::parse_options({"out", "--dt", "3.4e38"}).dt == 3.4e38, "exponent and leading-point dt");
    for (const char *bad : {"0", "-2", "1.0", "5000", "x"})
      check(rejected({"out", "--cpu-threads", bad}), "invalid cpu threads");
    check(d.every == 0 && d.pace_ms == 0, "live options default off");
    const auto l = viewport::parse_options({"out", "--every=10", "--pace-ms", "250"});
    check(l.every == 10 && l.pace_ms == 250, "live options");
    check(viewport::parse_options({"out", "--pace-ms", "0"}).pace_ms == 0, "zero pace");
    for (const char *bad : {"0", "-1", "1.5", "", "x"})
      check(rejected({"out", "--every", bad}), "invalid every");
    for (const char *bad : {"-1", "1.5", "", "x", "86400001"})
      check(rejected({"out", "--pace-ms", bad}), "invalid pace");
    auto schedule = [](const viewport::Options &s) {
      std::vector<std::uint64_t> at;
      for (std::uint64_t t = 0; t <= s.ticks; ++t) if (viewport::captures_at(t, s)) at.push_back(t);
      return at;
    };
    check(schedule(d) == std::vector<std::uint64_t>({0, 8}), "default schedule: tick 0 and final");
    auto s = d;
    s.every = 3;
    check(schedule(s) == std::vector<std::uint64_t>({0, 3, 6, 8}), "every K plus final");
    s.every = 4;
    check(schedule(s) == std::vector<std::uint64_t>({0, 4, 8}), "final on a multiple is not repeated");
    s.every = 100;
    check(schedule(s) == std::vector<std::uint64_t>({0, 8}), "every beyond ticks");
    check(rejected({"out", "--ticks", "2", "--ticks", "3"}), "repeated option");
    check(rejected({"out", "--ticks"}), "missing value");
    std::cout << checks << " viewport option checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "viewport options test: " << e.what() << '\n';
    return 1;
  }
}
