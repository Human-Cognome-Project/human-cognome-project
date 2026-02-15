# Entity Catalog: A Room with a View (Gutenberg #2641)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AH

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 2641 |
| Title | A Room with a View |
| Author | E. M. Forster (Edward Morgan Forster) |
| Author birth | 1879-01-01 |
| Author death | 1970-06-07 |
| First published | 1908 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~55,000 |
| Classification | Fiction |
| Word count | ~67,000 |
| Structure | 2 parts, 20 chapters |

### Gutenberg Subject Headings (LCSH)

- British -- Italy -- Fiction
- England -- Fiction
- Florence (Italy) -- Fiction
- Humorous stories
- Young women -- Fiction

### Gutenberg Bookshelves

- Category: British Literature
- Category: Novels
- Category: Romance
- Italy

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: Romance" → fiction
- **Subjects**: "British -- Italy -- Fiction", "Young women -- Fiction" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Lucy Honeychurch

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Lucy Honeychurch |
| Name tokens | `lucy` + `Honeychurch` (label) |
| Gender | female |
| Status | alive |
| Species | human |
| Age | young woman |
| Occupation | none (young gentlewoman, accomplished pianist) |
| Description | Young Englishwoman who travels to Florence with her cousin Charlotte. Falls in love with George Emerson despite her engagement to Cecil Vyse. Eventually breaks with convention to marry George. |
| Prominence | major (protagonist) |

**Key relationships:**
- `engaged_to` → Cecil Vyse (broken off)
- `spouse_of` → George Emerson
- `sibling_of` → Freddy Honeychurch
- `child_of` → Mrs. Honeychurch
- `chaperoned_by` → Charlotte Bartlett

---

#### George Emerson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | George Emerson |
| Name tokens | `George` (label) + `Emerson` (label) |
| Gender | male |
| Status | alive |
| Description | Unconventional young man who believes in truth and passion over social convention. Falls in love with Lucy in Florence. His father is a free-thinking socialist. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Lucy Honeychurch
- `child_of` → Mr. Emerson

---

#### Cecil Vyse

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Cecil Vyse |
| Name tokens | `cecil` + `Vyse` |
| Gender | male |
| Status | alive |
| Occupation | aesthete, man of culture |
| Description | Lucy's fiancé, a refined but emotionally stunted London aesthete. Treats Lucy as an art object rather than a person. She eventually breaks off the engagement. |
| Prominence | major |

**Key relationships:**
- `engaged_to` → Lucy Honeychurch (broken off)

---

#### Charlotte Bartlett

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Charlotte Bartlett |
| Name tokens | `charlotte` + `Bartlett` (label) |
| Gender | female |
| Occupation | companion/chaperone |
| Description | Lucy's older cousin and chaperone in Florence. Perpetually anxious about propriety. Inadvertently catalyzes the romance by telling Mr. Emerson about Lucy's feelings. |
| Prominence | major |

**Key relationships:**
- `cousin_of` → Lucy Honeychurch
- `chaperoning` → Lucy Honeychurch

---

### 2.2 Supporting Characters

#### Mr. Emerson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Emerson |
| Name tokens | `Emerson` (label) |
| Gender | male |
| Description | George's elderly father, a free-thinking socialist. Kind-hearted and unconventional. Offers his room with a view to Charlotte and Lucy at the Pensione Bertolini. |
| Prominence | supporting |

**Key relationships:**
- `parent_of` → George Emerson

---

#### Rev. Mr. Beebe

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Beebe |
| Name tokens | `Beebe` (label) |
| Gender | male |
| Occupation | clergyman |
| Description | Genial clergyman who knows Lucy from their Surrey parish. Appreciates her piano playing. Initially supportive but disapproves of her final choice. |
| Prominence | supporting |

---

#### Rev. Cuthbert Eager

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Cuthbert Eager |
| Name tokens | `Cuthbert` (label) + `eager` |
| Gender | male |
| Occupation | English chaplain in Florence |
| Description | The English chaplain in Florence. Pompous and snobbish. Leads the excursion to Fiesole where George first kisses Lucy. |
| Prominence | supporting |

---

#### Miss Eleanor Lavish

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Eleanor Lavish |
| Name tokens | `Eleanor` (label) + `lavish` |
| Gender | female |
| Occupation | novelist |
| Description | A flamboyant novelist staying at the Pensione Bertolini. Takes Lucy on an adventure through Florence, during which Lucy loses her way and encounters George. Publishes a novel based on Lucy's kiss. |
| Prominence | supporting |

