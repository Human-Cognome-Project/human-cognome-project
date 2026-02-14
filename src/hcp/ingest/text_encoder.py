"""
Text Encoder: Plain text → PBM content stream.

Main orchestrator for the PBM text encoder pipeline.
Handles: Gutenberg boilerplate stripping, structure detection (paragraphs,
chapters, sections), scanning, resolving, and building the PBM content stream.

Usage:
    encoder = TextEncoder()
    result = encoder.encode_file("data/gutenberg/texts/00084_Frankenstein...")
    # result.stream is the list of ResolvedTokens
    # result.stats has encoding statistics
"""

import re
import hashlib
from dataclasses import dataclass, field
from pathlib import Path

from scanner import Scanner, TokenType
from resolver import Resolver, ResolvedToken, MARKERS


@dataclass
class Block:
    """A structural block detected from plain text."""
    type: str           # 'paragraph', 'chapter_heading', 'section_heading', 'title_block', 'blank'
    lines: list[str]    # The raw text lines
    indent_level: int = 0
    line_number: int = 0  # Starting line number in original text

    @property
    def text(self) -> str:
        """Join lines with spaces (unwrap hard line breaks)."""
        return ' '.join(line.strip() for line in self.lines if line.strip())


@dataclass
class EncoderStats:
    """Statistics from an encoding run."""
    total_words: int = 0
    total_punctuation: int = 0
    total_markers: int = 0
    total_sic: int = 0
    total_stream_entries: int = 0
    paragraphs: int = 0
    chapters: int = 0
    unknown_words: int = 0
    exact_matches: int = 0
    case_relaxed: int = 0
    splits: int = 0


@dataclass
class EncodeResult:
    """Result of encoding a text file."""
    stream: list[ResolvedToken]
    stats: EncoderStats
    doc_id: str
    source_path: str
    source_checksum: str
    unknown_log: list[dict]


