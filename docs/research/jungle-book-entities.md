# Entity Catalog: The Jungle Book (Gutenberg #236)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 236 |
| Title | The Jungle Book |
| Author | Rudyard Kipling |
| Author birth | 1865-12-30 |
| Author death | 1936-01-18 |
| First published | 1894 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~50,000 |
| Structure | 7 stories + verse |

### Classification Signals

- **Content**: Children's fiction / fable collection
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Mowgli

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mowgli |
| Name tokens | `Mowgli` (label, new) |
| Gender | male |
| Description | Human child raised by wolves in the Indian jungle. Learns the Law of the Jungle from Baloo and Bagheera. Eventually defeats Shere Khan and returns to human society. |
| Prominence | major (protagonist) |

#### Shere Khan

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Shere Khan |
| Name tokens | `Shere` (label) + `khan` |
| Species | tiger |
| Status | dead |
| Description | Man-eating Bengal tiger. Mowgli's nemesis who claims the man-cub as his prey. Killed by Mowgli using a buffalo stampede. |
| Prominence | major (antagonist) |

#### Baloo

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Baloo |
| Name tokens | `Baloo` (label, new) |
| Species | bear |
| Description | Sleepy old brown bear who teaches the wolf cubs the Law of the Jungle. Mowgli's teacher and protector. |
| Prominence | major |

#### Bagheera

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Bagheera |
| Name tokens | `Bagheera` (label) |
| Species | black panther |
| Description | Sleek black panther who mentors and protects Mowgli. Born in captivity, escaped to the jungle. |
| Prominence | major |

#### Akela

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Akela |
| Name tokens | `akela` |
| Species | wolf |
| Description | The great grey Lone Wolf, leader of the Seeonee Pack. Allows Mowgli into the pack. Eventually deposed and later killed in battle. |
| Prominence | supporting |

#### Kaa

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Kaa |
| Name tokens | `Kaa` (label, new) |
| Species | python |
| Description | Thirty-foot rock python. Ancient, wise, and feared. Helps rescue Mowgli from the Bandar-log (monkeys). |
| Prominence | supporting |

#### Rikki-Tikki-Tavi

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Rikki-Tikki-Tavi |
| Name tokens | `Rikki` (label, new) + `tikki` + `Tavi` (label, new) |
| Species | mongoose |
| Description | Brave young mongoose adopted by a human family. Protects them by killing the cobras Nag and Nagaina. Protagonist of his own story within the collection. |
| Prominence | major (in his story) |

#### Nag

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Nag |
| Name tokens | `nag` |
| Species | cobra |
| Status | dead |
| Description | Large black cobra who terrorizes the garden. Killed by Rikki-Tikki-Tavi with the help of the big man. |
| Prominence | supporting (in Rikki's story) |

---

## 3. Non-Fiction People Entities (y*)

#### Rudyard Kipling

| Name tokens | `Rudyard` (label) + `Kipling` (label) |
| Gender | male |
| Born | 1865-12-30, Bombay, India |
| Died | 1936-01-18, London |
| Occupation | author, poet |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VJ | Mowgli | Fictional name, name-only |
| AB.AB.CA.HX.VK | Baloo | Fictional animal name, name-only |
| AB.AB.CA.HX.VL | Kaa | Fictional animal name, name-only |
| AB.AB.CA.HX.VM | Rikki | Part of Rikki-Tikki-Tavi, name-only |
| AB.AB.CA.HX.VN | Tavi | Part of Rikki-Tikki-Tavi, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Shere | AB.AB.CA.HL.YP | label |
| khan | AB.AB.CA.Cx.xb | noun |
| Bagheera | AB.AB.CA.GT.aP | label |
| akela | AB.AB.CA.AG.TZ | noun |
| tikki | AB.AB.CA.Fk.Mt | noun |
| nag | AB.AB.CA.Dh.hb | noun |
| hathi | AB.AB.CA.CY.lX | noun |
| Rudyard | AB.AB.CA.HI.yf | label |
| Kipling | AB.AB.CA.Gs.EB | label |
