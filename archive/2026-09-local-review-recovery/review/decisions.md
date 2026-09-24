# Review decisions (Patrick, 2026-08-30)

1. **NAPIER survives.** Not a proprietary inference engine — inference is what it's all about. Expansion recovered from the old engine ROADMAP: "Not Another Proprietary Inference Engine, Really!"
2. **The anti-statistical-AI stance stays**, upgraded from stance to measured claim: statistical training welds sampling frequency to amount (likelihood → truth-weight) — the exact ledger error.
3. **Archive lives in-repo** (`archive/`); issues close en masse with an announcement pointer. "Planning a git is a necessary evil."
4. **O/o alphabet drift: CORRECT IT.** Remap O/o-bearing ids into the canonical 50-letter space at extraction (mapping table kept). Rationale: addresses must be easily human-parsable — O reads as 0 and creates friction; 50 is a clean number. May rebase later; for now clean-for-humans governs.
5. **Essays: reviewer's discretion.** No engine-build value; they exist for whoever writes the how-this-came-to-be story. Archive verbatim in the history layer; gathering into Gists etc. is fine later.
6. **Private communications do not belong in the git.** Letters and discussions with specific people are private communications, not public records. Consequence executed in Pass 7: `docs/outreach-drafts-2026-06-11.md` and `docs/entry-points/skavysh-physics-lens.md` removed from the tree; named-contact content in the archived `validation.md` redacted, with the redaction declared in the archive ledger. (History scrub not performed — tree removal only; say the word if history matters.)
7. **Recruitment threads**: parked; any future approach happens on the new basis, and named contacts stay out of public records per (6).
