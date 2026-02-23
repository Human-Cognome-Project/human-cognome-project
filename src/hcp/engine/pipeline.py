"""PBM Engine Pipeline — full roundtrip: text → PBM → text.

Usage:
    python -m hcp.engine.pipeline <gutenberg_text_file>

Processes a Gutenberg text through:
1. Vocabulary loading (hash table from DB)
2. Tokenization (engine-native lookup)
3. Disassembly (pair bond counting)
4. Storage (prefix tree schema in hcp_fic_pbm)
5. Load back from DB
6. Reassembly (graph walk, then text reconstruction)
7. Comparison (original vs reconstructed)
"""

import sys
import re
import time
from pathlib import Path

from .vocab import VocabularyCache
from .tokenizer import tokenize
from .disassemble import disassemble
from .storage import connect, store_pbm, load_pbm
from .reassemble import reassemble_sequence, reassemble_text


# Century mapping for known Gutenberg authors
# Author birth century → century code
AUTHOR_CENTURIES = {
    # 18th century authors (born 1700s)
    "Austen": "AR",
    "Shelley": "AR",
    "Scott": "AR",
    "Goethe": "AR",
    # 19th century authors (born 1800s)
    "Dickens": "AS",
    "Twain": "AS",
    "Doyle": "AS",
    "Brontë": "AS",
    "Bronte": "AS",
    "Verne": "AS",
    "Tolstoy": "AS",
    "Dostoyevsky": "AS",
    "Dostoevsky": "AS",
    "Stoker": "AS",
    "Wilde": "AS",
    "Hardy": "AS",
    "Carroll": "AS",
    "Stevenson": "AS",
    "Melville": "AS",
    "Hawthorne": "AS",
    "Poe": "AS",
    "Dumas": "AS",
    "Hugo": "AS",
    "Flaubert": "AS",
    "James": "AS",
    "London": "AS",
    "Kipling": "AS",
    "Baum": "AS",
    "Alcott": "AS",
    "Irving": "AS",
    "Barrie": "AS",
    "Conrad": "AS",
    "Maupassant": "AS",
    "Wells": "AS",
    "Chekhov": "AS",
    "Gaskell": "AS",
    "Thackeray": "AS",
    "Defoe": "AR",
    "Swift": "AR",
    "Fielding": "AR",
    "Smollett": "AR",
    "Walpole": "AR",
    "Voltaire": "AR",
    "Cervantes": "AQ",  # 16th century
    # 20th century authors (born 1900s or late 1800s publishing in 1900s)
    "Fitzgerald": "AS",
    "Joyce": "AS",
    "Christie": "AS",
    "Wodehouse": "AS",
    "Montgomery": "AS",
    "Burnett": "AS",
    "Milne": "AS",
    "Proust": "AS",
    "Rizal": "AS",
    "Chambers": "AS",
    "Le Fanu": "AS",
    "Leiber": "AT",
}

# Default century for fiction when author unknown
DEFAULT_CENTURY = "AS"  # 19th century (most Gutenberg fiction)


def strip_gutenberg_boilerplate(text):
    """Remove Gutenberg header and footer."""
    start_pattern = r'\*\*\* START OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
    match = re.search(start_pattern, text, re.IGNORECASE)
    if match:
        text = text[match.end():]
    end_pattern = r'\*\*\* END OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
    match = re.search(end_pattern, text, re.IGNORECASE)
    if match:
        text = text[:match.start()]
    return text.strip()


def guess_century(filename):
    """Guess the century code from a filename by matching known authors."""
    name = Path(filename).stem
    for author, century in AUTHOR_CENTURIES.items():
        if author.lower() in name.lower():
            return century
    return DEFAULT_CENTURY


