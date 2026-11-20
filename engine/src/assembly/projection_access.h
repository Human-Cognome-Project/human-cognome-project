#pragma once
#include "assembly/letter_projection.h"
#include "assembly/preparation_work.h"

namespace assembly::detail {
// Rows carry numeric source identity only. Every contextual read selects a
// checked index in the owning projection; no reader accepts a detached row.
// Reads borrow synchronously and allocate nothing on frozen paths.
struct ProjectionAccess {
  static std::size_t parent_count(const LetterProjection &p,bool spare) {
    return spare?p.spare_parents_.size():p.parents_.size();
  }
  static const ProjectionParentContribution &parent(const LetterProjection &p,bool spare,std::size_t index) {
    const auto &parents=spare?p.spare_parents_:p.parents_;
    if(index>=parents.size())throw warm::Error("projection parent index out of range");
    return parents[index];
  }
  static bool parent_matches(const LetterProjection &p,bool spare,std::size_t index,
                             const warm::Uuid &study,const warm::OccurrenceId &source,PreparationWork &work) {
    work.charge(1);
    return index<parent_count(p,spare) && work.equal(p.study_,study) &&
        work.equal(parent(p,spare,index).source,source) && p.resolve_parent(spare,index,&work);
  }
  static bool parent_matches(const LetterProjection &p,bool spare,std::size_t index,
                             const OccurrenceKey &key,PreparationWork &work) {
    return parent_matches(p,spare,index,key.study,key.source,work);
  }
  static const warm::Parent *resolve_parent(const LetterProjection &p,bool spare,std::size_t index,
                                           PreparationWork *work=nullptr) {
    return p.resolve_parent(spare,index,work);
  }
  static const warm::ToolRef &parent_target(const LetterProjection &p,bool spare,std::size_t index,
                                           PreparationWork *work=nullptr) {
    const auto *edge=resolve_parent(p,spare,index,work);
    if(!edge)throw warm::Error("projection parent index/source mismatch");
    return edge->target;
  }
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
  // A byte's logical aggregate can be inactive and have no native claim.
  // Invalid source/provenance/index associations never acquire owner context.
  static std::size_t byte_owner_index(const LetterProjection &p, std::size_t index,
                                     PreparationWork *work = nullptr) {
    if (!resolve(p,index,work)) return p.occurrence_count();
    const auto &value=row(p,index);
    return value.byte ? value.byte->owner_index : p.occurrence_count();
  }
};
}
