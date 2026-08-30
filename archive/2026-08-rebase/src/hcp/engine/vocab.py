"""Vocabulary cache — hash table for engine tokenization.

Loads token mappings from PostgreSQL into memory:
  - char_to_token: single character → token_id (core byte codes)
  - word_to_token: word string → token_id (english vocabulary)
  - token_to_surface: token_id → surface string (for reconstruction)
  - token_to_category: token_id → category string (for spacing rules)
"""

import psycopg
from hcp.core.token_id import encode_pair


# Anchor token IDs
STREAM_START = "AA.AE.AF.AA.AA"
STREAM_END = "AA.AE.AF.AA.AB"


def _byte_token_id(byte_val):
    """Compute token_id for an ASCII byte value."""
    return f"AA.AA.AA.AA.{encode_pair(byte_val)}"


class VocabularyCache:
    """In-memory hash tables for token lookup."""

    def __init__(self, db_host="localhost", db_user="hcp", db_pass="hcp_dev"):
        self.db_host = db_host
        self.db_user = db_user
        self.db_pass = db_pass

        # char → token_id (ASCII printable + whitespace)
        self.char_to_token = {}
        # lowercase word → token_id
        self.word_to_token = {}
        # exact word (case-sensitive) → token_id (for label/proper noun tokens)
        self.label_to_token = {}
        # token_id → surface form
        self.token_to_surface = {}
        # token_id → category
        self.token_to_category = {}

    def load(self):
        """Load all vocabulary from databases."""
        self._load_core()
        self._load_english()
        # Register anchor tokens
        self.token_to_surface[STREAM_START] = ""
        self.token_to_surface[STREAM_END] = ""
        self.token_to_category[STREAM_START] = "pbm_anchor"
        self.token_to_category[STREAM_END] = "pbm_anchor"

    def _connect(self, dbname):
        return psycopg.connect(
            dbname=dbname, user=self.db_user,
            password=self.db_pass, host=self.db_host
        )

    def _load_core(self):
        """Load byte codes and structural tokens from hcp_core."""
        # Build char→token_id from byte codes (ASCII 0-127)
        for byte_val in range(128):
            ch = chr(byte_val)
            tid = _byte_token_id(byte_val)
            self.char_to_token[ch] = tid
            # Surface form: the actual character for printable, empty for control
            if 0x20 <= byte_val <= 0x7E:
                self.token_to_surface[tid] = ch
            elif byte_val == 0x0A:  # LINE FEED
                self.token_to_surface[tid] = "\n"
            elif byte_val == 0x0D:  # CARRIAGE RETURN
                self.token_to_surface[tid] = "\r"
            elif byte_val == 0x09:  # TAB
                self.token_to_surface[tid] = "\t"

        # Load categories from DB for all core tokens
        conn = self._connect("hcp_core")
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT token_id, name, category FROM tokens WHERE ns = 'AA'"
                )
                for tid, name, cat in cur.fetchall():
                    self.token_to_category[tid] = cat
                    if tid not in self.token_to_surface:
                        self.token_to_surface[tid] = name
        finally:
            conn.close()

    def _load_english(self):
        """Load word tokens from hcp_english."""
        conn = self._connect("hcp_english")
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT token_id, name, layer, subcategory FROM tokens"
                )
                for tid, name, layer, subcat in cur.fetchall():
                    # Label tokens (proper nouns) are case-sensitive
                    if subcat == "label":
                        self.label_to_token[name] = tid
                    else:
                        self.word_to_token[name.lower()] = tid
                    self.token_to_surface[tid] = name
                    self.token_to_category[tid] = subcat or layer or "word"
        finally:
            conn.close()

    def lookup(self, text):
        """Look up a token string → token_id.

        Tries in order:
        1. Single character → byte code token
        2. Exact match → label token (proper nouns)
        3. Lowercase match → word token
        Returns None if not found.
        """
        if len(text) == 1 and text in self.char_to_token:
            return self.char_to_token[text]
        if text in self.label_to_token:
            return self.label_to_token[text]
        return self.word_to_token.get(text.lower())

    def surface(self, token_id):
        """Get surface form for a token_id."""
        return self.token_to_surface.get(token_id, f"<{token_id}>")

    def category(self, token_id):
        """Get category for a token_id."""
        return self.token_to_category.get(token_id, "unknown")
