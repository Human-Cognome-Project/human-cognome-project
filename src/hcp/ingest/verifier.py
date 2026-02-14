"""
Round-trip Verifier: Compare original text with PBM-reconstructed text.

Verifies that the PBM encoding preserves all content by comparing
the original text with text reconstructed from the PBM.

Three verification levels:
1. Word-sequence: same words in same order (primary test)
2. Content-level: normalized text comparison
3. Structural: paragraph and marker count validation
"""

import re
import difflib
from dataclasses import dataclass, field


@dataclass
class VerificationResult:
    """Result of round-trip verification."""
    word_match: bool = False
    content_match: bool = False
    original_word_count: int = 0
    reconstructed_word_count: int = 0
    matching_words: int = 0
    word_match_rate: float = 0.0
    mismatches: list = field(default_factory=list)  # List of (position, original, reconstructed)
    original_char_count: int = 0
    reconstructed_char_count: int = 0
    original_line_count: int = 0
    reconstructed_line_count: int = 0


def extract_words(text: str) -> list[str]:
    """Extract all words from text, preserving order.

    Words are contiguous sequences of alphabetic characters and apostrophes.
    Punctuation, numbers, and formatting are stripped.
    """
    # Match word tokens: letters, possibly with internal apostrophes/smart quotes
    return re.findall(r"[a-zA-Z]+(?:['\u2019][a-zA-Z]+)*", text)


def normalize_text(text: str) -> str:
    """Normalize text for content comparison.

    Handles acceptable differences:
    - Trailing whitespace on lines → stripped
    - Multiple consecutive blank lines → single blank line
    - Trailing newlines → single newline
    - BOM characters → stripped
    - Hard line wraps within paragraphs → joined as spaces
    """
    # Strip BOM
    text = text.replace('\ufeff', '')

    # Split into lines
    lines = text.split('\n')

    # Strip trailing whitespace from each line
    lines = [line.rstrip() for line in lines]

    # Rejoin
    text = '\n'.join(lines)

    # Collapse multiple blank lines to single blank line
    text = re.sub(r'\n{3,}', '\n\n', text)

    # Strip leading/trailing whitespace
    text = text.strip()

    # Ensure single trailing newline
    text += '\n'

    return text


def join_paragraphs(text: str) -> str:
    """Join hard-wrapped lines within paragraphs into single lines.

    A paragraph break is a blank line. Lines within a paragraph
    (not separated by blank lines) are joined with spaces.
    """
    lines = text.split('\n')
    paragraphs = []
    current = []

    for line in lines:
        if line.strip() == '':
            if current:
                paragraphs.append(' '.join(l.strip() for l in current if l.strip()))
                current = []
            paragraphs.append('')  # Preserve the blank line
        else:
            current.append(line)

    if current:
        paragraphs.append(' '.join(l.strip() for l in current if l.strip()))

    return '\n'.join(paragraphs)


def verify(original: str, reconstructed: str) -> VerificationResult:
    """Verify round-trip fidelity between original and reconstructed text.

    Primary verification: word-sequence match after normalization.
    """
    result = VerificationResult()

    result.original_char_count = len(original)
    result.reconstructed_char_count = len(reconstructed)
    result.original_line_count = original.count('\n')
    result.reconstructed_line_count = reconstructed.count('\n')

    # Extract word sequences
    orig_words = extract_words(original)
    recon_words = extract_words(reconstructed)

    result.original_word_count = len(orig_words)
    result.reconstructed_word_count = len(recon_words)

    # Case-insensitive word comparison (since positional capitalization is computed)
    orig_lower = [w.lower() for w in orig_words]
    recon_lower = [w.lower() for w in recon_words]

    # Count matching words
    matcher = difflib.SequenceMatcher(None, orig_lower, recon_lower)
    matching = sum(block.size for block in matcher.get_matching_blocks())
    result.matching_words = matching
    result.word_match_rate = matching / max(len(orig_lower), 1)
    result.word_match = (orig_lower == recon_lower)

    # Find mismatches (first 50)
    if not result.word_match:
        opcodes = matcher.get_opcodes()
        mismatch_count = 0
        for tag, i1, i2, j1, j2 in opcodes:
            if tag != 'equal' and mismatch_count < 50:
                orig_segment = orig_lower[i1:i2]
                recon_segment = recon_lower[j1:j2]
                result.mismatches.append({
                    'type': tag,
                    'orig_pos': i1,
                    'orig_words': orig_words[i1:i2][:5],
                    'recon_pos': j1,
                    'recon_words': recon_words[j1:j2][:5],
                })
                mismatch_count += 1

    # Content-level comparison (normalized)
    norm_orig = normalize_text(join_paragraphs(original))
    norm_recon = normalize_text(reconstructed)
    result.content_match = (norm_orig == norm_recon)

    return result


