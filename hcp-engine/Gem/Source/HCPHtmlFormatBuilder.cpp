#include "HCPHtmlFormatBuilder.h"
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>

namespace HCPEngine
{
    /**
     * Requirement: Extract plain text from HTML for the Tokenizer pipeline.
     * Strategy: Use a State Machine to strip tags while preserving structural 
     * boundaries (newlines) to prevent token collisions between paragraphs.
     */
    AZStd::string HTMLFormatBuilder::ExtractCleanText(const AZStd::string& rawHtml)
    {
        AZStd::string output;
        
        // Performance optimization: Reserve memory to avoid multiple reallocations
        // during string growth, assuming output is roughly the same size as input.
        output.reserve(rawHtml.size());
        
        bool inTag = false;
        AZStd::string currentTag;

        for (size_t i = 0; i < rawHtml.size(); ++i)
        {
            char c = rawHtml[i];

            // Transition: Start of an HTML tag
            if (c == '<') {
                inTag = true;
                currentTag.clear();
                continue;
            }
            
            // Transition: End of an HTML tag
            if (c == '>') {
                inTag = false;
                
                // If the tag we just closed is a "Block Element" (like <p>),
                // we insert a newline. This ensures the Tokenizer treats 
                // separate paragraphs as separate cognitive structures.
                if (IsBlockElement(currentTag)) {
                    output += "\n";
                }
                continue;
            }

            if (inTag) {
                // Collect the tag name (lowercase) while ignoring slashes 
                // and attributes (e.g., <div class="test"> becomes "div")
                if (c != '/' && !AZStd::isspace(c)) {
                    currentTag += static_cast<char>(AZStd::tolower(c));
                }
            }
            else {
                /**
                 * Character Processing:
                 * We handle basic HTML entities here. While a full regex or 
                 * lookup table is possible, skipping to ';' prevents entity 
                 * codes from being treated as literal words/tokens.
                 */
                if (c == '&') {
                    // Skip until the end of the entity to keep the map clean
                    while (i < rawHtml.size() && rawHtml[i] != ';') i++;
                    output += ' '; // Replace entity with a space delimiter
                } else {
                    // Append valid human-readable character to output
                    output += c;
                }
            }
        }
        
        return output;
    }

    /**
     * Helper to define "Structural Physics" for the Cognome Map.
     * Tags listed here will force a line break, maintaining the 
     * logical separation of thoughts/data in the resulting token bed.
     */
    bool HTMLFormatBuilder::IsBlockElement(const AZStd::string& tag)
    {
        // Add or remove tags here based on the desired resolution of the document map
        return (tag == "p" || tag == "div" || tag == "br" || 
                tag == "h1" || tag == "h2" || tag == "li" || 
                tag == "title" || tag == "header" || tag == "footer");
    }
}
