# Entity Catalog: Alice's Adventures in Wonderland (Gutenberg #11)

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-14
**Status:** Complete — entities populated in DBs
**PBM Document:** zA.AB.CA.AA.AB

---

## 1. Document Metadata

| Field | Value |
|-------|-------|
| Gutenberg ID | 11 |
| Title | Alice's Adventures in Wonderland |
| Author | Lewis Carroll (Charles Lutwidge Dodgson) |
| Author birth | 1832-01-27 |
| Author death | 1898-01-14 |
| First published | 1865 |
| Language | English (en) |
| Copyright | false (public domain in the US) |
| Rights status | public_domain |
| Download count | ~53,000 |
| Classification | Fiction |
| Word count | ~27,000 |
| Structure | 12 chapters |

### Gutenberg Subject Headings (LCSH)

- Alice (Fictitious character from Carroll) -- Juvenile fiction
- Children's stories
- Fantasy fiction
- Imaginary places -- Juvenile fiction

### Gutenberg Bookshelves

- Category: British Literature
- Category: Children & Young Adult Reading
- Category: Classics of Literature
- Category: Novels
- Children's Literature

### Classification Signals

- **Bookshelves**: "Category: Novels", "Children's Literature" → fiction
- **Subjects**: "Fantasy fiction", "Imaginary places" → fiction
- **Subject entities**: "(Fictitious character from Carroll)" pattern → fiction
- **Verdict**: Fiction (high confidence, multiple signals)

---

## 2. Fiction People Entities (u*)

### 2.1 Major Characters

#### Alice

| Field | Value |
|-------|-------|
| Namespace | u* |
| Sub-type | AA (individual) |
| Primary name | Alice |
| Name tokens | `Alice` (label) |
| Gender | female |
| Status | alive |
| Species | human |
| Age | approximately 7 years old |
| Description | A young English girl who falls down a rabbit hole into Wonderland. Curious, polite, and increasingly assertive. Grows and shrinks repeatedly. The protagonist and sole point-of-view character. |
| Prominence | major (protagonist) |

**Key relationships:**
- `owner_of` → Dinah (her cat)
- `interacts_with` → White Rabbit, Cheshire Cat, Queen of Hearts, Mad Hatter, etc.

---

### 2.2 Major Wonderland Characters

#### The White Rabbit

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The White Rabbit |
| Name tokens | `the` + `white` + `rabbit` |
| Aliases | White Rabbit |
| Gender | male |
| Species | anthropomorphic rabbit |
| Description | A waistcoat-wearing, pocket-watch-carrying rabbit perpetually worried about being late. Alice follows him down the rabbit hole. He serves as herald to the King and Queen of Hearts. |
| Prominence | major |

---

#### The Cheshire Cat

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Cheshire Cat |
| Name tokens | `the` + `Cheshire` (label) + `cat` |
| Aliases | Cheshire Cat, Cheshire-Puss |
| Gender | male |
| Species | anthropomorphic cat |
| Description | A mysterious grinning cat belonging to the Duchess. Can appear and disappear at will, often leaving only his grin visible. Provides philosophical and cryptic advice to Alice. |
| Prominence | major |

---

#### The Queen of Hearts

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The Queen of Hearts |
| Name tokens | `the` + `queen` + `of` + `hearts` |
| Aliases | Queen |
| Gender | female |
| Species | playing-card person |
| Description | Tyrannical ruler of Wonderland who constantly orders executions ("Off with their heads!"). Hosts the croquet game with flamingo mallets and hedgehog balls. Her orders are rarely carried out. |
| Prominence | major |

---

#### The Mad Hatter

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The Hatter |
| Name tokens | `the` + `Hatter` |
| Aliases | Mad Hatter |
| Gender | male |
| Species | human (Wonderland) |
| Description | Host of the perpetual tea party, stuck at 6 o'clock after a quarrel with Time. Poses unanswerable riddles and makes nonsensical conversation. A witness at the trial of the Knave of Hearts. |
| Prominence | major |

---

#### The March Hare

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The March Hare |
| Name tokens | `the` + `march` + `hare` |
| Gender | male |
| Species | anthropomorphic hare |
| Description | Co-host of the mad tea party with the Hatter. Equally nonsensical. Offers Alice wine that doesn't exist and makes contradictory statements. |
| Prominence | supporting |

---

