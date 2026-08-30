#!/usr/bin/env python3
"""
cleanup_morpheme_derivatives.py

Scans hcp_english for tokens that are regular morpheme derivations of another
token already present in the DB.  Covers the full set of productive English
morphemes from Wiktionary Category:English_morphemes.

Inflectional suffixes  : -ing, -ed, -t, -s/-es, -er(comp), -est, -en(pp)
Derivational suffixes  : -er/-or(agent), -ly, -ness, -ment, -ion, -able/-ible,
                         -ful, -less, -ish, -y(adj), -en(caus), -ize/-ise/-ify,
                         -ism, -ist, -ity/-ty, -hood, -ship, -dom, -ward/-wards,
                         -wise, -some, -like, -al, -ary, -ance/-ence, -ant/-ent,
                         -th(ordinal)
Bound prefixes         : un-, re-, pre-, mis-, dis-, de-, non-, in-/im-/il-/ir-,
                         anti-

PoS-based morpheme acceptance is the primary false-positive filter:
only V_MAIN, N_COMMON, and ADJ roots accept any of these morphemes.
All other PoS (pronoun, auxiliary, copula, preposition, conjunction,
determiner, interjection, particle, numeral) accept none from this rule set.

Note: N_PRONOUN does accept clitic morphemes ('ve, 'll, 'd, 's, 're, 'm)
but those are a separate subsystem handled at clitic-parsing level.

Usage:
    python3 cleanup_morpheme_derivatives.py [--execute] [--log FILE]

Options:
    --execute    Commit deletions (default: dry-run, report only)
    --log FILE   Log file (default: cleanup_morpheme_derivatives.log)
"""

import argparse
import csv
import logging
import os
import sys
from collections import defaultdict

import psycopg

DB_DSN = 'dbname=hcp_english'
DOUB_C = set('bdfgmnprt')

# characteristics bits (from pass2_insert_token_pos.py)
ARCHAIC   = 1 << 8
DATED     = 1 << 9
BORROWING = 1 << 24

# Bits that must match between token and root for a derivation to be lexically valid.
# Mismatch signals etymological-but-not-lexical connection (debit/bit, debut/but, etc.)
REGISTER_MUST_MATCH = ARCHAIC | DATED | BORROWING

