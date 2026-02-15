# Entity Catalog: Pride and Prejudice (Gutenberg #1342)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AF

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 1342 |
| Title | Pride and Prejudice |
| Author | Jane Austen |
| Author birth | 1775-12-16 |
| Author death | 1817-07-18 |
| First published | 1813 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~83,000 |
| Classification | Fiction |
| Word count | ~122,000 |
| Structure | 3 volumes, 61 chapters |

### Gutenberg Subject Headings (LCSH)

- Courtship -- Fiction
- Domestic fiction
- England -- Fiction
- Love stories
- Sisters -- Fiction
- Social classes -- Fiction
- Young women -- Fiction

### Gutenberg Bookshelves

- Best Books Ever Listings
- Category: British Literature
- Category: Classics of Literature
- Category: Novels
- Category: Romance
- Harvard Classics

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: Romance" → fiction
- **Subjects**: "Domestic fiction", "Love stories" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Elizabeth Bennet

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Elizabeth Bennet |
| Name tokens | `Elizabeth` (label) + `bennet` |
| Aliases | Eliza, Lizzy |
| Gender | female |
| Description | Second eldest Bennet daughter. Intelligent, witty, and independent-minded. Initially prejudiced against Darcy, she gradually recognizes her misjudgment and falls in love with him. The protagonist. |
| Prominence | major (protagonist) |

**Key relationships:**
- `spouse_of` → Fitzwilliam Darcy
- `sibling_of` → Jane, Mary, Kitty, Lydia Bennet
- `child_of` → Mr. Bennet, Mrs. Bennet
- `friend_of` → Charlotte Lucas

---

#### Fitzwilliam Darcy

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Fitzwilliam Darcy |
| Name tokens | `Fitzwilliam` (label) + `darcy` |
| Aliases | Mr. Darcy |
| Gender | male |
| Occupation | gentleman, landowner of Pemberley |
| Description | Wealthy, proud gentleman from Derbyshire. Initially appears arrogant but is revealed to be honorable, generous, and deeply in love with Elizabeth. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Elizabeth Bennet
- `sibling_of` → Georgiana Darcy
- `friend_of` → Charles Bingley
- `enemy_of` → George Wickham

---

#### Jane Bennet

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jane Bennet |
| Name tokens | `jane` + `bennet` |
| Gender | female |
| Description | Eldest Bennet daughter. Beautiful, kind, and sees the best in everyone. Falls in love with Bingley. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Charles Bingley
- `sibling_of` → Elizabeth Bennet

---

#### Charles Bingley

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Charles Bingley |
| Name tokens | `Charles` (label) + `Bingley` (label) |
| Gender | male |
| Occupation | gentleman |
| Description | Darcy's amiable, wealthy friend who leases Netherfield Park. Falls in love with Jane Bennet. Easily influenced by his sisters and Darcy. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Jane Bennet
- `friend_of` → Fitzwilliam Darcy
- `sibling_of` → Caroline Bingley

---

### 2.2 Supporting Characters

#### George Wickham

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | George Wickham |
| Name tokens | `George` (label) + `Wickham` (label) |
| Gender | male |
| Occupation | militia officer |
| Description | Charming but deceitful officer. Son of Darcy's father's steward. Attempted to elope with Georgiana Darcy. Eventually elopes with Lydia Bennet. |
| Prominence | supporting |

---

#### Mr. Bennet

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Bennet |
| Name tokens | `bennet` |
| Gender | male |
| Occupation | gentleman |
| Description | Father of the five Bennet sisters. Witty, sardonic, and intellectual. Retreats to his library to escape his wife's nerves. |
| Prominence | supporting |

---

#### Mrs. Bennet

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mrs. Bennet |
| Name tokens | `bennet` |
| Gender | female |
| Description | Mother of the five sisters. Obsessed with marrying off her daughters. Nervous, foolish, and often embarrassing. |
| Prominence | supporting |

---

#### Mr. Collins

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Collins |
| Name tokens | `collins` |
| Gender | male |
| Occupation | clergyman |
| Description | The Bennet estate's heir (entailment). Pompous, obsequious clergyman patronized by Lady Catherine de Bourgh. Marries Charlotte Lucas after Elizabeth refuses him. |
| Prominence | supporting |

---

#### Charlotte Lucas

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Charlotte Lucas |
| Name tokens | `charlotte` + `Lucas` (label) |
| Gender | female |
| Description | Elizabeth's practical, sensible best friend. Marries Mr. Collins for security rather than love. |
| Prominence | supporting |

---

#### Lady Catherine de Bourgh

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lady Catherine de Bourgh |
| Name tokens | `lady` + `Catherine` (label) + `de` + `Bourgh` (label, new) |
| Gender | female |
| Occupation | noblewoman |
| Description | Darcy's wealthy, imperious aunt. Patroness of Mr. Collins. Opposes Elizabeth's match with Darcy, inadvertently hastening it. |
| Prominence | supporting |

---

#### Lydia Bennet

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lydia Bennet |
| Name tokens | `lydia` + `bennet` |
| Gender | female |
| Description | Youngest Bennet sister. Wild, flirtatious, and thoughtless. Elopes with Wickham, causing a family scandal. |
| Prominence | supporting |

