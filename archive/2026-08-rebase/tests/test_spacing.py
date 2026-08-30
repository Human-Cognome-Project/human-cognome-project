"""Tests for spacing reconstruction."""

from hcp.reconstruction.spacing import SpacingReconstructor, Token


class TestSpacingReconstructor:
    def setup_method(self):
        self.r = SpacingReconstructor()  # in-memory test DB

    def test_basic_sentence(self):
        tokens = [Token("The", "word"), Token("cat", "word"),
                  Token("sat", "word"), Token(".", "punctuation")]
        assert self.r.reconstruct(tokens) == "The cat sat."

    def test_comma_spacing(self):
        tokens = [Token("Hello", "word"), Token(",", "punctuation"),
                  Token("world", "word")]
        assert self.r.reconstruct(tokens) == "Hello, world"

    def test_consecutive_punctuation(self):
        tokens = [Token("What", "word"), Token("?", "punctuation"),
                  Token("!", "punctuation")]
        assert self.r.reconstruct(tokens) == "What?!"

    def test_number_in_sentence(self):
        tokens = [Token("I", "word"), Token("have", "word"),
                  Token("3", "number"), Token("cats", "word")]
        assert self.r.reconstruct(tokens) == "I have 3 cats"

    def test_control_tokens_no_space(self):
        tokens = [Token("Line", "word"), Token("1", "number"),
                  Token("[NL]", "control"), Token("[NL]", "control"),
                  Token("Line", "word"), Token("2", "number")]
        assert self.r.reconstruct(tokens) == "Line 1[NL][NL]Line 2"

    def test_empty_list(self):
        assert self.r.reconstruct([]) == ""

    def test_single_token(self):
        assert self.r.reconstruct([Token("hello", "word")]) == "hello"
