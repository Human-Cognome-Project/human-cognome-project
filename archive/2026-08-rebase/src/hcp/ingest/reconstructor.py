"""
Reconstructor: PBM content stream → plain text.

Takes a PBM content stream (list of ResolvedToken) and reconstructs the
original text by applying whitespace rules, positional capitalization,
and formatting marker rendering.

This is the inverse of encoding, and its correctness is what round-trip
verification tests.
"""

from resolver import ResolvedToken, MARKERS, PUNCTUATION_MAP, UNICODE_PUNCT_MAP


# Invert MARKERS dict for quick lookup: token_id → marker_name
MARKER_IDS = {v: k for k, v in MARKERS.items()}

# Build reverse punctuation map: token_id → character
PUNCT_TO_CHAR = {}
PUNCT_TO_CHAR.update({v: k for k, v in PUNCTUATION_MAP.items()})
PUNCT_TO_CHAR.update({v: k for k, v in UNICODE_PUNCT_MAP.items()})

# Punctuation that suppresses space BEFORE it (attaches to preceding token)
NO_SPACE_BEFORE = {
    '.', ',', ';', ':', '!', '?',
    ')', ']', '}',
    '-',       # Hyphen (when part of compound word split)
    '\u201d',  # " RIGHT DOUBLE QUOTATION MARK
    '\u2019',  # ' RIGHT SINGLE QUOTATION MARK (apostrophe/closing)
    "'",       # ASCII apostrophe when closing
    '_',       # Underscore (italic end marker rendered as _)
}

# Punctuation that suppresses space AFTER it (attaches to following token)
NO_SPACE_AFTER = {
    '(', '[', '{',
    '-',       # Hyphen (when part of compound word split)
    '\u201c',  # " LEFT DOUBLE QUOTATION MARK
    '\u2018',  # ' LEFT SINGLE QUOTATION MARK
    '_',       # Underscore (italic start marker rendered as _)
}

# Sentence-ending punctuation
SENTENCE_ENDERS = {'.', '!', '?'}


