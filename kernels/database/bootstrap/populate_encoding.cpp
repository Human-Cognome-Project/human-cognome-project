// Bootstrap driver: populates the multi-byte character encoding tables.
//
//   populate_encoding <conninfo> sample   -- U+00E9, U+20AC, U+1F600 only
//   populate_encoding <conninfo> full     -- all of U+0080..U+10FFFF
//
//   populate_encoding <conninfo> endpoints-label   -- create the UTF-8 label only
//   populate_encoding <conninfo> endpoints-sample  -- endpoints for U+00E9, U+20AC, U+1F600
//   populate_encoding <conninfo> endpoints         -- endpoints for all of U+0080..U+10FFFF
//
// The endpoint modes need the UTF-8 label (endpoints-label first) and the
// byte-couplet tier (sample/full) already in place; they create neither.
//
// Additive and idempotent: category labels are created via SEE, characters
// are fixed-address mints, membership adds are conflict-safe. Nothing is
// ever deleted or reset. Progress goes to stderr, sparsely. An existing
// endpoint is verified whole on a re-run and rejected, never healed.
#include <cstdio>
#include <cstring>
#include <exception>

#include "encoding_populate.h"

namespace {

bool is_mode(const char *arg, const char *mode) { return std::strcmp(arg, mode) == 0; }

int run_endpoints(const char *conninfo, const char *mode) {
  dbk::Controller ctl(conninfo);
  dbk::bootstrap::verify_floor(ctl);
  if (is_mode(mode, "endpoints-label")) {
    dbk::bootstrap::create_utf8_label(ctl);
    std::fprintf(stderr, "UTF-8 label present\n");
    return 0;
  }
  dbk::bootstrap::verify_utf8_label(ctl);
  long minted = 0, existing = 0;
  auto one = [&](uint32_t cp) {
    (dbk::bootstrap::populate_endpoint(ctl, cp) ? minted : existing)++;
  };
  if (is_mode(mode, "endpoints-sample")) {
    dbk::bootstrap::verify_combinations(ctl, dbk::bootstrap::sample_codepoints());
    for (uint32_t cp : dbk::bootstrap::sample_codepoints()) {
      one(cp);
    }
  } else {
    for (uint32_t cp = 0x80; cp <= 0x10FFFF; ++cp) {
      if (!dbk::bootstrap::is_populated_codepoint(cp)) {
        continue;
      }
      one(cp);
      if (cp % 0x10000 == 0) {
        std::fprintf(stderr, "U+%06X\n", cp);
      }
    }
  }
  std::fprintf(stderr, "done: %ld minted, %ld already present\n", minted, existing);
  return 0;
}

}  // namespace

int main(int argc, char **argv) {
  const bool endpoint_mode =
      argc == 3 && (is_mode(argv[2], "endpoints-label") || is_mode(argv[2], "endpoints-sample") ||
                    is_mode(argv[2], "endpoints"));
  if (argc != 3 || (!endpoint_mode && !is_mode(argv[2], "sample") && !is_mode(argv[2], "full"))) {
    std::fprintf(stderr,
                 "usage: %s <conninfo> sample|full|endpoints-label|endpoints-sample|endpoints\n",
                 argv[0]);
    return 2;
  }
  try {
    if (endpoint_mode) {
      return run_endpoints(argv[1], argv[2]);
    }
    dbk::Controller ctl(argv[1]);
    dbk::bootstrap::verify_floor(ctl);
    dbk::bootstrap::create_category_labels(ctl);
    long minted = 0, existing = 0;
    auto one = [&](uint32_t cp) {
      (dbk::bootstrap::populate_character(ctl, cp) ? minted : existing)++;
    };
    if (std::strcmp(argv[2], "sample") == 0) {
      for (uint32_t cp : dbk::bootstrap::sample_codepoints()) {
        one(cp);
      }
    } else {
      for (uint32_t cp = 0x80; cp <= 0x10FFFF; ++cp) {
        if (!dbk::bootstrap::is_populated_codepoint(cp)) {
          continue;
        }
        one(cp);
        if (cp % 0x10000 == 0) {
          std::fprintf(stderr, "U+%06X\n", cp);
        }
      }
    }
    std::fprintf(stderr, "done: %ld minted, %ld already present\n", minted, existing);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "populate_encoding failed: %s\n", e.what());
    return 1;
  }
  return 0;
}