---

#### Freddy Honeychurch

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Freddy Honeychurch |
| Name tokens | `Freddy` (label) + `Honeychurch` (label) |
| Gender | male |
| Description | Lucy's younger brother. Good-natured and athletic. Befriends George Emerson and invites the Emersons to live nearby. |
| Prominence | supporting |

**Key relationships:**
- `sibling_of` → Lucy Honeychurch

---

### 2.3 Minor Characters

**Mrs. Honeychurch:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mrs. Honeychurch |
| Name tokens | `Honeychurch` (label) |
| Gender | female |
| Description | Lucy and Freddy's sensible, warm-hearted mother. |
| Prominence | minor |

---

**Miss Catharine Alan / Miss Teresa Alan:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name (Catharine) | Catharine Alan |
| Name tokens | `Catharine` (label) + `alan` |
| Primary name (Teresa) | Teresa Alan |
| Name tokens | `teresa` + `alan` |
| Description | Elderly sisters at the Pensione Bertolini. Their vacated rooms trigger Cecil's invitation to the Emersons. |
| Prominence | minor |

---

**Minnie Beebe:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Minnie Beebe |
| Name tokens | `minnie` + `Beebe` (label) |
| Gender | female |
| Description | Mr. Beebe's young niece. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### E. M. Forster

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Edward Morgan Forster |
| Name tokens | `Edward` (label) + `morgan` + `Forster` (label) |
| Gender | male |
| Born | 1879-01-01, London |
| Died | 1970-06-07, Coventry |
| Occupation | novelist, essayist |
| Description | English novelist. Known for humanist themes and examination of class and convention. |

---

### 3.2 Historical Figures Referenced

#### Giotto

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Giotto |
| Name tokens | `Giotto` (label, new) |
| Gender | male |
| Born | c. 1267 |
| Died | 1337-01-08 |
| Occupation | painter, architect |
| Description | Italian painter and architect. His frescoes in Santa Croce are discussed by characters in Florence. |

---

## 4. Non-Fiction Place Entities (x*)

### 4.1 New Places

#### Florence

Already referenced in Gutenberg subjects. Major setting of Part I.

| Field | Value |
|-------|-------|
| Sub-type | AA (settlement) |
| Name tokens | `Florence` (label) |
| Description | Italian city, capital of Tuscany. Setting of Part I of the novel. |

---

#### Fiesole

| Field | Value |
|-------|-------|
| Sub-type | AA (settlement) |
| Name tokens | `Fiesole` (label, new) |
| Description | Hilltop town near Florence. Site of the excursion where George first kisses Lucy. |

---

## 5. Label Token Audit

### 5.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Tv | Fiesole | Italian place name, no English dictionary definition |
| AB.AB.CA.HX.Tw | Giotto | Italian painter's name, no English dictionary definition |

### 5.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Honeychurch | AB.AB.CA.Gm.vw | label |
| Vyse | AB.AB.CA.GD.jB | noun |
| Emerson | AB.AB.CA.Gf.fz | label |
| Beebe | AB.AB.CA.GU.ij | label |
| Bartlett | AB.AB.CA.GU.LN | label |
| Cuthbert | AB.AB.CA.Gc.QJ | label |
| Eleanor | AB.AB.CA.Gf.VV | label |
| Freddy | AB.AB.CA.Gh.hi | label |
| George | AB.AB.CA.Gi.mc | label |
| Catharine | AB.AB.CA.GZ.Kn | label |
| Florence | AB.AB.CA.Gh.NU | label |
| Forster | AB.AB.CA.Gh.YC | label |
| Edward | AB.AB.CA.Gf.KG | label |
| Bertolini | AB.AB.CA.GV.LR | label |
| lucy | AB.AB.CA.DK.TP | noun |
| cecil | AB.AB.CA.Aw.Sh | noun |
| charlotte | AB.AB.CA.Ay.ey | noun |
| eager | AB.AB.CA.Bl.KA | noun |
| lavish | AB.AB.CA.DD.Xy | noun |
| alan | AB.AB.CA.AG.Zd | noun |
| teresa | AB.AB.CA.Fg.Nc | noun |
| minnie | AB.AB.CA.DZ.eg | noun |
| morgan | AB.AB.CA.Dd.dJ | noun |