#### The Caterpillar

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Caterpillar |
| Name tokens | `the` + `caterpillar` |
| Aliases | Hookah-Smoking Caterpillar |
| Gender | male |
| Species | anthropomorphic caterpillar |
| Description | A blue caterpillar sitting on a mushroom, smoking a hookah. Interrogates Alice about her identity ("Who are you?"). Tells her the mushroom can change her size. |
| Prominence | supporting |

---

#### The Mock Turtle

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Mock Turtle |
| Name tokens | `the` + `mock` + `turtle` |
| Gender | male |
| Species | mock turtle (fictional creature) |
| Description | A melancholy creature who was once a real turtle. Tells Alice about his schooldays in the sea and sings "Turtle Soup." Accompanied by the Gryphon. |
| Prominence | supporting |

---

#### The Gryphon

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Gryphon |
| Name tokens | `the` + `gryphon` |
| Gender | male |
| Species | gryphon (mythical creature) |
| Description | A gryphon who accompanies the Mock Turtle. More brisk and practical than his companion. Takes Alice to meet the Mock Turtle on the Queen's orders. |
| Prominence | supporting |

---

### 2.3 Minor Characters

**The Duchess:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The Duchess |
| Name tokens | `the` + `duchess` |
| Gender | female |
| Description | Owner of the Cheshire Cat. First appears in a chaotic kitchen with a sneezing baby and a violent cook. Later friendly and moralizing at the croquet ground. |
| Prominence | supporting |

---

**The King of Hearts:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The King of Hearts |
| Name tokens | `the` + `king` + `of` + `hearts` |
| Gender | male |
| Description | The Queen's husband. Presides over the trial of the Knave of Hearts. Much less forceful than the Queen; quietly pardons those she condemns. |
| Prominence | supporting |

---

**The Knave of Hearts:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The Knave of Hearts |
| Name tokens | `the` + `knave` + `of` + `hearts` |
| Gender | male |
| Description | Accused of stealing the Queen's tarts. His trial forms the climax of the story. |
| Prominence | minor |

---

**The Dormouse:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Dormouse |
| Name tokens | `the` + `dormouse` |
| Gender | male |
| Description | Third member of the tea party. Perpetually sleepy, periodically tells a story about three sisters who lived in a treacle-well. |
| Prominence | minor |

---

**Bill the Lizard:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Bill the Lizard |
| Name tokens | `bill` + `the` + `lizard` |
| Aliases | Bill |
| Gender | male |
| Description | A hapless lizard sent down the chimney by the White Rabbit. Gets kicked out by Alice when she's grown too large. Later a juror at the trial. |
| Prominence | minor |

---

**The Mouse:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Mouse |
| Name tokens | `the` + `mouse` |
| Description | The first creature Alice meets in Wonderland's pool of tears. Tells a dry history and his own "long and sad tale/tail." |
| Prominence | minor |

---

**The Dodo:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Dodo |
| Name tokens | `the` + `dodo` |
| Description | Organizes the Caucus-Race to dry off the animals from the pool of tears. Speaks in a pompous, formal manner. |
| Prominence | minor |

---

**The Lory:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Lory |
| Name tokens | `the` + `lory` |
| Description | A parrot-like bird in the pool of tears scene. Arguments with Alice about age and authority. |
| Prominence | minor |

---

**The Pigeon:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | The Pigeon |
| Name tokens | `the` + `pigeon` |
| Description | Attacks Alice when her neck grows long, mistaking her for a serpent after eggs. |
| Prominence | minor |

---

**The Cook:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | The Cook |
| Name tokens | `the` + `cook` |
| Description | The Duchess's violent cook. Throws dishes and puts too much pepper in everything. |
| Prominence | minor |

---

**Pat:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Pat |
| Name tokens | `pat` |
| Description | The White Rabbit's servant, heard outside when Alice is trapped in the house. |
| Prominence | minor |

---

**Dinah:**

| Field | Value |
|-------|-------|
| Sub-type | AD (named creature) |
| Primary name | Dinah |
| Name tokens | `Dinah` (label) |
| Species | cat |
| Description | Alice's cat in the real world. Alice mentions her fondly throughout her adventures, often inadvertently frightening other animals. |
| Prominence | minor (referenced only, not present in Wonderland) |

---

**Mary Ann:**

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Mary Ann |
| Name tokens | `mary` + `ann` |
| Description | The White Rabbit's housemaid. The Rabbit mistakes Alice for Mary Ann and sends her on an errand. Never actually appears. |
| Prominence | minor (referenced only) |

---

## 3. Non-Fiction People Entities (y*)

### 3.1 Author

