#pragma once
#include "assembly/projection_access.h"
namespace assembly::testing {
struct ProjectionIndicesAccess {
  static auto &parents(LetterProjection &p,bool spare) {return spare?p.spare_parents_:p.parents_;}
  static auto &rows(LetterProjection &p) { return p.occurrences_; }
};
}
