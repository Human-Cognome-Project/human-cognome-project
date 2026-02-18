#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    class HCPVocabulary;

    //! Tokenize text into a sequence of token IDs using the vocabulary hash table.
    //! This IS the engine's tokenizer — straight hash lookup, no preprocessing outside the engine.
    //!
    //! @param text The input text
    //! @param vocab The loaded vocabulary
    //! @return Vector of token IDs including stream_start and stream_end anchors
    AZStd::vector<AZStd::string> Tokenize(const AZStd::string& text, const HCPVocabulary& vocab);

} // namespace HCPEngine
