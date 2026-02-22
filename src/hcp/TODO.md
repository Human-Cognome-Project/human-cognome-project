# Python Tools TODO

## Current
- [ ] Keep `src/hcp/engine/` reference code in sync with C++ Gem decisions
- [ ] Review `src/hcp/cache/resolver.py` against C++ spec (docs/spec/cache-miss-resolver-spec.md)
- [ ] `scripts/ingest_texts.py` — verify works with current DB schema

## Format Builders (contributor tasks)
- [ ] PDF text extractor
- [ ] EPUB text extractor
- [ ] HTML text extractor
- [ ] Markdown text extractor
- [ ] Wikipedia dump builder

## Test Coverage
- [ ] Unicode edge cases for tokenizer
- [ ] Adversarial inputs for PBM reconstruction
- [ ] Token addressing collision tests
- [ ] Round-trip validation (encode → decode → compare)

## Cleanup
- [ ] Audit `src/hcp/ingest/` for stale imports and dead code
- [ ] `gutenberg_ingest_pbm.py` SyntaxWarning (line 42, invalid escape in SQL regex)
- [ ] Review module boundaries against C++ Gem architecture
