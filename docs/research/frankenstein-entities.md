# Entity Catalog: Frankenstein (Gutenberg #84)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Entity preparation — ready for DB population when entity databases exist
**Source:** Gutenberg #84, plain text downloaded to /tmp/frankenstein.txt

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 84 |
| Title | Frankenstein; Or, The Modern Prometheus |
| Author | Mary Wollstonecraft (Godwin) Shelley |
| Author birth | 1797-08-30 |
| Author death | 1851-02-01 |
| First published | 1818 (anonymous); 1823 (credited) |
| This edition | 1831 revised edition |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~136,000 |
| Classification | Fiction |
| Word count | ~75,000 |
| Structure | 4 letters + 24 chapters |

### Gutenberg Subject Headings (LCSH)

- Frankenstein's monster (Fictitious character) -- Fiction
- Frankenstein, Victor (Fictitious character) -- Fiction
- Gothic fiction
- Horror tales
- Monsters -- Fiction
- Science fiction
- Scientists -- Fiction

### Gutenberg Bookshelves

- Category: British Literature
- Category: Classics of Literature
- Category: Novels
- Category: Science-Fiction & Fantasy
- Gothic Fiction
- Movie Books
- Precursors of Science Fiction
- Science Fiction by Women

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: Science-Fiction & Fantasy" → fiction
- **Subjects**: "Gothic fiction", "Horror tales", "Science fiction" → fiction
- **Subject entities**: "(Fictitious character)" pattern → fiction
- **Verdict**: Fiction (high confidence, multiple signals)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Victor Frankenstein

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Victor Frankenstein |
| Name tokens | `victor` + `frankenstein` |
| Aliases | (none in text — referred to as "Frankenstein" and "Victor") |
| Gender | male |
| Status | dead |
| Species | human |
| Occupation | natural philosopher, scientist |
| Description | Swiss scientist who creates a sapient creature from dead tissue at the University of Ingolstadt. The novel's protagonist and primary narrator of the central story. Consumed by guilt and obsession after abandoning his creation. |
| Prominence | major (protagonist) |

