"""
Batch encoder: Encode Gutenberg fiction texts as PBMs.

Coordinates with the librarian specialist via /tmp/hcp_ingest_delta.log.
Reuses encoder + reconstructor caches across all texts.

Usage:
    python3 batch_encode.py              # Normal mode: wait for librarian
    python3 batch_encode.py --no-wait    # Encode all available, don't wait
    python3 batch_encode.py --start N    # Start from text N (1-indexed)
"""

import sys
import os
import time
import subprocess
from pathlib import Path
from datetime import datetime

# Add ingest dir to path for local imports
sys.path.insert(0, str(Path(__file__).parent))

from text_encoder import TextEncoder, write_to_db
from reconstructor import Reconstructor
from verifier import verify

TEXT_DIR = Path('/opt/project/repo/data/gutenberg/texts')
DELTA_LOG = '/tmp/hcp_ingest_delta.log'
RESULTS_LOG = '/tmp/batch_encode_results.txt'

# Already encoded document IDs (from first batch)
DONE_GIDS = {11, 43, 84, 145, 1260, 1342, 2554, 2641, 2701, 37106}

# Base-52 charset for token ID generation
B52 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'


def p5_for_index(idx):
    """Generate 2-char p5 value for a sequential index."""
    return B52[idx // 52] + B52[idx % 52]


def build_text_list():
    """Build ordered list of texts to encode (smallest first), with doc IDs."""
    texts = []
    for f in TEXT_DIR.iterdir():
        if f.suffix == '.txt':
            gid = int(f.name.split('_')[0])
            texts.append((f.stat().st_size, gid, f.name))
    texts.sort()  # Sort by file size

    # Assign sequential doc IDs for new texts only
    # First batch used positions 0-9 (AA-AJ), new texts start at 10 (AK)
    result = []
    idx = 0
    for size, gid, name in texts:
        if gid in DONE_GIDS:
            continue
        p5 = p5_for_index(idx + 10)
        result.append({
            'gid': gid,
            'name': name,
            'size': size,
            'doc_id': f'zA.AB.CA.AA.{p5}',
            'p5': p5,
            'idx': idx,
        })
        idx += 1
    return result


def log_delta(msg):
    """Append a timestamped message to the delta log."""
    ts = datetime.now().strftime('%H:%M:%S')
    line = f"{ts} {msg}\n"
    with open(DELTA_LOG, 'a') as f:
        f.write(line)
    print(f"  [LOG] {line.strip()}")


def get_lib_done():
    """Get set of titles the librarian has completed."""
    done = set()
    if not os.path.exists(DELTA_LOG):
        return done
    with open(DELTA_LOG) as f:
        for line in f:
            if 'LIB DONE:' in line:
                # Extract title after "LIB DONE: "
                title = line.split('LIB DONE:')[1].strip()
                # Strip any trailing stats (after " — ")
                if ' — ' in title:
                    title = title.split(' — ')[0].strip()
                # Also try matching on Gutenberg ID
                done.add(title.lower())
    return done


def get_pbm_done():
    """Get set of titles already PBM-encoded (from delta log)."""
    done = set()
    if not os.path.exists(DELTA_LOG):
        return done
    with open(DELTA_LOG) as f:
        for line in f:
            if 'PBM DONE:' in line:
                title = line.split('PBM DONE:')[1].strip()
                if ' — ' in title:
                    title = title.split(' — ')[0].strip()
                done.add(title.lower())
    return done


def title_from_filename(name):
    """Extract a readable title from a Gutenberg filename."""
    return name.rsplit('.', 1)[0]  # Strip .txt


def match_title(text_entry, lib_done_set):
    """Check if a text has been completed by the librarian."""
    name = text_entry['name']
    title = title_from_filename(name).lower()
    stem = name.rsplit('.', 1)[0].lower()
    gid_str = str(text_entry['gid'])

    for done_title in lib_done_set:
        dt = done_title.lower()
        # Match by full filename stem
        if dt == stem or dt == title:
            return True
        # Match by Gutenberg ID prefix
        if dt.startswith(gid_str + '_') or gid_str.zfill(5) + '_' in dt:
            return True
        # Fuzzy: check if the GID appears
        if gid_str in dt and any(word in dt for word in stem.split('_')[1:3]):
            return True
    return False


def encode_one(encoder, reconstructor, text_entry):
    """Encode a single text. Returns result dict."""
    filepath = TEXT_DIR / text_entry['name']
    doc_id = text_entry['doc_id']
    title = title_from_filename(text_entry['name'])

    # Clear unknown log
    encoder.resolver.unknown_log = []

    # Encode
    t0 = time.time()
    result = encoder.encode_file(str(filepath), doc_id=doc_id)
    t_encode = time.time() - t0

    # Reconstruct
    recon_text = reconstructor.reconstruct(result.stream)

    # Verify
    original_text = filepath.read_text(encoding='utf-8')
    vresult = verify(original_text, recon_text)

    # Write to DB
    db_ok = False
    try:
        write_to_db(result)
        db_ok = True
    except Exception as e:
        print(f"  DB write FAILED: {e}")

    return {
        'title': title,
        'doc_id': doc_id,
        'gid': text_entry['gid'],
        'size': text_entry['size'],
        'stream_entries': result.stats.total_stream_entries,
        'words': result.stats.total_words,
        'punctuation': result.stats.total_punctuation,
        'markers': result.stats.total_markers,
        'sic': result.stats.total_sic,
        'unknowns': result.stats.unknown_words,
        'unknown_list': list(result.unknown_log),
        'paragraphs': result.stats.paragraphs,
        'chapters': result.stats.chapters,
        'exact': result.stats.exact_matches,
        'case_relaxed': result.stats.case_relaxed,
        'splits': result.stats.splits,
        'word_match': vresult.word_match,
        'match_rate': vresult.word_match_rate,
        'match_count': vresult.matching_words,
        'total_words_v': vresult.original_word_count,
        'mismatches': vresult.mismatches[:5] if not vresult.word_match else [],
        'encode_time': t_encode,
        'db_ok': db_ok,
    }


def print_result(r):
    """Print encoding result for one text."""
    status = "PASS" if r['word_match'] else f"FAIL ({r['match_rate']:.4%})"
    print(f"  Stream: {r['stream_entries']:>9,}  Words: {r['words']:>8,}  "
          f"Sic: {r['sic']:>6,}  Unknown: {r['unknowns']:>5,}  "
          f"Match: {status}  Time: {r['encode_time']:.1f}s")
    if not r['word_match'] and r['mismatches']:
        for m in r['mismatches'][:3]:
            tag = m['type']
            if tag == 'replace':
                print(f"    [{m['orig_pos']}] {' '.join(m['orig_words'])} → "
                      f"{' '.join(m['recon_words'])}")


def write_summary(results, all_unknowns):
    """Write cumulative results summary to file."""
    with open(RESULTS_LOG, 'w') as f:
        f.write(f"BATCH PBM ENCODING RESULTS (100 texts)\n")
        f.write(f"Updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("=" * 80 + "\n\n")

        f.write(f"{'#':>3} {'GID':>5} {'Title':<45} {'Entries':>9} {'Words':>8} "
                f"{'Sic':>6} {'Unk':>5} {'Match':>6} {'Time':>5}\n")
        f.write("-" * 100 + "\n")

        t_entries = t_words = t_sic = t_unk = 0
        all_pass = True
        for i, r in enumerate(results):
            s = "PASS" if r['word_match'] else f"{r['match_rate']:.1%}"
            if not r['word_match']:
                all_pass = False
            short = r['title'][:45]
            f.write(f"{i+1:>3} {r['gid']:>5} {short:<45} {r['stream_entries']:>9,} "
                    f"{r['words']:>8,} {r['sic']:>6,} {r['unknowns']:>5,} "
                    f"{s:>6} {r['encode_time']:>4.1f}s\n")
            t_entries += r['stream_entries']
            t_words += r['words']
            t_sic += r['sic']
            t_unk += r['unknowns']

        f.write("-" * 100 + "\n")
        f.write(f"    TOTAL{' ':<45} {t_entries:>9,} {t_words:>8,} "
                f"{t_sic:>6,} {t_unk:>5,}\n\n")
        f.write(f"All word-sequence matches: {'PASS' if all_pass else 'SOME FAILURES'}\n")
        f.write(f"Texts encoded: {len(results)}\n\n")

        sorted_unk = sorted(all_unknowns.items(), key=lambda x: -x[1])
        f.write(f"Top 50 unknowns ({len(all_unknowns):,} unique):\n")
        for word, count in sorted_unk[:50]:
            f.write(f"  {word:<35} {count:>5}x\n")


def main():
    no_wait = '--no-wait' in sys.argv
    start_at = 0
    for arg in sys.argv[1:]:
        if arg.startswith('--start'):
            start_at = int(sys.argv[sys.argv.index(arg) + 1]) - 1

    texts = build_text_list()
    print(f"PBM BATCH ENCODER — {len(texts)} texts to encode")
    print(f"Mode: {'no-wait' if no_wait else 'coordinated with librarian'}")
    if start_at:
        print(f"Starting at text #{start_at + 1}")
    print()

    # Initialize encoder and reconstructor (cache loads once)
    encoder = TextEncoder()
    encoder.resolver.load_cache()

    reconstructor = Reconstructor()
    reconstructor.load_surface_cache()

    results = []
    all_unknowns = {}
    encoded_count = 0
    commit_batch = []

    # Check what's already been encoded in this session
    pbm_done = get_pbm_done()

    for i, text_entry in enumerate(texts):
        if i < start_at:
            continue

        title = title_from_filename(text_entry['name'])

        # Skip if already done this session
        if title.lower() in pbm_done:
            print(f"[{i+1}/{len(texts)}] SKIP (already done): {title}")
            continue

        # Wait for librarian if not in no-wait mode
        if not no_wait:
            while True:
                lib_done = get_lib_done()
                # Count how many texts ahead of us the librarian has completed
                ahead = 0
                for j in range(i, min(i + 10, len(texts))):
                    if match_title(texts[j], lib_done):
                        ahead += 1
                    else:
                        break

                if ahead >= 1:  # At least current text is done
                    break

                print(f"  Waiting for librarian... ({len(lib_done)} done, need text #{i+1})")
                time.sleep(15)

        # Encode
        print(f"\n[{i+1}/{len(texts)}] {title}")
        print(f"  File: {text_entry['name']} ({text_entry['size']:,} bytes)")
        print(f"  Doc ID: {text_entry['doc_id']}")

        log_delta(f"PBM START: {title}")

        try:
            r = encode_one(encoder, reconstructor, text_entry)
            print_result(r)
            results.append(r)
            encoded_count += 1
            commit_batch.append(title)

            # Track unknowns
            for entry in r['unknown_list']:
                word = entry['text'].lower()
                all_unknowns[word] = all_unknowns.get(word, 0) + 1

            match_str = "100%" if r['word_match'] else f"{r['match_rate']:.4%}"
            log_delta(f"PBM DONE: {title} — {r['words']:,} words, {match_str}, "
                      f"{r['encode_time']:.1f}s, {r['unknowns']} unknown")

        except Exception as e:
            print(f"  ERROR: {e}")
            log_delta(f"PBM ERROR: {title} — {e}")
            import traceback
            traceback.print_exc()
            continue

        # Write summary every 5 texts
        if encoded_count % 5 == 0:
            write_summary(results, all_unknowns)

    # Final summary
    if results:
        write_summary(results, all_unknowns)
        print(f"\n{'=' * 70}")
        print(f"COMPLETE: {encoded_count} texts encoded")
        t_entries = sum(r['stream_entries'] for r in results)
        t_words = sum(r['words'] for r in results)
        all_pass = all(r['word_match'] for r in results)
        print(f"Total: {t_entries:,} stream entries, {t_words:,} words")
        print(f"All matches: {'PASS' if all_pass else 'SOME FAILURES'}")
        print(f"Unique unknowns: {len(all_unknowns):,}")
        print(f"Results: {RESULTS_LOG}")


if __name__ == '__main__':
    main()
