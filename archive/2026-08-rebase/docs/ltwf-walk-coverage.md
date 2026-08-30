# LTWF Lesson-Walk Coverage

Running coverage of the LTWF lesson-walk (claim 460). Defined-set seeded with the official
**65 NSM primes** (our L1). Lessons 1–2 = the 61 primes (ostensive: example sentence + picture,
because primes are word-indefinable). Lessons 3–12 = ~300 molecules (L2, composition-checked).

**Status:** L1 confirmed (lessons 1–2). L2 (lesson 3+) pending careful-walk.

---

## L1 — primes (lessons 1–2): 61 LTWF items → 61 primes, ALL covered

`item | LTWF headword(+forms) → prime`

- 1-01 see → **SEE** = read_channel(visual)
- 1-02 thing/something/what → **SOMETHING~THING**
- 1-03 this/these → **THIS** = bind_index(focus)
- 1-04 other/else → **OTHER** = negate(SAME)
- 1-05 same/the same **+ be/am/are/is** → **THE SAME** (+ BE copula bundled)
- 1-06 one → **ONE**
- 1-07 two → **TWO**
- 1-08 person/people → **PEOPLE**
- 1-09 many/much → **MUCH~MANY**
- 1-10 inside → **INSIDE**
- 1-11 not → **NOT**
- 1-12 some → **SOME**
- 1-13 all → **ALL**
- 1-14 there is/are → **THERE IS** (exist)
- 1-15 more → **MORE**
- 1-16 live/alive → **LIVE**
- 1-17 big → **BIG**
- 1-18 small → **SMALL**
- 1-19 very → **VERY**
- 1-20 kind → **KIND** = link_tree/edge(class=tree)
- 1-21 if → **IF** = open_scope(conditional)
- 1-22 touch → **TOUCH**
- 1-23 far → **FAR**
- 1-24 near → **NEAR**
- 1-25 place/where → **WHERE~PLACE**
- 1-26 above → **ABOVE**
- 1-27 side → **SIDE**
- 1-28 hear → **HEAR** = read_channel(audio)
- 1-29 say → **SAY** = write_channel(expression)
- 1-30 word/words → **WORDS**
- 1-31 true → **TRUE**
- 2-01 like → **LIKE~AS~WAY** = declare(SOFTLINK)
- 2-02 have/belong → possession → **(IS) MINE** (LTWF uses old "have")
- 2-03 part → **PART** = link_tree/edge(class=tree)
- 2-04 do → **DO** = addForce→settle
- 2-05 happen → **HAPPEN** = DO(actor=null)
- 2-06 because → **BECAUSE** = edge(causal)
- 2-07 think → **THINK** = write(workspace)
- 2-08 know → **KNOW** = assert(warrant=high)
- 2-09 want → **WANT** = addForce(no settle)
- 2-10 can → **CAN**
- 2-11 bad → **BAD**
- 2-12 good → **GOOD**
- 2-13 feel → **FEEL** = write_channel(affect)
- 2-14 time/when → **WHEN~TIME**
- 2-15 before → **BEFORE** = order_edge
- 2-16 a long time → **A LONG TIME**
- 2-17 a short time → **A SHORT TIME**
- 2-18 move → **MOVE** = addForce(Δpos)→settle
- 2-19 I/me → **I** = mint/access primary ToM envelope
- 2-20 you → **YOU** = secondary-direct envelope
- 2-21 here → **HERE** = bind_index(spatial,proximal)
- 2-22 now → **NOW** = bind_index(temporal,present)
- 2-23 someone/who → **SOMEONE** = secondary-indirect-undefined envelope
- 2-24 after → **AFTER** = order_edge (inverse of BEFORE)
- 2-25 for some time → **FOR SOME TIME**
- 2-26 moment → **MOMENT**
- 2-27 body → **BODY** = Newtonian container
- 2-28 die → **DIE** = addForce(life:alive→dead)
- 2-29 maybe → **MAYBE** = set_confidence(sub_certain)
- 2-30 below → **BELOW** = ABOVE(b,a)

## Deltas (our 65 vs LTWF's 61 — covered by us, folded by LTWF)

- **LITTLE~FEW** (quantifier) — not separate in LTWF
- **DON'T WANT** (mental) — folded as not+want
- **BE (SOMEWHERE)** / **BE (SOMEONE/SOMETHING)** — bundled into 1-05
- terminology: `have` (LTWF) → `(IS) MINE` (current canon)