#### Lewis Carroll

| Field | Value |
|-------|-------|
| Sub-type | AA (individual) |
| Primary name | Lewis Carroll |
| Name tokens | `lewis` + `carroll` |
| Real name | Charles Lutwidge Dodgson |
| Real name tokens | `Charles` (label) + `Lutwidge` (label) + `Dodgson` (label) |
| Gender | male |
| Born | 1832-01-27, Daresbury, Cheshire |
| Died | 1898-01-14, Guildford, Surrey |
| Occupation | author, mathematician, logician, photographer |
| Description | English author and Oxford mathematics lecturer. Pen name of Charles Lutwidge Dodgson. Wrote Alice's Adventures in Wonderland for Alice Liddell. |

---

## 4. Non-Fiction Place Entities (x*)

Alice's Adventures in Wonderland contains no explicitly named real-world locations. The story begins by a riverbank (implied Oxford area) and takes place entirely in Wonderland. No real places need to be cataloged for this text.

---

## 5. Non-Fiction Thing Entities (w*)

No organizations, objects, or other real-world things are explicitly named in the text.

---

## 6. Label Token Audit

### 6.1 Existing Tokens (no creation needed)

All character names in Alice in Wonderland are composed of common English words or existing labels:

| Name | Token(s) | Status |
|------|----------|--------|
| Alice | `Alice` (AB.AB.CA.GR.PL, label) | exists |
| Cheshire | `Cheshire` (AB.AB.CA.GZ.zE, label) | exists |
| Dinah | `Dinah` (AB.AB.CA.Gd.mU, label) | exists |
| Ada | `Ada` (AB.AB.CA.GQ.aP, label) | exists |
| Edith | `Edith` (AB.AB.CA.Gf.Ig, label) | exists |
| Mabel | `Mabel` (AB.AB.CA.Gw.UU, label) | exists |
| rabbit | `rabbit` (AB.AB.CA.Ef.Fq, noun) | exists |
| hatter | `hatter` (AB.AB.CA.CY.mr, noun) | exists |
| caterpillar | `caterpillar` (AB.AB.CA.Av.cL, noun) | exists |
| dormouse | `dormouse` (AB.AB.CA.Bh.Ue, noun) | exists |
| gryphon | `gryphon` (AB.AB.CA.CU.XE, noun) | exists |
| dodo | `dodo` (AB.AB.CA.Bg.RU, noun) | exists |
| lory | `lory` (AB.AB.CA.DJ.hM, noun) | exists |
| eaglet | `eaglet` (AB.AB.CA.Bl.Kx, noun) | exists |
| turtle | `turtle` (AB.AB.CA.Ft.lV, noun) | exists |
| duchess | `duchess` (AB.AB.CA.Bj.mM, noun) | exists |
| queen | `queen` (AB.AB.CA.Ee.PK, noun) | exists |
| king | `king` (AB.AB.CA.Cy.mD, noun) | exists |
| knave | `knave` (AB.AB.CA.Cz.Yy, noun) | exists |
| cook | `cook` (AB.AB.CA.BL.Tc, noun) | exists |
| pat | `pat` (AB.AB.CA.EE.ap, noun) | exists |
| bill | `bill` (AB.AB.CA.Ad.ZZ, noun) | exists |
| mary | `mary` (AB.AB.CA.DQ.km, noun) | exists |
| ann | `ann` (AB.AB.CA.AL.PW, noun) | exists |
| mouse | `mouse` (AB.AB.CA.De.bc, noun) | exists |
| pigeon | `pigeon` (AB.AB.CA.EM.Pv, noun) | exists |
| lizard | `lizard` (AB.AB.CA.DI.LR, noun) | exists |
| fish | `fish` (AB.AB.CA.CB.qQ, noun) | exists |
| frog | `frog` (AB.AB.CA.CH.el, noun) | exists |

### 6.2 New Labels Created

None — all names composed from existing tokens.

### 6.3 Author Tokens

| Name | Token(s) | Status |
|------|----------|--------|
| Lewis | `lewis` (AB.AB.CA.DF.gh, noun) | exists |
| Carroll | `carroll` (AB.AB.CA.Au.fe, noun) | exists |
| Charles | `Charles` (AB.AB.CA.GZ.nI, label) | exists |
| Lutwidge | `Lutwidge` (AB.AB.CA.Gw.Mn, label) | exists |
| Dodgson | `Dodgson` (AB.AB.CA.Gd.ye, label) | exists |
