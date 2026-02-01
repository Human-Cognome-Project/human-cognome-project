# Data Sources

This directory contains manifests and scripts for external data used by the HCP. Large data files are not stored in git — they are downloaded by `fetch.sh` into `sources/data/` (gitignored).

## Planned Sources

### Kaikki (Wiktionary extracts)
- **What:** Machine-readable dictionary data extracted from Wiktionary
- **Use:** Word definitions, etymologies, cross-lingual mappings for building covalent bonding tables
- **Source:** https://kaikki.org

### Word frequency lists
- **What:** Word frequency data by language
- **Use:** Establishing bonding-strength baselines for FBR calibration
- **Source:** TBD (OpenSubtitles, Google Ngrams, or similar)

### NSM primitive reference
- **What:** Canonical list of ~65 NSM primitives with definitions and cross-linguistic equivalents
- **Use:** Populating the `00`-mode namespace
- **Source:** NSM-Approach.net and published literature

### Encoding tables
- **What:** Unicode character database, ASCII tables
- **Use:** Covalent bonding tables for text mode byte-code mapping
- **Source:** unicode.org
