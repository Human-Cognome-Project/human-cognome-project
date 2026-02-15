# Entity Catalog: Moby Dick; Or, The Whale (Gutenberg #2701)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AI

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 2701 |
| Title | Moby Dick; Or, The Whale |
| Author | Herman Melville |
| Author birth | 1819-08-01 |
| Author death | 1891-09-28 |
| First published | 1851 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~115,000 |
| Classification | Fiction |
| Word count | ~215,000 |
| Structure | Etymology, Extracts, 135 chapters, Epilogue |

### Gutenberg Subject Headings (LCSH)

- Adventure stories
- Ahab, Captain (Fictitious character) -- Fiction
- Mentally ill -- Fiction
- Psychological fiction
- Sea stories
- Ship captains -- Fiction
- Whales -- Fiction
- Whaling -- Fiction
- Whaling ships -- Fiction

### Gutenberg Bookshelves

- Best Books Ever Listings
- Category: Adventure
- Category: American Literature
- Category: Classics of Literature
- Category: Novels

### Classification Signals

- **Bookshelves**: "Category: Novels", "Category: Adventure" → fiction
- **Subjects**: "Adventure stories", "Sea stories" → fiction
- **Subject entities**: "Ahab, Captain (Fictitious character)" → fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Captain Ahab

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Ahab |
| Name tokens | `Ahab` (label) |
| Gender | male |
| Status | dead (drowned, entangled in harpoon line) |
| Occupation | captain, whaler |
| Description | Monomaniacal captain of the Pequod. Lost his leg to Moby Dick and obsessively pursues the white whale across the oceans. His quest destroys his ship and crew. |
| Prominence | major (antagonist/tragic hero) |

**Key relationships:**
- `captain_of` → The Pequod (s*)
- `enemy_of` → Moby Dick (named creature)
- `employer_of` → Starbuck, Stubb, Flask

---

#### Ishmael

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Ishmael |
| Name tokens | `Ishmael` (label) |
| Gender | male |
| Occupation | sailor, teacher |
| Description | The narrator and sole survivor of the Pequod. A philosophical young man who ships aboard as an ordinary seaman. His friendship with Queequeg is central to the early narrative. |
| Prominence | major (protagonist/narrator) |

**Key relationships:**
- `friend_of` → Queequeg

---

#### Queequeg

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Queequeg |
| Name tokens | `Queequeg` (label) |
| Gender | male |
| Status | dead |
| Occupation | harpooner |
| Description | A tattooed Polynesian prince who became a whaler. Ishmael's closest friend and bunkmate. Skilled harpooner assigned to Starbuck's boat. His coffin becomes Ishmael's life buoy. |
| Prominence | major |

---

#### Starbuck

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Starbuck |
| Name tokens | `Starbuck` (label) |
| Gender | male |
| Status | dead |
| Occupation | first mate |
| Description | Quaker first mate of the Pequod. Prudent, moral, and courageous. The only crew member who openly opposes Ahab's obsessive hunt, but lacks the will to act against him. |
| Prominence | major |

---

### 2.2 Supporting Characters

#### Stubb

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Stubb |
| Name tokens | `Stubb` (label, new) |
| Gender | male |
| Status | dead |
| Occupation | second mate |
| Description | Good-humored, easygoing second mate. Smokes his pipe constantly. Accepts fate with equanimity. |
| Prominence | supporting |

---

#### Flask

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Flask |
| Name tokens | `flask` |
| Gender | male |
| Status | dead |
| Occupation | third mate |
| Description | Short, pugnacious third mate from Tisbury. Approaches whaling with businesslike practicality, seeing whales as personal enemies. |
| Prominence | supporting |

---

#### Tashtego

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tashtego |
| Name tokens | `Tashtego` (label, new) |
| Gender | male |
| Status | dead |
| Occupation | harpooner |
| Description | Gay Head (Wampanoag) Native American harpooner assigned to Stubb's boat. Sinewy and skilled. |
| Prominence | supporting |

---

#### Daggoo

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Daggoo |
| Name tokens | `Daggoo` (label, new) |
| Gender | male |
| Status | dead |
| Occupation | harpooner |
| Description | Gigantic African harpooner assigned to Flask's boat. Noble and physically imposing. |
| Prominence | supporting |

---

