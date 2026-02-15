# Entity Catalog: Middlemarch (Gutenberg #145)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AD

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 145 |
| Title | Middlemarch |
| Author | George Eliot (Mary Ann Evans) |
| Author birth | 1819-11-22 |
| Author death | 1880-12-22 |
| First published | 1871–1872 (serial) |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~40,000 |
| Classification | Fiction |
| Word count | ~316,000 |
| Structure | 8 books, 86 chapters, Prelude, Finale |

### Gutenberg Subject Headings (LCSH)

- City and town life -- Fiction
- Didactic fiction
- Domestic fiction
- England -- Fiction
- Love stories
- Married people -- Fiction
- Young women -- Fiction

### Gutenberg Bookshelves

- Best Books Ever Listings
- Category: British Literature
- Category: Classics of Literature
- Category: Novels

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: British Literature" → fiction
- **Subjects**: "Domestic fiction", "Love stories" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Dorothea Brooke

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Dorothea Brooke |
| Name tokens | `Dorothea` (label) + `brooke` |
| Aliases | Dodo |
| Gender | female |
| Description | Idealistic, intelligent young woman who marries the elderly scholar Casaubon, hoping to assist his great work. After his death, she defies his will to marry Will Ladislaw. The protagonist. |
| Prominence | major (protagonist) |

**Key relationships:**
- `spouse_of` → Edward Casaubon (first marriage)
- `spouse_of` → Will Ladislaw (second marriage)
- `sibling_of` → Celia Brooke

---

#### Edward Casaubon

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Edward Casaubon |
| Name tokens | `Edward` (label) + `Casaubon` (label) |
| Gender | male |
| Status | dead |
| Occupation | clergyman, scholar |
| Description | Elderly, pedantic scholar working on his unfinished "Key to All Mythologies." Marries Dorothea but proves cold and jealous. Dies of heart disease, leaving a codicil to disinherit Dorothea if she marries Ladislaw. |
| Prominence | major |

---

#### Will Ladislaw

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Will Ladislaw |
| Name tokens | `will` + `Ladislaw` (label, new) |
| Gender | male |
| Occupation | artist, journalist, politician |
| Description | Casaubon's young cousin. Artistic, passionate, and idealistic. Falls in love with Dorothea. Eventually becomes a Reform politician and Member of Parliament. |
| Prominence | major |

---

#### Tertius Lydgate

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tertius Lydgate |
| Name tokens | `Tertius` (label, new) + `Lydgate` (label) |
| Gender | male |
| Status | dead (dies at 50) |
| Occupation | physician |
| Description | Ambitious young doctor who comes to Middlemarch to pursue medical reform. His idealism is undermined by his marriage to Rosamond and entanglement with Bulstrode. Dies unfulfilled. |
| Prominence | major |

---

#### Rosamond Vincy

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Rosamond Vincy |
| Name tokens | `Rosamond` (label) + `Vincy` (label, new) |
| Gender | female |
| Description | Beautiful, vain daughter of the mayor. Marries Lydgate for status but undermines his career with her extravagance and inflexibility. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Tertius Lydgate
- `sibling_of` → Fred Vincy

---

### 2.2 Supporting Characters

#### Fred Vincy

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Fred Vincy |
| Name tokens | `Fred` (label) + `Vincy` (label, new) |
| Gender | male |
| Description | Rosamond's good-natured but feckless brother. Hopes to inherit from Featherstone. Eventually reforms through love for Mary Garth and works under Caleb Garth. |
| Prominence | supporting |

**Key relationships:**
- `spouse_of` → Mary Garth
- `sibling_of` → Rosamond Vincy

---

#### Mary Garth

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mary Garth |
| Name tokens | `mary` + `garth` |
| Gender | female |
| Description | Plain, sensible, witty young woman. Nurses old Featherstone. Refuses to marry Fred until he proves himself worthy. Daughter of Caleb Garth. |
| Prominence | supporting |

**Key relationships:**
- `spouse_of` → Fred Vincy
- `child_of` → Caleb Garth

---

#### Caleb Garth

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Caleb Garth |
| Name tokens | `Caleb` (label) + `garth` |
| Gender | male |
| Occupation | land agent, surveyor |
| Description | Honest, hardworking land agent. Devoted to his craft. Father of Mary. Takes Fred Vincy under his wing. |
| Prominence | supporting |

---

#### Nicholas Bulstrode

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Nicholas Bulstrode |
| Name tokens | `Nicholas` (label) + `Bulstrode` (label) |
| Gender | male |
| Occupation | banker |
| Description | Wealthy, pious banker with a dark past. His connection to Raffles exposes his former life as a fence's partner. Disgraced when his role in Raffles's death is suspected. |
| Prominence | supporting |

