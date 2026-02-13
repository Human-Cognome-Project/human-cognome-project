#!/usr/bin/env python3
"""
Tests for session to PBM converter.

Tests session parsing, entry extraction, and bond creation.
"""

import sys
import os
import tempfile

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))


SAMPLE_SESSION = """
**Silas (02:30):** Starting work on entity registry.

**Planner (02:35):** @Silas - Good idea. I'll handle the design doc.

**Silas (02:40):** @Planner - Confirmed. Working on implementation.

**Planner (02:45):** Design doc ready. See notes directory.
"""


def test_parse_entry_regex():
    """Test entry pattern matching."""
    import re

    # Pattern from session_to_pbm.py
    entry_pattern = re.compile(r'\*\*(\w+)\s*\(([^)]+)\):\*\*\s*(.*)', re.DOTALL)

    test_line = "**Silas (02:30):** Starting work on entity registry."
    match = entry_pattern.match(test_line)

    assert match is not None, "Entry pattern should match"
    assert match.group(1) == "Silas", "Should extract agent name"
    assert match.group(2) == "02:30", "Should extract timestamp"
    assert "Starting work" in match.group(3), "Should extract content"

    print("Entry pattern test passed!")


def test_mention_extraction():
    """Test @mention extraction."""
    import re

    mention_pattern = re.compile(r'@(\w+)')

    text = "@Silas - Good idea. @Planner will review."
    mentions = mention_pattern.findall(text)

    assert "Silas" in mentions, "Should find Silas mention"
    assert "Planner" in mentions, "Should find Planner mention"
    assert len(mentions) == 2, "Should find exactly 2 mentions"

    print("Mention extraction test passed!")


def test_session_entry_dataclass():
    """Test SessionEntry creation."""
    from session_to_pbm import SessionEntry

    entry = SessionEntry(
        agent="Silas",
        timestamp="02:30",
        content="Starting work on entity registry.",
        mentions=["Planner"],
        topics=["entity", "registry"],
        line_number=1
    )

    assert entry.agent == "Silas"
    assert entry.timestamp == "02:30"
    assert "Planner" in entry.mentions
    assert entry.line_number == 1

    print("SessionEntry dataclass test passed!")


def test_agent_activity():
    """Test agent activity counting."""
    from session_to_pbm import SessionEntry, SessionPBM

    entries = [
        SessionEntry("Silas", "02:30", "msg1", [], [], 1),
        SessionEntry("Planner", "02:35", "msg2", [], [], 2),
        SessionEntry("Silas", "02:40", "msg3", [], [], 3),
        SessionEntry("Silas", "02:45", "msg4", [], [], 4),
    ]

    pbm = SessionPBM(
        entries=entries,
        agent_bonds={},
        topic_bonds={},
        mention_bonds={}
    )

    activity = pbm.agent_activity()

    assert activity.get("Silas") == 3, "Silas should have 3 entries"
    assert activity.get("Planner") == 1, "Planner should have 1 entry"

    print("Agent activity test passed!")


def run_all_tests():
    """Run all tests."""
    print("Running session_to_pbm tests...\n")

    test_parse_entry_regex()
    test_mention_extraction()
    test_session_entry_dataclass()
    test_agent_activity()

    print("\n All session_to_pbm tests passed!")


if __name__ == '__main__':
    run_all_tests()