#### Fedallah

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Fedallah |
| Name tokens | `Fedallah` (label, new) |
| Gender | male |
| Status | dead |
| Occupation | harpooner, Ahab's personal crew |
| Description | A mysterious Parsee who leads Ahab's secret boat crew. Prophesies Ahab's death. Killed by Moby Dick, his body lashed to the whale. |
| Prominence | supporting |

---

#### Father Mapple

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Father Mapple |
| Name tokens | `Mapple` (label, new) |
| Gender | male |
| Occupation | chaplain |
| Description | Former whaler turned chaplain at the Whaleman's Chapel in New Bedford. Delivers the sermon on Jonah. |
| Prominence | minor |

---

#### Bildad

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Bildad |
| Name tokens | `Bildad` (label, new) |
| Gender | male |
| Occupation | ship owner, retired captain |
| Description | Quaker co-owner of the Pequod. Parsimonious and Bible-quoting. |
| Prominence | minor |

---

#### Peleg

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Peleg |
| Name tokens | `Peleg` (label) |
| Gender | male |
| Occupation | ship owner, retired captain |
| Description | Co-owner of the Pequod with Bildad. More worldly than his partner. Recruits Ishmael and Queequeg. |
| Prominence | minor |

---

#### Pip

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Pip |
| Name tokens | `pip` |
| Gender | male |
| Status | dead |
| Occupation | cabin boy |
| Description | Young African-American cabin boy. Goes mad after being abandoned in the ocean during a whale chase. Becomes Ahab's companion in his final days. |
| Prominence | supporting |

---

### 2.3 Named Creatures

#### Moby Dick

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Moby Dick |
| Name tokens | `moby` + `dick` |
| Species | sperm whale (albino) |
| Description | The great white sperm whale. Object of Ahab's obsessive quest. Enormous, scarred, and seemingly malevolent. Destroys the Pequod and kills Ahab. |
| Prominence | major |

---

## 3. Fiction Thing Entities (s*)

#### The Pequod

| Field | Value |
|-------|-------|
| Namespace | s* |
| Sub-type | AA (object) |
| Primary name | Pequod |
| Name tokens | `Pequod` (label) |
| Description | An old Nantucket whaling ship fitted out with whale bone. Commanded by Captain Ahab. Sunk by Moby Dick. |

---

## 4. Non-Fiction People Entities (y*)

### 4.1 Author

#### Herman Melville

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Herman Melville |
| Name tokens | `Herman` (label) + `Melville` (label) |
| Gender | male |
| Born | 1819-08-01, New York City |
| Died | 1891-09-28, New York City |
| Occupation | novelist, poet, short story writer |
| Description | American author. Moby-Dick was poorly received in his lifetime but now considered one of the great American novels. |

---

## 5. Non-Fiction Place Entities (x*)

Nantucket is a real place. New Bedford is referenced. Most maritime locations are generic.

---

## 6. Label Token Audit

### 6.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.UD | Stubb | Name-only word (fictional surname) |
| AB.AB.CA.HX.UE | Tashtego | Name-only word (Wampanoag name) |
| AB.AB.CA.HX.UF | Daggoo | Name-only word (fictional African name) |
| AB.AB.CA.HX.UG | Fedallah | Name-only word (fictional Parsee name) |
| AB.AB.CA.HX.UH | Mapple | Name-only word (fictional surname) |
| AB.AB.CA.HX.UI | Bildad | Name-only word (biblical name) |

### 6.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Ahab | AB.AB.CA.GQ.rn | label |
| Ishmael | AB.AB.CA.Gp.RV | label |
| Queequeg | AB.AB.CA.HG.uf | label |
| Starbuck | AB.AB.CA.HN.Rt | label |
| Peleg | AB.AB.CA.HE.fm | label |
| Pequod | AB.AB.CA.HE.pk | label |
| Herman | AB.AB.CA.Gm.DS | label |
| Melville | AB.AB.CA.Gz.Dj | label |
| Nantucket | AB.AB.CA.HB.Rd | label |
| Bulkington | AB.AB.CA.GX.ma | label |
| flask | AB.AB.CA.CC.fm | noun |
| pip | AB.AB.CA.EN.Kf | noun |
| carpenter | AB.AB.CA.Au.XC | noun |
| fleece | AB.AB.CA.CC.vU | noun |
| Perth | AB.AB.CA.HE.vg | label |
