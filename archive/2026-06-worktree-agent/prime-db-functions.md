# Primes as DB Functions

The meaning of a prime is **not** its label or its English gloss — it is a **pure function over a
small substrate of database operations**. This page is the reference for that substrate (the ISA)
and for the 65 NSM primes expressed as signatures over it. How a *molecule* or word composes these
into an explication is covered in [explication.md](explication.md); the particle taxonomy itself is
in [primes-and-molecules.md](primes-and-molecules.md).

Sources: claims 385 (substrate op library), 386 (prime pure-function signatures), 410 (merged ISA),
411 (WAY/manner), 393 (substantive pronouns), 374 (the compile plan), 419 (labels are handles).

> **Status — design, not yet built.** These are the *design* definitions held in the orchestrator
> claim-graph. They are **not yet finalized in `hcp_core`**, and the NSM data currently sitting in
> core is a structurally-sound-but-content-dirty candidate pool, *not* a trusted source (claim 416).
> Finalizing these signatures into core L1 — attached to the prime tokens, symlinked to their English
> senses — is the next build step (claim 418).

---

## The db-function set (the ISA)

Per the "compared-to-where" principle (claim 409), only the **irreducible floor** is an absolute
stratum; everything above it is a level-relative convenience placed for the build.

### Irreducible floor (~10 ops)

| op | role |
|----|------|
| `assert_node(x)` | bring a node into being (`mint` = this + fresh id + type) |
| `edge(a,b,rel,class)` | typed relation; `class ∈ {tree, symlink, softlink, plain}` governs propagation — subsumes `link_tree`, `declare`, `order_edge` |
| `set_attr(node,dim,val,frame?)` | set a dimension — subsumes `set_confidence`, and `amplify`/`increment` (delta mode) |
| `negate(x)` | flip |
| `bind(ref)` | resolve a reference to a node — subsumes `ref` |
| `addForce(tgt)→settle` | dynamics; **settled** = action, **unsettled** = intention |
| `index-access` (get/set) | over focus / temporal / spatial registers — fuses `bind_index` + `locate` |
| `channel-access` (get/set) | over modalities incl. workspace — fuses `read_channel` + `write_channel` |
| `select_subset(set,pred)` | predicate selection |
| `open_scope(kind,gate,body)` | gated scope — subsumes `scope` |

### Convenience layer

The named ops used in the signatures below; each is a scope-relative SYMLINK to a floor op and is
kept only for build legibility: `amplify`/`increment` → `set_attr`-delta · `ref`/`region` →
`bind`/`locate` · `gate` & `assert`-with-warrant → `assert_node` + warrant-as-attr ·
`write(workspace)` → channel-access · `scope` → `open_scope` · `link_tree`/`declare`/`order_edge` →
`edge`+class · `mint_instance` → `assert_node`+fresh+type.

### Illocutionary / boundary family

The punctuation-borne operators NSM omits (it works word-only and assumes the listener decodes
prosody — the interpreter-baked assumption we explicitly model instead). Already extracted in claims
360/371; folding into the basis is part of closing the ISA.

`.` = COMMIT + close envelope · `!` = EMPH (force-amplification) · `,` `;` = graded boundary-force ·
`?` = open query-scope + flip `assert`→`read` + request-fill on an undefined slot · `"…"` =
frame/channel shift (mention-not-use) · `()` `—` = subordinate scope.

Same words, different op-tree: `going.` = `assert`+COMMIT · `going?` = open-query+`read` · `going!` =
`assert`+EMPH.

---

## The 65 primes as explications

