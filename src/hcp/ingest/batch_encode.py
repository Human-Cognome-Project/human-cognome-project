"""
Batch encoder: Encode all Gutenberg fiction texts as PBMs.

Reuses the encoder instance (and token cache) across all texts.
Encodes, reconstructs, verifies round-trip, and stores in hcp_en_pbm.
"""

import sys
import time
from pathlib import Path

# Add ingest dir to path for local imports
sys.path.insert(0, str(Path(__file__).parent))

from text_encoder import TextEncoder, write_to_db
from reconstructor import Reconstructor
from verifier import verify, print_result

# Document ID assignments (zA.AB.CA.AA.XX)
# Ordered by Gutenberg ID
TEXTS = [
    ('00011_Alices Adventures in Wonderland.txt',         'zA.AB.CA.AA.AB'),
    ('00043_The Strange Case of Dr Jekyll and Mr Hyde.txt', 'zA.AB.CA.AA.AC'),
    # 00084 Frankenstein already encoded as zA.AB.CA.AA.AA
    ('00145_Middlemarch.txt',                              'zA.AB.CA.AA.AD'),
    ('01260_Jane Eyre An Autobiography.txt',               'zA.AB.CA.AA.AE'),
    ('01342_Pride and Prejudice.txt',                      'zA.AB.CA.AA.AF'),
    ('02554_Crime and Punishment.txt',                     'zA.AB.CA.AA.AG'),
    ('02641_A Room with a View.txt',                       'zA.AB.CA.AA.AH'),
    ('02701_Moby Dick Or The Whale.txt',                   'zA.AB.CA.AA.AI'),
    ('37106_Little Women Or Meg Jo Beth and Amy.txt',      'zA.AB.CA.AA.AJ'),
]

TEXT_DIR = Path('/opt/project/repo/data/gutenberg/texts')


