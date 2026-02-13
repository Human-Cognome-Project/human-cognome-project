"""
Pytest configuration for work/tests.

This conftest ensures that tests in work/tests/ import from work/hcp/
(the prototype implementation) rather than src/hcp/ (the production code).
"""
import sys
from pathlib import Path

# Add work/ directory to Python path BEFORE src/
# This ensures `import hcp` resolves to work/hcp/ not src/hcp/
work_dir = Path(__file__).parent.parent
if str(work_dir) not in sys.path:
    sys.path.insert(0, str(work_dir))