# ---------------------------------------------------------------------------
# Per-PoS morpheme acceptance
# Maps morpheme label → set of ROOT PoS values that can produce it.
# Any root whose PoS set has no intersection with this is a false positive.
# ---------------------------------------------------------------------------
ROOT_ACCEPTS = {
    # Inflectional
    'PROG':         {'V_MAIN'},
    'PAST':         {'V_MAIN'},
    'PAST_T':       {'V_MAIN'},                   # -t variant: dreamt, learnt
    'PLURAL_3SG':   {'V_MAIN', 'N_COMMON'},
    'COMP':         {'ADJ'},
    'SUP':          {'ADJ'},
    'PP_EN':        {'V_MAIN'},                   # -en past participle: broken, taken
    # Agent / instrument
    'AGENT':        {'V_MAIN'},                   # -er: walker, teacher
    'AGENT_OR':     {'V_MAIN'},                   # -or: actor, creator
    # Derivational suffixes  (A→N, N→A, V→N, N→V, A→ADV …)
    'ADV_LY':       {'ADJ'},                      # quickly, happily
    'NMLZ':         {'ADJ'},                      # darkness, sadness  (-ness)
    'NMLZ_MENT':    {'V_MAIN'},                   # movement, treatment  (-ment)
    'NMLZ_ION':     {'V_MAIN'},                   # action, creation  (-ion)
    'NMLZ_ISM':     {'N_COMMON', 'ADJ'},          # realism, modernism
    'NMLZ_IST':     {'N_COMMON', 'ADJ'},          # realist, socialist
    'NMLZ_ITY':     {'ADJ'},                      # reality, activity  (-ity/-ty)
    'NMLZ_HOOD':    {'N_COMMON'},                 # childhood, brotherhood
    'NMLZ_SHIP':    {'N_COMMON', 'ADJ'},          # friendship, hardship
    'NMLZ_DOM':     {'N_COMMON', 'ADJ'},          # freedom, kingdom
    'ADJ_ABLE':     {'V_MAIN', 'N_COMMON'},       # readable, movable  (-able/-ible)
    'ADJ_FUL':      {'N_COMMON'},                 # careful, helpful
    'ADJ_LESS':     {'N_COMMON'},                 # careless, helpless
    'ADJ_ISH':      {'N_COMMON', 'ADJ'},          # reddish, childish
    'ADJ_Y':        {'N_COMMON'},                 # cloudy, rainy
    'ADJ_AL':       {'N_COMMON'},                 # national, musical  (-al)
    'ADJ_ARY':      {'N_COMMON'},                 # elementary, honorary  (-ary)
    'ADJ_ANT':      {'V_MAIN'},                   # pleasant, abundant  (-ant/-ent)
    'ADJ_SOME':     {'N_COMMON', 'V_MAIN'},       # awesome, troublesome
    'ADJ_LIKE':     {'N_COMMON'},                 # catlike, warlike
    'ADV_WARD':     {'N_COMMON'},                 # northward, eastward
    'ADV_WISE':     {'N_COMMON'},                 # clockwise, likewise
    'V_EN':         {'ADJ'},                      # darken, brighten  (-en caus.)
    'V_IZE':        {'N_COMMON', 'ADJ'},          # modernize, organize
    'V_IFY':        {'N_COMMON', 'ADJ'},          # clarify, amplify
    'V_ANCE':       {'V_MAIN', 'ADJ'},            # performance, importance  (-ance/-ence)
    # Bound prefixes
    'PFX_NEG':      {'ADJ', 'V_MAIN'},            # un+happy, un+do
    'PFX_ITER':     {'V_MAIN'},                   # re+write
    'PFX_PRE':      {'V_MAIN'},                   # pre+pay
    'PFX_MIS':      {'V_MAIN'},                   # mis+use
    'PFX_NEG_DIS':  {'V_MAIN', 'ADJ'},            # dis+agree, dis+loyal
    'PFX_REV':      {'V_MAIN'},                   # de+code, de+frost
    'PFX_NEG_NON':  {'N_COMMON', 'ADJ'},          # non+sense, non+stop
    'PFX_NEG_IN':   {'ADJ'},                      # in+correct
    'PFX_NEG_IM':   {'ADJ'},                      # im+possible
    'PFX_NEG_IL':   {'ADJ'},                      # il+legal
    'PFX_NEG_IR':   {'ADJ'},                      # ir+regular
    'PFX_ANTI':     {'N_COMMON', 'ADJ'},          # anti+war, anti+social
}

TOKEN_POS_FOR = {
    'PROG':         {'V_MAIN', 'N_COMMON'},
    'PAST':         {'V_MAIN'},
    'PAST_T':       {'V_MAIN'},
    'PLURAL_3SG':   {'V_MAIN', 'N_COMMON'},
    'COMP':         {'ADJ'},
    'SUP':          {'ADJ'},
    'PP_EN':        {'V_MAIN'},
    'AGENT':        {'N_COMMON'},
    'AGENT_OR':     {'N_COMMON'},
    'ADV_LY':       {'ADV'},
    'NMLZ':         {'N_COMMON'},
    'NMLZ_MENT':    {'N_COMMON'},
    'NMLZ_ION':     {'N_COMMON'},
    'NMLZ_ISM':     {'N_COMMON'},
    'NMLZ_IST':     {'N_COMMON'},
    'NMLZ_ITY':     {'N_COMMON'},
    'NMLZ_HOOD':    {'N_COMMON'},
    'NMLZ_SHIP':    {'N_COMMON'},
    'NMLZ_DOM':     {'N_COMMON'},
    'ADJ_ABLE':     {'ADJ'},
    'ADJ_FUL':      {'ADJ'},
    'ADJ_LESS':     {'ADJ'},
    'ADJ_ISH':      {'ADJ'},
    'ADJ_Y':        {'ADJ'},
    'ADJ_AL':       {'ADJ'},
    'ADJ_ARY':      {'ADJ'},
    'ADJ_ANT':      {'ADJ'},
    'ADJ_SOME':     {'ADJ'},
    'ADJ_LIKE':     {'ADJ'},
    'ADV_WARD':     {'ADV', 'ADJ'},
    'ADV_WISE':     {'ADV'},
    'V_EN':         {'V_MAIN'},
    'V_IZE':        {'V_MAIN'},
    'V_IFY':        {'V_MAIN'},
    'V_ANCE':       {'N_COMMON'},
    # Prefixes preserve PoS — token PoS must match root_accepts
    'PFX_NEG':      {'ADJ', 'V_MAIN'},
    'PFX_ITER':     {'V_MAIN'},
    'PFX_PRE':      {'V_MAIN'},
    'PFX_MIS':      {'V_MAIN'},
    'PFX_NEG_DIS':  {'V_MAIN', 'ADJ'},
    'PFX_REV':      {'V_MAIN'},
    'PFX_NEG_NON':  {'N_COMMON', 'ADJ'},
    'PFX_NEG_IN':   {'ADJ'},
    'PFX_NEG_IM':   {'ADJ'},
    'PFX_NEG_IL':   {'ADJ'},
    'PFX_NEG_IR':   {'ADJ'},
    'PFX_ANTI':     {'N_COMMON', 'ADJ'},
}

