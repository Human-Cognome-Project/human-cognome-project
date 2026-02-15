# Entity Catalog: The Turn of the Screw (Gutenberg #209)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 209 |
| Title | The Turn of the Screw |
| Author | Henry James |
| Author birth | 1843-04-15 |
| Author death | 1916-02-28 |
| First published | 1898 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~43,000 |
| Structure | 24 chapters + prologue |

### Classification Signals

- **Content**: Gothic horror / psychological novella
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Flora

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Flora |
| Name tokens | `flora` |
| Gender | female |
| Description | A beautiful, seemingly innocent young girl at Bly. The governess becomes convinced Flora is in communication with the ghost of Miss Jessel. |
| Prominence | major |

#### Miles

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Miles |
| Name tokens | `miles` |
| Gender | male |
| Status | dead |
| Description | Flora's older brother, expelled from school for unknown reasons. Charming and precocious. The governess believes he communicates with Quint's ghost. Dies in the governess's arms during a final confrontation. |
| Prominence | major |

#### Peter Quint

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Peter Quint |
| Name tokens | `quint` |
| Gender | male |
| Status | dead (ghost) |
| Description | Former valet at Bly, now a ghost. Had a corrupting influence on Miles during his life. The governess sees his apparition repeatedly. |
| Prominence | supporting |

#### Miss Jessel

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Miss Jessel |
| Name tokens | `Jessel` (label, new) |
| Gender | female |
| Status | dead (ghost) |
| Description | The previous governess at Bly, now a ghost. Had a relationship with Quint. The governess sees her apparition near Flora. |
| Prominence | supporting |

#### Mrs. Grose

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mrs. Grose |
| Name tokens | `Grose` (label) |
| Gender | female |
| Occupation | housekeeper |
| Description | The illiterate, kindly housekeeper at Bly. The governess's confidante and sounding board. Eventually takes Flora away to London. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Henry James

| Name tokens | `henry` + `James` (label) |
| Gender | male |
| Born | 1843-04-15, New York City |
| Died | 1916-02-28, London |
| Occupation | novelist |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VC | Jessel | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| flora | AB.AB.CA.CD.XU | noun |
| miles | AB.AB.CA.DY.WU | noun |
| quint | AB.AB.CA.Ee.qh | noun |
| Grose | AB.AB.CA.Gk.Is | label |
| henry | AB.AB.CA.Cb.CJ | noun |
| James | AB.AB.CA.Gp.hx | label |
