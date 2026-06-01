# English Sub-categorization Pattern Catalog

**From:** hcp_ling (Linguistics Specialist)
**For:** hcp_db (DB Specialist) and Project Lead
**Date:** 2026-02-13
**Status:** MVP reference — covers core patterns and high-frequency verbs

---

## Q1 Answer: Linguistic Categories vs. Existing PoS Tokens

**Question:** Are the linguistic structural categories (N, V, A, P, NP, VP, etc.) the same entities as the existing PoS categories in hcp_english (noun, verb, adj), or distinct concepts?

### Answer: Same entities, different levels of granularity.

The p3 namespace in hcp_english already encodes PoS structurally:

| p3 | PoS | Linguistic Category | Count |
|---|---|---|---|
| CA | noun | N (Noun) | 930,925 |
| CB | verb | V (Verb) | 180,945 |
| CC | adj | A (Adjective) | 148,596 |
| CD | adv | Adv (Adverb) | 23,729 |
| CE | prep | P (Preposition) | 509 |
| CF | conj | C (Conjunction) | 209 |
| CG | det | DET (Determiner) | 173 |
| CH | pron | Pronoun (stands for NP) | 608 |
| CI | intj | Interjection | 3,024 |
| CJ | num | Numeral/QA | 456 |

**These ARE the same categories.** The textbook's N, V, A, P, Adv map directly to the p3 namespaces CA, CB, CC, CE, CD. No new entities needed for lexical categories.