def run_pipeline(text_file, century_code=None, log_file=None):
    """Run the full PBM roundtrip pipeline on a text file.

    Args:
        text_file: Path to Gutenberg text file
        century_code: Century code override (e.g., 'AS')
        log_file: Optional path to write detailed log

    Returns:
        dict with pipeline results and statistics
    """
    text_path = Path(text_file)
    log = open(log_file, "w") if log_file else None

    def _log(msg):
        if log:
            log.write(msg + "\n")
        print(msg)

    _log(f"=== PBM Pipeline: {text_path.name} ===")

    # Step 1: Load vocabulary
    t0 = time.time()
    _log("Loading vocabulary...")
    vocab = VocabularyCache()
    vocab.load()
    t1 = time.time()
    _log(f"  Vocabulary loaded: {len(vocab.word_to_token):,} words, "
         f"{len(vocab.label_to_token):,} labels, "
         f"{len(vocab.char_to_token):,} chars "
         f"({t1 - t0:.2f}s)")

    # Step 2: Read and clean text
    text = text_path.read_text(encoding="utf-8", errors="replace")
    text = strip_gutenberg_boilerplate(text)
    _log(f"  Text: {len(text):,} characters")

    # Step 3: Tokenize
    t2 = time.time()
    _log("Tokenizing...")
    token_ids = tokenize(text, vocab)
    t3 = time.time()
    _log(f"  Tokens: {len(token_ids):,} (incl. anchors) ({t3 - t2:.2f}s)")

    # Step 4: Disassemble
    t4 = time.time()
    _log("Disassembling...")
    pbm_data = disassemble(token_ids)
    t5 = time.time()
    _log(f"  Bonds: {len(pbm_data['bonds']):,} unique pairs from "
         f"{pbm_data['total_pairs']:,} total ({t5 - t4:.2f}s)")
    _log(f"  Unique tokens: {len(pbm_data['unique_tokens']):,}")

    # Step 5: Store to DB
    t6 = time.time()
    _log("Storing PBM...")
    if century_code is None:
        century_code = guess_century(text_path.name)
    doc_name = text_path.stem
    # Clean up filename for display name
    parts = doc_name.split("_", 1)
    if len(parts) > 1:
        doc_name = parts[1]

    conn = connect()
    try:
        doc_id = store_pbm(
            conn, doc_name, century_code, pbm_data,
            metadata={"source_file": text_path.name}
        )
    finally:
        conn.close()
    t7 = time.time()
    _log(f"  Stored as: {doc_id} ({t7 - t6:.2f}s)")

    # Step 6: Load back
    t8 = time.time()
    _log("Loading PBM from DB...")
    conn = connect()
    try:
        loaded = load_pbm(conn, doc_id)
    finally:
        conn.close()
    t9 = time.time()
    _log(f"  Loaded: {len(loaded['bonds']):,} bonds ({t9 - t8:.2f}s)")

    # Step 7: Reassemble
    t10 = time.time()
    _log("Reassembling...")
    recon_sequence = reassemble_sequence(loaded)
    recon_text = reassemble_text(recon_sequence, vocab)
    t11 = time.time()
    _log(f"  Reconstructed: {len(recon_text):,} characters ({t11 - t10:.2f}s)")

    # Step 8: Compare
    _log("\n--- Comparison ---")
    # Strip spaces from original for fair comparison (spaces are extrapolated)
    orig_no_space = text.replace(" ", "")
    recon_no_space = recon_text.replace(" ", "")

    if orig_no_space == recon_no_space:
        _log("  EXACT MATCH (ignoring space placement)")
    else:
        # Find first difference
        min_len = min(len(orig_no_space), len(recon_no_space))
        diff_pos = min_len
        for i in range(min_len):
            if orig_no_space[i] != recon_no_space[i]:
                diff_pos = i
                break

        _log(f"  Length: original={len(orig_no_space)}, "
             f"reconstructed={len(recon_no_space)}")
        _log(f"  First difference at position {diff_pos}")
        ctx = 40
        _log(f"  Original:      ...{orig_no_space[max(0,diff_pos-ctx):diff_pos+ctx]}...")
        _log(f"  Reconstructed: ...{recon_no_space[max(0,diff_pos-ctx):diff_pos+ctx]}...")

    total_time = t11 - t0
    _log(f"\n  Total pipeline time: {total_time:.2f}s")

    # Show first 500 chars of reconstruction
    _log(f"\n--- First 500 chars of reconstruction ---")
    _log(recon_text[:500])

    if log:
        log.close()

    return {
        "doc_id": doc_id,
        "token_count": len(token_ids),
        "bond_count": len(pbm_data["bonds"]),
        "unique_tokens": len(pbm_data["unique_tokens"]),
        "recon_length": len(recon_text),
        "original_length": len(text),
        "total_time": total_time,
    }


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python -m hcp.engine.pipeline <gutenberg_text_file> [century_code]")
        sys.exit(1)

    text_file = sys.argv[1]
    century = sys.argv[2] if len(sys.argv) > 2 else None
    log_path = Path(text_file).stem + "_pipeline.log"

    result = run_pipeline(text_file, century_code=century,
                          log_file=f"/tmp/{log_path}")
    print(f"\nLog written to: /tmp/{log_path}")
