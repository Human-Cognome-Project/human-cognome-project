# Entity Catalog: Around the World in Eighty Days (Gutenberg #103)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 103 |
| Title | Around the World in Eighty Days |
| Author | Jules Verne |
| Author birth | 1828-02-08 |
| Author death | 1905-03-24 |
| First published | 1873 |
| Language | English (en) — translation |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~62,000 |
| Structure | 37 chapters |

### Classification Signals

- **Content**: Adventure novel
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Phileas Fogg

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Phileas Fogg |
| Name tokens | `Phileas` (label, new) + `Fogg` (label) |
| Gender | male |
| Description | An eccentric, methodical English gentleman who wagers he can travel around the world in eighty days. Wins the bet by exploiting the International Date Line. Marries Aouda. |
| Prominence | major (protagonist) |

#### Passepartout

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Passepartout |
| Name tokens | `Passepartout` (label, new) |
| Gender | male |
| Occupation | valet |
| Description | Fogg's loyal French valet. Resourceful and excitable. His forgetting to adjust his watch proves crucial to winning the wager. |
| Prominence | major |

#### Detective Fix

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Detective Fix |
| Name tokens | `fix` |
| Gender | male |
| Occupation | detective |
| Description | A Scotland Yard detective who pursues Fogg around the world, convinced he is a bank robber. Repeatedly delays and hinders the journey. |
| Prominence | supporting (antagonist) |

#### Aouda

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Aouda |
| Name tokens | `Aouda` (label, new) |
| Gender | female |
| Description | A young Indian woman rescued from suttee (forced immolation) by Fogg and Passepartout. Educated, beautiful. Falls in love with Fogg and marries him. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### Jules Verne

| Name tokens | `Jules` (label) + `Verne` (label) |
| Gender | male |
| Born | 1828-02-08, Nantes, France |
| Died | 1905-03-24, Amiens, France |
| Occupation | author |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VV | Phileas | Fictional first name, name-only |
| AB.AB.CA.HX.VW | Passepartout | Fictional name, name-only |
| AB.AB.CA.HX.VX | Aouda | Fictional name, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Fogg | AB.AB.CA.Gh.Rg | label |
| fix | AB.AB.CA.CC.Jd | noun |
| Jules | AB.AB.CA.Gq.Sx | label |
| Verne | AB.AB.CA.HT.Xe | label |
