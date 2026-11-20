#pragma once
#include "assembly/letter_projection.h"

namespace assembly::detail {
// Contextual reads borrow only from the live projection; no owning export or
// allocation is performed on preparation/frozen execution paths.
struct ProjectionAccess {
  static const auto &rows(const LetterProjection &p) { return p.occurrences_; }
  static const warm::Tool *resolve(const LetterProjection &p, const ProjectionOccurrence &row,
                                  PreparationWork *work = nullptr) {
    return p.resolve_definition(row,work);
  }
  static const warm::Tool &tool(const LetterProjection &p, const ProjectionOccurrence &row,
                               PreparationWork *work = nullptr) {
    const auto *value=resolve(p,row,work);
    if (!value) throw warm::Error("projection definition index/source mismatch");
    return *value;
  }
  static const warm::ToolRef &definition(const LetterProjection &p, const ProjectionOccurrence &row,
                                       PreparationWork *work = nullptr) {
    return tool(p,row,work).ref;
  }
};
}