def main():
    print("=" * 70)
    print("BATCH PBM ENCODING — 9 Gutenberg Fiction Texts")
    print("=" * 70)

    # Create encoder once — cache loads on first encode_file call
    encoder = TextEncoder()

    results_summary = []
    all_unknowns = {}  # word → count across all texts

    for filename, doc_id in TEXTS:
        filepath = TEXT_DIR / filename
        title = filepath.stem
        print(f"\n{'=' * 70}")
        print(f"ENCODING: {title}")
        print(f"  File: {filepath.name} ({filepath.stat().st_size:,} bytes)")
        print(f"  Doc ID: {doc_id}")
        print(f"{'=' * 70}")

        # Clear unknown log for this book
        encoder.resolver.unknown_log = []

        # Encode
        t0 = time.time()
        result = encoder.encode_file(str(filepath), doc_id=doc_id)
        t_encode = time.time() - t0

        print(f"\n  Encoding: {t_encode:.1f}s")
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

        # Write PBM file for inspection
        pbm_path = f'/tmp/pbm_{filepath.stem}.pbm'
        encoder.encode_to_file(result, pbm_path)

        # Reconstruct
        t0 = time.time()
        reconstructor = Reconstructor()
        reconstructor.load_surface_cache()
        recon_text = reconstructor.reconstruct(result.stream)
        t_recon = time.time() - t0
        print(f"  Reconstruction: {t_recon:.1f}s")

        # Verify
        original_text = filepath.read_text(encoding='utf-8')
        vresult = verify(original_text, recon_text)
        print(f"\n  VERIFICATION:")
        print(f"    Word-sequence match: {'PASS' if vresult.word_match else 'FAIL'}")
        print(f"    Matching words: {vresult.matching_words:,} / "
              f"{vresult.original_word_count:,} ({vresult.word_match_rate:.1%})")
        print(f"    Content match: {'PASS' if vresult.content_match else 'FAIL'}")

        if not vresult.word_match and vresult.mismatches:
            print(f"    First 5 mismatches:")
            for m in vresult.mismatches[:5]:
                tag = m['type']
                if tag == 'replace':
                    print(f"      [{m['orig_pos']}] {' '.join(m['orig_words'])} → "
                          f"{' '.join(m['recon_words'])}")
                elif tag == 'delete':
                    print(f"      [{m['orig_pos']}] MISSING: {' '.join(m['orig_words'])}")
                elif tag == 'insert':
                    print(f"      [{m['recon_pos']}] EXTRA: {' '.join(m['recon_words'])}")

        # Write to DB
        t0 = time.time()
        try:
            write_to_db(result)
            t_db = time.time() - t0
            print(f"  DB write: {t_db:.1f}s")
            db_ok = True
        except Exception as e:
            print(f"  DB write FAILED: {e}")
            db_ok = False

        # Track unknowns
        for entry in result.unknown_log:
            word = entry['text'].lower()
            all_unknowns[word] = all_unknowns.get(word, 0) + 1

        results_summary.append({
            'title': title,
            'doc_id': doc_id,
            'stream_entries': result.stats.total_stream_entries,
            'words': result.stats.total_words,
            'sic': result.stats.total_sic,
            'unknowns': result.stats.unknown_words,
            'word_match': vresult.word_match,
            'match_rate': vresult.word_match_rate,
            'encode_time': t_encode,
            'db_ok': db_ok,
        })

    # Final summary
    print(f"\n\n{'=' * 70}")
    print("BATCH ENCODING SUMMARY")
    print(f"{'=' * 70}")
    print(f"\n{'Title':<50} {'Entries':>8} {'Words':>8} {'Sic':>6} "
          f"{'Unknown':>7} {'Match':>6} {'Time':>6}")
    print("-" * 100)

    total_entries = 0
    total_words = 0
    total_sic = 0
    total_unknowns = 0
    all_pass = True

    for r in results_summary:
        status = "PASS" if r['word_match'] else f"{r['match_rate']:.1%}"
        if not r['word_match']:
            all_pass = False
        print(f"{r['title'][:50]:<50} {r['stream_entries']:>8,} {r['words']:>8,} "
              f"{r['sic']:>6,} {r['unknowns']:>7,} {status:>6} {r['encode_time']:>5.1f}s")
        total_entries += r['stream_entries']
        total_words += r['words']
        total_sic += r['sic']
        total_unknowns += r['unknowns']

    print("-" * 100)
    print(f"{'TOTAL':<50} {total_entries:>8,} {total_words:>8,} "
          f"{total_sic:>6,} {total_unknowns:>7,}")
    print(f"\nAll word-sequence matches: {'PASS' if all_pass else 'SOME FAILURES'}")

    # Top unknowns
    print(f"\nTop 30 unknown words (across all texts):")
    sorted_unknowns = sorted(all_unknowns.items(), key=lambda x: -x[1])
    for word, count in sorted_unknowns[:30]:
        print(f"  {word:<30} {count:>5}x")

    print(f"\nTotal unique unknowns: {len(all_unknowns):,}")

    # Write summary to file
    with open('/tmp/batch_encode_results.txt', 'w') as f:
        f.write("BATCH PBM ENCODING RESULTS\n")
        f.write(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("=" * 70 + "\n\n")
        for r in results_summary:
            f.write(f"{r['title']}\n")
            f.write(f"  Doc ID: {r['doc_id']}\n")
            f.write(f"  Stream entries: {r['stream_entries']:,}\n")
            f.write(f"  Words: {r['words']:,}\n")
            f.write(f"  Sic tokens: {r['sic']:,}\n")
            f.write(f"  Unknowns: {r['unknowns']:,}\n")
            f.write(f"  Word match: {'PASS' if r['word_match'] else 'FAIL'} "
                    f"({r['match_rate']:.1%})\n")
            f.write(f"  Encode time: {r['encode_time']:.1f}s\n")
            f.write(f"  DB write: {'OK' if r['db_ok'] else 'FAILED'}\n\n")
        f.write(f"\nTotal unique unknowns: {len(all_unknowns):,}\n")
        f.write(f"Top 50 unknowns:\n")
        for word, count in sorted_unknowns[:50]:
            f.write(f"  {word:<30} {count:>5}x\n")

    print(f"\nDetailed results: /tmp/batch_encode_results.txt")


if __name__ == '__main__':
    main()