# ---------------------------------------------------------------------------
# Strip functions: word → list of candidate root strings
# ---------------------------------------------------------------------------

def _ing(w):                            # PROG
    if len(w) <= 4 or not w.endswith('ing'): return []
    stem = w[:-3]
    out = [stem, stem + 'e']
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(stem[:-1])           # running → run
    return out

def _ed(w):                             # PAST
    if len(w) <= 3 or not w.endswith('ed'): return []
    out = [w[:-2], w[:-1]]              # walked; loved
    if w.endswith('ied') and len(w) > 4:
        out.append(w[:-3] + 'y')        # tried → try
    stem = w[:-2]
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(w[:-3])              # stopped → stop
    return out

def _t_past(w):                         # PAST_T: dreamt, learnt, burnt, spelt
    if len(w) <= 3 or not w.endswith('t'): return []
    out = [w[:-1], w[:-1] + 'e']       # dreamt→dream; smelt→smell(+l?)
    # try adding common endings that get shortened
    return [c for c in out if len(c) >= 3]

def _s(w):                              # PLURAL_3SG
    if len(w) <= 2 or not w.endswith('s'): return []
    out = []
    if not w.endswith('ss'):
        out.append(w[:-1])
    if w.endswith('ies') and len(w) > 3:
        out.append(w[:-3] + 'y')        # cities → city
    if w.endswith('es') and len(w) > 3:
        if len(w) > 4 and w[-3] in 'szx':
            out.append(w[:-2])          # passes → pass
        if len(w) > 5 and w[-4:-2] in ('ch', 'sh'):
            out.append(w[:-2])          # watches → watch
    return out

def _er(w):                             # COMP / AGENT
    if len(w) <= 3 or not w.endswith('er'): return []
    stem = w[:-2]
    out = [stem, stem + 'e']
    if w.endswith('ier') and len(w) > 4:
        out.append(w[:-3] + 'y')        # happier → happy
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(stem[:-1])           # bigger → big
    return out

def _est(w):                            # SUP
    if len(w) <= 4 or not w.endswith('est'): return []
    stem = w[:-3]
    out = [stem, stem + 'e']
    if w.endswith('iest') and len(w) > 5:
        out.append(w[:-4] + 'y')
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(stem[:-1])
    return out

def _pp_en(w):                          # PP_EN: broken, taken, fallen
    if len(w) <= 4 or not w.endswith('en'): return []
    stem = w[:-2]
    return [stem, stem + 'e']           # broken→broke; taken→take

def _or_agent(w):                       # AGENT_OR: actor, creator
    if len(w) <= 4 or not w.endswith('or'): return []
    stem = w[:-2]
    return [stem, stem + 'e', stem + 'at', stem + 'ate']
    # creator→creat→create; senator→senat→senate

def _ly(w):                             # ADV_LY
    if len(w) <= 3 or not w.endswith('ly'): return []
    out = [w[:-2]]
    if w.endswith('ily') and len(w) > 4:
        out.append(w[:-3] + 'y')        # happily → happy
    return out

def _ness(w):                           # NMLZ
    if len(w) <= 5 or not w.endswith('ness'): return []
    out = [w[:-4]]
    if w.endswith('iness') and len(w) > 6:
        out.append(w[:-5] + 'y')        # happiness → happy
    return out

