#pragma once

#include <cstdio>
#include <string>

// A test is a program that prints one line per check and exits non-zero if
// any check failed. No test framework: the engine is the only dependency.
namespace harness {

inline int &failures() {
  static int count = 0;
  return count;
}

inline void record(bool ok, const std::string &what, const std::string &detail) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s  %s\n", what.c_str(), detail.c_str());
    ++failures();
  }
  std::fflush(stdout);
}

inline void note(const std::string &what) {
  std::printf("--   %s\n", what.c_str());
  std::fflush(stdout);
}

inline int report(const char *name) {
  if (failures() == 0) {
    std::printf("PASS %s\n", name);
    return 0;
  }
  std::printf("FAIL %s (%d failed)\n", name, failures());
  return 1;
}

}  // namespace harness

#define CHECK_TRUE(cond, what) harness::record((cond), (what), "expected true")

#define CHECK_EQ_INT(actual, expected, what)                        \
  harness::record((actual) == (expected), (what),                   \
                  "got " + std::to_string(actual) + ", expected " + \
                      std::to_string(expected))
