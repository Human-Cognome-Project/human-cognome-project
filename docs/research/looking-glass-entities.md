# Entity Catalog: Through the Looking-Glass (Gutenberg #12)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 12 |
| Title | Through the Looking-Glass, and What Alice Found There |
| Author | Lewis Carroll (Charles Lutwidge Dodgson) |
| Author birth | 1832-01-27 |
| Author death | 1898-01-14 |
| First published | 1871 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~30,000 |
| Structure | 12 chapters |

### Classification Signals

- **Content**: Children's fantasy, sequel to Alice's Adventures in Wonderland
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

Alice already exists as uA.AA.AA.AB.AA from Alice's Adventures in Wonderland.

#### Humpty Dumpty

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Humpty Dumpty |
| Name tokens | `humpty` + `dumpty` |
| Description | Egg-shaped character sitting on a wall. Argues about the meaning of words with Alice. Famous for his definition: "When I use a word, it means just what I choose it to mean." |
| Prominence | supporting |

#### Tweedledum

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tweedledum |
| Name tokens | `Tweedledum` (label, new) |
| Gender | male |
| Description | One of the identical twin brothers. Recites "The Walrus and the Carpenter." Fights Tweedledee over a rattle. |
| Prominence | supporting |

#### Tweedledee

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tweedledee |
| Name tokens | `Tweedledee` (label, new) |
| Gender | male |
| Description | The other identical twin. Indistinguishable from his brother. |
| Prominence | supporting |

#### The White Knight

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | White Knight |
| Name tokens | `white` + `knight` |
| Gender | male |
| Description | Kind, bumbling inventor. Escorts Alice to the final square. Often seen as Carroll's self-portrait. |
| Prominence | supporting |

#### The Jabberwock

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Jabberwock |
| Name tokens | `Jabberwock` (label) |
| Description | The fearsome creature from the poem "Jabberwocky." Slain by the young hero of the poem. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

Lewis Carroll already exists (yA.AA.AA.AA.AG). Entity appearance links this text to existing author record.

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.Uv | Tweedledum | Fictional character name, name-only |
| AB.AB.CA.HX.Uw | Tweedledee | Fictional character name, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Jabberwock | AB.AB.CA.Gp.bS | label |
| humpty | AB.AB.CA.Ch.If | noun |
| dumpty | AB.AB.CC.AM.mk | adj |
| white | AB.AB.CA.GH.cx | noun |
| knight | AB.AB.CA.Cz.gI | noun |
