# Entity Catalog: The King in Yellow (Gutenberg #8492)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 8492 |
| Title | The King in Yellow |
| Author | Robert W. Chambers |
| Author birth | 1865-05-26 |
| Author death | 1933-12-16 |
| First published | 1895 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~70,000 |
| Structure | 10 stories |

### Classification Signals

- **Content**: Weird fiction / horror short story collection
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

Note: Collection of loosely connected stories. The first four share the motif of a maddening play called "The King in Yellow." Characters from the most notable story are cataloged.

#### Hildred Castaigne

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Hildred Castaigne |
| Name tokens | `Hildred` (label) + `Castaigne` (label, new) |
| Gender | male |
| Description | Unreliable narrator of "The Repairer of Reputations." Driven mad by reading the play. Believes himself to be the rightful King of a restored Imperial Dynasty of America. |
| Prominence | major (in his story) |

#### Cassilda

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Cassilda |
| Name tokens | `Cassilda` (label, new) |
| Gender | female |
| Description | Character from the fictional play within the book. Speaks the famous lines about Carcosa and the Hyades. A meta-fictional entity. |
| Prominence | supporting (meta-fictional) |

#### Camilla

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Camilla |
| Name tokens | `Camilla` (label) |
| Gender | female |
| Description | Another character from the fictional play, who speaks with Cassilda. Part of the frame-within-frame structure. |
| Prominence | minor (meta-fictional) |

---

## 3. Non-Fiction People Entities (y*)

#### Robert W. Chambers

| Name tokens | `Robert` (label) + `Chambers` (label) |
| Gender | male |
| Born | 1865-05-26, Brooklyn, New York |
| Died | 1933-12-16, New York City |
| Occupation | author, illustrator |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.WC | Castaigne | Fictional surname, name-only |
| AB.AB.CA.HX.WD | Cassilda | Fictional name, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Hildred | AB.AB.CA.Gm.RI | label |
| Camilla | AB.AB.CA.GY.bi | label |
| Robert | AB.AB.CA.HI.TT | label |
| chambers | AB.AB.CA.Ax.zz | noun |
