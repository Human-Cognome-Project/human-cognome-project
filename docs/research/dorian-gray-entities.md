# Entity Catalog: The Picture of Dorian Gray (Gutenberg #174)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 174 |
| Title | The Picture of Dorian Gray |
| Author | Oscar Wilde |
| Author birth | 1854-10-16 |
| Author death | 1900-11-30 |
| First published | 1890 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~78,000 |
| Structure | 20 chapters + preface |

### Classification Signals

- **Content**: Gothic / philosophical novel
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Dorian Gray

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Dorian Gray |
| Name tokens | `dorian` + `gray` |
| Gender | male |
| Status | dead |
| Description | A beautiful, impressionable young man whose portrait ages while he remains young. Corrupted by Lord Henry's philosophy, he sinks into a life of vice. Dies when he stabs the portrait. |
| Prominence | major (protagonist) |

#### Lord Henry Wotton

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lord Henry Wotton |
| Name tokens | `henry` + `Wotton` (label) |
| Gender | male |
| Description | Witty, cynical aristocrat whose New Hedonism philosophy corrupts Dorian. Speaks in brilliant paradoxes. Never changes or develops. |
| Prominence | major |

#### Basil Hallward

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Basil Hallward |
| Name tokens | `basil` + `Hallward` (label, new) |
| Gender | male |
| Status | dead (murdered) |
| Description | The painter who creates Dorian's portrait. Devoted to Dorian. Murdered by Dorian when he confronts him about his debauchery. |
| Prominence | major |

#### Sibyl Vane

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Sibyl Vane |
| Name tokens | `sibyl` + `vane` |
| Gender | female |
| Status | dead (suicide) |
| Occupation | actress |
| Description | Young actress in a cheap theatre whom Dorian briefly loves. Her love for Dorian destroys her art. She kills herself when Dorian cruelly rejects her. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Oscar Wilde

| Name tokens | `oscar` + `wilde` |
| Gender | male |
| Born | 1854-10-16, Dublin |
| Died | 1900-11-30, Paris |
| Occupation | author, playwright |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.WF | Hallward | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| dorian | AB.AB.CA.Bh.SV | noun |
| gray | AB.AB.CA.CS.qG | noun |
| henry | AB.AB.CA.Cb.CJ | noun |
| Wotton | AB.AB.CA.HV.qD | label |
| basil | AB.AB.CA.AZ.DU | noun |
| sibyl | AB.AB.CA.FF.Nz | noun |
| vane | AB.AB.CA.Fz.mb | noun |
| oscar | AB.AB.CA.Dx.Uv | noun |
| wilde | AB.AB.CC.BI.if | adj |