class TextEncoder:
    """Encode plain text into a PBM content stream."""

    # Patterns for detecting chapter/section headings
    CHAPTER_RE = re.compile(
        r'^(?:chapter|chap\.?)\s+(\d+|[ivxlcdm]+)\.?\s*$',
        re.IGNORECASE
    )
    LETTER_RE = re.compile(
        r'^letter\s+(\d+)\.?\s*$',
        re.IGNORECASE
    )
    CONTENTS_RE = re.compile(r'^\s*contents\s*$', re.IGNORECASE)

    # Abbreviations that take periods without ending sentences
    ABBREVIATIONS = {
        'mr', 'mrs', 'ms', 'dr', 'st', 'rev', 'prof', 'gen', 'col', 'sgt',
        'dec', 'jan', 'feb', 'mar', 'apr', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov',
        'vs', 'etc', 'approx', 'dept', 'est', 'inc', 'jr', 'sr',
        'no', 'vol', 'ed', 'trans', 'illus',
    }

    def __init__(self, db_config: dict | None = None):
        self.scanner = Scanner()
        self.resolver = Resolver(db_config)

    def encode_file(self, path: str, doc_id: str = 'zA.AB.CA.AA.AA',
                    fiction: bool = True,
                    ) -> EncodeResult:
        """Encode a plain text file into a PBM content stream.

        The ENTIRE file is encoded — nothing is stripped. A PBM is a faithful,
        lossless encoding of the source document. Boilerplate, headers, licenses
        are all part of the content. Deduplication of repeated blocks (like
        Gutenberg boilerplate) happens later via sub-PBM references, not by
        removing content at encoding time.

        Args:
            path: Path to the text file.
            doc_id: PBM document token ID.
            fiction: Whether this is fiction (affects namespace routing).

        Returns:
            EncodeResult with the complete PBM stream and statistics.
        """
        # Load resolver cache
        self.resolver.load_cache()

        # Read text — the full file, nothing stripped
        text = Path(path).read_text(encoding='utf-8')
        source_checksum = hashlib.sha256(text.encode('utf-8')).hexdigest()

        # Detect structure
        blocks = self._detect_structure(text)

        # Build PBM stream
        stream = []
        stats = EncoderStats()

        # Document start
        stream.append(self.resolver.make_marker('document_start'))

        for block in blocks:
            if block.type == 'blank':
                continue

            if block.type == 'chapter_heading':
                stream.append(self.resolver.make_marker('chapter_break'))
                stream.append(self.resolver.make_marker('title_start'))
                self._encode_block_text(block, stream, stats)
                stream.append(self.resolver.make_marker('title_end'))
                stats.chapters += 1

            elif block.type == 'section_heading':
                stream.append(self.resolver.make_marker('section_break'))
                stream.append(self.resolver.make_marker('title_start'))
                self._encode_block_text(block, stream, stats)
                stream.append(self.resolver.make_marker('title_end'))

            elif block.type == 'title_block':
                stream.append(self.resolver.make_marker('title_start'))
                # Title blocks may have multiple lines — encode with line_break between them
                for i, line in enumerate(block.lines):
                    if i > 0 and line.strip():
                        stream.append(self.resolver.make_marker('line_break'))
                    if line.strip():
                        self._encode_text(line.strip(), stream, stats)
                stream.append(self.resolver.make_marker('title_end'))

            else:  # paragraph
                stream.append(self.resolver.make_marker('paragraph_start'))
                stats.paragraphs += 1
                self._encode_block_text(block, stream, stats)
                stream.append(self.resolver.make_marker('paragraph_end'))

        # Document end
        stream.append(self.resolver.make_marker('document_end'))

        stats.total_stream_entries = len(stream)
        stats.total_markers = sum(1 for t in stream if t.source == 'marker')
        stats.unknown_words = len(self.resolver.unknown_log)

        return EncodeResult(
            stream=stream,
            stats=stats,
            doc_id=doc_id,
            source_path=str(path),
            source_checksum=source_checksum,
            unknown_log=list(self.resolver.unknown_log),
        )

    def _encode_block_text(self, block: Block, stream: list[ResolvedToken],
                           stats: EncoderStats):
        """Encode a block's text content into the stream.

        Joins lines (unwrapping hard line breaks) and processes as a single
        text segment.
        """
        text = block.text
        if text:
            self._encode_text(text, stream, stats)

    def _encode_text(self, text: str, stream: list[ResolvedToken],
                     stats: EncoderStats):
        """Scan and resolve a text string, appending results to stream."""
        raw_tokens = self.scanner.scan(text)

        for rt in raw_tokens:
            resolved = self.resolver.resolve(rt, stream)
            for token in resolved:
                stream.append(token)

                # Track stats
                if token.source == 'marker':
                    pass  # Already counted
                elif token.source == 'punctuation':
                    stats.total_punctuation += 1
                elif token.source == 'sic':
                    stats.total_sic += 1
                elif token.source == 'exact':
                    stats.exact_matches += 1
                    stats.total_words += 1
                elif token.source == 'case_relaxed':
                    stats.case_relaxed += 1
                    stats.total_words += 1
                elif token.source == 'split':
                    stats.splits += 1
                    stats.total_words += 1

    def _detect_structure(self, text: str) -> list[Block]:
        """Detect document structure from plain text.

        Groups lines into blocks separated by blank lines, then classifies
        each block as paragraph, heading, title, etc.
        """
        lines = text.split('\n')
        blocks = []
        current_lines = []
        current_start = 0

        for i, line in enumerate(lines):
            if line.strip() == '':
                if current_lines:
                    block = self._classify_block(current_lines, current_start)
                    blocks.append(block)
                    current_lines = []
                # Don't create blank blocks — paragraph gaps are implicit
            else:
                if not current_lines:
                    current_start = i + 1  # 1-indexed line number
                current_lines.append(line)

        # Handle final block
        if current_lines:
            block = self._classify_block(current_lines, current_start)
            blocks.append(block)

        return blocks

    def _classify_block(self, lines: list[str], start_line: int) -> Block:
        """Classify a block of non-blank lines.

        Returns a Block with the appropriate type.
        """
        # Single-line blocks get special classification
        if len(lines) == 1:
            stripped = lines[0].strip()

            # Chapter heading: "Chapter N" or "CHAPTER N"
            if self.CHAPTER_RE.match(stripped):
                return Block('chapter_heading', lines, line_number=start_line)

            # Letter heading: "Letter N"
            if self.LETTER_RE.match(stripped):
                return Block('section_heading', lines, line_number=start_line)

            # Contents heading
            if self.CONTENTS_RE.match(stripped):
                return Block('section_heading', lines, line_number=start_line)

        # Check for leading indentation
        indent = self._detect_indent(lines[0])

        return Block('paragraph', lines, indent_level=indent, line_number=start_line)

    def _detect_indent(self, line: str) -> int:
        """Detect indentation level from leading spaces.

        Returns indent level (0 = no indent, 1 = basic indent, etc.)
        """
        spaces = len(line) - len(line.lstrip(' '))
        if spaces < 2:
            return 0
        # Simple heuristic: 2-4 spaces = level 1, 5-8 = level 2, etc.
        return min((spaces + 1) // 4, 8) or 1

    def encode_to_file(self, result: EncodeResult, output_path: str):
        """Write PBM content stream to a file for inspection.

        Format: one entry per line, "position\ttoken_id\tsurface\tsource"
        """
        with open(output_path, 'w') as f:
            f.write(f"# PBM Content Stream: {result.source_path}\n")
            f.write(f"# Document ID: {result.doc_id}\n")
            f.write(f"# Source checksum: {result.source_checksum}\n")
            f.write(f"# Total entries: {result.stats.total_stream_entries}\n")
            f.write(f"# Words: {result.stats.total_words}, "
                    f"Punctuation: {result.stats.total_punctuation}, "
                    f"Markers: {result.stats.total_markers}, "
                    f"Sic: {result.stats.total_sic}\n")
            f.write(f"# Paragraphs: {result.stats.paragraphs}, "
                    f"Chapters: {result.stats.chapters}\n")
            f.write(f"# Unknown words: {result.stats.unknown_words}\n")
            f.write("#\n")
            f.write("# pos\ttoken_id\tsurface\tsource\n")

            for pos, entry in enumerate(result.stream, 1):
                surface = entry.surface.replace('\t', '\\t').replace('\n', '\\n')
                f.write(f"{pos}\t{entry.token_id}\t{surface}\t{entry.source}\n")

        # Write unknown words log
        if result.unknown_log:
            log_path = output_path.replace('.pbm', '_unknowns.log')
            if log_path == output_path:
                log_path = output_path + '.unknowns'
            with open(log_path, 'w') as f:
                f.write(f"# Unknown words: {len(result.unknown_log)}\n")
                for entry in result.unknown_log:
                    f.write(f"line {entry.get('line', '?')}: {entry['text']}\n")


def write_to_db(result: EncodeResult, db_config: dict | None = None):
    """Write a PBM encoding result to the hcp_en_pbm database.

    Inserts into: tokens, pbm_content, document_provenance.
    """
    import psycopg
    from datetime import date

    config = db_config or {
        'host': 'localhost',
        'dbname': 'hcp_en_pbm',
        'user': 'hcp',
        'password': 'hcp_dev',
    }

    # Decompose document ID
    doc_parts = result.doc_id.split('.')
    while len(doc_parts) < 5:
        doc_parts.append(None)
    doc_ns, doc_p2, doc_p3, doc_p4, doc_p5 = doc_parts

    conn = psycopg.connect(**config)
    try:
        with conn.cursor() as cur:
            # 1. Register document in tokens table
            cur.execute("""
                INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                ON CONFLICT (token_id) DO NOTHING
            """, (
                doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                Path(result.source_path).stem,
                'book', 'fiction',
                '{}'
            ))

            # 2. Insert content stream into pbm_content
            # Batch insert in groups of 5000
            batch = []
            for pos, entry in enumerate(result.stream, 1):
                tok_parts = entry.token_id.split('.')
                while len(tok_parts) < 5:
                    tok_parts.append(None)
                batch.append((
                    doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                    pos,
                    tok_parts[0], tok_parts[1], tok_parts[2], tok_parts[3], tok_parts[4],
                ))

                if len(batch) >= 5000:
                    cur.executemany("""
                        INSERT INTO pbm_content (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                                                 position, ns, p2, p3, p4, p5)
                        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                    """, batch)
                    batch = []

            if batch:
                cur.executemany("""
                    INSERT INTO pbm_content (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                                             position, ns, p2, p3, p4, p5)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                """, batch)

            # 3. Insert provenance
            cur.execute("""
                INSERT INTO document_provenance
                    (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                     source_type, source_path, source_format,
                     acquisition_date, source_checksum, encoder_version,
                     rights_status, license_type, reproduction_rights)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """, (
                doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,
                'file', result.source_path, 'gutenberg_txt',
                date.today(), result.source_checksum, 'plaintext-v1',
                'public_domain', 'gutenberg', 'unrestricted',
            ))

        conn.commit()
        print(f"  Written to DB: {result.doc_id} ({result.stats.total_stream_entries} entries)")

    except Exception as e:
        conn.rollback()
        raise
    finally:
        conn.close()


if __name__ == '__main__':
    import sys
    import time

    path = sys.argv[1] if len(sys.argv) > 1 else \
        'data/gutenberg/texts/00084_Frankenstein Or The Modern Prometheus.txt'

    print(f"Encoding: {path}")
    t0 = time.time()

    encoder = TextEncoder()
    result = encoder.encode_file(path)

    t1 = time.time()
    print(f"\nEncoding complete in {t1-t0:.1f}s")
    print(f"  Stream entries: {result.stats.total_stream_entries:,}")
    print(f"  Words: {result.stats.total_words:,}")
    print(f"  Punctuation: {result.stats.total_punctuation:,}")
    print(f"  Markers: {result.stats.total_markers:,}")
    print(f"  Sic tokens: {result.stats.total_sic:,}")
    print(f"  Paragraphs: {result.stats.paragraphs:,}")
    print(f"  Chapters: {result.stats.chapters:,}")
    print(f"  Unknown words: {result.stats.unknown_words:,}")
    print(f"  Resolution: {result.stats.exact_matches:,} exact, "
          f"{result.stats.case_relaxed:,} case-relaxed, "
          f"{result.stats.splits:,} splits")

    # Write to file
    output = '/tmp/frankenstein.pbm'
    encoder.encode_to_file(result, output)
    print(f"\nPBM stream written to: {output}")

    # Try writing to DB
    try:
        write_to_db(result)
        print("Written to hcp_en_pbm database")
    except Exception as e:
        print(f"DB write skipped: {e}")

    # Show some unknowns
    if result.unknown_log:
        print(f"\nFirst 20 unknown words:")
        for entry in result.unknown_log[:20]:
            print(f"  line {entry.get('line', '?')}: {entry['text']}")
