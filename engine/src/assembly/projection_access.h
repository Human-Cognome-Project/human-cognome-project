#pragma once
#include "assembly/letter_projection.h"
#include "assembly/preparation_work.h"

namespace assembly::detail {
// Rows carry numeric source identity only. Every contextual read selects a
// checked index in the owning projection; no reader accepts a detached row.
// Reads borrow synchronously and allocate nothing on frozen paths.
struct ProjectionAccess {
  static const auto &rows(const LetterProjection &p) { return p.occurrences_; }
  static const ProjectionOccurrence &row(const LetterProjection &p, std::size_t index) {
    if (index >= p.occurrences_.size()) throw warm::Error("projection occurrence index out of range");
    return p.occurrences_[index];
  }
  static bool matches(const LetterProjection &p, std::size_t index, const OccurrenceKey &key,
                      PreparationWork &work) {
    const auto &value=row(p,index);
    return work.equal(p.study_,key.study) && work.equal(value.source,key.source);
  }
  static std::size_t find(const LetterProjection &p, const OccurrenceKey &key, PreparationWork &work) {
    if (work.equal(p.study_,key.study))
      for (std::size_t index=0;index<p.occurrence_count();++index)
        if (work.equal(row(p,index).source,key.source)) return index;
    return p.occurrence_count();
  }
  static const warm::Tool *resolve(const LetterProjection &p, std::size_t index,
                                  PreparationWork *work = nullptr) {
    return p.resolve_definition(index,work);
  }
  static const warm::Tool &tool(const LetterProjection &p, std::size_t index,
                               PreparationWork *work = nullptr) {
    const auto *value=resolve(p,index,work);
    if (!value) throw warm::Error("projection definition index/source mismatch");
    return *value;
  }
  static const warm::ToolRef &definition(const LetterProjection &p, std::size_t index,
                                       PreparationWork *work = nullptr) {
    return tool(p,index,work).ref;
  }
};
}
