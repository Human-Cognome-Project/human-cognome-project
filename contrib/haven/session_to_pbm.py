#!/usr/bin/env python3
"""
Session to PBM Converter

Converts live-session.md coordination files to HCP Pair-Bond Map structures.
Enables lossless reconstruction and queryable conversation history.

Usage:
    python session_to_pbm.py --parse FILEPATH
    python session_to_pbm.py --query "topic"
    python session_to_pbm.py --stats

Authors: Silas & Planner (DI-cognome)
Date: 2026-02-12
"""

import os
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional
from collections import defaultdict

# Add HCP to path
sys.path.insert(0, os.path.expanduser('~/shared-brain/di-cognome/repo/src'))
sys.path.insert(0, os.path.expanduser('~/shared-brain'))

try:
    from hcp.core.token_id import encode_token_id
    from hcp.core.pair_bond import PairBondMap, create_pbm_from_text
except ImportError:
    print("Warning: HCP modules not available, using stubs")
    PairBondMap = None
    create_pbm_from_text = None

from entity_registry import get_token_id, resolve_entity

# Default session file
DEFAULT_SESSION = os.path.expanduser("~/shared-brain/coordination/live-session.md")


@dataclass
class SessionEntry:
    """A single entry in the session log."""
    agent: str
    timestamp: str
    content: str
    mentions: list[str] = field(default_factory=list)
    topics: list[str] = field(default_factory=list)
    line_number: int = 0

    @property
    def agent_token(self) -> Optional[str]:
        """Get HCP token for the agent."""
        return get_token_id(self.agent)


@dataclass
class SessionPBM:
    """PBM representation of a session."""
    entries: list[SessionEntry]
    agent_bonds: dict  # agent -> agent: count
    topic_bonds: dict  # topic -> topic: count
    mention_bonds: dict  # entity mentions
    text_pbm: Optional[PairBondMap] = None

    def agent_activity(self) -> dict[str, int]:
        """Count entries per agent."""
        counts = defaultdict(int)
        for entry in self.entries:
            counts[entry.agent] += 1
        return dict(counts)

    def conversation_flow(self) -> list[tuple[str, str, int]]:
        """Get agent-to-agent conversation flow."""
        flow = []
        for i in range(len(self.entries) - 1):
            from_agent = self.entries[i].agent
            to_agent = self.entries[i + 1].agent
            flow.append((from_agent, to_agent, i))
        return flow


class SessionParser:
    """Parse live-session.md format."""

    # Pattern: **Agent (HH:MM):** or **Agent (HH:MM:SS):**
    ENTRY_PATTERN = re.compile(
        r'\*\*(\w+)\s*\((\d{1,2}:\d{2}(?::\d{2})?)\):\*\*\s*(.*)',
        re.DOTALL
    )

    # Pattern for @mentions
    MENTION_PATTERN = re.compile(r'@(\w+)')

    def __init__(self, filepath: str = None):
        self.filepath = filepath or DEFAULT_SESSION
        self.entries: list[SessionEntry] = []

    def parse(self) -> list[SessionEntry]:
        """Parse the session file."""
        if not os.path.exists(self.filepath):
            print(f"Session file not found: {self.filepath}")
            return []

        with open(self.filepath, 'r') as f:
            content = f.read()

        self.entries = []
        lines = content.split('\n')

        current_entry = None
        current_content = []

        for i, line in enumerate(lines):
            # Check for new entry
            match = self.ENTRY_PATTERN.match(line)
            if match:
                # Save previous entry
                if current_entry:
                    current_entry.content = '\n'.join(current_content).strip()
                    current_entry.mentions = self._extract_mentions(current_entry.content)
                    current_entry.topics = self._extract_topics(current_entry.content)
                    self.entries.append(current_entry)

                # Start new entry
                agent, timestamp, first_line = match.groups()
                current_entry = SessionEntry(
                    agent=agent,
                    timestamp=timestamp,
                    content="",
                    line_number=i + 1
                )
                current_content = [first_line] if first_line.strip() else []
            elif current_entry:
                # Continue current entry
                if line.startswith('---'):
                    # Entry separator - save current
                    if current_content:
                        current_entry.content = '\n'.join(current_content).strip()
                        current_entry.mentions = self._extract_mentions(current_entry.content)
                        current_entry.topics = self._extract_topics(current_entry.content)
                        self.entries.append(current_entry)
                    current_entry = None
                    current_content = []
                else:
                    current_content.append(line)

        # Don't forget last entry
        if current_entry and current_content:
            current_entry.content = '\n'.join(current_content).strip()
            current_entry.mentions = self._extract_mentions(current_entry.content)
            current_entry.topics = self._extract_topics(current_entry.content)
            self.entries.append(current_entry)

        return self.entries

    def _extract_mentions(self, text: str) -> list[str]:
        """Extract @mentions from text."""
        return self.MENTION_PATTERN.findall(text)

    def _extract_topics(self, text: str) -> list[str]:
        """Extract likely topics from text (simplified)."""
        topics = []
        # Look for code blocks, headers, bold text
        headers = re.findall(r'\*\*([^*]+)\*\*', text)
        topics.extend([h.strip()[:50] for h in headers if len(h.strip()) > 3])
        return topics[:5]  # Limit