def _ment(w):                           # NMLZ_MENT
    if len(w) <= 5 or not w.endswith('ment'): return []
    stem = w[:-4]
    return [stem, stem + 'e']           # movement→move; argument→argue

def _ion(w):                            # NMLZ_ION: action, creation
    if len(w) <= 4 or not w.endswith('ion'): return []
    stem = w[:-3]
    return [stem, stem + 'e', stem + 't']  # action→act; creation→create

def _ism(w):                            # NMLZ_ISM
    if len(w) <= 4 or not w.endswith('ism'): return []
    return [w[:-3]]                     # realism→real

def _ist(w):                            # NMLZ_IST
    if len(w) <= 4 or not w.endswith('ist'): return []
    return [w[:-3]]                     # realist→real

def _ity(w):                            # NMLZ_ITY
    if len(w) <= 4: return []
    out = []
    if w.endswith('ity') and len(w) > 4:
        stem = w[:-3]
        out += [stem, stem + 'e']       # reality→real; activity→active
    if w.endswith('ty') and len(w) > 3 and not w.endswith('ity'):
        out.append(w[:-2])              # beauty→beau (not useful but safe)
    return out

def _hood(w):                           # NMLZ_HOOD
    if len(w) <= 5 or not w.endswith('hood'): return []
    return [w[:-4]]                     # childhood→child

def _ship(w):                           # NMLZ_SHIP
    if len(w) <= 5 or not w.endswith('ship'): return []
    return [w[:-4]]                     # friendship→friend

def _dom(w):                            # NMLZ_DOM
    if len(w) <= 4 or not w.endswith('dom'): return []
    return [w[:-3]]                     # freedom→free; kingdom→king

def _able(w):                           # ADJ_ABLE
    if len(w) <= 5: return []
    out = []
    if w.endswith('able'):
        stem = w[:-4]
        out += [stem, stem + 'e']       # readable→read; movable→move
    if w.endswith('ible'):
        stem = w[:-4]
        out += [stem, stem + 'e', stem + 'ss']  # responsible→respons→response
    return out

def _ful(w):                            # ADJ_FUL
    if len(w) <= 4 or not w.endswith('ful'): return []
    return [w[:-3]]                     # careful→care; helpful→help

def _less(w):                           # ADJ_LESS
    if len(w) <= 5 or not w.endswith('less'): return []
    return [w[:-4]]                     # careless→care; helpless→help

def _ish(w):                            # ADJ_ISH
    if len(w) <= 4 or not w.endswith('ish'): return []
    stem = w[:-3]
    out = [stem]
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(stem[:-1])           # reddish → red
    return out

def _y_adj(w):                          # ADJ_Y: cloudy→cloud, rainy→rain
    if len(w) <= 3 or not w.endswith('y'): return []
    stem = w[:-1]
    if len(stem) < 3: return []
    out = [stem]
    if len(stem) >= 2 and stem[-1] == stem[-2] and stem[-1] in DOUB_C:
        out.append(stem[:-1])           # sunny → sun
    return out

def _al(w):                             # ADJ_AL: national→nation, musical→music
    if len(w) <= 4 or not w.endswith('al'): return []
    stem = w[:-2]
    if len(stem) < 3: return []
    return [stem, stem + 'e', stem + 'ion']  # nation→national

def _ary(w):                            # ADJ_ARY: elementary→element
    if len(w) <= 4 or not w.endswith('ary'): return []
    return [w[:-3]]

def _ant_ent(w):                        # ADJ_ANT: abundant, pleasant
    if len(w) <= 4: return []
    out = []
    for sfx in ('ant', 'ent'):
        if w.endswith(sfx):
            stem = w[:-3]
            out += [stem, stem + 'e']
    return out

def _some(w):                           # ADJ_SOME: awesome→awe, troublesome→trouble
    if len(w) <= 5 or not w.endswith('some'): return []
    return [w[:-4]]

def _like(w):                           # ADJ_LIKE: catlike→cat
    if len(w) <= 5 or not w.endswith('like'): return []
    return [w[:-4]]

def _ward(w):                           # ADV_WARD: northward→north
    if len(w) <= 5: return []
    out = []
    if w.endswith('wards'):
        out.append(w[:-5])
    if w.endswith('ward') and not w.endswith('wards'):
        out.append(w[:-4])
    return out

def _wise(w):                           # ADV_WISE: clockwise→clock
    if len(w) <= 5 or not w.endswith('wise'): return []
    return [w[:-4]]

