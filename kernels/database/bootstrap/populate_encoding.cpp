// Bootstrap driver: populates the multi-byte character encoding tables.
//
//   populate_encoding <conninfo> sample   -- U+00E9, U+20AC, U+1F600 only
//   populate_encoding <conninfo> full     -- all of U+0080..U+10FFFF
//
// Additive and idempotent: category labels are created via SEE, characters
// are fixed-address mints, membership adds are conflict-safe. Nothing is
// ever deleted or reset. Progress goes to stderr, sparsely.
#include <cstdio>
#include <cstring>
#include <exception>

#include "encoding_populate.h"

int main(int argc, char **argv) {
  if (argc != 3 || (std::strcmp(argv[2], "sample") != 0 && std::strcmp(argv[2], "full") != 0)) {
    std::fprintf(stderr, "usage: %s <conninfo> sample|full\n", argv[0]);
    return 2;
  }
  try {
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
