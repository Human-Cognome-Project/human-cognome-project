#pragma once
#include "assembly/prepared_submission.h"

namespace viewport {
struct PreparedLetterProfileError : warm::Error { using warm::Error::Error; };
// Explicit, bounded demonstration profile, selected by --study. Source pins
// supply exact construction; these functions supply the documented model and
// resource choices. Neither consumes a detached capture or numerical starts.
warm::Bundle prepared_english_pair(warm::Bundle definitions, const warm::ToolRef &english);
assembly::PreparedStudySubmission prepared_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path = field::HostPath::BulkCopy);
}
