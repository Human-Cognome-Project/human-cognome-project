"""
HCP package entry point.

Allows running: python -m hcp [command] [args]
"""
import sys
from .api.cli import main

if __name__ == "__main__":
    sys.exit(main())
