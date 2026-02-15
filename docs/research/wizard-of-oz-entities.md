# Entity Catalog: The Wonderful Wizard of Oz (Gutenberg #55)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 55 |
| Title | The Wonderful Wizard of Oz |
| Author | L. Frank Baum (Lyman Frank Baum) |
| Author birth | 1856-05-15 |
| Author death | 1919-05-06 |
| First published | 1900 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~38,000 |
| Structure | 24 chapters |

### Classification Signals

- **Content**: Children's fantasy
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Dorothy

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Dorothy |
| Name tokens | `Dorothy` (label) |
| Gender | female |
| Description | A Kansas farm girl swept to the Land of Oz by a tornado. Seeks the Wizard to help her return home. Discovers the power was in her silver shoes all along. |
| Prominence | major (protagonist) |

#### The Scarecrow

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Scarecrow |
| Name tokens | `scarecrow` |
| Gender | male |
| Description | Animated scarecrow who wants a brain. Proves himself clever throughout the journey. Becomes ruler of the Emerald City. |
| Prominence | major |

#### The Tin Woodman

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tin Woodman |
| Name tokens | `tin` + `woodman` |
| Gender | male |
| Description | A woodman made entirely of tin who wants a heart. Shows great tenderness throughout. Becomes ruler of the Winkies. |
| Prominence | major |

#### The Cowardly Lion

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Cowardly Lion |
| Name tokens | `cowardly` + `lion` |
| Species | lion |
| Description | A lion who wants courage. Proves himself brave many times. Becomes King of the Beasts. |
| Prominence | major |

#### Glinda

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Glinda |
| Name tokens | `Glinda` (label, new) |
| Gender | female |
| Occupation | Good Witch of the South |
| Description | The Good Witch of the South who reveals to Dorothy how to use the silver shoes to go home. |
| Prominence | supporting |

#### Toto

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Toto |
| Name tokens | `toto` |
| Species | dog |
| Description | Dorothy's little black dog. Her faithful companion throughout the adventure. |
| Prominence | supporting |

---

## 3. Non-Fiction People Entities (y*)

#### L. Frank Baum

| Name tokens | `Lyman` (label) + `frank` + `Baum` (label) |
| Gender | male |
| Born | 1856-05-15, Chittenango, New York |
| Died | 1919-05-06, Hollywood, California |
| Occupation | author, playwright |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VA | Glinda | Fictional witch name, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| Dorothy | AB.AB.CA.Ge.KS | label |
| Baum | AB.AB.CA.GU.WB | label |
| Lyman | AB.AB.CA.Gw.Qq | label |
| scarecrow | AB.AB.CA.Ew.In | noun |
| tin | AB.AB.CA.Fk.ni | noun |
| woodman | AB.AB.CA.GK.GH | noun |
| lion | AB.AB.CA.DH.Mv | noun |
| toto | AB.AB.CA.Fm.xj | noun |
| oz | AB.AB.CA.Dz.vQ | noun |
| frank | AB.AB.CA.CG.fq | noun |
| cowardly | AB.AB.CC.AK.Ct | adj |
