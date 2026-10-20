#pragma once

#include <cctype>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Command line of viewport_letter_capture. Pure parsing: no engine, no I/O.
namespace viewport {
struct UsageError : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct Options {
  std::string output;
  bool cuda{false};
  std::uint64_t ticks{8};
  double dt{0.1};
  int cpu_threads{1};
};
inline const char *usage() {
  return "usage: viewport_letter_capture OUTPUT_DIRECTORY [options]\n"
    "  --backend cpu|cuda  execution backend (default cpu; cuda fails if the Taichi build lacks it)\n"
    "  --ticks N           total completed ticks, N >= 1 (default 8)\n"
    "  --dt X              finite tick length > 0 (default 0.1)\n"
    "  --cpu-threads N     CPU worker threads, N >= 1 (default 1; used by the cpu backend)\n"
    "HCP_WARM_TEST_CONNINFO: fresh warm DB with schema.sql; "
    "HCP_CORE_TEST_CONNINFO/HCP_LING_TEST_CONNINFO: retained read-only sources\n";
}
// Whole-string unsigned decimal in [lo, hi]; no sign, space or suffix.
inline std::uint64_t parse_count(const std::string &name, const std::string &text,
                                 std::uint64_t lo, std::uint64_t hi) {
  std::uint64_t v = 0;
  const char *end = text.data() + text.size();
  const auto r = std::from_chars(text.data(), end, v);
  if (text.empty() || r.ec != std::errc() || r.ptr != end || v < lo || v > hi)
    throw UsageError("invalid value for " + name + ": '" + text + "'");
  return v;
}
// Plain decimal number (digits, '.', exponent; no sign, space, hex, inf or nan) that is
// representable as a normal positive engine float: FLT_MIN <= v <= FLT_MAX.
inline double parse_positive(const std::string &name, const std::string &text) {
  const UsageError bad("invalid value for " + name + ": '" + text + "'");
  if (text.empty() || !(std::isdigit(static_cast<unsigned char>(text[0])) || text[0] == '.')) throw bad;
  for (const char c : text)
    if (!std::isdigit(static_cast<unsigned char>(c)) && !std::strchr(".eE+-", c)) throw bad;
  char *end = nullptr;
  const double v = std::strtod(text.c_str(), &end);
  if (end != text.c_str() + text.size() || !std::isfinite(v) || v > FLT_MAX || v < FLT_MIN) throw bad;
  return v;
}
// args excludes the program name. Accepts "--name value" and "--name=value".
inline Options parse_options(const std::vector<std::string> &args) {
  if (args.empty() || args[0].empty() || args[0].rfind("--", 0) == 0)
    throw UsageError("missing OUTPUT_DIRECTORY");
  Options o;
  o.output = args[0];
  std::set<std::string> seen;
  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string name = args[i], value;
    bool inline_value = false;
    if (const auto eq = name.find('='); eq != std::string::npos && name.rfind("--", 0) == 0) {
      value = name.substr(eq + 1);
      name.resize(eq);
      inline_value = true;
    }
    if (name.rfind("--", 0) != 0) throw UsageError("unexpected argument: '" + name + "'");
    if (!inline_value) {
      if (i + 1 >= args.size()) throw UsageError("missing value for " + name);
      value = args[++i];
    }
    if (!seen.insert(name).second) throw UsageError("repeated option " + name);
    if (name == "--backend") {
      if (value != "cpu" && value != "cuda") throw UsageError("invalid value for --backend: '" + value + "'");
      o.cuda = value == "cuda";
    } else if (name == "--ticks") o.ticks = parse_count(name, value, 1, UINT64_MAX);
    else if (name == "--dt") o.dt = parse_positive(name, value);
    else if (name == "--cpu-threads") o.cpu_threads = int(parse_count(name, value, 1, 4096));
    else throw UsageError("unknown option: '" + name + "'");
  }
  return o;
}
}  // namespace viewport