class Reconstructor:
    """Reconstruct plain text from a PBM content stream."""

    def __init__(self, word_surface_cache: dict[str, str] | None = None):
        self.word_cache = word_surface_cache or {}
        self._loaded = bool(word_surface_cache)

    def load_surface_cache(self, db_config: dict | None = None):
        """Load word surface forms from hcp_english."""
        if self._loaded:
            return

        import psycopg
        config = db_config or {
            'host': 'localhost',
            'dbname': 'hcp_english',
            'user': 'hcp',
            'password': 'hcp_dev',
        }

        print("Loading surface form cache...")
        conn = psycopg.connect(**config)
        try:
            with conn.cursor() as cur:
                cur.execute("SELECT token_id, name FROM tokens WHERE ns = 'AB' AND p2 = 'AB'")
                for token_id, name in cur:
                    if token_id not in self.word_cache:
                        self.word_cache[token_id] = name
        finally:
            conn.close()

        print(f"  Loaded {len(self.word_cache):,} surface forms")
        self._loaded = True

    def reconstruct(self, stream: list[ResolvedToken]) -> str:
        """Reconstruct plain text from a PBM content stream.

        Args:
            stream: List of ResolvedToken from the encoder.

        Returns:
            Reconstructed plain text string.
        """
        output_parts = []
        capitalize_next = False
        in_all_caps = False
        in_sic = False
        prev_surface = None      # Surface of last emitted text (None = nothing yet)
        prev_was_marker = True   # Did previous entry produce no output?

        for entry in stream:
            marker_name = MARKER_IDS.get(entry.token_id)

            # --- Handle structural markers ---
            if marker_name:
                if marker_name == 'document_start':
                    continue
                elif marker_name == 'document_end':
                    continue

                elif marker_name == 'paragraph_start':
                    if output_parts:
                        output_parts.append('\n\n')
                    capitalize_next = True
                    prev_was_marker = True
                    continue

                elif marker_name == 'paragraph_end':
                    prev_was_marker = True
                    continue

                elif marker_name in ('chapter_break', 'section_break'):
                    if output_parts:
                        output_parts.append('\n\n\n')
                    prev_was_marker = True
                    prev_surface = None
                    continue

                elif marker_name == 'title_start':
                    capitalize_next = True
                    prev_was_marker = True
                    continue

                elif marker_name == 'title_end':
                    prev_was_marker = True
                    continue

                elif marker_name == 'line_break':
                    # Line break within a title or special block
                    if output_parts:
                        output_parts.append('\n')
                    prev_was_marker = True
                    prev_surface = None
                    continue

                elif marker_name == 'italic_start':
                    # Render as Gutenberg underscore convention
                    if prev_surface is not None and prev_surface not in NO_SPACE_AFTER:
                        output_parts.append(' ')
                    output_parts.append('_')
                    prev_surface = '_'
                    prev_was_marker = False
                    continue

                elif marker_name == 'italic_end':
                    output_parts.append('_')
                    prev_surface = '_'
                    prev_was_marker = False
                    continue

                elif marker_name == 'all_caps_start':
                    in_all_caps = True
                    continue
                elif marker_name == 'all_caps_end':
                    in_all_caps = False
                    continue

                elif marker_name == 'sic_start':
                    in_sic = True
                    # Need space before sic content
                    if prev_surface is not None and prev_surface not in NO_SPACE_AFTER:
                        output_parts.append(' ')
                    prev_was_marker = False
                    continue
                elif marker_name == 'sic_end':
                    in_sic = False
                    continue

                else:
                    continue

            # --- Handle content tokens ---

            # Get the surface form
            if in_sic:
                # In sic mode: emit the character surface directly
                surface = entry.surface
                output_parts.append(surface)
                prev_surface = surface
                prev_was_marker = False
                continue

            surface = self._get_surface(entry)
            if not surface:
                continue

            # Determine if we need a space before this token
            if not prev_was_marker and prev_surface is not None:
                need_space = self._needs_space(prev_surface, surface)
                if need_space:
                    output_parts.append(' ')

            # Apply positional capitalization
            if capitalize_next and surface and surface[0].isalpha():
                surface = surface[0].upper() + surface[1:]
                capitalize_next = False

            # Apply ALL CAPS
            if in_all_caps:
                surface = surface.upper()

            # Check for sentence-ending punctuation
            if surface in SENTENCE_ENDERS:
                capitalize_next = True

            output_parts.append(surface)
            prev_surface = surface
            prev_was_marker = False

        # Build final text
        text = ''.join(output_parts)
        if text and not text.endswith('\n'):
            text += '\n'

        return text

    def _needs_space(self, prev: str, current: str) -> bool:
        """Determine if a space is needed between two surface forms."""
        # No space before closing/attaching punctuation
        if current in NO_SPACE_BEFORE:
            return False
        # No space after opening punctuation
        if prev in NO_SPACE_AFTER:
            return False
        # No space after apostrophe/smart-quote before 's' (possessive)
        if prev in ("'", '\u2019') and current == 's':
            return False
        return True

    def _get_surface(self, entry: ResolvedToken) -> str:
        """Get the surface form for a resolved token."""
        # If the entry has a surface form from encoding, use it
        if entry.surface:
            return entry.surface

        # Check if it's a punctuation token
        if entry.token_id in PUNCT_TO_CHAR:
            return PUNCT_TO_CHAR[entry.token_id]

        # Word token — look up in surface cache
        if entry.token_id in self.word_cache:
            return self.word_cache[entry.token_id]

        return ''


def reconstruct_from_file(pbm_path: str, db_config: dict | None = None) -> str:
    """Reconstruct text from a PBM file (TSV format from text_encoder)."""
    tokens = []
    with open(pbm_path) as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split('\t')
            if len(parts) >= 4:
                pos, token_id, surface, source = parts[0], parts[1], parts[2], parts[3]
                surface = surface.replace('\\t', '\t').replace('\\n', '\n')
                tokens.append(ResolvedToken(token_id, surface, source))

    reconstructor = Reconstructor()
    reconstructor.load_surface_cache(db_config)
    return reconstructor.reconstruct(tokens)


if __name__ == '__main__':
    import sys
    pbm_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/frankenstein.pbm'
    print(f"Reconstructing from: {pbm_path}")
    text = reconstruct_from_file(pbm_path)
    output_path = pbm_path.replace('.pbm', '_reconstructed.txt')
    with open(output_path, 'w') as f:
        f.write(text)
    print(f"Reconstructed text written to: {output_path}")
    print(f"  Length: {len(text):,} characters")
    print(f"  Lines: {text.count(chr(10)):,}")
