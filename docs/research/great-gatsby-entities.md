# Entity Catalog: The Great Gatsby (Gutenberg #64317)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-15
**Status:** Complete — entities populated in DBs

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 64317 |
| Title | The Great Gatsby |
| Author | F. Scott Fitzgerald |
| Author birth | 1896-09-24 |
| Author death | 1940-12-21 |
| First published | 1925 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Classification | Fiction |
| Word count | ~47,000 |
| Structure | 9 chapters |

### Classification Signals

- **Content**: Novel / Jazz Age fiction
- **Verdict**: Fiction (high confidence)

---

## 2. Fiction People Entities (u*)

#### Jay Gatsby

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jay Gatsby |
| Name tokens | `jay` + `gatsby` |
| Gender | male |
| Status | dead (murdered) |
| Description | Mysterious millionaire who throws lavish parties at his West Egg mansion, all to win back Daisy Buchanan. Born James Gatz. Shot by George Wilson. |
| Prominence | major (title character) |

#### Nick Carraway

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Nick Carraway |
| Name tokens | `nick` + `carraway` |
| Gender | male |
| Description | The narrator. A Yale graduate from Minnesota who moves to West Egg and becomes Gatsby's neighbor and friend. Cousin to Daisy. |
| Prominence | major (narrator) |

#### Daisy Buchanan

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Daisy Buchanan |
| Name tokens | `daisy` + `Buchanan` (label) |
| Gender | female |
| Description | Beautiful, careless socialite. Gatsby's lost love. Married to Tom. Kills Myrtle Wilson while driving Gatsby's car but lets Gatsby take the blame. |
| Prominence | major |

#### Tom Buchanan

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Tom Buchanan |
| Name tokens | `tom` + `Buchanan` (label) |
| Gender | male |
| Description | Daisy's husband. Wealthy, brutish former football player. Has an affair with Myrtle Wilson. Directs George Wilson to Gatsby. |
| Prominence | major |

#### Jordan Baker

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Jordan Baker |
| Name tokens | `jordan` + `baker` |
| Gender | female |
| Occupation | professional golfer |
| Description | Daisy's friend and Nick's love interest. Cynical, dishonest professional golfer. |
| Prominence | supporting |

#### Myrtle Wilson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Myrtle Wilson |
| Name tokens | `myrtle` + `Wilson` (label) |
| Gender | female |
| Status | dead |
| Description | George Wilson's wife and Tom Buchanan's mistress. Killed when struck by Gatsby's car driven by Daisy. |
| Prominence | supporting |

#### George Wilson

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | George Wilson |
| Name tokens | `George` (label) + `Wilson` (label) |
| Gender | male |
| Status | dead (suicide) |
| Occupation | garage owner |
| Description | Myrtle's husband. Dull, spiritless garage owner. Murders Gatsby believing he killed Myrtle, then shoots himself. |
| Prominence | supporting |

#### Meyer Wolfsheim

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Meyer Wolfsheim |
| Name tokens | `Meyer` (label) + `Wolfsheim` (label, new) |
| Gender | male |
| Description | Gatsby's shady business associate. A gangster said to have fixed the 1919 World Series. |
| Prominence | minor |

---

## 3. Non-Fiction People Entities (y*)

#### F. Scott Fitzgerald

| Name tokens | `Scott` (label) + `Fitzgerald` (label) |
| Gender | male |
| Born | 1896-09-24, Saint Paul, Minnesota |
| Died | 1940-12-21, Hollywood, California |
| Occupation | novelist |

---

## 4. Label Token Audit

### 4.1 New Labels Created

| Token ID | Name | Reason |
|----------|------|--------|
| AB.AB.CA.HX.VP | Wolfsheim | Fictional surname, name-only |

### 4.2 Existing Tokens Used

| Name | Token ID | Type |
|------|----------|------|
| jay | AB.AB.CA.Ct.nN | noun |
| gatsby | AB.AB.CA.CL.CH | noun |
| nick | AB.AB.CA.Dm.JW | noun |
| carraway | AB.AB.CA.Au.cv | noun |
| daisy | AB.AB.CA.BV.dQ | noun |
| Buchanan | AB.AB.CA.GX.dP | label |
| tom | AB.AB.CA.Fl.ti | noun |
| jordan | AB.AB.CA.Cv.Aj | noun |
| baker | AB.AB.CA.AW.uV | noun |
| myrtle | AB.AB.CA.Dh.Ty | noun |
| Wilson | AB.AB.CA.HV.QW | label |
| George | AB.AB.CA.Gi.mc | label |
| Meyer | AB.AB.CA.Gz.Se | label |
| Scott | AB.AB.CA.HK.ga | label |
| Fitzgerald | AB.AB.CA.Gh.IB | label |