def _en_caus(w):                        # V_EN: darken→dark, brighten→bright
    if len(w) <= 4 or not w.endswith('en'): return []
    stem = w[:-2]
    if len(stem) < 3: return []
    return [stem, stem + 'e']           # widen→wide

def _ize(w):                            # V_IZE: modernize→modern, modernise→modern
    if len(w) <= 4: return []
    out = []
    for sfx in ('ize', 'ise'):
        if w.endswith(sfx):
            out.append(w[:-3])
    return out

def _ify(w):                            # V_IFY: clarify→clear? simplify→simple
    if len(w) <= 4 or not w.endswith('ify'): return []
    stem = w[:-3]
    return [stem, stem + 'e', stem + 'le']

def _ance(w):                           # V_ANCE: performance→perform, importance→import
    if len(w) <= 5: return []
    out = []
    for sfx in ('ance', 'ence'):
        if w.endswith(sfx):
            stem = w[:-4]
            out += [stem, stem + 'e']
    return out

# ---------------------------------------------------------------------------
# Rule table: (label, strip_fn)
# Order matters for tiebreak: more specific rules first.
# ---------------------------------------------------------------------------
SUFFIX_RULES = [
    ('PROG',        _ing),
    ('PAST',        _ed),
    ('PAST_T',      _t_past),
    ('PLURAL_3SG',  _s),
    ('COMP',        _er),
    ('AGENT',       _er),
    ('SUP',         _est),
    ('PP_EN',       _pp_en),
    ('AGENT_OR',    _or_agent),
    ('ADV_LY',      _ly),
    ('NMLZ',        _ness),
    ('NMLZ_MENT',   _ment),
    ('NMLZ_ION',    _ion),
    ('NMLZ_ISM',    _ism),
    ('NMLZ_IST',    _ist),
    ('NMLZ_ITY',    _ity),
    ('NMLZ_HOOD',   _hood),
    ('NMLZ_SHIP',   _ship),
    ('NMLZ_DOM',    _dom),
    ('ADJ_ABLE',    _able),
    ('ADJ_FUL',     _ful),
    ('ADJ_LESS',    _less),
    ('ADJ_ISH',     _ish),
    ('ADJ_Y',       _y_adj),
    ('ADJ_AL',      _al),
    ('ADJ_ARY',     _ary),
    ('ADJ_ANT',     _ant_ent),
    ('ADJ_SOME',    _some),
    ('ADJ_LIKE',    _like),
    ('ADV_WARD',    _ward),
    ('ADV_WISE',    _wise),
    ('V_EN',        _en_caus),
    ('V_IZE',       _ize),
    ('V_IFY',       _ify),
    ('V_ANCE',      _ance),
]

PREFIX_RULES = [
    ('un',    'PFX_NEG'),
    ('re',    'PFX_ITER'),
    ('pre',   'PFX_PRE'),
    ('mis',   'PFX_MIS'),
    ('dis',   'PFX_NEG_DIS'),
    ('de',    'PFX_REV'),
    ('non',   'PFX_NEG_NON'),
    ('in',    'PFX_NEG_IN'),
    ('im',    'PFX_NEG_IM'),
    ('il',    'PFX_NEG_IL'),
    ('ir',    'PFX_NEG_IR'),
    ('anti',  'PFX_ANTI'),
]

def all_single_strips(word):
    for label, fn in SUFFIX_RULES:
        for cand in fn(word):
            if cand and cand != word and len(cand) >= 2:
                yield (cand, label)
    n = len(word)
    for pfx, label in PREFIX_RULES:
        plen = len(pfx)
        if word.startswith(pfx) and n - plen >= 3:
            yield (word[plen:], label)

def chars_compatible(token_chars, root_chars):
    """Return False if characteristics signal etymological-only (not lexically productive) connection.

    Asymmetric rules:
    - token=BORROWING, root=not BORROWING → reject.
      The word was borrowed as a unit (debit, debut), not formed from the English root.
    - root=BORROWING, token=not BORROWING → allow.
      Productive English prefix on a Latin-origin root (defund←fund, deallocate←allocate).
    - root=ARCHAIC|DATED, token=not → reject.
      Can't productively derive modern words from obsolete bases.
    """
    # Borrowed whole — not a productive prefix application
    if (token_chars & BORROWING) and not (root_chars & BORROWING):
        return False
    # Root is obsolete/dated but derived form is in common use — etymological only
    if (root_chars & (ARCHAIC | DATED)) and not (token_chars & (ARCHAIC | DATED)):
        return False
    return True

