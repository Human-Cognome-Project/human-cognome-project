"""
Command-line interface for HCP.

Provides access to all HCP functionality via simple commands.
"""
from __future__ import annotations

import argparse
import sys
from typing import NoReturn


def cmd_demo(args: argparse.Namespace) -> int:
    """Run the full HCP demo."""
    from .demo import run_demo

    text = args.text or "The quik brwon fox jumps oevr the layz dog"
    run_demo(text, verbose=args.verbose)
    return 0


def cmd_correct(args: argparse.Namespace) -> int:
    """Correct spelling in text."""
    from ..physics.engine import correct

    result = correct(args.text)
    print(result)
    return 0


def cmd_decompose(args: argparse.Namespace) -> int:
    """Show NSM decomposition of text."""
    from ..abstraction.decomposer import visualize

    print(visualize(args.text))
    return 0


def cmd_tokenize(args: argparse.Namespace) -> int:
    """Tokenize text and show tokens."""
    from ..atomizer.tokenizer import Tokenizer, TokenizerConfig

    config = TokenizerConfig(promote_words=not args.bytes_only)
    tokenizer = Tokenizer(config)
    tokens = tokenizer.tokenize_text(args.text)

    print(f"Text: {args.text}")
    print(f"Tokens ({len(tokens)}):")
    for i, token in enumerate(tokens):
        if token.is_byte():
            char = chr(token.value) if 32 <= token.value < 127 else f"0x{token.value:02X}"
            print(f"  {i}: {token} = '{char}'")
        elif token.is_word():
            word = tokenizer.get_word_str(token)
            print(f"  {i}: {token} = '{word}'")
        else:
            print(f"  {i}: {token}")
    return 0


def cmd_pbm(args: argparse.Namespace) -> int:
    """Create and display PBM for text."""
    from ..core.pair_bond import create_pbm_from_text

    pbm = create_pbm_from_text(args.text)
    print(f"Text: {args.text}")
    print(f"Bytes: {len(args.text.encode('utf-8'))}")
    print()
    print(pbm)
    return 0


def cmd_validate(args: argparse.Namespace) -> int:
    """Validate reconstruction roundtrip."""
    from ..assembly.validator import RoundtripValidator

    validator = RoundtripValidator()
    result = validator.validate_text(args.text)
    print(f"Text: {args.text}")
    print(f"Result: {result}")
    return 0


def cmd_meter(args: argparse.Namespace) -> int:
    """Measure abstraction level of text."""
    from ..abstraction.abstraction_meter import measure_abstraction

    metrics = measure_abstraction(args.text)
    print(f"Text: {args.text}")
    print()
    print(metrics)
    return 0


def create_parser() -> argparse.ArgumentParser:
    """Create the argument parser."""
    parser = argparse.ArgumentParser(
        prog="hcp",
        description="Human Cognome Project - Structural decomposition and reconstruction",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output",
    )

    subparsers = parser.add_subparsers(dest="command", help="Command to run")

    # Demo command
    demo_parser = subparsers.add_parser("demo", help="Run the full HCP demo")
    demo_parser.add_argument("text", nargs="?", help="Text to process (default: misspelled pangram)")
    demo_parser.set_defaults(func=cmd_demo)

    # Correct command
    correct_parser = subparsers.add_parser("correct", help="Correct spelling in text")
    correct_parser.add_argument("text", help="Text to correct")
    correct_parser.set_defaults(func=cmd_correct)

    # Decompose command
    decompose_parser = subparsers.add_parser("decompose", help="Show NSM decomposition")
    decompose_parser.add_argument("text", help="Text to decompose")
    decompose_parser.set_defaults(func=cmd_decompose)

    # Tokenize command
    tokenize_parser = subparsers.add_parser("tokenize", help="Tokenize text")
    tokenize_parser.add_argument("text", help="Text to tokenize")
    tokenize_parser.add_argument(
        "--bytes-only",
        action="store_true",
        help="Only use byte-level tokens",
    )
    tokenize_parser.set_defaults(func=cmd_tokenize)

    # PBM command
    pbm_parser = subparsers.add_parser("pbm", help="Create PBM from text")
    pbm_parser.add_argument("text", help="Text to process")
    pbm_parser.set_defaults(func=cmd_pbm)

    # Validate command
    validate_parser = subparsers.add_parser("validate", help="Validate roundtrip")
    validate_parser.add_argument("text", help="Text to validate")
    validate_parser.set_defaults(func=cmd_validate)

    # Meter command
    meter_parser = subparsers.add_parser("meter", help="Measure abstraction")
    meter_parser.add_argument("text", help="Text to measure")
    meter_parser.set_defaults(func=cmd_meter)

    return parser


def main(argv: list[str] | None = None) -> int:
    """Main entry point."""
    parser = create_parser()
    args = parser.parse_args(argv)

    if args.command is None:
        # Default to demo
        args.text = None
        args.verbose = args.verbose
        return cmd_demo(args)

    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
