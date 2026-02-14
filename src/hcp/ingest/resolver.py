"""
Resolver: Raw tokens → HCP token IDs.

Stage 2 of the PBM text encoder pipeline. Maps scanner output to token IDs
from hcp_english (words) and hcp_core (punctuation, structural markers).

Pre-loads the full hcp_english token cache (~1.3M entries, ~77MB) for O(1) lookup.
"""

from dataclasses import dataclass, field
from scanner import RawToken, TokenType
import psycopg


@dataclass
class ResolvedToken:
    """A token resolved to an HCP token ID."""
    token_id: str       # Full token ID (e.g., 'AB.AB.CA.AA.dF')
    surface: str        # Surface form used for reconstruction
    source: str         # How this was resolved: 'exact', 'case_relaxed', 'split', 'sic', 'punctuation', 'marker'

    @property
    def ns(self) -> str:
        return self.token_id.split('.')[0]

    @property
    def p2(self) -> str | None:
        parts = self.token_id.split('.')
        return parts[1] if len(parts) > 1 else None

    @property
    def p3(self) -> str | None:
        parts = self.token_id.split('.')
        return parts[2] if len(parts) > 2 else None

    @property
    def p4(self) -> str | None:
        parts = self.token_id.split('.')
        return parts[3] if len(parts) > 3 else None

    @property
    def p5(self) -> str | None:
        parts = self.token_id.split('.')
        return parts[4] if len(parts) > 4 else None


# Structural marker token IDs (from hcp_core AA.AE namespace)
MARKERS = {
    'document_start':   'AA.AE.AA.AA',
    'document_end':     'AA.AE.AA.AB',
    'part_break':       'AA.AE.AA.AC',
    'chapter_break':    'AA.AE.AA.AD',
    'section_break':    'AA.AE.AA.AE',
    'paragraph_start':  'AA.AE.AA.AI',
    'paragraph_end':    'AA.AE.AA.AJ',
    'line_break':       'AA.AE.AA.AK',
    'title_start':      'AA.AE.AA.Ag',
    'title_end':        'AA.AE.AA.Ah',
    'italic_start':     'AA.AE.AB.AC',
    'italic_end':       'AA.AE.AB.AD',
    'all_caps_start':   'AA.AE.AB.AM',
    'all_caps_end':     'AA.AE.AB.AN',
    'sic_start':        'AA.AE.AC.AA',
    'sic_end':          'AA.AE.AC.AB',
    'tbd':              'AA.AE.AC.AL',
}

# ASCII punctuation → hcp_core byte code token IDs
PUNCTUATION_MAP = {
    '.':  'AA.AA.AA.AA.Aw',
    ',':  'AA.AA.AA.AA.Au',
    ';':  'AA.AA.AA.AA.BJ',
    ':':  'AA.AA.AA.AA.BI',
    '!':  'AA.AA.AA.AA.Ai',
    '?':  'AA.AA.AA.AA.BN',
    '(':  'AA.AA.AA.AA.Aq',
    ')':  'AA.AA.AA.AA.Ar',
    '[':  'AA.AA.AA.AA.BT',
    ']':  'AA.AA.AA.AA.BV',
    '{':  'AA.AA.AA.AA.Ca',
    '}':  'AA.AA.AA.AA.Cc',
    '-':  'AA.AA.AA.AA.Av',
    '"':  'AA.AA.AA.AA.Aj',
    "'":  'AA.AA.AA.AA.Ap',
    '_':  'AA.AA.AA.AA.Bv',
    '/':  'AA.AA.AA.AA.Ax',
    '*':  'AA.AA.AA.AA.As',
    '#':  'AA.AA.AA.AA.Al',
    '$':  'AA.AA.AA.AA.Am',
    '%':  'AA.AA.AA.AA.An',
}

# Unicode punctuation → hcp_core Unicode character token IDs
UNICODE_PUNCT_MAP = {
    '\u201c': 'AA.AB.AA.AY.AE',  # " LEFT DOUBLE QUOTATION MARK
    '\u201d': 'AA.AB.AA.AY.AF',  # " RIGHT DOUBLE QUOTATION MARK
    '\u2018': 'AA.AB.AA.AY.AA',  # ' LEFT SINGLE QUOTATION MARK
    '\u2019': 'AA.AB.AA.AY.AB',  # ' RIGHT SINGLE QUOTATION MARK
    '\u2014': 'AA.AB.AA.AW.AE',  # — EM DASH
    '\u2013': 'AA.AB.AA.AW.AD',  # – EN DASH
    '\u2026': 'AA.AB.AA.AW.AJ',  # … HORIZONTAL ELLIPSIS (verify token exists)
}

