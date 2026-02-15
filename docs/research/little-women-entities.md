# Entity Catalog: Little Women (Gutenberg #37106)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AJ

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 37106 |
| Title | Little Women |
| Author | Louisa May Alcott |
| Author birth | 1832-11-29 |
| Author death | 1888-03-06 |
| First published | 1868 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~50,000 |
| Classification | Fiction |
| Word count | ~187,000 |
| Structure | 2 parts (Part First: 23 chapters, Part Second: 24 chapters) |

### Gutenberg Subject Headings (LCSH)

- Autobiographical fiction
- Domestic fiction
- Family -- Fiction
- Fathers and daughters -- Fiction
- March family (Fictitious characters) -- Fiction
- Mothers and daughters -- Fiction
- New England -- Fiction
- Sisters -- Fiction
- Young women -- Fiction

### Gutenberg Bookshelves

- Best Books Ever Listings
- Category: American Literature
- Category: Classics of Literature
- Category: Novels

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: American Literature" → fiction
- **Subjects**: "Domestic fiction", "Sisters -- Fiction" → fiction
- **Subject entities**: "March family (Fictitious characters)" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Jo March

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Jo March |
| Name tokens | `jo` + `march` |
| Full name | Josephine March |
| Aliases | Jo |
| Gender | female |
| Occupation | writer, governess |
| Description | Second eldest March sister. Tomboyish, fiercely independent, and aspiring writer. Sells her stories to support the family. Rejects Laurie's proposal. Eventually marries Professor Bhaer and opens a school. The protagonist. |
| Prominence | major (protagonist) |

**Key relationships:**
- `sibling_of` → Meg, Beth, Amy March
- `child_of` → Marmee, Mr. March
- `friend_of` → Theodore Laurence (Laurie)
- `spouse_of` → Friedrich Bhaer

---

#### Meg March

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Meg March |
| Name tokens | `meg` + `march` |
| Full name | Margaret March |
| Aliases | Meg |
| Gender | female |
| Description | Eldest March sister. Pretty, gentle, and domestic. Marries John Brooke and has twins. |
| Prominence | major |

**Key relationships:**
- `sibling_of` → Jo, Beth, Amy March
- `spouse_of` → John Brooke

---

#### Beth March

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Beth March |
| Name tokens | `beth` + `march` |
| Full name | Elizabeth March |
| Aliases | Beth |
| Gender | female |
| Status | dead (scarlet fever complications) |
| Description | Third March sister. Shy, sweet, musical. Contracts scarlet fever from the Hummel family. Never fully recovers and dies in Part Second. |
| Prominence | major |

---

#### Amy March

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Amy March |
| Name tokens | `Amy` (label) + `march` |
| Gender | female |
| Occupation | artist |
| Description | Youngest March sister. Artistic, ladylike, and initially vain. Travels to Europe, matures, and marries Laurie. |
| Prominence | major |

**Key relationships:**
- `sibling_of` → Jo, Meg, Beth March
- `spouse_of` → Theodore Laurence (Laurie)

---

#### Theodore Laurence (Laurie)

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Laurie |
| Name tokens | `Laurie` (label) |
| Full name | Theodore Laurence |
| Aliases | Laurie, Teddy |
| Gender | male |
| Description | The March sisters' wealthy young neighbor and Jo's best friend. Grandson of Mr. Laurence. Falls in love with Jo, is rejected, eventually marries Amy. |
| Prominence | major |

**Key relationships:**
- `friend_of` → Jo March
- `spouse_of` → Amy March
- `grandchild_of` → Mr. Laurence

---

### 2.2 Supporting Characters

#### Marmee (Mrs. March)

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Marmee |
| Name tokens | `Marmee` (label, new) |
| Gender | female |
| Description | The March sisters' loving, wise mother. Moral center of the family. Tends to the poor and teaches her daughters by example. |
| Prominence | supporting |

---

#### Mr. March

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. March |
| Name tokens | `march` |
| Gender | male |
| Occupation | chaplain (Civil War) |
| Description | Father of the March sisters. Away serving as a chaplain in the Civil War for much of the story. Falls ill; his homecoming is a pivotal moment. |
| Prominence | supporting |

---

#### Friedrich Bhaer

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Friedrich Bhaer |
| Name tokens | `Friedrich` (label) + `Bhaer` (label, new) |
| Gender | male |
| Occupation | professor |
| Description | German professor and scholar. Kind, intellectual, and poor. Courts Jo in New York. They marry and open Plumfield school together. |
| Prominence | supporting |

**Key relationships:**
- `spouse_of` → Jo March

---

#### John Brooke

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | John Brooke |
| Name tokens | `john` + `brooke` |
| Gender | male |
| Occupation | tutor |
| Status | dead (in Part Second) |
| Description | Laurie's tutor who falls in love with Meg. Steady, hardworking, and modest. They marry and have twins. |
| Prominence | supporting |

**Key relationships:**
- `spouse_of` → Meg March

---

#### Mr. Laurence

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Laurence |
| Name tokens | `laurence` |
| Gender | male |
| Occupation | gentleman |
| Description | Laurie's wealthy grandfather and the Marches' next-door neighbor. Initially gruff but becomes fond of Beth and generous to the family. |
| Prominence | supporting |

---

#### Hannah

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Hannah |
| Name tokens | `Hannah` (label) |
| Gender | female |
| Occupation | servant |
| Description | The March family's faithful servant. Loyal and long-serving. |
| Prominence | minor |

---

#### Aunt March

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Aunt March |
| Name tokens | `march` |
| Gender | female |
| Description | Mr. March's wealthy, sharp-tongued aunt. Jo reads to her for money. Leaves her estate Plumfield to Jo. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### Louisa May Alcott

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Name tokens | `Louisa` (label) + `may` + `Alcott` (label) |
| Gender | female |
| Born | 1832-11-29, Germantown, Pennsylvania |
| Died | 1888-03-06, Boston, Massachusetts |
| Occupation | novelist, poet |
| Description | American novelist. Little Women is semi-autobiographical, based on her own family. |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Uf | Marmee | Fictional nickname, name-only |
| AB.AB.CA.HX.Ug | Bhaer | German surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Amy | AB.AB.CA.GR.nm | label |
| Laurie | AB.AB.CA.Gu.Qk | label |
| Friedrich | AB.AB.CA.Gh.ma | label |
| Hannah | AB.AB.CA.Gl.EL | label |
| Theodore | AB.AB.CA.HQ.fa | label |
| Josephine | AB.AB.CA.Gq.NA | label |
| Margaret | AB.AB.CA.Gx.Vp | label |
| Elizabeth | AB.AB.CA.Gf.XT | label |
| Louisa | AB.AB.CA.Gv.tZ | label |
| Alcott | AB.AB.CA.GR.HX | label |
| jo | AB.AB.CA.Cu.gf | noun |
| march | AB.AB.CA.DP.zP | noun |
| meg | AB.AB.CA.DS.mh | noun |
| beth | AB.AB.CA.Ac.Ut | noun |
| brooke | AB.AB.CA.Am.YM | noun |
| john | AB.AB.CA.Cu.qL | noun |
| laurence | AB.AB.CA.DD.Tl | noun |
| may | AB.AB.CA.DR.lj | noun |