**What IS distinct:** Phrasal categories (NP, VP, AP, PP, S, S', S'') do NOT exist as tokens and should NOT be tokens. They are **LoD-level labels** — they describe assembled structures, not individual words. An NP is what you get when you assemble tokens according to force rules; it's not itself a token in the DB.

### Specific observations about the current data:

1. **Dual-PoS words are only stored under one PoS.** "run", "give", "read" appear only in CA (noun), not CB (verb). Their verb inflections ("gave", "ran") sometimes appear in CB, sometimes don't. This is a data completeness issue, not a structural problem — the force patterns need to know that "run" CAN function as V, even though its primary entry is in CA.

2. **Solution for dual-PoS:** The sub-cat pattern system handles this. A token in CA (noun) can ALSO have a verb sub-cat pattern. The sub-cat is what determines how the token behaves structurally, not which p3 namespace it's in. Think of p3 as "primary PoS" and sub-cat as "structural capabilities."

3. **Concept tokens in hcp_core have existing PoS labels:** "noun" (AA.AC.AA.AB.Fh), "verb" (AA.AC.AA.AB.Fl), "adjective" (AA.AC.AA.AC.Ai) exist as concepts in the nsm_ab/nsm_ac layers. These are the CONCEPTS of grammatical categories — useful for meta-linguistic discussion but not the same as the structural categories used in parsing.

### Recommendation

- **No new category tokens needed.** The existing p3 namespace system IS the PoS structure.
- **Sub-cat patterns are what's new.** These are force constants that sit ON TOP of the PoS system, defining structural behavior within each category.
- **Dual-PoS needs a mechanism.** Either: (a) allow tokens to have multiple sub-cat patterns across PoS types (e.g., "run" has N_BARE + V_INTRANS + V_TRANS), or (b) create secondary entries in the appropriate p3 namespace. Option (a) is cleaner — the sub-cat pattern table is the authority on structural capabilities, not the p3 namespace.

---

## Pattern Definitions

### Verb Sub-categorization Patterns

Each pattern defines: slot count, slot types, grammatical functions, and the corresponding NSM conceptual frame.

#### Core Phrasal Patterns (complements are phrases, not clauses)

**V_INTRANS — Intransitive**
```
Slots:     0
Structure: V
Frame:     "X {does} something." — agent→action
Example:   "She laughed"
```

**V_TRANS — Transitive**
```
Slots:     1
Slot 1:    NP (direct object)
Structure: V + NP:dO
Frame:     "X {does} something {_to} Y." — agent→action→patient
Example:   "She read the book"
```

**V_DITRANS — Ditransitive**
```
Slots:     2
Slot 1:    NP (indirect object — recipient)
Slot 2:    NP (direct object — theme)
Structure: V + NP:iO + NP:dO
Frame:     "X {does} something {_to} Y {_with} Z." — agent→action→recipient+theme
Example:   "She gave him a book"
Alt form:  V + NP:dO + PP[to]:iO — "She gave a book to him"
```

**V_INTENS — Intensive (copular)**
```
Slots:     1
Slot 1:    AP / NP / PP (subject-predicative)
Structure: V + {AP|NP|PP}:sP
Frame:     "X {is} something." — theme→property
Example:   "She seemed happy" / "She became a doctor" / "She is in the garden"
```

**V_COMPLEX — Complex Transitive**
```
Slots:     2
Slot 1:    NP (direct object)
Slot 2:    AP / NP / PP (object-predicative)
Structure: V + NP:dO + {AP|NP|PP}:oP
Frame:     "X {does} something {_to} Y. After, Y {is} Z." — agent→action→patient+resultant_property
Example:   "She made him happy" / "They elected her president"
```

**V_PREP — Prepositional Verb**
```
Slots:     1
Slot 1:    PP with SPECIFIC preposition (prepositional complement)
Structure: V + PP[P]:PC
Frame:     "X {does} something {_P} Y." — agent→action→role(P-determined)
Example:   "She relied on him" / "He looked at the painting"
Note:      The specific preposition is part of the verb's pattern. "rely" selects "on".
```

**V_TRANS_PREP — Transitive + Prepositional Complement**
```
Slots:     2
Slot 1:    NP (direct object)
Slot 2:    PP with specific preposition
Structure: V + NP:dO + PP[P]:PC
Frame:     "X {does} something {_to} Y {_P} Z." — agent→action→patient+role
Example:   "She put the book on the shelf" / "He blamed the failure on bad luck"
Note:      "put" requires BOTH NP and PP — neither is optional.
```

**V_PARTICLE — Phrasal Verb (verb + particle)**
```
Slots:     1
Slot 1:    NP (direct object)
Particle:  Specified by verb entry
Structure: V + PART + NP:dO  OR  V + NP:dO + PART
Frame:     "X {does} something {_to} Y." — agent→action→patient (same as V_TRANS)
Example:   "She looked up the word" / "She looked the word up"
Note:      Particle obligatorily follows pronoun objects: "looked it up" not *"looked up it"
```

#### Clausal Complement Patterns (complements are clauses)

**V_THAT — That-clause complement**
```
Slots:     1
Slot 1:    S' (that-clause, declarative)
Structure: V + S':comp
Frame:     "X {knows/thinks/says} [proposition]." — experiencer/agent→proposition
Example:   "She knows that he left" / "He said that it was raining"
Note:      "that" often optional: "She knows ∅ he left"
```

**V_WH — Wh-clause complement**
```
Slots:     1
Slot 1:    S'' (wh-clause, interrogative)
Structure: V + S'':comp
Frame:     "X {wants_to_know} [question]." — experiencer→question
Example:   "She wonders where he went" / "He asked who came"
```

**V_INF — To-infinitive complement**
```
Slots:     1
Slot 1:    to-VP (to-infinitive clause, subject controlled by main subject)
Structure: V + [PRO to VP]:comp
Frame:     "X {wants_to_do} something." — agent→intended_action
Example:   "She wants to leave" / "He tried to help"
Control:   Main subject controls infinitive subject: She_i wants [PRO_i to leave]
```

**V_ING — -ing complement**
```
Slots:     1
Slot 1:    -ing VP (-ing participle clause)
Structure: V + [-ing VP]:comp
Frame:     "X {does/feels} something [ongoing]." — experiencer→ongoing_action
Example:   "She enjoys swimming" / "He avoids working"
Control:   Main subject controls -ing subject: She_i enjoys [PRO_i swimming]
```

**V_BARE — Bare infinitive complement (perception/causative)**
```
Slots:     2
Slot 1:    NP (perceived entity / caused agent)
Slot 2:    bare VP (bare infinitive clause)
Structure: V + NP + [bare VP]:comp
Frame:     "X {sees/hears} Y {do} something." — experiencer→perceived_agent+perceived_action
Example:   "She saw him leave" / "They made her cry"
Note:      NP is the subject of the bare infinitive
```

**V_NP_INF_I — NP + to-infinitive, Type I (raising/believe-type)**
```
Slots:     1 (complex)
Slot 1:    NP + to-VP (NP is subject of lower clause ONLY)
Structure: V + NP + [to VP]:comp
Frame:     "X {thinks} Y {is/does} something." — experiencer→believed_proposition
Example:   "She believes him to be guilty" / "I expected them to win"
Note:      NP is NOT the object of the main verb — it's the subject of the infinitive.
           Passivizes: "He is believed to be guilty" (NP moves to main subject)
```

**V_NP_INF_II — NP + to-infinitive, Type II (control/persuade-type)**
```
Slots:     2
Slot 1:    NP (direct object of main verb)
Slot 2:    to-VP (to-infinitive clause, subject controlled by slot 1)
Structure: V + NP:dO + [PRO to VP]:comp
Frame:     "X {does} something {_to} Y. Y then {does} Z." — agent→patient→caused_action
Example:   "She persuaded him to leave" / "I told them to wait"
Note:      NP IS the object of the main verb AND controls the infinitive subject.
           Passivizes differently: "He was persuaded to leave" (NP was main object)
```

**V_NP_ING — NP + -ing complement**
```
Slots:     2
Slot 1:    NP (perceived/found entity)
Slot 2:    -ing VP
Structure: V + NP + [-ing VP]:comp
Frame:     "X {sees/finds} Y {doing} something." — experiencer→perceived_agent+ongoing_action
Example:   "She saw him running" / "I found them sleeping"
```

**V_NP_THAT — NP + that-clause (communication verbs)**
```
Slots:     2
Slot 1:    NP (addressee)
Slot 2:    S' (that-clause)
Structure: V + NP:iO + S':comp
Frame:     "X {says} something {_to} Y: [proposition]." — agent→addressee+proposition
Example:   "She told him that it was raining" / "I convinced her that I was right"
```

---

## High-Frequency Verb Classification

### Methodology
The following classification covers the ~200 most commonly used English verbs, drawn from standard frequency lists. Each verb is classified into all patterns it participates in, with approximate relative frequency weights (how often it appears in each pattern relative to its total uses).

Frequency weights:
- **1.0** = Primary/dominant pattern (>60% of uses)
- **0.7** = Common alternate pattern (30-60%)
- **0.5** = Regular alternate pattern (15-30%)
- **0.3** = Occasional pattern (<15%)
- **0.1** = Rare but attested pattern

### BE (lexical)
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She is happy / a doctor / in the garden" |
Note: Lexical "be" always behaves syntactically as auxiliary (inverts, takes "not" directly). Always intensive — the prototypical copular verb. Also functions as auxiliary (PROG, PASS) but those are separate grammatical roles, not sub-cat patterns.

### HAVE (lexical)
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She has a book" |
| V_NP_INF_II | 0.3 | "She had him fix the car" |
| V_NP_ING | 0.3 | "I won't have you saying that" |
Note: Also functions as PERF auxiliary — separate role.

### DO (lexical)
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She did the work" |
| V_INTRANS | 0.5 | "She did well" |
Note: Also functions as support auxiliary — separate role.

### SAY
| Pattern | Weight | Example |
|---|---|---|
| V_THAT | 1.0 | "She said that he left" |
| V_TRANS | 0.5 | "She said something" |
| V_NP_THAT | 0.3 | "She said to him that..." |

### GET
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She got a book" |
| V_INTENS | 0.7 | "She got angry" |
| V_NP_INF_II | 0.5 | "She got him to leave" |
| V_NP_ING | 0.3 | "She got the car running" |
| V_COMPLEX | 0.3 | "She got the door open" |
| V_PREP | 0.3 | "She got to the station" |
Note: One of the most pattern-versatile verbs in English.

### MAKE
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She made a cake" |
| V_COMPLEX | 0.7 | "She made him happy" |
| V_BARE | 0.5 | "She made him leave" |
| V_NP_INF_II | 0.3 | "She made him to leave" (formal/archaic) |

### GO
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She went" |
| V_PREP | 0.7 | "She went to the store" |
| V_ING | 0.3 | "She went swimming" |

### KNOW
| Pattern | Weight | Example |
|---|---|---|
| V_THAT | 1.0 | "She knows that he left" |
| V_TRANS | 0.7 | "She knows the answer" |
| V_WH | 0.5 | "She knows where he went" |
| V_NP_INF_I | 0.3 | "She knows him to be honest" |

### TAKE
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She took the book" |
| V_DITRANS | 0.3 | "She took him a present" |
| V_TRANS_PREP | 0.3 | "She took the book to him" |
| V_PARTICLE | 0.5 | "She took off her coat" |

### COME
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She came" |
| V_PREP | 0.7 | "She came to the door" |
| V_INF | 0.3 | "She came to understand" |

### THINK
| Pattern | Weight | Example |
|---|---|---|
| V_THAT | 1.0 | "She thinks that he's right" |
| V_PREP | 0.5 | "She thinks about the problem" |
| V_NP_INF_I | 0.3 | "She thinks him to be honest" |
| V_TRANS | 0.3 | "She thinks good thoughts" |

### SEE
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She saw the car" |
| V_BARE | 0.5 | "She saw him leave" |
| V_NP_ING | 0.5 | "She saw him running" |
| V_THAT | 0.3 | "She saw that he was right" |
| V_WH | 0.3 | "She saw where he went" |

### WANT
| Pattern | Weight | Example |
|---|---|---|
| V_INF | 1.0 | "She wants to leave" |
| V_TRANS | 0.7 | "She wants a book" |
| V_NP_INF_II | 0.5 | "She wants him to leave" |

### GIVE
| Pattern | Weight | Example |
|---|---|---|
| V_DITRANS | 1.0 | "She gave him a book" |
| V_TRANS | 0.5 | "She gave a speech" |
| V_TRANS_PREP | 0.7 | "She gave the book to him" |

### TELL
| Pattern | Weight | Example |
|---|---|---|
| V_NP_THAT | 1.0 | "She told him that it was raining" |
| V_NP_INF_II | 0.7 | "She told him to leave" |
| V_DITRANS | 0.5 | "She told him a story" |
| V_TRANS | 0.5 | "She told the truth" |
| V_NP_WH | 0.3 | "She told him where to go" |

### ASK
| Pattern | Weight | Example |
|---|---|---|
| V_WH | 1.0 | "She asked where he went" |
| V_NP_INF_II | 0.7 | "She asked him to leave" |
| V_TRANS | 0.5 | "She asked a question" |
| V_NP_WH | 0.5 | "She asked him where he went" |
| V_THAT | 0.3 | "She asked that he be excused" |

### FIND
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She found the book" |
| V_THAT | 0.5 | "She found that it was true" |
| V_COMPLEX | 0.5 | "She found him guilty" |
| V_NP_ING | 0.3 | "She found him sleeping" |
| V_WH | 0.3 | "She found where he was" |

### PUT
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS_PREP | 1.0 | "She put the book on the shelf" |
Note: "put" REQUIRES both NP and PP — neither can be omitted. Unusual in having an obligatory PP that isn't a prepositional verb pattern (the preposition varies: on, in, under, etc.).

### SEEM
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She seemed happy / a good choice / in trouble" |
| V_INF | 0.5 | "She seemed to understand" |
| V_THAT | 0.3 | "It seems that she left" (with expletive subject) |

### BELIEVE
| Pattern | Weight | Example |
|---|---|---|
| V_THAT | 1.0 | "She believes that he's right" |
| V_TRANS | 0.5 | "She believes the story" |
| V_NP_INF_I | 0.5 | "She believes him to be guilty" |
| V_PREP | 0.3 | "She believes in honesty" |

### PERSUADE
| Pattern | Weight | Example |
|---|---|---|
| V_NP_INF_II | 1.0 | "She persuaded him to leave" |
| V_TRANS | 0.3 | "She persuaded him" |

### ENJOY
| Pattern | Weight | Example |
|---|---|---|
| V_ING | 1.0 | "She enjoys swimming" |
| V_TRANS | 0.5 | "She enjoys the music" |

### AVOID
| Pattern | Weight | Example |
|---|---|---|
| V_ING | 1.0 | "She avoids working late" |
| V_TRANS | 0.7 | "She avoids confrontation" |

### EXPECT
| Pattern | Weight | Example |
|---|---|---|
| V_INF | 1.0 | "She expects to win" |
| V_NP_INF_I | 0.7 | "She expects him to win" |
| V_THAT | 0.5 | "She expects that he'll win" |
| V_TRANS | 0.5 | "She expects the worst" |

### HELP
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She helped him" |
| V_NP_INF_II | 0.7 | "She helped him to escape" |
| V_BARE | 0.7 | "She helped him escape" |
| V_INF | 0.3 | "She helped to clean up" |

### LET
| Pattern | Weight | Example |
|---|---|---|
| V_BARE | 1.0 | "She let him leave" |
| V_TRANS | 0.3 | "She let the room" (British: rent out) |

### KEEP
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She kept the book" |
| V_ING | 0.7 | "She kept running" |
| V_COMPLEX | 0.5 | "She kept him waiting" / "She kept the door open" |
| V_PREP | 0.3 | "She kept to the path" |

### FEEL
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She felt happy / a fool / in danger" |
| V_TRANS | 0.5 | "She felt the fabric" |
| V_THAT | 0.5 | "She felt that he was wrong" |
| V_NP_ING | 0.3 | "She felt it moving" |
| V_BARE | 0.3 | "She felt it move" |

### BECOME
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She became happy / a doctor / of interest" |

### REMAIN
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She remained calm / a teacher / in the room" |

### APPEAR
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 1.0 | "She appeared calm / a threat" |
| V_INF | 0.5 | "She appeared to understand" |
| V_INTRANS | 0.5 | "She appeared suddenly" |

### LOOK
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 0.7 | "She looked happy / a mess" |
| V_PREP | 1.0 | "She looked at the painting" |
| V_PARTICLE | 0.5 | "She looked up the word" |
| V_INTRANS | 0.3 | "She looked carefully" |

### TURN
| Pattern | Weight | Example |
|---|---|---|
| V_INTENS | 0.7 | "She turned red" |
| V_TRANS | 0.7 | "She turned the page" |
| V_INTRANS | 0.5 | "She turned slowly" |
| V_PREP | 0.5 | "She turned to him" |
| V_PARTICLE | 0.5 | "She turned off the light" |

### LEAVE
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She left the room" |
| V_INTRANS | 0.7 | "She left" |
| V_COMPLEX | 0.5 | "She left the door open" |
| V_DITRANS | 0.3 | "She left him a message" |

### SHOW
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She showed the painting" |
| V_DITRANS | 0.7 | "She showed him the painting" |
| V_NP_THAT | 0.5 | "She showed him that it was true" |
| V_THAT | 0.5 | "The results show that..." |
| V_NP_WH | 0.3 | "She showed him where to go" |

### RUN
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She ran" |
| V_TRANS | 0.5 | "She ran the company" |
| V_PREP | 0.5 | "She ran into the room" |

### HEAR
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She heard the noise" |
| V_BARE | 0.5 | "She heard him leave" |
| V_NP_ING | 0.5 | "She heard him singing" |
| V_THAT | 0.3 | "She heard that he left" |

### SPEAK / TALK
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She spoke / talked" |
| V_PREP | 0.7 | "She spoke about the issue" / "talked to him" |

### WAIT
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She waited" |
| V_PREP | 0.7 | "She waited for him" |
| V_INF | 0.3 | "She waited to see" |

### DEPEND / RELY
| Pattern | Weight | Example |
|---|---|---|
| V_PREP | 1.0 | "She depends on him" / "relies on him" |

### LAUGH / SMILE / CRY / SLEEP / ARRIVE / EXIST / DIE
| Pattern | Weight | Example |
|---|---|---|
| V_INTRANS | 1.0 | "She laughed / smiled / cried / slept / arrived / existed / died" |
| V_PREP | 0.3 | "She laughed at him" / "cried about it" |
Note: Core intransitive verbs. Some take optional PP adjuncts (not complements).

### READ / WRITE / EAT / DRINK / SING / PLAY / DRIVE / BUILD / BREAK / OPEN / CLOSE / MOVE / CARRY / HOLD / PULL / PUSH / CUT / HIT / THROW / CATCH / DRAW / COOK / WASH / CLEAN / FIX
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She read the book" / "wrote a letter" / "ate the food" |
| V_INTRANS | 0.5 | "She read all day" / "wrote for hours" / "ate quickly" |
Note: Core transitive verbs with intransitive (absolutive) alternation. When used intransitively, the activity itself is the focus, not a specific patient. These verbs can also take various PP adjuncts.

### SEND
| Pattern | Weight | Example |
|---|---|---|
| V_DITRANS | 1.0 | "She sent him a letter" |
| V_TRANS | 0.7 | "She sent the letter" |
| V_TRANS_PREP | 0.7 | "She sent the letter to him" |

### OFFER / PROMISE
| Pattern | Weight | Example |
|---|---|---|
| V_DITRANS | 1.0 | "She offered/promised him a job" |
| V_INF | 0.7 | "She offered/promised to help" |
| V_TRANS | 0.5 | "She offered/promised her support" |

### TRY / ATTEMPT / MANAGE / FAIL
| Pattern | Weight | Example |
|---|---|---|
| V_INF | 1.0 | "She tried/attempted/managed/failed to escape" |
| V_TRANS | 0.3 | "She tried the soup" (try only) |

### CONSIDER
| Pattern | Weight | Example |
|---|---|---|
| V_TRANS | 1.0 | "She considered the options" |
| V_COMPLEX | 0.7 | "She considered him a fool / dangerous" |
| V_ING | 0.5 | "She considered leaving" |
| V_NP_INF_I | 0.3 | "She considered him to be a fool" |

### WONDER
| Pattern | Weight | Example |
|---|---|---|
| V_WH | 1.0 | "She wondered where he went" |
| V_THAT | 0.1 | "She wondered that he had survived" (literary) |

### STOP / START / BEGIN / CONTINUE / FINISH
| Pattern | Weight | Example |
|---|---|---|
| V_ING | 1.0 | "She stopped/started/began/continued/finished working" |
| V_INF | 0.7 | "She started/began/continued to work" (not stop/finish) |
| V_TRANS | 0.5 | "She stopped the car" / "started the engine" |

---

## Pattern Summary Statistics

| Pattern | Count of verbs above | Key exemplars |
|---|---|---|
| V_INTRANS | ~35 | laugh, arrive, sleep, die, run, go, come |
| V_TRANS | ~50 | read, hit, take, find, see, want, keep |
| V_DITRANS | ~8 | give, send, tell, show, offer, promise |
| V_INTENS | ~10 | be, seem, appear, become, remain, feel, look |
| V_COMPLEX | ~8 | make, consider, find, leave, keep, get |
| V_PREP | ~12 | rely, depend, look-at, think-about, go-to |
| V_TRANS_PREP | ~5 | put, blame, send-to, take-to |
| V_PARTICLE | ~5 | look-up, turn-off, take-off, pick-up |
| V_THAT | ~15 | know, think, believe, say, find, show, feel |
| V_WH | ~6 | wonder, ask, know, see, find, show |
| V_INF | ~12 | want, try, manage, start, offer, seem |
| V_ING | ~10 | enjoy, avoid, stop, start, keep, consider |
| V_BARE | ~5 | see, hear, let, make, help |
| V_NP_INF_I | ~5 | believe, expect, know, consider |
| V_NP_INF_II | ~8 | persuade, tell, ask, want, get, help |
| V_NP_ING | ~4 | see, hear, find, feel |
| V_NP_THAT | ~4 | tell, show, convince, inform |

### Multi-Pattern Distribution

Most verbs participate in 1-3 patterns. The most versatile verbs (get, make, take, see, find, tell, show, keep, feel, turn, look) participate in 4-6 patterns. These high-versatility verbs are also the highest-frequency verbs — there's a strong correlation between frequency and pattern diversity.

---

## NSM Frame Connection

Each sub-cat pattern activates a specific conceptual frame template from the NSM concept definitions:

| Pattern | NSM Prime Frame | Conceptual Structure |
|---|---|---|
| V_INTRANS | "X does something." | [agent]→[action] |
| V_TRANS | "X does something to Y." | [agent]→[action]→[patient] |
| V_DITRANS | "X does something to Y with Z." | [agent]→[action]→[recipient]+[theme] |
| V_INTENS | "X is something." | [theme]→[property] |
| V_COMPLEX | "X does something to Y. Y is Z." | [agent]→[action]→[patient]+[result_state] |
| V_PREP | "X does something P Y." | [agent]→[action]→[role:P-determined] |
| V_THAT | "X knows/thinks [that ...]." | [experiencer]→[proposition] |
| V_WH | "X wants-to-know [wh ...]." | [experiencer]→[question] |
| V_INF | "X wants-to-do something." | [agent]→[intended_action] |
| V_ING | "X does/feels something [ongoing]." | [experiencer]→[ongoing_action] |
| V_BARE | "X sees/hears Y do something." | [experiencer]→[perceived_agent]+[perceived_action] |
| V_NP_INF_I | "X thinks Y is/does Z." | [experiencer]→[believed_proposition] |
| V_NP_INF_II | "X does something to Y. Y does Z." | [agent]→[patient]→[caused_action] |

These frame connections enable the translation layer:
- **Input:** Grammatical analysis identifies the sub-cat pattern → selects NSM frame → maps surface tokens to concept tokens in the frame slots
- **Output:** Conceptual mesh has a frame structure → force patterns select the matching sub-cat pattern → surface tokens assembled according to ordering/compatibility rules

---

## Noun Sub-categorization Patterns

Less complex than verbs. Most nouns take no complements (adjuncts only).

**N_BARE — No complement** (vast majority)
```
Example: dog, idea, house, book, car
```

**N_PP_OF — PP[of] complement**
```
Example: "student of physics", "destruction of the city", "fear of heights"
These are often deverbal nouns (destruction←destroy, fear←fear) that inherit
argument structure from their verbal base.
```

**N_PP_VAR — PP complement with other preposition**
```
Example: "belief in God", "interest in music", "anger at the decision"
The preposition is lexically specified per noun.
```

**N_THAT — That-clause complement**
```
Example: "the claim that she left", "the fact that it rained", "the idea that he's right"
```

**N_INF — To-infinitive complement**
```
Example: "a desire to leave", "an attempt to escape", "the decision to stay"
Often deverbal: desire←desire, attempt←attempt.
```

## Adjective Sub-categorization Patterns

**A_BARE — No complement** (majority)
```
Example: tall, red, big, happy, old
```

**A_PP — PP complement**
```
Example: "fond of cheese", "angry at him", "interested in music", "good at chess"
Preposition lexically specified per adjective.
```

**A_THAT — That-clause complement**
```
Example: "aware that she left", "glad that it worked", "certain that he's right"
```

**A_INF_A — To-infinitive, Type A (subject → lower subject)**
```
Example: "eager to please", "reluctant to leave", "happy to help"
Control: She_i is eager [PRO_i to please (people)]
```

**A_INF_B — To-infinitive, Type B (subject → lower object)**
```
Example: "easy to please", "impossible to find", "hard to believe"
Control: She_i is easy [PRO (for people) to please PRO_i]
The subject of "easy" is the OBJECT of the infinitive.
```

## Preposition Sub-categorization Patterns

**P_NP — NP complement** (default for all prepositions)
```
Example: "in the garden", "on the table", "with a knife"
```

**P_S — S complement (temporal prepositions)**
```
Example: "after she left", "before he arrived", "until it stopped"
Only: after, before, until, since
```

**P_ING — -ing clause complement**
```
Example: "without leaving", "by working hard", "despite knowing"
```

---

## Implementation Notes for DB Specialist

### Pattern Table Design
The patterns above should be encoded as a lookup table in hcp_english. Recommended structure per the DB requirements doc:

```sql
CREATE TABLE sub_cat_patterns (
    pattern_id   TEXT PRIMARY KEY,    -- 'V_TRANS', 'V_DITRANS', etc.
    pos_category TEXT NOT NULL,       -- 'V', 'N', 'A', 'P'
    slot_count   SMALLINT NOT NULL,   -- 0-3
    description  TEXT,
    nsm_frame    TEXT                 -- NSM frame template label
);

CREATE TABLE sub_cat_slots (
    pattern_id   TEXT REFERENCES sub_cat_patterns(pattern_id),
    slot_num     SMALLINT NOT NULL,
    slot_cat     TEXT NOT NULL,       -- required category: NP, PP, S', to-VP, -ing-VP, bare-VP
    slot_func    TEXT NOT NULL,       -- grammatical function: dO, iO, sP, oP, PC, comp
    specific_p   TEXT,                -- for V_PREP: which preposition (on, at, about, etc.)
    control_type TEXT,                -- for clausal: 'subj_ctrl', 'obj_ctrl', 'raising', NULL
    PRIMARY KEY (pattern_id, slot_num)
);

CREATE TABLE token_sub_cat (
    token_id     TEXT NOT NULL,       -- FK to tokens table
    pattern_id   TEXT REFERENCES sub_cat_patterns(pattern_id),
    frequency    REAL DEFAULT 0.5,    -- relative frequency weight (0.0-1.0)
    specific_p   TEXT,                -- override for verb-specific preposition
    particle     TEXT,                -- for V_PARTICLE: the particle (up, off, out, etc.)
    PRIMARY KEY (token_id, pattern_id)
);
```

### Priority for MVP
1. Create the pattern table (17 verb + 5 noun + 5 adj + 3 prep = 30 patterns)
2. Create the slot table (~35 slots across all patterns)
3. Start classifying high-frequency verbs (the ~45 verbs detailed above)
4. Extend to remaining ~180,000 verb tokens over time

### Dual-PoS Handling
The `token_sub_cat` junction table handles dual-PoS words naturally. "run" (primary entry in CA/noun namespace) can have entries for both N_BARE and V_INTRANS + V_TRANS. The sub-cat table is the authority on what structural capabilities a token has, regardless of its primary p3 namespace.