def check(label, token_pos_s, root_pos_s):
    root_ok  = ROOT_ACCEPTS.get(label)
    if root_ok and not (root_pos_s & root_ok):
        return False
    token_ok = TOKEN_POS_FOR.get(label)
    if token_ok is not None and not (token_pos_s & token_ok):
        return False
    return True

# ---------------------------------------------------------------------------
# Candidate discovery (1- and 2-level)
# ---------------------------------------------------------------------------

def find_candidates(by_name, glosses):
    seen = set()

    def emit(tid, morph, root, root_tid):
        key = (tid, morph)
        if key not in seen:
            seen.add(key)
            return True
        return False

    for name, entries in by_name.items():
        token_ids   = [e[0] for e in entries]
        token_pos_s = frozenset(e[1] for e in entries)
        # OR all characteristics bits across every PoS entry for this token
        token_chars = 0
        for e in entries:
            token_chars |= e[2]

        for root1, lbl1 in all_single_strips(name):
            if root1 in by_name:
                root1_entries = by_name[root1]
                root1_pos   = frozenset(e[1] for e in root1_entries)
                root1_chars = 0
                for e in root1_entries:
                    root1_chars |= e[2]
                if not check(lbl1, token_pos_s, root1_pos):
                    continue
                if not chars_compatible(token_chars, root1_chars):
                    continue
                root1_tid = root1_entries[0][0]
                for tid in token_ids:
                    action = classify_by_gloss(tid, root1, glosses)
                    if action == 'keep':
                        continue
                    if emit(tid, lbl1, root1, root1_tid):
                        yield (tid, name, lbl1, root1, root1_tid, action)
                        break
            else:
                token_ok = TOKEN_POS_FOR.get(lbl1)
                if token_ok is not None and not (token_pos_s & token_ok):
                    continue
                for root2, lbl2 in all_single_strips(root1):
                    if root2 not in by_name:
                        continue
                    root2_entries = by_name[root2]
                    root2_pos   = frozenset(e[1] for e in root2_entries)
                    root2_chars = 0
                    for e in root2_entries:
                        root2_chars |= e[2]
                    if not check(lbl2, frozenset(), root2_pos):
                        continue
                    if not chars_compatible(token_chars, root2_chars):
                        continue
                    root2_tid = root2_entries[0][0]
                    combined  = f'{lbl1}+{lbl2}'
                    for tid in token_ids:
                        action = classify_by_gloss(tid, root2, glosses)
                        if action == 'keep':
                            continue
                        if emit(tid, combined, root2, root2_tid):
                            yield (tid, name, combined, root2, root2_tid, action)
                            break
                    break

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def load_tokens(conn):
    cur = conn.cursor()
    cur.execute("""
        SELECT t.token_id, t.name, tp.pos, t.characteristics
        FROM tokens t
        JOIN token_pos tp ON t.token_id = tp.token_id
        WHERE t.ns = 'AB'
        ORDER BY t.name
    """)
    by_name = defaultdict(list)
    for token_id, name, pos, chars in cur:
        by_name[name].append((token_id, pos, chars))
    return dict(by_name)

def load_glosses(conn):
    """Return dict: token_id -> list of gloss_text (lowercased)."""
    cur = conn.cursor()
    cur.execute("""
        SELECT tg.token_id, tg.gloss_text
        FROM token_glosses tg
        JOIN tokens t ON tg.token_id = t.token_id
        WHERE t.ns = 'AB'
    """)
    glosses = defaultdict(list)
    for token_id, gloss_text in cur:
        glosses[token_id].append(gloss_text.lower())
    return dict(glosses)