**Defined-set after L1 = all 65 primes (db-functions per 386/410/411).** Ready for L2.

---

## L2 — molecules (lessons 3–12)

Each item: does its explication compose from L1 + already-defined molecules? covered / GAP.

### Lesson 3 (3-01..3-31) — DI first-pass, Patrick-judgment pending. Coverage: CLEAN (no gaps).

**MOLECULES (15, covered)** — animal, use, become, make, contain, container, try, change,
surface, choose, machine, damage, difficult, easy, control
(all compose from L1 primes + earlier lesson-3 molecules in dependency order).

**FUNCTION-WORDS → construction layer (not L2)** — that, and, or, it/they/them, its/their,
your, my, a/an, the, but, each

**PRIME-OVERLAP → already L1 (drop from L2)** — cause(→BECAUSE), exist(→THERE IS), different(→not-SAME)

**BORDERLINE (Patrick's call)** — between, from (spatial: molecule vs construction); each (distributive quantifier)

### Lessons 4–12 — DI first-pass (batched). Circuit-breaker: PASSED (coherent, no compounding uncertainty).
Coverage clean by LTWF non-circular design; deep per-item composition-proof deferred to compile-time.

**RECLASSIFICATIONS / FLAGS (Patrick review):**
- **NUMERALS → SYMLINK to number-value area, base-50 value as symlink root** (Patrick 2026-06-07):
  three(5-05) four(5-06) five(5-07) six(8-08) seven(8-09) eight(8-10) nine(8-11) ten(8-12)
  hundred(8-30) thousand(10-01) zero(10-08). NOT molecules. (one/two = primes ONE/TWO, also point at values 1/2.)
- **MATH-OPERATORS → math-operator treatment (456, symbol+word symlinks):** add(12-13), multiply(10-25).
  (count(5-30) = action molecule, keep.)
- **UNITS (special molecules):** metre(10-04), kilogram(11-01). (day/month/year/hour = time molecules.)
- **META-LINGUISTIC (construction/meta, your call):** noun(12-12), verb(12-16), sentence(11-02), line(12-09).
- **BORDERLINE prime-overlap (descriptor-molecule vs prime, your call):** thin(7-03), tall(10-21),
  better(11-28→MORE+GOOD), similar(10-24→LIKE), hurt(7-17), piece(7-26), able(4-02→CAN), less(4-20→MORE).
- **FUNCTION-WORDS → construction (not L2):** towards, then, out, into, will, through, most, down, up,
  several, behind, around, he, she, only, we, by, which, yes, no, why, how, please

**L2 MOLECULE INVENTORY = all M-tagged below minus the reclassifications above (~230).**

--- raw first-pass classified lines (item | M/F/P | gloss) ---

# Lesson 4
4-01 put|M  4-02 able|P  4-03 shape|M  4-04 colour|M  4-05 towards|F  4-06 hold|M  4-07 pull|M
4-08 then|F  4-09 out|F  4-10 into|F  4-11 eye|M  4-12 look|M  4-13 mark|M  4-14 write|M  4-15 draw|M
4-16 plan|M  4-17 expect|M  4-18 important|M  4-19 tell|M  4-20 less|P  4-21 will|F  4-22 through|F
4-23 need|M  4-24 most|F  4-25 bottom|M  4-26 down|F  4-27 air|M  4-28 breathe|M  4-29 eat|M  4-30 food|M
# Lesson 5
5-01 gas|M  5-02 solid|M  5-03 hole|M  5-04 liquid|M  5-05 three|NUM  5-06 four|NUM  5-07 five|NUM
5-08 group|M  5-09 child|M  5-10 female|M  5-11 male|M  5-12 parent|M  5-13 mouth|M  5-14 drink|M
5-15 young|M  5-16 milk|M  5-17 end|M  5-18 up|F  5-19 lift|M  5-20 long|M  5-21 grow|M  5-22 heavy|M
5-23 length|M  5-24 connect|M  5-25 often|M  5-26 white|M  5-27 light|M  5-28 building|M  5-29 number|M
5-30 count|M  5-31 enjoy|M
# Lesson 6
6-01 water|M  6-02 plant|M  6-03 ground|M  6-04 dry|M  6-05 distance|M  6-06 narrow|M  6-07 wide|M
6-08 several|F  6-09 top|M  6-10 front|M  6-11 back|M  6-12 behind|F  6-13 quick|M  6-14 centre|M
6-15 round|M  6-16 around|F  6-17 sound|M  6-18 loud|M  6-19 high|M  6-20 low|M  6-21 prevent|M
6-22 fall|M  6-23 head|M  6-24 hit|M  6-25 stop|M  6-26 hot|M  6-27 cold|M  6-28 compare|M  6-29 weight|M  6-30 measure|M
# Lesson 7
7-01 flat|M  7-02 green|M  7-03 thin|P  7-04 tree|M  7-05 carry|M  7-06 sleep|M  7-07 arm|M  7-08 hand|M
7-09 adult|M  7-10 man|M  7-11 woman|M  7-12 he|F  7-13 she|F  7-14 start|M  7-15 burn|M  7-16 music|M
7-17 hurt|P  7-18 hard|M  7-19 press|M  7-20 promise|M  7-21 sexual|M  7-22 marry|M  7-23 family|M
7-24 likely|M  7-25 cut|M  7-26 piece|P  7-27 taste|M  7-28 circle|M  7-29 picture|M  7-30 stone|M  7-31 find|M
# Lesson 8
8-01 allow|M  8-02 turn|M  8-03 metal|M  8-04 vehicle|M  8-05 hair|M  8-06 twist|M  8-07 string|M
8-08 six|NUM  8-09 seven|NUM  8-10 eight|NUM  8-11 nine|NUM  8-12 ten|NUM  8-13 give|M  8-14 mix|M
8-15 paper|M  8-16 cover|M  8-17 rule|M  8-18 government|M  8-19 work|M  8-20 money|M  8-21 leg|M
8-22 help|M  8-23 disease|M  8-24 healthy|M  8-25 straight|M  8-26 sun|M  8-27 day|M  8-28 sky|M
8-29 mean|M  8-30 hundred|NUM  8-31 learn|M
# Lesson 9
9-01 seed|M  9-02 fruit|M  9-03 buy|M  9-04 black|M  9-05 clothing|M  9-06 cloth|M  9-07 bread|M
9-08 month|M  9-09 year|M  9-10 fly|M  9-11 bird|M  9-12 egg|M  9-13 yellow|M  9-14 red|M  9-15 square|M
9-16 electricity|M  9-17 blood|M  9-18 amount|M  9-19 read|M  9-20 country|M  9-21 soldier|M  9-22 story|M
9-23 push|M  9-24 atom|M  9-25 chemical|M  9-26 sweet|M
# Lesson 10
10-01 thousand|NUM  10-02 boat|M  10-03 rub|M  10-04 metre|UNIT  10-05 show|M  10-06 laugh|M  10-07 wheel|M
10-08 zero|NUM  10-09 business|M  10-10 hour|M  10-11 clay|M  10-12 explode|M  10-13 happy|M  10-14 angry|M
10-15 fear|M  10-16 radio|M  10-17 sad|M  10-18 careful|M  10-19 brown|M  10-20 love|M  10-21 tall|P
10-22 name|M  10-23 sit|M  10-24 similar|P  10-25 multiply|MATH  10-26 alcohol|M  10-27 fish|M
10-28 grain|M  10-29 salt|M  10-30 fat|M  10-31 coal|M
# Lesson 11
11-01 kilogram|UNIT  11-02 sentence|META  11-03 cat|M  11-04 sour|M  11-05 bone|M  11-06 clean|M
11-07 sheep|M  11-08 decide|M  11-09 god|M  11-10 nose|M  11-11 win|M  11-12 tube|M  11-13 flower|M
11-14 blue|M  11-15 smooth|M  11-16 school|M  11-17 lead|M  11-18 book|M  11-19 only|F  11-20 go|M
11-21 we|F  11-22 pay|M  11-23 first|M  11-24 explain|M  11-25 by|F  11-26 lesson|M  11-27 take|M
11-28 better|P  11-29 own|M  11-30 which|F
# Lesson 12
12-01 doctor|M  12-02 police|M  12-03 law|M  12-04 ask|M  12-05 question|M  12-06 yes|F  12-07 no|F
12-08 room|M  12-09 line|META  12-10 toilet|M  12-11 floor|M  12-12 noun|META  12-13 add|MATH  12-14 sharp|M
12-15 dangerous|M  12-16 verb|META  12-17 strong|M  12-18 why|F  12-19 how|F  12-20 right|M  12-21 left|M
12-22 tool|M  12-23 wood|M  12-24 dog|M  12-25 ear|M  12-26 car|M  12-27 house|M  12-28 hello|M
12-29 please|F  12-30 thank|M  12-31 sorry|M