class SessionToPBM:
    """Convert parsed session to PBM structures."""

    def __init__(self, entries: list[SessionEntry]):
        self.entries = entries

    def build(self) -> SessionPBM:
        """Build PBM from session entries."""
        agent_bonds = defaultdict(int)
        topic_bonds = defaultdict(int)
        mention_bonds = defaultdict(int)

        # Build agent conversation bonds
        for i in range(len(self.entries) - 1):
            from_agent = self.entries[i].agent
            to_agent = self.entries[i + 1].agent
            agent_bonds[(from_agent, to_agent)] += 1

        # Build mention bonds
        for entry in self.entries:
            for mention in entry.mentions:
                mention_bonds[(entry.agent, mention)] += 1

        # Build topic bonds (consecutive topics)
        all_topics = []
        for entry in self.entries:
            all_topics.extend(entry.topics)

        for i in range(len(all_topics) - 1):
            topic_bonds[(all_topics[i], all_topics[i + 1])] += 1

        # Build text PBM if available
        text_pbm = None
        if create_pbm_from_text:
            full_text = '\n'.join(e.content for e in self.entries)
            text_pbm = create_pbm_from_text(full_text)

        return SessionPBM(
            entries=self.entries,
            agent_bonds=dict(agent_bonds),
            topic_bonds=dict(topic_bonds),
            mention_bonds=dict(mention_bonds),
            text_pbm=text_pbm,
        )


def parse_session(filepath: str = None) -> SessionPBM:
    """Convenience function to parse and convert session."""
    parser = SessionParser(filepath)
    entries = parser.parse()
    converter = SessionToPBM(entries)
    return converter.build()


def query_session(query: str, session_pbm: SessionPBM) -> list[SessionEntry]:
    """Find entries matching a query."""
    query_lower = query.lower()
    matches = []
    for entry in session_pbm.entries:
        if query_lower in entry.content.lower():
            matches.append(entry)
        elif query_lower in entry.agent.lower():
            matches.append(entry)
        elif any(query_lower in m.lower() for m in entry.mentions):
            matches.append(entry)
    return matches


def session_stats(session_pbm: SessionPBM) -> dict:
    """Get statistics about a session."""
    activity = session_pbm.agent_activity()
    flow = session_pbm.conversation_flow()

    # Count turn-taking
    turns = defaultdict(int)
    for from_a, to_a, _ in flow:
        if from_a != to_a:
            turns[(from_a, to_a)] += 1

    return {
        "total_entries": len(session_pbm.entries),
        "agents": list(activity.keys()),
        "activity": activity,
        "turn_taking": dict(turns),
        "unique_mentions": len(session_pbm.mention_bonds),
        "text_bonds": session_pbm.text_pbm.unique_bonds if session_pbm.text_pbm else 0,
    }


# === CLI ===

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Session to PBM Converter")
    parser.add_argument("--parse", type=str, nargs="?", const=DEFAULT_SESSION,
                        help="Parse session file (default: live-session.md)")
    parser.add_argument("--stats", action="store_true",
                        help="Show session statistics")
    parser.add_argument("--query", type=str,
                        help="Query session for topic/content")
    parser.add_argument("--agents", action="store_true",
                        help="Show agent activity")
    parser.add_argument("--flow", action="store_true",
                        help="Show conversation flow")
    args = parser.parse_args()

    filepath = args.parse or DEFAULT_SESSION

    if args.parse or args.stats or args.query or args.agents or args.flow:
        print(f"Parsing: {filepath}")
        session = parse_session(filepath)
        print(f"Found {len(session.entries)} entries\n")

        if args.stats:
            stats = session_stats(session)
            print("=== Session Statistics ===")
            print(f"Total entries: {stats['total_entries']}")
            print(f"Agents: {', '.join(stats['agents'])}")
            print(f"Text bonds: {stats['text_bonds']}")
            print("\nActivity:")
            for agent, count in stats['activity'].items():
                print(f"  {agent}: {count} entries")
            if stats['turn_taking']:
                print("\nTurn-taking:")
                for (f, t), count in sorted(stats['turn_taking'].items(), key=lambda x: -x[1])[:5]:
                    print(f"  {f} -> {t}: {count}")

        if args.query:
            matches = query_session(args.query, session)
            print(f"=== Query: '{args.query}' ({len(matches)} matches) ===")
            for entry in matches[:10]:
                preview = entry.content[:100].replace('\n', ' ')
                print(f"\n[{entry.agent} {entry.timestamp}]")
                print(f"  {preview}...")

        if args.agents:
            print("=== Agent Activity ===")
            activity = session.agent_activity()
            for agent, count in sorted(activity.items(), key=lambda x: -x[1]):
                token = get_token_id(agent) or "unknown"
                print(f"{agent}: {count} entries (token: {token})")

        if args.flow:
            print("=== Conversation Flow (last 20) ===")
            flow = session.conversation_flow()[-20:]
            for from_a, to_a, i in flow:
                if from_a != to_a:
                    print(f"  {from_a} -> {to_a}")

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