# Em dash equivalents: double hyphen maps to em dash
EM_DASH_TOKENS = {
    '--': 'AA.AB.AA.AW.AE',    # Double hyphen → EM DASH token
    '\u2014': 'AA.AB.AA.AW.AE',  # Unicode em dash
}

# Ellipsis
ELLIPSIS_TOKEN = {
    '...': 'AA.AB.AA.AW.AJ',  # Triple period → HORIZONTAL ELLIPSIS (verify)
}


class Resolver:
    """Resolve raw tokens to HCP token IDs."""

    def __init__(self, db_config: dict | None = None):
        self.db_config = db_config or {
            'host': 'localhost',
            'dbname': 'hcp_english',
            'user': 'hcp',
            'password': 'hcp_dev',
        }
        self.word_cache: dict[str, str] = {}  # name → token_id
        self.char_cache: dict[str, str] = {}  # character → token_id (for sic)
        self.unknown_log: list[dict] = []     # Track unknown words
        self._loaded = False

    def load_cache(self):
        """Pre-load the full hcp_english token cache into memory."""
        if self._loaded:
            return

        print("Loading hcp_english token cache...")
        conn = psycopg.connect(**self.db_config)
        try:
            with conn.cursor() as cur:
                # Load all word tokens (C* = standard POS categories)
                cur.execute("""
                    SELECT name, token_id
                    FROM tokens
                    WHERE ns = 'AB' AND p2 = 'AB'
                    ORDER BY p3, p4, p5
                """)
                for name, token_id in cur:
                    # First match wins (deterministic, p3-ordered)
                    if name not in self.word_cache:
                        self.word_cache[name] = token_id
        finally:
            conn.close()

        # Load character tokens from hcp_core for sic fallback
        print("Loading character token cache...")
        self._build_char_cache()

        print(f"  Loaded {len(self.word_cache):,} word tokens")
        print(f"  Loaded {len(self.char_cache):,} character tokens")
        self._loaded = True

    def _build_char_cache(self):
        """Build character cache for sic encoding and punctuation lookup.

        Loads from both hcp_core ASCII byte codes (AA.AA.AA.AA.*) and
        Unicode character table (AA.AB.AA.*). Also includes hardcoded
        punctuation maps.
        """
        # Start with hardcoded punctuation maps
        for char, token_id in PUNCTUATION_MAP.items():
            self.char_cache[char] = token_id
        for char, token_id in UNICODE_PUNCT_MAP.items():
            self.char_cache[char] = token_id

        core_config = dict(self.db_config)
        core_config['dbname'] = 'hcp_core'
        conn = psycopg.connect(**core_config)
        try:
            with conn.cursor() as cur:
                # Load ASCII byte code tokens (AA.AA.AA.AA.*)
                cur.execute("""
                    SELECT name, token_id
                    FROM tokens
                    WHERE ns = 'AA' AND p2 = 'AA' AND p3 = 'AA' AND p4 = 'AA'
                    ORDER BY token_id
                """)
                for name, token_id in cur:
                    char = self._name_to_char(name)
                    if char is not None and char not in self.char_cache:
                        self.char_cache[char] = token_id

                # Load Unicode character tokens (AA.AB.AA.*)
                # These cover accented letters, special punctuation, etc.
                cur.execute("""
                    SELECT name, token_id
                    FROM tokens
                    WHERE ns = 'AA' AND p2 = 'AB' AND p3 = 'AA'
                    ORDER BY token_id
                """)
                for name, token_id in cur:
                    char = self._name_to_char(name)
                    if char is not None and char not in self.char_cache:
                        self.char_cache[char] = token_id
        finally:
            conn.close()

    def _name_to_char(self, name: str) -> str | None:
        """Convert a character name from hcp_core to the actual character.

        Handles hcp_core naming conventions which may differ from standard
        Unicode names (e.g., "DIGIT 0" vs "DIGIT ZERO", lowercase in
        "LATIN SMALL LETTER a" vs "LATIN SMALL LETTER A").
        """
        import unicodedata

        # Direct unicodedata lookup (works for standard names)
        try:
            return unicodedata.lookup(name)
        except KeyError:
            pass

        # Handle hcp_core digit names: "DIGIT 0" through "DIGIT 9"
        if name.startswith('DIGIT ') and len(name) == 7:
            digit = name[-1]
            if digit.isdigit():
                return digit

        # Handle hcp_core letter names with lowercase:
        # "LATIN SMALL LETTER a" → "LATIN SMALL LETTER A"
        # "LATIN CAPITAL LETTER a" → "LATIN CAPITAL LETTER A"
        if 'LETTER' in name:
            # Try uppercasing the last character of the name
            fixed = name[:-1] + name[-1].upper()
            try:
                return unicodedata.lookup(fixed)
            except KeyError:
                pass
            # Also try the original with just the letter portion fixed
            parts = name.rsplit(' ', 1)
            if len(parts) == 2 and len(parts[1]) == 1:
                fixed = parts[0] + ' ' + parts[1].upper()
                try:
                    return unicodedata.lookup(fixed)
                except KeyError:
                    pass

        # Common control/whitespace names
        name_upper = name.upper()
        control_map = {
            'SPACE': ' ',
            'LINE FEED': '\n',
            'NEWLINE': '\n',
            'CARRIAGE RETURN': '\r',
            'HORIZONTAL TABULATION': '\t',
            'CHARACTER TABULATION': '\t',
            'NULL': '\x00',
            'BACKSPACE': '\x08',
            'DELETE': '\x7f',
            'ESCAPE': '\x1b',
        }
        if name_upper in control_map:
            return control_map[name_upper]

        return None

    def resolve(self, raw_token: RawToken,
                preceding_tokens: list[ResolvedToken] | None = None) -> list[ResolvedToken]:
        """Resolve a raw token to one or more HCP token IDs.

        Args:
            raw_token: The raw token from the scanner.
            preceding_tokens: Previously resolved tokens (for context like
                              positional capitalization).

        Returns:
            List of ResolvedToken (usually 1, but splits produce multiple).
        """
        if not self._loaded:
            self.load_cache()

        # Italic markers → structural tokens
        if raw_token.type == TokenType.ITALIC_START:
            return [ResolvedToken(MARKERS['italic_start'], '', 'marker')]
        if raw_token.type == TokenType.ITALIC_END:
            return [ResolvedToken(MARKERS['italic_end'], '', 'marker')]

        # Punctuation → byte code tokens
        if raw_token.type == TokenType.PUNCTUATION:
            return self._resolve_punctuation(raw_token)

        # Numbers → look up as word, fallback to sic
        if raw_token.type == TokenType.NUMBER:
            return self._resolve_word_token(raw_token, preceding_tokens)

        # Words → the main resolution path
        return self._resolve_word_token(raw_token, preceding_tokens)

    def _resolve_punctuation(self, raw_token: RawToken) -> list[ResolvedToken]:
        """Resolve punctuation to hcp_core byte code tokens."""
        text = raw_token.text

        # Multi-character punctuation
        if text in EM_DASH_TOKENS:
            return [ResolvedToken(EM_DASH_TOKENS[text], text, 'punctuation')]
        if text in ELLIPSIS_TOKEN:
            return [ResolvedToken(ELLIPSIS_TOKEN[text], text, 'punctuation')]

        # Single character
        if text in PUNCTUATION_MAP:
            return [ResolvedToken(PUNCTUATION_MAP[text], text, 'punctuation')]
        if text in UNICODE_PUNCT_MAP:
            return [ResolvedToken(UNICODE_PUNCT_MAP[text], text, 'punctuation')]

        # Unknown punctuation — encode as character
        if text in self.char_cache:
            return [ResolvedToken(self.char_cache[text], text, 'punctuation')]

        # Truly unknown character — sic wrap
        return self._sic_encode(text)

    def _resolve_word_token(self, raw_token: RawToken,
                            preceding_tokens: list[ResolvedToken] | None = None
                            ) -> list[ResolvedToken]:
        """Resolve a word/number token to HCP token IDs.

        Resolution order:
        1. Exact match in word cache
        2. Case relaxation (lowercase) if capitalized
        3. Possessive/apostrophe split
        4. Hyphenated compound split
        5. sic fallback (character-by-character encoding)
        """
        text = raw_token.text

        # Normalize smart apostrophes to ASCII for cache lookup
        normalized = text.replace('\u2019', "'")

        # Step 1: Exact match
        token_id = self.word_cache.get(normalized)
        if token_id:
            return [ResolvedToken(token_id, text, 'exact')]

        # Step 2: Case relaxation
        if raw_token.is_capitalized:
            lowercase = normalized.lower()
            token_id = self.word_cache.get(lowercase)
            if token_id:
                return [ResolvedToken(token_id, text, 'case_relaxed')]

        # Also try lowercase for non-capitalized but unusual case
        lowercase = normalized.lower()
        if lowercase != normalized:
            token_id = self.word_cache.get(lowercase)
            if token_id:
                return [ResolvedToken(token_id, text, 'case_relaxed')]

        # Step 3: Possessive / trailing apostrophe split
        if normalized.endswith("'s"):
            base = normalized[:-2]
            base_id = self.word_cache.get(base) or self.word_cache.get(base.lower())
            if base_id:
                # Determine which apostrophe was used
                apos_token = UNICODE_PUNCT_MAP.get('\u2019', PUNCTUATION_MAP["'"])
                if '\u2019' in text:
                    apos_token = UNICODE_PUNCT_MAP['\u2019']
                    apos_surface = '\u2019'
                else:
                    apos_token = PUNCTUATION_MAP["'"]
                    apos_surface = "'"
                s_id = self.word_cache.get('s')
                if s_id:
                    return [
                        ResolvedToken(base_id, base, 'split'),
                        ResolvedToken(apos_token, apos_surface, 'punctuation'),
                        ResolvedToken(s_id, 's', 'split'),
                    ]
                else:
                    return [
                        ResolvedToken(base_id, base, 'split'),
                        ResolvedToken(apos_token, apos_surface + 's', 'punctuation'),
                    ]

        if normalized.endswith("'"):
            base = normalized[:-1]
            base_id = self.word_cache.get(base) or self.word_cache.get(base.lower())
            if base_id:
                apos_token = PUNCTUATION_MAP["'"]
                apos_surface = "'"
                if text.endswith('\u2019'):
                    apos_token = UNICODE_PUNCT_MAP['\u2019']
                    apos_surface = '\u2019'
                return [
                    ResolvedToken(base_id, base, 'split'),
                    ResolvedToken(apos_token, apos_surface, 'punctuation'),
                ]

        # Step 4: Hyphenated compound split
        if '-' in normalized:
            norm_parts = normalized.split('-')
            orig_parts = text.split('-')  # Preserve original chars for surface
            resolved_parts = []
            all_found = True
            for i, part in enumerate(norm_parts):
                part_id = self.word_cache.get(part) or self.word_cache.get(part.lower())
                if part_id:
                    surface = orig_parts[i] if i < len(orig_parts) else part
                    resolved_parts.append(ResolvedToken(part_id, surface, 'split'))
                else:
                    all_found = False
                    break

            if all_found:
                # Interleave with hyphen tokens
                result = []
                hyphen = ResolvedToken(PUNCTUATION_MAP['-'], '-', 'punctuation')
                for i, rt in enumerate(resolved_parts):
                    if i > 0:
                        result.append(hyphen)
                    result.append(rt)
                return result

        # Step 5: Unknown word → sic encoding
        self.unknown_log.append({
            'text': text,
            'line': raw_token.line_number,
            'offset': raw_token.char_offset,
        })
        return self._sic_encode(text)

    def _sic_encode(self, text: str) -> list[ResolvedToken]:
        """Encode unknown text character-by-character wrapped in sic markers."""
        result = [ResolvedToken(MARKERS['sic_start'], '', 'marker')]

        for char in text:
            if char in self.char_cache:
                result.append(ResolvedToken(self.char_cache[char], char, 'sic'))
            else:
                # Character not in our cache — this shouldn't happen for ASCII
                # For now, log and skip
                self.unknown_log.append({
                    'text': f'<unknown char U+{ord(char):04X}>',
                    'line': -1,
                    'offset': -1,
                })
                # Still encode it so we don't lose content
                result.append(ResolvedToken(MARKERS['tbd'], char, 'marker'))

        result.append(ResolvedToken(MARKERS['sic_end'], '', 'marker'))
        return result

    def make_marker(self, marker_name: str) -> ResolvedToken:
        """Create a structural marker token."""
        if marker_name not in MARKERS:
            raise ValueError(f"Unknown marker: {marker_name}")
        return ResolvedToken(MARKERS[marker_name], '', 'marker')