**Key relationships:**
- `child_of` → Alphonse Frankenstein
- `child_of` → Caroline Beaufort Frankenstein
- `sibling_of` → William Frankenstein
- `sibling_of` → Ernest Frankenstein
- `spouse_of` → Elizabeth Lavenza (married briefly before her death)
- `created` → The Creature
- `ally_of` → Henry Clerval
- `student_of` → M. Waldman
- `student_of` → M. Krempe
- `lives_in` → Geneva (x*)
- `visited` → Ingolstadt (x*)
- `visited` → Scotland (x*)
- `visited` → Orkney Islands (x*)
- `visited` → Ireland (x*)
- `died_in` → The Arctic (x*)
- `killed_by` → (dies of exhaustion/illness on Walton's ship)

---

#### The Creature

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AD (named creature) |
| Primary name | The Creature |
| Name tokens | `the` + `creature` |
| Aliases | the monster, the daemon, the fiend, the wretch, the being, Frankenstein's monster, Adam (self-identification) |
| Alias tokens | `the` + `monster`; `the` + `daemon`; `the` + `fiend`; `the` + `wretch`; `adam` |
| Gender | male |
| Status | presumed dead (self-immolation on funeral pyre) |
| Species | artificial being (unique) |
| Description | An eight-foot-tall being assembled from dead tissue and animated by Victor Frankenstein. Intelligent, articulate, and deeply emotional. Learns language and philosophy from the De Lacey family. Turns to violence after repeated rejection. Narrator of Chapters 11-16. |
| Prominence | major (co-protagonist/antagonist) |

**Key relationships:**
- `created_by` → Victor Frankenstein
- `killed` → William Frankenstein
- `killed` → Henry Clerval
- `killed` → Elizabeth Lavenza
- `enemy_of` → Victor Frankenstein
- `mentor_of` (observed) → De Lacey (learned from)
- `lives_in` → De Lacey cottage (t*), later nomadic

**Note:** The Creature is never named in the novel. "Frankenstein's monster" is a later cultural convention. The sub-type AD (named creature) fits because although unnamed, the Creature is a specific individual entity, not a category. The developer label could be `the_creature` or `frankensteins_creature`.

---

#### Elizabeth Lavenza

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Elizabeth Lavenza |
| Name tokens | `Elizabeth` (label) + `Lavenza` (needs creation) |
| Aliases | Elizabeth Frankenstein (after marriage) |
| Gender | female |
| Status | dead (murdered by the Creature on wedding night) |
| Description | Adopted into the Frankenstein family as a child (Italian orphan in 1831 edition; cousin in 1818 edition). Victor's love interest and eventual wife. Kind, steadfast, devoted to the family. Murdered by the Creature on her wedding night. |
| Prominence | major |

**Key relationships:**
- `spouse_of` → Victor Frankenstein
- `child_of` (adopted) → Alphonse Frankenstein (qualifier: adoptive)
- `sibling_of` (adopted) → William Frankenstein, Ernest Frankenstein
- `ally_of` → Justine Moritz
- `killed_by` → The Creature
- `born_in` → Italy (x*)
- `lives_in` → Geneva (x*)
- `died_in` → Evian / Geneva area (x*)

---

#### Henry Clerval

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Henry Clerval |
| Name tokens | `henry` + `Clerval` (needs creation) |
| Gender | male |
| Status | dead (murdered by the Creature) |
| Description | Victor's closest friend since childhood. A romantic and literary-minded young man interested in languages and Eastern literature. Accompanies Victor to England and Scotland. Strangled by the Creature in Ireland. |
| Prominence | major |

**Key relationships:**
- `ally_of` → Victor Frankenstein
- `killed_by` → The Creature
- `lives_in` → Geneva (x*)
- `died_in` → Ireland (x*)
- `visited` → London (x*), Oxford (x*), Scotland (x*)

---

#### Robert Walton

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Robert Walton |
| Name tokens | `Robert` (label) + `Walton` (label) |
| Aliases | Captain Walton |
| Gender | male |
| Status | alive (at end of novel) |
| Occupation | Arctic explorer, ship captain |
| Description | English explorer attempting to reach the North Pole. Narrator of the framing story through letters to his sister Margaret Saville. Rescues Victor from the ice and records his story. |
| Prominence | major (frame narrator) |

**Key relationships:**
- `sibling_of` → Margaret Saville
- `ally_of` → Victor Frankenstein
- `lives_in` → England (x*)
- `visited` → St. Petersburgh (x*), Archangel (x*), the Arctic (x*)

---

#### Alphonse Frankenstein

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Alphonse Frankenstein |
| Name tokens | `Alphonse` (label) + `frankenstein` |
| Gender | male |
| Status | dead (dies of grief after Elizabeth's murder) |
| Occupation | Syndic of Geneva |
| Description | Victor's father. A respected public figure in Geneva. Devoted to his family. Dies of grief shortly after Elizabeth is murdered. |
| Prominence | supporting |

**Key relationships:**
- `parent_of` → Victor Frankenstein, William Frankenstein, Ernest Frankenstein
- `spouse_of` → Caroline Beaufort Frankenstein
- `child_of` (implied) → Frankenstein family
- `ally_of` → Beaufort
- `lives_in` → Geneva (x*)

---

### 2.2 Supporting Characters

#### William Frankenstein

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | William Frankenstein |
| Name tokens | `William` (label) + `frankenstein` |
| Gender | male |
| Status | dead (murdered by the Creature) |
| Description | Victor's youngest brother. An innocent child whose murder by the Creature is the first killing and the catalyst for the novel's central tragedy. |
| Prominence | supporting |

**Key relationships:**
- `child_of` → Alphonse Frankenstein, Caroline Beaufort Frankenstein
- `sibling_of` → Victor Frankenstein, Ernest Frankenstein
- `killed_by` → The Creature
- `died_in` → Plainpalais, near Geneva (x*)

---

#### Justine Moritz

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Justine Moritz |
| Name tokens | `Justine` (needs creation) + `Moritz` (label) |
| Gender | female |
| Status | dead (executed, wrongfully convicted) |
| Description | A beloved servant/companion of the Frankenstein household. Wrongfully accused and convicted of William's murder after the Creature plants evidence on her. Confesses under pressure despite her innocence. |
| Prominence | supporting |

**Key relationships:**
- `serves` → Frankenstein family
- `ally_of` → Elizabeth Lavenza
- `killed_by` (indirect) → The Creature (qualifier: wrongfully executed for his crime)
- `lives_in` → Geneva (x*)

---

#### Ernest Frankenstein

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Ernest Frankenstein |
| Name tokens | `Ernest` (needs creation) + `frankenstein` |
| Gender | male |
| Status | alive (at end of novel — sole surviving Frankenstein) |
| Description | Victor's younger brother. The only member of the immediate Frankenstein family to survive the novel. Mentioned but not prominent. |
| Prominence | minor |

**Key relationships:**
- `child_of` → Alphonse Frankenstein, Caroline Beaufort Frankenstein
- `sibling_of` → Victor Frankenstein, William Frankenstein

---

#### Caroline Beaufort Frankenstein

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Caroline Beaufort |
| Name tokens | `Caroline` (needs creation) + `Beaufort` (label) |
| Aliases | Caroline Frankenstein (after marriage) |
| Gender | female |
| Status | dead (scarlet fever, before main events) |
| Description | Victor's mother. Daughter of Beaufort, Alphonse's friend. Marries Alphonse after her father's death. Adopts Elizabeth. Dies of scarlet fever caught while nursing Elizabeth. |
| Prominence | supporting (backstory) |

**Key relationships:**
- `spouse_of` → Alphonse Frankenstein
- `parent_of` → Victor Frankenstein, William Frankenstein, Ernest Frankenstein
- `child_of` → Beaufort
- `parent_of` (adoptive) → Elizabeth Lavenza

---

#### Margaret Saville

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Margaret Saville |
| Name tokens | `Margaret` (label) + `Saville` (label) |
| Aliases | Mrs. Saville |
| Gender | female |
| Status | alive |
| Description | Robert Walton's sister, living in England. Recipient of Walton's letters that form the framing narrative. Never appears directly. |
| Prominence | minor (addressee only) |

**Key relationships:**
- `sibling_of` → Robert Walton
- `lives_in` → England (x*)

---

#### The De Lacey Family

**Old De Lacey:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | De Lacey |
| Name tokens | `de` + `Lacey` (needs creation as label) |
| Gender | male |
| Status | alive (at end of novel) |
| Description | A blind old man living in exile in a German cottage. Formerly a respected Parisian. The Creature observes him and his children, learning language and human behavior. The only human who briefly shows kindness to the Creature before Felix intervenes. |
| Prominence | supporting |

**Felix De Lacey:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Felix De Lacey |
| Name tokens | `Felix` (label) + `de` + `Lacey` |
| Gender | male |
| Status | alive |
| Description | De Lacey's son. Helped Safie's father escape prison in Paris, leading to the family's exile. Attacks the Creature when he finds him with his blind father. |
| Prominence | supporting |

**Key relationships:**
- `child_of` → De Lacey
- `sibling_of` → Agatha De Lacey
- `betrothed_to` → Safie
- `lives_in` → Germany (cottage) (x*)

**Agatha De Lacey:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Agatha De Lacey |
| Name tokens | `Agatha` (label) + `de` + `Lacey` |
| Gender | female |
| Status | alive |
| Description | De Lacey's daughter. Gentle and caring. The Creature observes her domestic routines while hiding in the cottage's hovel. |
| Prominence | minor |

**Key relationships:**
- `child_of` → De Lacey
- `sibling_of` → Felix De Lacey

---

#### Safie

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Safie |
| Name tokens | `Safie` (needs creation) |
| Gender | female |
| Status | alive |
| Description | A young woman of Turkish/Arabian heritage. Her Christian Arab mother raised her with liberal values. Felix helped her father escape prison; she later travels alone to find Felix at the cottage. The Creature learns French alongside her. |
| Prominence | supporting |

**Key relationships:**
- `betrothed_to` → Felix De Lacey
- `lives_in` → De Lacey cottage
- `born_in` → Turkey / Constantinople area (x*)

---

#### M. Krempe

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | M. Krempe |
| Name tokens | `Krempe` (needs creation) |
| Aliases | Professor Krempe |
| Gender | male |
| Occupation | Professor of natural philosophy |
| Description | Professor at the University of Ingolstadt. Dismissive and rude to Victor about his interest in outdated alchemists. Teaches natural philosophy. |
| Prominence | minor |

**Key relationships:**
- `mentor_of` → Victor Frankenstein (qualifier: briefly)
- `member_of` → University of Ingolstadt (w*)

---

#### M. Waldman

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | M. Waldman |
| Name tokens | `Waldman` (label) |
| Aliases | Professor Waldman |
| Gender | male |
| Occupation | Professor of chemistry |
| Description | Victor's inspiring professor at Ingolstadt. Unlike Krempe, Waldman is kind and encouraging. His eloquent lecture on modern chemistry inspires Victor's fateful research. |
| Prominence | supporting |

**Key relationships:**
- `mentor_of` → Victor Frankenstein
- `member_of` → University of Ingolstadt (w*)

---

#### Beaufort

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Beaufort |
| Name tokens | `Beaufort` (label) |
| Gender | male |
| Status | dead (dies in poverty before main events) |
| Description | A merchant and close friend of Alphonse Frankenstein. Falls into poverty and dies. His daughter Caroline is taken in by Alphonse. |
| Prominence | minor (backstory) |

**Key relationships:**
- `parent_of` → Caroline Beaufort
- `ally_of` → Alphonse Frankenstein

---

#### Mr. Kirwin

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mr. Kirwin |
| Name tokens | `Kirwin` (label) |
| Gender | male |
| Occupation | Magistrate |
| Description | Irish magistrate who oversees Victor's arrest and trial for Henry Clerval's murder. Initially suspicious, becomes sympathetic after Victor's illness. |
| Prominence | minor |

**Key relationships:**
- `lives_in` → Ireland (x*)

---

#### Daniel Nugent

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Daniel Nugent |
| Name tokens | `Daniel` (needs creation as label) + `Nugent` (label) |
| Gender | male |
| Occupation | Fisherman |
| Description | Irish fisherman who discovers Henry Clerval's body. Gives testimony at Victor's trial. |
| Prominence | background |

---

### 2.3 Complete Fiction People Entity List

| # | Entity | Sub-type | Prominence | Name tokens needed |
|---|--------|----------|------------|-------------------|
| 1 | Victor Frankenstein | individual | major | `victor`(word), `frankenstein`(word) |
| 2 | The Creature | named_creature | major | `creature`(word) |
| 3 | Elizabeth Lavenza | individual | major | `Elizabeth`(label), `Lavenza`(**MISSING**) |
| 4 | Henry Clerval | individual | major | `henry`(word), `Clerval`(**MISSING**) |
| 5 | Robert Walton | individual | major | `Robert`(label), `Walton`(label) |
| 6 | Alphonse Frankenstein | individual | supporting | `Alphonse`(label), `frankenstein`(word) |
| 7 | William Frankenstein | individual | supporting | `William`(label), `frankenstein`(word) |
| 8 | Justine Moritz | individual | supporting | `Justine`(**MISSING**), `Moritz`(label) |
| 9 | Ernest Frankenstein | individual | minor | `Ernest`(**MISSING** as label), `frankenstein`(word) |
| 10 | Caroline Beaufort | individual | supporting | `Caroline`(**MISSING** as label), `Beaufort`(label) |
| 11 | Margaret Saville | individual | minor | `Margaret`(label), `Saville`(label) |
| 12 | De Lacey | individual | supporting | `de`(word), `Lacey`(**MISSING** as label) |
| 13 | Felix De Lacey | individual | supporting | `Felix`(label), `de`(word), `Lacey` |
| 14 | Agatha De Lacey | individual | minor | `Agatha`(label), `de`(word), `Lacey` |
| 15 | Safie | individual | supporting | `Safie`(**MISSING**) |
| 16 | M. Krempe | individual | minor | `Krempe`(**MISSING**) |
| 17 | M. Waldman | individual | supporting | `Waldman`(label) |
| 18 | Beaufort | individual | minor | `Beaufort`(label) |
| 19 | Mr. Kirwin | individual | minor | `Kirwin`(label) |
| 20 | Daniel Nugent | individual | background | `Daniel`(**MISSING** as label), `Nugent`(label) |

---

## 3. Non-Fiction People Entities (y*)

Real people referenced in or associated with the text. These live in the non-fiction entity DB but are linked to this fiction PBM via entity_appearances.

### 3.1 Author

#### Mary Wollstonecraft Shelley

| Field | Value |
|-------|-------|
| Namespace | y* |
| Sub-type | AA (individual) |
| Primary name | Mary Shelley |
| Name tokens | `Mary`(**MISSING** as label) + `Shelley`(label) |
| Aliases | Mary Wollstonecraft Godwin (birth name), Mary Wollstonecraft Shelley |
| Gender | female |
| Birth | 1797-08-30 |
| Death | 1851-02-01 |
| Nationality | British |
| Occupation | Novelist, short story writer |
| Rights status | public_domain (died 1851, copyright long expired) |
| Gutenberg agent | (pgterms:agent for Gutenberg ID 84) |
| Role in this work | author |

### 3.2 Historical Figures Referenced in Text

These appear in the narrative as figures Victor or the Creature read about:

| # | Entity | Sub-type | Role in text | Name tokens |
|---|--------|----------|-------------|-------------|
| 1 | Cornelius Agrippa | individual | Victor's early reading; alchemist | `Cornelius`(label), `Agrippa`(label) |
| 2 | Paracelsus | individual | Victor's early reading; alchemist | `Paracelsus`(label) |
| 3 | Albertus Magnus | individual | Victor's early reading; philosopher | needs `Albertus` label, `Magnus`(label) |
| 4 | Plutarch | individual | Creature reads his Lives | `Plutarch`(label) |
| 5 | Johann Wolfgang von Goethe | individual | Author of Sorrows of Young Werther | needs `Goethe` label |

All of these are **public_domain** (died centuries ago).

---

## 4. Place Entities (x* — Real Places)

All places in Frankenstein are real geographic locations used as fictional settings. They belong in x* (non-fiction places) and are linked to the fiction PBM via entity_appearances.

### 4.1 Major Settings

| # | Entity | Sub-type | Role | Label token status |
|---|--------|----------|------|-------------------|
| 1 | Geneva | settlement (AA) | Frankenstein family home, central location | `geneva`(word), `Geneva`(**MISSING** as label) |
| 2 | Ingolstadt | settlement (AA) | University town where Creature is created | `Ingolstadt`(label) |
| 3 | The Arctic | geographic_feature (AB) | Framing story setting | `arctic`(word) |
| 4 | England | region (AD) | Victor and Clerval travel here | `England`(label) |

### 4.2 Secondary Settings

| # | Entity | Sub-type | Role | Label token status |
|---|--------|----------|------|-------------------|
| 5 | Mont Blanc | geographic_feature (AB) | Victor meets the Creature | `mont`(word), `Blanc`(**MISSING** as label) |
| 6 | Chamounix (Chamonix) | settlement (AA) | Valley near Mont Blanc | `Chamounix`(**MISSING**) |
| 7 | London | settlement (AA) | Victor and Clerval visit | `London`(label) |
| 8 | Oxford | settlement (AA) | Victor and Clerval visit | `oxford`(word), `Oxford`(abbreviation) |
| 9 | Scotland | region (AD) | Victor works on second creature | `Scotland`(label) |
| 10 | Orkney Islands | geographic_feature (AB) | Victor destroys second creature | needs `Orkney` label |
| 11 | Ireland | region (AD) | Victor imprisoned after Clerval's death | `Ireland`(label) |
| 12 | St. Petersburgh | settlement (AA) | Walton writes from here | `Petersburgh`(label) |
| 13 | Archangel | settlement (AA) | Walton's expedition departure | `archangel`(word), `Archangel`(**MISSING** as label) |
| 14 | Switzerland | region (AD) | Frankenstein family's country | `Switzerland`(label) |
| 15 | Italy | region (AD) | Elizabeth's origin | `Italy`(label) |
| 16 | France | region (AD) | De Lacey family's origin | `France`(label) |
| 17 | Germany | region (AD) | De Lacey cottage location | `Germany`(label) |

### 4.3 Minor / Mentioned Places

| # | Entity | Sub-type | Label token status |
|---|--------|----------|-------------------|
| 18 | Lake Geneva | geographic_feature (AB) | `Geneva`(**MISSING** as label) |
| 19 | Belrive | settlement (AA) | `Belrive`(**MISSING**) |
| 20 | Plainpalais | settlement (AA) | `Plainpalais`(**MISSING**) |
| 21 | Strasburgh (Strasbourg) | settlement (AA) | `Strasburgh`(label) |
| 22 | The Rhine | geographic_feature (AB) | `rhine`(word), `Rhine`(**MISSING** as label) |
| 23 | The Alps | geographic_feature (AB) | `alps`(word), `Alps`(**MISSING** as label) |
| 24 | Leghorn (Livorno) | settlement (AA) | `leghorn`(word), `Leghorn`(**MISSING** as label) |
| 25 | Milan | settlement (AA) | `Milan`(label) |
| 26 | Lucerne | settlement (AA) | `lucerne`(word), `Lucerne`(**MISSING** as label) |
| 27 | Evian | settlement (AA) | `Evian`(**MISSING**) |
| 28 | Como | settlement/geographic (AA/AB) | `Como`(label) |
| 29 | Jura | geographic_feature (AB) | `Jura`(label) |
| 30 | The Arve | geographic_feature (AB) | `Arve`(**MISSING**) |
| 31 | The Rhone | geographic_feature (AB) | needs `Rhone` label |
| 32 | Edinburgh | settlement (AA) | `Edinburgh`(label) |
| 33 | Perth | settlement (AA) | `Perth`(label) |
| 34 | Cumberland | region (AD) | `Cumberland`(label) |
| 35 | Westmorland | region (AD) | `Westmorland`(label) |
| 36 | Paris | settlement (AA) | `Paris`(label) |
| 37 | Russia | region (AD) | `Russia`(label) |
| 38 | Turkey | region (AD) | `Turkey`(label) |
| 39 | Matlock | settlement (AA) | needs `Matlock` label |
| 40 | Derby | settlement (AA) | `derby`(word), needs `Derby` label |
| 41 | Servox | settlement (AA) | `Servox`(**MISSING**) |
| 42 | Manoir (Le Manoir) | settlement (AA) | `Manoir`(**MISSING**) |

---

## 5. Non-Fiction Thing Entities (w*)

### 5.1 Organizations

| # | Entity | Sub-type | Label token status |
|---|--------|----------|-------------------|
| 1 | University of Ingolstadt | organization (AB) | `Ingolstadt`(label) |

### 5.2 Referenced Literary Works

These are real published works referenced within the text. They would eventually be PBM-encoded texts themselves (z* namespace), but as entities referenced within Frankenstein they are thing entities in w*:

| # | Work | Author | Role in text |
|---|------|--------|-------------|
| 1 | Paradise Lost | John Milton | The Creature reads it; identifies with Adam and Satan |
| 2 | Plutarch's Lives | Plutarch | The Creature reads it; learns about human society |
| 3 | The Sorrows of Young Werther | Goethe | The Creature reads it; learns about human emotion |
| 4 | The Rime of the Ancient Mariner | Coleridge | Walton alludes to it in Letter 2 |

---

## 6. Language Shard Audit

### 6.1 Tokens Already Present as Labels (46 found)

These exist in hcp_english with `subcategory = 'label'` and can be used directly for entity names:

```
Agatha, Agrippa, Alphonse, Arthur, Beaufort, Como, Cornelius,
Cumberland, Edinburgh, Elizabeth, England, Felix, France, Germany,
Godwin, Greece, Ingolstadt, Ireland, Italy, Jura, Kirwin, London,
Magnus, Mainz, Margaret, Milan, Moritz, Nugent, Paracelsus, Paris,
Perth, Petersburgh, Plutarch, Robert, Russia, Saville, Scotland,
Shelley, Strasburgh, Switzerland, Thomas, Turkey, Waldman, Walton,
Westmorland, William
```

### 6.2 Tokens Present as Words but NOT as Labels (20 found)

These exist in hcp_english as regular word tokens (nouns, adjectives, etc.) but have no label variant. The word token may serve as a reference for the entity name, but proper label tokens with capitalized forms would be cleaner:

| Name | Existing token | Subcategory |
|------|---------------|-------------|
| adam | AB.AB.CA.AD.Lf | noun |
| alps | AB.AB.CA.AI.KI | noun |
| archangel | AB.AB.CA.AP.zZ | noun |
| blanc | AB.AB.CA.Ag.HK | noun |
| caroline | AB.AB.CA.Au.Ta | noun |
| creature | AB.AB.CA.BQ.Uu | noun |
| daemon | AB.AB.CA.BV.Uy | noun |
| daniel | AB.AB.CA.BV.ws | noun |
| derby | AB.AB.CA.Ba.IX | noun |
| ernest | AB.AB.CA.Bt.CE | noun |
| frankenstein | AB.AB.CA.CG.gf | noun |
| geneva | AB.AB.CA.CL.sv | noun |
| henry | AB.AB.CA.Cb.CJ | noun |
| leghorn | AB.AB.CA.DE.Zx | noun |
| lucerne | AB.AB.CA.DK.Pt | noun |
| mary | AB.AB.CA.DQ.km | noun |
| mont | AB.AB.CA.Dc.zM | noun |
| oxford | AB.AB.CA.Dz.cR | noun |
| rhine | AB.AB.CA.En.EI | noun |
| satan | AB.AB.CA.Ev.Py | noun |
| victor | AB.AB.CA.GB.gG | noun |

**Question for linguistics/DB specialist:** Should entity names reference the existing word token (e.g., `AB.AB.CA.GB.gG` for "victor") and rely on the shift-cap mechanism for capitalization? Or should separate label tokens be created? The design notes say label tokens carry capitalized forms baked in, which suggests creating labels. But many of these words (adam, henry, caroline) already function as both common words and proper names. Need a policy decision.

### 6.3 Tokens Completely Missing (need creation)

These names do not exist in hcp_english at all and would need new tokens:

```
Arve, Belrive, Chamounix, Clerval, Evian, Justine, Krempe,
Lacey, Lavenza, Manoir, Orkney, Plainpalais, Safie, Servox, Werter
```

Plus label variants needed for names that only exist as word tokens:

```
Adam (label), Alps (label), Archangel (label), Blanc (label),
Caroline (label), Daniel (label), Derby (label), Ernest (label),
Frankenstein (label), Geneva (label), Henry (label),
Leghorn (label), Lucerne (label), Mary (label), Mont (label),
Oxford (label), Rhine (label), Satan (label), Victor (label)
```

**Also needed but not yet checked:**
```
Albertus, Goethe, Matlock, Orkney, Rhone
```

### 6.4 Summary

| Category | Count | Action |
|----------|-------|--------|
| Label tokens already exist | 46 | Ready to use |
| Word tokens exist, need label variants | ~20 | Policy decision needed |
| Completely missing tokens | ~15 | Must create before entity DB population |
| Uncertain / need to check | ~5 | Check individually |
| **Total unique name tokens needed** | **~86** | Across all entity types |

---

## 7. Entity Relationship Summary

### 7.1 Family Tree: The Frankensteins

```
Beaufort ──── married ──── (unnamed wife)
    │
    └── Caroline Beaufort ──── married ──── Alphonse Frankenstein
            │
            ├── Victor Frankenstein ──── married ──── Elizabeth Lavenza (adopted)
            ├── Ernest Frankenstein
            └── William Frankenstein
```

### 7.2 Key Antagonistic Chain

```
Victor Frankenstein ── created ──► The Creature
The Creature ── killed ──► William Frankenstein
The Creature ── killed (indirect) ──► Justine Moritz (wrongful execution)
The Creature ── killed ──► Henry Clerval
The Creature ── killed ──► Elizabeth Lavenza
(Alphonse dies of grief)
Victor ── dies ──► (on Walton's ship)
The Creature ── self-destructs ──► (funeral pyre, implied)
```

### 7.3 The De Lacey Thread

```
De Lacey (blind father)
    ├── Felix De Lacey ──── betrothed ──── Safie
    └── Agatha De Lacey

Safie's father (the Turk) ── helped by ──► Felix (led to family's exile)
The Creature ── observed ──► De Lacey family (learned language/culture)
```

### 7.4 Academic Thread

```
Cornelius Agrippa ── inspired (reading) ──► Victor Frankenstein
Paracelsus ── inspired (reading) ──► Victor Frankenstein
Albertus Magnus ── inspired (reading) ──► Victor Frankenstein
M. Krempe ── taught ──► Victor Frankenstein
M. Waldman ── mentored ──► Victor Frankenstein
```

### 7.5 Cross-Entity-Type Relationships

| Source (type) | Relationship | Target (type) |
|---------------|-------------|----------------|
| Victor (u*) | lives_in | Geneva (x*) |
| Victor (u*) | visited | Ingolstadt (x*) |
| Victor (u*) | member_of | University of Ingolstadt (w*) |
| The Creature (u*) | lives_in | De Lacey cottage |
| Walton (u*) | visited | Archangel (x*) |
| Walton (u*) | visited | The Arctic (x*) |
| Krempe (u*) | member_of | University of Ingolstadt (w*) |
| Waldman (u*) | member_of | University of Ingolstadt (w*) |
| M. Shelley (y*) | author_of | Frankenstein PBM (v*) |

---

## 8. Rights Summary

| Entity | Rights Status | Basis |
|--------|--------------|-------|
| All fiction entities (u*) | public_domain | Source work published 1818, author died 1851 |
| Mary Shelley (y*) | public_domain | Died 1851 |
| All real places (x*) | public_domain | Geographic facts |
| All historical figures (y*) | public_domain | All died centuries ago |
| University of Ingolstadt (w*) | public_domain | Historical institution (dissolved 1800) |
| Frankenstein PBM (v*) | public_domain | Gutenberg `copyright: false` |

This text is an ideal proof-of-concept candidate: zero IP complications.