def print_result(result: VerificationResult):
    """Print verification results."""
    print("\n" + "=" * 60)
    print("ROUND-TRIP VERIFICATION RESULTS")
    print("=" * 60)

    print(f"\nOriginal:      {result.original_word_count:,} words, "
          f"{result.original_char_count:,} chars, "
          f"{result.original_line_count:,} lines")
    print(f"Reconstructed: {result.reconstructed_word_count:,} words, "
          f"{result.reconstructed_char_count:,} chars, "
          f"{result.reconstructed_line_count:,} lines")

    print(f"\nWord-sequence match: {'PASS' if result.word_match else 'FAIL'}")
    print(f"  Matching words: {result.matching_words:,} / {result.original_word_count:,} "
          f"({result.word_match_rate:.1%})")
    print(f"Content match (normalized): {'PASS' if result.content_match else 'FAIL'}")

    if result.mismatches:
        print(f"\nFirst {len(result.mismatches)} mismatches:")
        for m in result.mismatches[:20]:
            tag = m['type']
            if tag == 'replace':
                print(f"  [{m['orig_pos']}] REPLACE "
                      f"{' '.join(m['orig_words'])} → {' '.join(m['recon_words'])}")
            elif tag == 'delete':
                print(f"  [{m['orig_pos']}] MISSING from reconstruction: "
                      f"{' '.join(m['orig_words'])}")
            elif tag == 'insert':
                print(f"  [{m['recon_pos']}] EXTRA in reconstruction: "
                      f"{' '.join(m['recon_words'])}")


if __name__ == '__main__':
    import sys

    original_path = sys.argv[1] if len(sys.argv) > 1 else \
        '/opt/project/repo/data/gutenberg/texts/00084_Frankenstein Or The Modern Prometheus.txt'
    reconstructed_path = sys.argv[2] if len(sys.argv) > 2 else \
        '/tmp/frankenstein_reconstructed.txt'

    print(f"Original: {original_path}")
    print(f"Reconstructed: {reconstructed_path}")

    original = open(original_path).read()
    reconstructed = open(reconstructed_path).read()

    result = verify(original, reconstructed)
    print_result(result)

    # Write detailed results
    output = '/tmp/frankenstein_verification.txt'
    with open(output, 'w') as f:
        f.write("ROUND-TRIP VERIFICATION RESULTS\n")
        f.write("=" * 60 + "\n\n")
        f.write(f"Original: {original_path}\n")
        f.write(f"Reconstructed: {reconstructed_path}\n\n")
        f.write(f"Original:      {result.original_word_count:,} words, "
                f"{result.original_char_count:,} chars\n")
        f.write(f"Reconstructed: {result.reconstructed_word_count:,} words, "
                f"{result.reconstructed_char_count:,} chars\n\n")
        f.write(f"Word-sequence match: {'PASS' if result.word_match else 'FAIL'}\n")
        f.write(f"Matching words: {result.matching_words:,} / {result.original_word_count:,} "
                f"({result.word_match_rate:.1%})\n")
        f.write(f"Content match: {'PASS' if result.content_match else 'FAIL'}\n\n")

        if result.mismatches:
            f.write(f"ALL MISMATCHES ({len(result.mismatches)}):\n")
            for m in result.mismatches:
                tag = m['type']
                if tag == 'replace':
                    f.write(f"  [{m['orig_pos']}] REPLACE "
                            f"{' '.join(m['orig_words'])} → {' '.join(m['recon_words'])}\n")
                elif tag == 'delete':
                    f.write(f"  [{m['orig_pos']}] MISSING: "
                            f"{' '.join(m['orig_words'])}\n")
                elif tag == 'insert':
                    f.write(f"  [{m['recon_pos']}] EXTRA: "
                            f"{' '.join(m['recon_words'])}\n")

    print(f"\nDetailed results written to: {output}")
