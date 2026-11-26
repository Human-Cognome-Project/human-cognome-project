#pragma once
#include "assembly/prepared_submission.h"

namespace viewport {
warm::Bundle whole_english_pair(warm::Bundle, const warm::ToolRef &, const warm::ToolRef &membership_focus);
assembly::PreparedStudySubmission whole_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly>, const warm::ToolRef &, const warm::ToolRef &membership_focus,
    field::HostPath = field::HostPath::BulkCopy);
// Fixed disjoint í cohort with a prepared C3 AD alternate and retained focus.
assembly::PreparedStudySubmission unicode_i_submission(
    warm::Bundle definitions, const warm::ToolRef &endpoint, warm::Uuid owner,
    field::HostPath path = field::HostPath::BulkCopy);
// Bind the aggregate bodies declared in an already acquired shared v2 recipe.
// Bodies follow canonical occurrence order with sequential addresses from zero.
// The study still supplies its comparison tree, focus and capacity.
std::vector<assembly::detail::AdmissionBody> declared_aggregate_bodies(
    const assembly::ActiveAssembly &);
struct PreparedLetterProfileError : warm::Error { using warm::Error::Error; };
// Explicit, bounded demonstration profile, selected by --study. Source pins
// supply exact construction; these functions supply the documented model and
// resource choices. Neither consumes a detached capture or numerical starts.
warm::Bundle prepared_english_pair(warm::Bundle definitions, const warm::ToolRef &english);
assembly::PreparedStudySubmission prepared_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path = field::HostPath::BulkCopy);
// Declared English-pair catalogue, baseline physical/source mass two, existing
// geometry/comparisons, and reserved workspace for between-tick edits.
assembly::PreparedStudySubmission editable_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path = field::HostPath::BulkCopy);
warm::Bundle prepared_retained_english_pair(warm::Bundle definitions, const warm::ToolRef &english);
assembly::PreparedStudySubmission prepared_retained_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path = field::HostPath::BulkCopy);
assembly::PreparedStudySubmission editable_retained_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path = field::HostPath::BulkCopy);
}
