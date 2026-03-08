#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>

// You mentioned about html extracter in issue #16.
// This file is essentially an engine for it.
namespace HCPEngine
{
    class HTMLFormatBuilder
    {
    public:
        AZ_CLASS_ALLOCATOR(HTMLFormatBuilder, AZ::SystemAllocator);

        // Extracts clean text from raw HTML for the tokenizer pipeline
        static AZStd::string ExtractCleanText(const AZStd::string& rawHtml);

    private:
        // Internal helper to identify tags that should trigger a newline
        static bool IsBlockElement(const AZStd::string& tag);
    };
}