---

#### Celia Brooke

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Celia Brooke |
| Name tokens | `Celia` (label) + `brooke` |
| Aliases | Kitty |
| Gender | female |
| Description | Dorothea's pretty, practical younger sister. Marries Sir James Chettam. More conventional than Dorothea. |
| Prominence | supporting |

**Key relationships:**
- `sibling_of` → Dorothea Brooke
- `spouse_of` → Sir James Chettam

---

#### Sir James Chettam

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | James Chettam |
| Name tokens | `James` (label) + `Chettam` (label, new) |
| Gender | male |
| Occupation | baronet, landowner |
| Description | Good-natured baronet. Initially courts Dorothea, then happily marries her sister Celia. A conscientious landlord. |
| Prominence | supporting |

---

#### Mr. Farebrother

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Farebrother |
| Name tokens | `Farebrother` (label) |
| Gender | male |
| Occupation | vicar |
| Description | Genial, sensible vicar. Friend to Lydgate. Loves Mary Garth but nobly helps Fred win her instead. Naturalist in his spare time. |
| Prominence | supporting |

---

#### Peter Featherstone

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Peter Featherstone |
| Name tokens | `peter` + `Featherstone` (label) |
| Gender | male |
| Status | dead |
| Description | Wealthy, miserly old man. Multiple relatives compete for his inheritance. His two wills cause upheaval after his death. |
| Prominence | supporting |

---

#### Mr. Cadwallader

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Cadwallader |
| Name tokens | `Cadwallader` (label) |
| Gender | male |
| Occupation | rector |
| Description | Genial rector and neighbor. More interested in fishing than theology. His wife is the local gossip. |
| Prominence | minor |

---

#### John Raffles

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | John Raffles |
| Name tokens | `john` + `raffles` |
| Gender | male |
| Status | dead |
| Description | Dissolute man who knows Bulstrode's secret past. His return to Middlemarch and subsequent death trigger Bulstrode's downfall. |
| Prominence | supporting |

---

#### Joshua Rigg

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Joshua Rigg |
| Name tokens | `Joshua` (label) + `rigg` |
| Gender | male |
| Description | Featherstone's illegitimate son who inherits Stone Court. His stepfather is Raffles. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### George Eliot

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | George Eliot |
| Name tokens | `George` (label) + `Eliot` (label) |
| Real name | Mary Ann Evans |
| Gender | female |
| Born | 1819-11-22, Nuneaton, Warwickshire |
| Died | 1880-12-22, London |
| Occupation | novelist, poet, journalist |
| Description | English novelist, one of the leading writers of the Victorian era. Used a male pen name to ensure her works were taken seriously. |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Uh | Ladislaw | Fictional surname, name-only |
| AB.AB.CA.HX.Ui | Chettam | Fictional surname, name-only |
| AB.AB.CA.HX.Uj | Vincy | Fictional surname, name-only |
| AB.AB.CA.HX.Uk | Tertius | Latin given name, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Dorothea | AB.AB.CA.Ge.KN | label |
| Casaubon | AB.AB.CA.GZ.DP | label |
| Lydgate | AB.AB.CA.Gw.QI | label |
| Rosamond | AB.AB.CA.HI.iM | label |
| Bulstrode | AB.AB.CA.GX.ns | label |
| Caleb | AB.AB.CA.GY.UC | label |
| Celia | AB.AB.CA.GZ.Wj | label |
| Cadwallader | AB.AB.CA.GY.Ls | label |
| Farebrother | AB.AB.CA.Gg.Zz | label |
| Featherstone | AB.AB.CA.Gg.jY | label |
| Fred | AB.AB.CA.Gh.he | label |
| Edward | AB.AB.CA.Gf.KG | label |
| George | AB.AB.CA.Gi.mc | label |
| Eliot | AB.AB.CA.Gf.Ww | label |
| brooke | AB.AB.CA.Am.YM | noun |
| garth | AB.AB.CA.CK.jJ | noun |
| mary | AB.AB.CA.DQ.km | noun |
| will | AB.AB.CA.GI.ep | noun |
| James | AB.AB.CA.Gp.hx | label |
| Joshua | AB.AB.CA.Gq.NK | label |
| Nicholas | AB.AB.CA.HC.Bi | label |
| peter | AB.AB.CA.EI.dT | noun |
| john | AB.AB.CA.Cu.qL | noun |
| raffles | AB.AB.CA.Ef.tx | noun |
| rigg | AB.AB.CA.En.vd | noun |
