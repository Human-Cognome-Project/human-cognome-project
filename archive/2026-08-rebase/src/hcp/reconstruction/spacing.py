"""
Spacing reconstruction using database-stored rules.

Applies language-specific spacing rules to reconstruct text from token sequences.
"""

from typing import List, Optional
from dataclasses import dataclass
import sqlite3
from pathlib import Path


@dataclass
class Token:
    """A token with type information."""
    string: str
    type: str  # 'word', 'punctuation', 'number', 'control'


@dataclass
class SpacingRule:
    """A spacing rule from the database."""
    language: str
    rule_type: str
    current_token_type: Optional[str]
    next_token_type: Optional[str]
    current_token_value: Optional[str]
    next_token_value: Optional[str]
    spacing: str
    priority: int


class SpacingReconstructor:
    """Reconstruct text with proper spacing from token sequences."""

    def __init__(self, db_path: Optional[Path] = None):
        """
        Initialize with database connection.

        Args:
            db_path: Path to SQLite database with spacing_rules table.
                    If None, uses in-memory database (for testing).
        """
        if db_path:
            self.conn = sqlite3.connect(str(db_path))
        else:
            # In-memory database for testing
            self.conn = sqlite3.connect(':memory:')
            self._init_test_db()

        self.rules_cache = {}  # Cache rules by language

    def _init_test_db(self):
        """Initialize test database with English rules."""
        cursor = self.conn.cursor()

        # Create table
        cursor.execute("""
            CREATE TABLE spacing_rules (
                rule_id INTEGER PRIMARY KEY,
                language TEXT,
                rule_type TEXT,
                current_token_type TEXT,
                next_token_type TEXT,
                current_token_value TEXT,
                next_token_value TEXT,
                spacing TEXT,
                priority INTEGER,
                enabled BOOLEAN DEFAULT 1
            )
        """)

        # English rules
        # Universal: space after any token UNLESS:
        #   1. Next token is punctuation
        #   2. Next token is control
        #   3. Current token is control (handles consecutive control tokens)
        rules = [
            # Default: space after everything
            ('en', 'default', None, None, None, None, ' ', 1),
            # Exception: no space before punctuation (applies to ALL tokens including punctuation)
            ('en', 'exception', None, 'punctuation', None, None, '', 2),
            # Exception: no space before control
            ('en', 'exception', None, 'control', None, None, '', 2),
            # Exception: no space after control (handles consecutive control tokens)
            ('en', 'exception', 'control', None, None, None, '', 3),
        ]

        cursor.executemany("""
            INSERT INTO spacing_rules
            (language, rule_type, current_token_type, next_token_type,
             current_token_value, next_token_value, spacing, priority)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, rules)

        self.conn.commit()

    def load_rules(self, language: str) -> List[SpacingRule]:
        """
        Load spacing rules for a language from database.

        Args:
            language: Language code (e.g., 'en')

        Returns:
            List of SpacingRule objects, sorted by priority (highest first)
        """
        if language in self.rules_cache:
            return self.rules_cache[language]

        cursor = self.conn.cursor()
        cursor.execute("""
            SELECT language, rule_type,
                   current_token_type, next_token_type,
                   current_token_value, next_token_value,
                   spacing, priority
            FROM spacing_rules
            WHERE language = ? AND enabled = 1
            ORDER BY priority DESC
        """, (language,))

        rules = [
            SpacingRule(
                language=row[0],
                rule_type=row[1],
                current_token_type=row[2],
                next_token_type=row[3],
                current_token_value=row[4],
                next_token_value=row[5],
                spacing=row[6],
                priority=row[7]
            )
            for row in cursor.fetchall()
        ]

        self.rules_cache[language] = rules
        return rules

    def get_spacing(
        self,
        current_token: Token,
        next_token: Token,
        language: str = 'en'
    ) -> str:
        """
        Determine spacing between two tokens.

        Args:
            current_token: Current token
            next_token: Next token
            language: Language code

        Returns:
            Spacing string (' ', '', '\n', etc.)
        """
        rules = self.load_rules(language)

        # Check rules in priority order (highest first)
        for rule in rules:
            if self._rule_matches(rule, current_token, next_token):
                return rule.spacing

        # Fallback: no spacing (shouldn't happen with proper default rule)
        return ''

    def _rule_matches(
        self,
        rule: SpacingRule,
        current_token: Token,
        next_token: Token
    ) -> bool:
        """Check if a rule matches the token pair."""

        # Check current token type
        if rule.current_token_type is not None:
            if current_token.type != rule.current_token_type:
                return False

        # Check next token type
        if rule.next_token_type is not None:
            if next_token.type != rule.next_token_type:
                return False

        # Check current token value (specific token)
        if rule.current_token_value is not None:
            if current_token.string != rule.current_token_value:
                return False

        # Check next token value (specific token)
        if rule.next_token_value is not None:
            if next_token.string != rule.next_token_value:
                return False

        return True

    def reconstruct(
        self,
        tokens: List[Token],
        language: str = 'en'
    ) -> str:
        """
        Reconstruct text from token sequence with proper spacing.

        Args:
            tokens: List of Token objects
            language: Language code

        Returns:
            Reconstructed text with spacing
        """
        if not tokens:
            return ''

        text = tokens[0].string

        for i in range(len(tokens) - 1):
            current = tokens[i]
            next_tok = tokens[i + 1]

            # Get spacing between tokens
            spacing = self.get_spacing(current, next_tok, language)
            text += spacing + next_tok.string

        return text


# Example usage and testing
if __name__ == "__main__":
    # Test with sample tokens
    reconstructor = SpacingReconstructor()  # In-memory test DB

    # Test case 1: Basic sentence
    tokens = [
        Token("The", "word"),
        Token("cat", "word"),
        Token("jumped", "word"),
        Token(".", "punctuation")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 1: {result}")
    assert result == "The cat jumped.", f"Expected 'The cat jumped.', got '{result}'"

    # Test case 2: Punctuation followed by punctuation (no space before punctuation)
    tokens = [
        Token("What", "word"),
        Token("?", "punctuation"),
        Token("!", "punctuation")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 2: {result}")
    assert result == "What?!", f"Expected 'What?!' (no space before punctuation), got '{result}'"

    # Test case 3: Punctuation in middle
    tokens = [
        Token("Hello", "word"),
        Token(",", "punctuation"),
        Token("world", "word"),
        Token(".", "punctuation")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 3: {result}")
    assert result == "Hello, world.", f"Expected 'Hello, world.', got '{result}'"

    # Test case 4: Numbers
    tokens = [
        Token("I", "word"),
        Token("have", "word"),
        Token("3", "number"),
        Token("cats", "word"),
        Token(".", "punctuation")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 4: {result}")
    assert result == "I have 3 cats.", f"Expected 'I have 3 cats.', got '{result}'"

    # Test case 5: Consecutive control tokens (no spaces)
    tokens = [
        Token("Paragraph", "word"),
        Token("1", "number"),
        Token(".", "punctuation"),
        Token("[NEWLINE]", "control"),
        Token("[NEWLINE]", "control"),
        Token("Paragraph", "word"),
        Token("2", "number"),
        Token(".", "punctuation")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 5: {result}")
    expected = "Paragraph 1.[NEWLINE][NEWLINE]Paragraph 2."
    assert result == expected, f"Expected '{expected}' (no space after/between control tokens), got '{result}'"

    # Test case 6: Mixed control tokens
    tokens = [
        Token("Line", "word"),
        Token("1", "number"),
        Token("[NEWLINE]", "control"),
        Token("[TAB]", "control"),
        Token("Indented", "word")
    ]

    result = reconstructor.reconstruct(tokens)
    print(f"Test 6: {result}")
    expected = "Line 1[NEWLINE][TAB]Indented"
    assert result == expected, f"Expected '{expected}' (no space after control, no space between controls), got '{result}'"

    print("\nAll tests passed! ✓")
