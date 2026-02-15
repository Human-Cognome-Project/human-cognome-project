# Entity Catalog: The Call of the Wild (Gutenberg #215)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 215 |
| Title | The Call of the Wild |
| Author | Jack London |
| Author birth | 1876-01-12 |
| Author death | 1916-11-22 |
| First published | 1903 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~31,000 |
| Structure | 7 chapters |

### Classification Signals

- **Content**: Adventure fiction, animal protagonist
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### John Thornton

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | John Thornton |
| Name tokens | `john` + `Thornton` (label) |
| Gender | male |
| Status | dead (killed by Yeehat Indians) |
| Occupation | gold prospector |
| Description | Buck's beloved final master. Rescues Buck from Hal's cruelty. Killed by Yeehat Indians at his camp. |
| Prominence | major |

#### Hal

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Hal |
| Name tokens | `hal` |
| Gender | male |
| Status | dead (falls through ice) |
| Description | Inexperienced, cruel man from the States. Overloads the sled and mistreats the dogs. Falls through lake ice. |
| Prominence | supporting |

#### François

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | François |
| Name tokens | `françois` |
| Gender | male |
| Occupation | dog sled driver |
| Description | French-Canadian driver for the mail service. Competent and generally fair to the dogs. |
| Prominence | supporting |

#### Perrault

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Perrault |
| Name tokens | `Perrault` (label) |
| Gender | male |
| Occupation | mail courier |
| Description | Government mail courier. François's partner on the mail run. |
| Prominence | supporting |

### Named Creatures (u*.AD)

#### Buck

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Buck |
| Name tokens | `buck` |
| Species | dog (St. Bernard / Scotch Shepherd mix) |
| Description | The protagonist. A domesticated dog stolen from California and sold into service as a sled dog in the Yukon during the Gold Rush. Returns to the wild. |
| Prominence | major (protagonist) |

#### Spitz

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Spitz |
| Name tokens | `spitz` |
| Species | dog (Spitz breed) |
| Status | dead (killed by Buck) |
| Description | Buck's rival for lead dog position. Aggressive and cunning. Killed by Buck in a decisive fight. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Jack London

| Name tokens | `jack` + `London` (label) |
| Gender | male |
| Born | 1876-01-12, San Francisco |
| Died | 1916-11-22, Glen Ellen, California |
| Occupation | novelist, journalist |

---

## 4. Label Token Audit

### 4.1 New Labels Created

None — all names from existing tokens.

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Thornton | AB.AB.CA.HQ.mA | label |
| Perrault | AB.AB.CA.HE.ts | label |
| Mercedes | AB.AB.CA.Gz.Iz | label |
| London | AB.AB.CA.Gv.ir | label |
| buck | AB.AB.CA.An.GX | noun |
| spitz | AB.AB.CA.FP.dL | noun |
| hal | AB.AB.CA.CW.fG | noun |
| john | AB.AB.CA.Cu.qL | noun |
| jack | AB.AB.CA.Cs.uA | noun |