def classify_by_gloss(token_id, root_name, glosses):
    """Classify a candidate by its gloss relationship to the root.

    Returns:
        'delete' — no glosses (raw derivative import), or ALL glosses reference
                   the root (purely derivative meaning) → delete token entirely.
        'prune'  — SOME glosses reference root, some don't → keep token,
                   delete the root-referencing glosses, add token_variants entry.
        'keep'   — NO glosses reference root → fully independent word, not a
                   candidate (caller should skip it).
    """
    token_glosses = glosses.get(token_id)
    if not token_glosses:
        return 'delete'
    root_lower = root_name.lower()
    root_count = sum(1 for g in token_glosses if root_lower in g)
    if root_count == len(token_glosses):
        return 'delete'
    elif root_count > 0:
        return 'prune'
    else:
        return 'keep'

def run(execute, log_file):
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(levelname)s %(message)s',
        handlers=[
            logging.FileHandler(log_file),
            logging.StreamHandler(sys.stderr),
        ]
    )
    log = logging.getLogger(__name__)

    conn = psycopg.connect(DB_DSN)
    conn.autocommit = False

    log.info('Loading tokens...')
    by_name = load_tokens(conn)
    log.info('Loaded %d distinct name entries', len(by_name))

    log.info('Loading glosses...')
    glosses = load_glosses(conn)
    log.info('Loaded glosses for %d tokens', len(glosses))

    log.info('Finding morpheme derivatives (1- and 2-level)...')
    candidates = list(find_candidates(by_name, glosses))
    log.info('Found %d derivative candidates', len(candidates))

    report_path = log_file.replace('.log', '_report.csv')
    with open(report_path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['token_id', 'name', 'morpheme', 'root_name', 'root_token_id', 'action'])
        w.writerows(candidates)
    log.info('Report: %s', report_path)

    by_morph = defaultdict(int)
    by_action = defaultdict(int)
    for _, _, morph, _, _, action in candidates:
        by_morph[morph] += 1
        by_action[action] += 1
    for morph, count in sorted(by_morph.items(), key=lambda x: -x[1]):
        log.info('  %-30s %6d', morph, count)
    log.info('Actions: delete=%d  prune=%d',
             by_action['delete'], by_action['prune'])

    if not execute:
        log.info('DRY RUN — no changes. Re-run with --execute to apply.')
        conn.close()
        return

    # Split candidates by action
    delete_ids = list({tid for tid, _, _, _, _, action in candidates if action == 'delete'})
    delete_id_set = set(delete_ids)
    prune_list = [(tid, name, morph, root, root_tid)
                  for tid, name, morph, root, root_tid, action in candidates
                  if action == 'prune'
                  # Skip if root was itself deleted — FK would fail
                  and root_tid not in delete_id_set]

    # --- DELETE: token + cascade (token_pos, token_glosses, token_variants) ---
    log.info('Deleting %d tokens (CASCADE)...', len(delete_ids))
    BATCH = 500
    deleted = 0
    cur = conn.cursor()
    for i in range(0, len(delete_ids), BATCH):
        batch = delete_ids[i:i + BATCH]
        cur.execute("DELETE FROM tokens WHERE token_id = ANY(%s)", (batch,))
        deleted += cur.rowcount
        if (i // BATCH) % 10 == 0:
            log.info('  deleted %d / %d', deleted, len(delete_ids))
    log.info('Deleted %d tokens.', deleted)

    # --- PRUNE: delete root-referencing glosses + add token_variants entry ---
    log.info('Pruning %d tokens (remove root glosses, add variant entry)...', len(prune_list))
    pruned = 0
    for tid, name, morph, root, root_tid in prune_list:
        root_lower = root.lower()
        # Remove glosses that reference the root name
        cur.execute("""
            DELETE FROM token_glosses
            WHERE token_id = %s AND LOWER(gloss_text) LIKE %s
        """, (tid, f'%{root_lower}%'))
        # Record alternate morpheme reading in token_variants
        cur.execute("""
            INSERT INTO token_variants (canonical_id, name, morpheme, note)
            VALUES (%s, %s, %s, %s)
            ON CONFLICT (canonical_id, name, COALESCE(morpheme, '')) DO NOTHING
        """, (root_tid, name, morph, f'alternate reading: {name} = {root} + {morph}'))
        pruned += 1
        if pruned % 500 == 0:
            log.info('  pruned %d / %d', pruned, len(prune_list))

    conn.commit()
    log.info('Done. %d deleted, %d pruned.', deleted, pruned)
    conn.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--log', default=os.path.join(
        os.path.dirname(__file__), 'cleanup_morpheme_derivatives.log'))
    args = parser.parse_args()
    run(args.execute, args.log)