Signatures verbatim from claim 386, by NSM category, with the two post-386 refinements folded in
(see [Refinements](#refinements-since-386)).

### Substantives (claim 393)

Operation is `mint` (new) or `access` (existing). The animate substantives are a Theory-of-Mind
perspective taxonomy:

- `I` = primary ToM perspective envelope (the self)
- `YOU` = secondary / direct envelope (the addressee)
- `SOMEONE` = secondary / indirect envelope, **undefined** target
- `PEOPLE` = group ToM envelope (collective)
- `THING` / `SOMETHING` = a specific / undefined **literal instance** (grounded symbol, no ToM)
- `BODY` = the Newtonian container — the 3D vessel housing a perspective

### Relational

`KIND(parent,child) = link_tree(KIND,parent,child)` · `PART(whole,part) = link_tree(PART,whole,part)` ·
`LIKE(a,b,deg,scope) = declare(SOFTLINK,a,b,scope)` · `SAME(a,b) = declare(SYMLINK,a,b,local)` ·
`SYM(a,b,scope) = declare(SYMLINK,a,b,scope)`

### Determiners

`THIS(x) = bind_index(focus,x)` · `OTHER(x,ref) = negate(SAME(x,ref))`

### Quantifiers

`ONE/TWO(x) = set_attr(x,card,n)` · `SOME(set) = select_subset(set,partial)` ·
`ALL(set) = select_subset(set,complete)` · `MANY/FEW(x) = set_attr(x,card,large/small,frame)`

### Evaluators / Descriptors / Intensifiers

`GOOD/BAD(x,fr) = set_attr(x,valence,±,fr)` · `BIG/SMALL(x,fr) = set_attr(x,size,large/small,fr)` ·
`VERY(attr) = amplify(attr)` · `MORE(q) = increment(q)`

### Mental predicates

`THINK(c) = write(workspace,c)` · `KNOW(p) = assert(p,warrant=high)` ·
`WANT(s) = set_goal(target,s)` · `DON'T_WANT(s) = set_goal(avoid,s)` ·
`FEEL(s) = write_channel(affect,s)` · `SEE(x) = read_channel(visual)→x` ·
`HEAR(x) = read_channel(audio)→x`

### Speech

`SAY(c,addr) = write_channel(expression,c,tom=addr)` · `WORDS(x) = ref(symbol_store,x)` ·
`TRUE(p) = gate(p,warrant=correspondence)`

### Actions / Events / Movement

`DO(actor,tgt) = addForce(tgt,by=actor)→settle(S2)` · `HAPPEN(ev) = DO(null,ev)` ·
`MOVE(x,to) = addForce(x,Δpos)→settle(pos=to)`

### Existence / Possession

`EXIST(x) = assert_node(x)` · `BE_AT(x,pl) = edge(x,pl,location)` ·
`BE(x,P) = link_tree(KIND,P,x)` if-class else `set_attr(x,dim,P)` · `HAVE(p,q) = edge(p,q,HAS)` ·
`OWN(p,q) = HAVE(p,q) + set_attr(edge,responsibility,p)`

### Life

`LIVE(x) = set_attr(x,life,alive)` · `DIE(x) = addForce(x,life:alive→dead)`

### Time

`WHEN(e) = locate(temporal,e)` · `NOW() = bind_index(temporal,present)` ·
`BEFORE(a,b) = order_edge(a,b)` · `AFTER(a,b) = order_edge(b,a)` ·
`LONG/SHORT_TIME(i) = set_attr(i,dur,large/small)` · `FOR_SOME_TIME(s,i) = scope(s,dur=i)` ·
`MOMENT(t) = set_attr(t,dur,point)`

### Space

`WHERE(x) = locate(spatial,x)` · `HERE() = bind_index(spatial,proximal)` ·
`ABOVE(a,b) = edge(a,b,v_higher)` · `BELOW(a,b) = ABOVE(b,a)` ·
`FAR/NEAR(a,b) = set_attr(edge(a,b,dist),mag,large/small)` · `SIDE(x) = ref(region(x),lateral)` ·
`INSIDE(a,b) = edge(a,b,containment)` · `TOUCH(a,b) = edge(a,b,contact)`

### Logical

`NOT(x) = negate(x)` · `MAYBE(p) = set_confidence(p,sub_certain)` ·
`CAN(a,act) = set_attr(a,affordance,act)` · `BECAUSE(c,e) = edge(c,e,causal)` ·
`IF(cond,cons) = open_scope(conditional,gate=cond,body=cons)`

### Manner

`WAY(action,template) = declare(SOFTLINK, behaviour(action), template)` — a **behavioural LIKE**
(claim 411): LIKE applied to *how* an agent acts. This is why NSM clusters LIKE / AS / WAY together.

---

## Refinements since 386

Two signatures were refined after claim 386 was authored:

- **`WANT`** folds to `addForce(target=s)` *without* `settle` — a **pending force**. This yields the
  structural identity **`DO = WANT + settle`**: the mental/action split is just the
  unsettled/settled-force split (claim 410).
- **`WAY`** was added as the behavioural LIKE above (claim 411), closing the last coverage gap in the
  65.

### Structural regularities (claim 387)

The functional recast exposes algebra among the primes — evidence the basis is cut right:
`OTHER = negate(SAME)` · `HAPPEN = DO(actor=null)` · `AFTER(a,b) = BEFORE(b,a)` ·
`BELOW(a,b) = ABOVE(b,a)` · `OWN = HAVE + responsibility` · `DON'T_WANT = WANT` with an anti-target ·
`SAME = LIKE` with SYMLINK-for-SOFTLINK (hardness swap) · `SYM = SAME` at cross-shard scope (scope
swap) · `VERY` / `MORE` as higher-order modifiers of an attribute/declaration.

---

## See also

- [primes-and-molecules.md](primes-and-molecules.md) — the particle taxonomy these signatures realize.
- [explication.md](explication.md) — how a molecule/word composes these signatures into an explication.
- [punctuation-nonverbal.md](punctuation-nonverbal.md) — the illocutionary/boundary family in depth.
