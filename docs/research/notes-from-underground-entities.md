# Entity Catalog: Notes from the Underground (Gutenberg #600)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 600 |
| Title | Notes from the Underground |
| Author | Fyodor Dostoyevsky |
| Author birth | 1821-11-11 |
| Author death | 1881-02-09 |
| First published | 1864 |
| Language | English (en) — translation |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~44,000 |
| Structure | 2 parts (11 chapters + 10 chapters) |

### Classification Signals

- **Content**: Philosophical novella
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

Note: The narrator ("the Underground Man") is unnamed and does not receive an entity.

#### Liza

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Liza |
| Name tokens | `liza` |
| Gender | female |
| Occupation | prostitute |
| Description | A young prostitute whom the narrator meets at a brothel. He delivers a moralistic speech to her about her degradation, she comes to him seeking help, and he cruelly rejects her. |
| Prominence | major |

#### Zverkov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Zverkov |
| Name tokens | `Zverkov` (label, new) |
| Gender | male |
| Description | The narrator's former schoolmate. Handsome, successful, and self-satisfied. Object of the narrator's envy and resentment at the farewell dinner. |
| Prominence | supporting |

#### Simonov

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Simonov |
| Name tokens | `Simonov` (label, new) |
| Gender | male |
| Description | A former schoolmate. The only one of the narrator's old acquaintances who still tolerates him. Organizes Zverkov's farewell dinner. |
| Prominence | minor |

#### Apollon

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Apollon |
| Name tokens | `Apollon` (label) |
| Gender | male |
| Occupation | servant |
| Description | The narrator's elderly, imperious servant who torments him with silent contempt. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

#### Fyodor Dostoyevsky

Reused from Crime and Punishment catalog (yA.AA.AA.AA.AS).

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VE | Zverkov | Russian surname, name-only |
| AB.AB.CA.HX.VF | Simonov | Russian surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| liza | AB.AB.CA.DI.LQ | noun |
| Apollon | AB.AB.CA.GS.KG | label |