---

#### Caroline Bingley

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Caroline Bingley |
| Name tokens | `caroline` + `Bingley` (label) |
| Gender | female |
| Description | Bingley's snobbish sister. Pursues Darcy and schemes to separate Jane and Bingley. |
| Prominence | supporting |

---

#### Georgiana Darcy

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Georgiana Darcy |
| Name tokens | `Georgiana` (label) + `darcy` |
| Gender | female |
| Description | Darcy's shy, sweet younger sister. Nearly eloped with Wickham before the novel begins. |
| Prominence | minor |

---

### 2.3 Minor Characters

**Mr. Gardiner:** Uncle of the Bennet sisters. Sensible London businessman.
**Mrs. Gardiner:** Wife of Mr. Gardiner. A voice of reason.
**Kitty Bennet:** Fourth Bennet sister, follower of Lydia.
**Mary Bennet:** Middle Bennet sister, bookish and moralizing.

---

## 3. Fiction Place Entities (t*)

#### Pemberley

| Field | Value |
|-------|-------|
| Sub-type | CA (building/structure) |
| Name tokens | `Pemberley` (label, new) |
| Description | Darcy's grand estate in Derbyshire. Elizabeth's visit there begins to change her opinion of him. |

---

#### Netherfield Park

| Field | Value |
|-------|-------|
| Sub-type | CA (building/structure) |
| Name tokens | `Netherfield` (label) |
| Description | Estate near Longbourn leased by Bingley. |

---

#### Longbourn

| Field | Value |
|-------|-------|
| Sub-type | AA (settlement) |
| Name tokens | `Longbourn` (label, new) |
| Description | Village in Hertfordshire where the Bennets live. |

---

#### Rosings Park

| Field | Value |
|-------|-------|
| Sub-type | CA (building/structure) |
| Name tokens | `Rosings` (label, new) |
| Description | Lady Catherine de Bourgh's estate in Kent. |

---

#### Meryton

| Field | Value |
|-------|-------|
| Sub-type | AA (settlement) |
| Name tokens | `Meryton` (label, new) |
| Description | Market town near Longbourn where the militia is stationed. |

---

#### Hunsford

| Field | Value |
|-------|-------|
| Sub-type | AA (settlement) |
| Name tokens | `Hunsford` (label, new) |
| Description | Village near Rosings where Mr. Collins has his parsonage. |

---

## 4. Non-Fiction People Entities (y*)

### 4.1 Author

#### Jane Austen

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jane Austen |
| Name tokens | `jane` + `Austen` (label) |
| Gender | female |
| Born | 1775-12-16, Steventon, Hampshire |
| Died | 1817-07-18, Winchester |
| Occupation | novelist |
| Description | English novelist known for social commentary and wit. |

---

## 5. Non-Fiction Place Entities (x*)

Derbyshire, Hertfordshire, and Brighton already exist or can be reused. London exists from Frankenstein.

---

## 6. Label Token Audit

### 6.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Tx | Pemberley | Fictional estate name |
| AB.AB.CA.HX.Ty | Longbourn | Fictional village name |
| AB.AB.CA.HX.Tz | Rosings | Fictional estate name |
| AB.AB.CA.HX.UA | Meryton | Fictional town name |
| AB.AB.CA.HX.UB | Hunsford | Fictional village name |
| AB.AB.CA.HX.UC | Bourgh | Fictional surname component |

### 6.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Elizabeth | AB.AB.CA.Gf.XT | label |
| Fitzwilliam | AB.AB.CA.Gh.IZ | label |
| Bingley | AB.AB.CA.GV.fr | label |
| Wickham | AB.AB.CA.HV.EG | label |
| Lucas | AB.AB.CA.Gv.zw | label |
| Catherine | AB.AB.CA.GZ.Ky | label |
| Gardiner | AB.AB.CA.Gi.TQ | label |
| Georgiana | AB.AB.CA.Gi.ms | label |
| Charles | AB.AB.CA.GZ.nI | label |
| George | AB.AB.CA.Gi.mc | label |
| Austen | AB.AB.CA.GT.Fl | label |
| Netherfield | AB.AB.CA.HB.qr | label |
| Derbyshire | AB.AB.CA.Gd.Qc | label |
| Hertfordshire | AB.AB.CA.Gm.GG | label |
| Brighton | AB.AB.CA.GX.IH | label |
| bennet | AB.AB.CA.Ab.jl | noun |
| darcy | AB.AB.CA.BW.CU | noun |
| collins | AB.AB.CA.BH.dC | noun |
| jane | AB.AB.CA.Ct.Tz | noun |
| charlotte | AB.AB.CA.Ay.ey | noun |
| caroline | AB.AB.CA.Au.Ta | noun |
| lydia | AB.AB.CA.DK.zr | noun |
| kitty | AB.AB.CA.Cz.LP | noun |
| mary | AB.AB.CA.DQ.km | noun |
| lady | AB.AB.CA.DB.eB | noun |
| de | AB.AB.CA.BW.rv | noun |
