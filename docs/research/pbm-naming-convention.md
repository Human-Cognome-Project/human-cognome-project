# PBM Naming Convention

**Date:** 2026-02-18
**Status:** Decided
**Decided by:** Patrick

---

## Address Structure

```
{ns}.AB.{century}.{p4}.{p5}
```

| Segment | Role | Values |
|---------|------|--------|
| ns | PBM namespace | vA (fiction), zA (non-fiction) |
| p2 | Mode | AB (text) |
| p3 | Century | AA = BCE, AB = 1st CE, AC = 2nd CE, ... AR = 18th, AS = 19th, AT = 20th, AU = 21st |
| p4.p5 | Sequential count | Auto-assigned, next available within century. 6.25M slots per century. |

## Assignment Rules

- Ingestion pipeline assigns the next available p4.p5 when storing a PBM
- Counter is scoped to (ns, p2, p3) — i.e., per namespace + mode + century
- This convention applies across all PBM categories (fiction, non-fiction, all modes)
- Century is determined by author's birth century (if known), otherwise century of publication

## Century Encoding

Base-50 pair, sequential from AA:

| p3 | Century |
|----|---------|
| AA | BCE |
| AB | 1st CE |
| AC | 2nd CE |
| ... | ... |
| AR | 18th CE |
| AS | 19th CE |
| AT | 20th CE |
| AU | 21st CE |

Note: Century tokens may later be cross-referenced to concept anchors in hcp_core if they prove language-agnostic enough. For now, the convention stands without corresponding core tokens.

## Edge Cases (MVP dismissed)

- Multiple whitespace from formatting (centered text, tab-to-space conversion) — ignored, janky alignment is acceptable
- Unknown tokens (URLs, OCR artifacts) — wrapped in `<sic></sic>`, atomized to largest recognized sub-tokens
- Proper names — pre-registered by librarian pipeline as label tokens before engine processing

## Anchor Tokens

Start and end anchors are particle types (specialty tokens in core), not separate metadata fields. They participate in bond pairs like any other token but do not bifurcate (single bond each — start→first, last→end).
