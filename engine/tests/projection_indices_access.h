#pragma once
#include "assembly/projection_access.h"
namespace assembly::testing {
struct ProjectionIndicesAccess {
  static auto &rows(LetterProjection &p) { return p.occurrences_; }
};
}
