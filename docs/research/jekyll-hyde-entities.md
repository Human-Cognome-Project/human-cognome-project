# Entity Catalog: The Strange Case of Dr Jekyll and Mr Hyde (Gutenberg #43)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AC

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 43 |
| Title | The Strange Case of Dr. Jekyll and Mr. Hyde |
| Author | Robert Louis Stevenson |
| Author birth | 1850-11-13 |
| Author death | 1894-12-03 |
| First published | 1886 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~54,000 |
| Classification | Fiction |
| Word count | ~26,000 |
| Structure | 10 chapters |

### Gutenberg Subject Headings (LCSH)

- Horror tales
- London (England) -- Fiction
- Multiple personality -- Fiction
- Physicians -- Fiction
- Psychological fiction
- Science fiction
- Self-experimentation in medicine -- Fiction

### Gutenberg Bookshelves

- Category: British Literature
- Category: Classics of Literature
- Category: Crime, Thrillers and Mystery
- Category: Novels
- Horror
- Movie Books
- Precursors of Science Fiction

### Classification Signals

- **Bookshelves**: "Category: Novels", "Horror" → fiction
- **Subjects**: "Horror tales", "Psychological fiction", "Science fiction" → fiction
- **Subject entities**: "Multiple personality -- Fiction" → fiction
- **Verdict**: Fiction (high confidence, multiple signals)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Dr. Henry Jekyll / Mr. Edward Hyde

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Henry Jekyll |
| Name tokens | `henry` + `Jekyll` (label) |
| Aliases | Dr. Jekyll, Edward Hyde, Mr. Hyde |
| Alias name tokens | `Edward` (label) + `hyde` |
| Gender | male |
| Status | dead (suicide) |
| Species | human |
| Occupation | doctor, scientist |
| Description | A respected London physician who creates a potion that transforms him into Mr. Edward Hyde, his uninhibited evil alter ego. As Hyde, he commits increasingly violent acts. Eventually loses control of the transformations and takes his own life. |
| Prominence | major (dual protagonist/antagonist) |

**Key relationships:**
- `alter_ego_of` → (self — Jekyll and Hyde are the same person)
- `colleague_of` → Dr. Hastie Lanyon
- `friend_of` → Mr. Gabriel John Utterson
- `employer_of` → Poole
- `killed` → Sir Danvers Carew (as Hyde)
- `lives_in` → London (x*)

---

#### Mr. Gabriel John Utterson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Gabriel John Utterson |
| Name tokens | `Gabriel` (label) + `john` + `Utterson` (label, new) |
| Gender | male |
| Status | alive |
| Occupation | lawyer |
| Description | Jekyll's lawyer and the story's primary point-of-view character. A reserved, tolerant man who investigates the connection between Jekyll and Hyde. His perspective drives the narrative's mystery structure. |
| Prominence | major (protagonist/narrator perspective) |

**Key relationships:**
- `friend_of` → Henry Jekyll
- `kinsman_of` → Richard Enfield
- `employer_of` → Mr. Guest
- `lives_in` → London (x*)

---

### 2.2 Supporting Characters

#### Dr. Hastie Lanyon

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Hastie Lanyon |
| Name tokens | `Hastie` (label) + `Lanyon` (label) |
| Gender | male |
| Status | dead (dies of shock) |
| Occupation | doctor |
| Description | A hearty physician and former close friend of Jekyll. Witnesses Hyde's transformation back into Jekyll and dies of shock shortly after. His posthumous letter reveals the truth. |
| Prominence | supporting |

**Key relationships:**
- `colleague_of` → Henry Jekyll
- `friend_of` → Mr. Utterson

---

#### Mr. Richard Enfield

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Richard Enfield |
| Name tokens | `Richard` (label) + `enfield` |
| Gender | male |
| Occupation | man about town |
| Description | Utterson's distant kinsman. Witnesses Hyde trampling a young girl in the street, which sets the story in motion. A well-known man about town. |
| Prominence | supporting |

**Key relationships:**
- `kinsman_of` → Mr. Utterson

---

#### Poole

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Poole |
| Name tokens | `Poole` (label) |
| Gender | male |
| Occupation | butler |
| Description | Jekyll's faithful head butler. Alerts Utterson when Jekyll locks himself in his laboratory, and helps break down the door, discovering Hyde's body. |
| Prominence | supporting |

**Key relationships:**
- `servant_of` → Henry Jekyll

---

### 2.3 Minor Characters

**Sir Danvers Carew:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Danvers Carew |
| Name tokens | `Danvers` (label) + `Carew` (label) |
| Gender | male |
| Status | dead (murdered by Hyde) |
| Occupation | member of Parliament |
| Description | An aged and distinguished MP beaten to death by Hyde with a cane. His murder turns the investigation into a manhunt. |
| Prominence | minor |

---

**Mr. Guest:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Guest |
| Name tokens | `guest` |
| Gender | male |
| Occupation | clerk, handwriting expert |
| Description | Utterson's head clerk and a student of handwriting. Notices the similarity between Jekyll's and Hyde's handwriting. |
| Prominence | minor |

---

**Inspector Newcomen:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Newcomen |
| Name tokens | `Newcomen` (label) |
| Gender | male |
| Occupation | police inspector |
| Description | The police inspector investigating the Carew murder case. |
| Prominence | minor |

---

**Bradshaw:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bradshaw |
| Name tokens | `Bradshaw` (label) |
| Gender | male |
| Occupation | servant |
| Description | One of Jekyll's servants. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### Robert Louis Stevenson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Robert Louis Stevenson |
| Name tokens | `Robert` (label) + `louis` + `Stevenson` (label) |
| Gender | male |
| Born | 1850-11-13, Edinburgh, Scotland |
| Died | 1894-12-03, Vailima, Samoa |
| Occupation | novelist, essayist, poet, travel writer |
| Description | Scottish author known for adventure and horror fiction. |

---

## 4. Non-Fiction Place Entities (x*)

London already exists in the entity DB (xA.AA.AA.AA.AC from Frankenstein). Jekyll and Hyde is set entirely in London — no new place entities needed.

---

## 5. Label Token Audit

### 5.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Tu | Utterson | Name-only word (no dictionary definition) |

### 5.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Jekyll | AB.AB.CA.Gp.uh | label |
| hyde | AB.AB.CA.Ch.gt | noun |
| Lanyon | AB.AB.CA.Gu.Ct | label |
| Poole | AB.AB.CA.HF.wB | label |
| Carew | AB.AB.CA.GY.tT | label |
| Newcomen | AB.AB.CA.HB.uw | label |
| Bradshaw | AB.AB.CA.GW.rB | label |
| Enfield | AB.AB.CA.Br.CZ | noun |
| Gabriel | AB.AB.CA.Gi.Cw | label |
| Hastie | AB.AB.CA.Gl.Vx | label |
| Edward | AB.AB.CA.Gf.KG | label |
| Richard | AB.AB.CA.HI.DS | label |
| Danvers | AB.AB.CA.Gc.jY | label |
| henry | AB.AB.CA.Cb.CJ | noun |
| john | AB.AB.CA.Cu.qL | noun |
| guest | AB.AB.CA.CU.mU | noun |
| Robert | AB.AB.CA.HI.TT | label |
| louis | AB.AB.CA.DJ.nU | noun |
| Stevenson | AB.AB.CA.HN.dV | label |
