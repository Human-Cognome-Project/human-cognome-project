-- Migration 036: Remove leaked URI/programming tokens from hcp_english
--                Add missing 'tremens' (delirium tremens) to hcp_english
--
-- Context (see migration 035):
--   Pass 6 (pass6_insert_labels.py) inserted URI/programming tokens into the
--   AE (Initialisms) namespace in hcp_english. Some also leaked into AB (main
--   vocab via Kaikki) and AD (Labels namespace). These are language-invariant
--   tokens that now have canonical homes in hcp_core AA.AG.
--
-- Token disposition:
--   AD namespace: www, php, utc — purely technical, no English word meaning.
--     Remove all three (token_pos + token_glosses cascade automatically).
--
--   AE namespace: www, html, css, cli, php, dns, tcp, tls, sftp, ico — same.
--     Remove all (cascade).
--
--   AB namespace (main English vocab):
--     REMOVED (purely technical): http, https, smtp, imap, dhcp, ipv4, ipv6,
--       json, svg, js, xml, pop3, jpg, ieee, api (two entries), ascii, ssl,
--       sftp (already in AE), utc (already in AD/AE).
--     KEPT in hcp_english (dual-use — genuine English word meanings too):
--       net (fishing net, net result), zip (to zip, zip code),
--       org (organisation), com, gov, edu, iso (prefix, music), ansi,
--       pdf (to pdf), png, gif (to gif), txt (to text), avi, mp3, exe,
--       uri, url, ftp (to FTP), ip (intellectual property), csv, gui.
--
-- Missing token added:
--   tremens — used in "delirium tremens" (medical Latin, common in English
--   literature). Assigned to AB.AA.AT.AG.Ch (t-starting, 7-char bucket,
--   next sequential after max p5=Cg → Ch=133).
--
-- NOTE: After applying this migration, the LMDB vocab compiler (task #16)
--   must be updated to also compile hcp_core AA.AG tokens into the vocab
--   beds so the engine can resolve URI elements.

\connect hcp_english

BEGIN;

-- ============================================================
-- 1. Remove AD namespace URI tokens (cascade cleans token_pos, token_glosses)
-- ============================================================

-- AD: www, php, utc  (purely technical, no English meaning)
DELETE FROM tokens
WHERE ns = 'AD'
  AND name IN ('www', 'php', 'utc');

-- ============================================================
-- 2. Remove AE namespace URI tokens (cascade cleans dependents)
-- ============================================================

-- AE: www (duplicate), html, css, cli, php (duplicate), dns, tcp, tls, sftp, ico
DELETE FROM tokens
WHERE ns = 'AE'
  AND name IN ('www', 'html', 'css', 'cli', 'php', 'dns', 'tcp', 'tls', 'sftp', 'ico');

-- ============================================================
-- 3. Remove AB namespace URI tokens (purely technical, no English meaning)
-- ============================================================

-- First: remove canonical_id FK references from uppercase variants.
--   Migration 028 created uppercase variants (HTTP, XML, JS, Js) that reference
--   the lowercase URI canonicals via canonical_id. Delete the variants first.
DELETE FROM tokens
WHERE canonical_id IN (
    SELECT token_id FROM tokens
    WHERE ns = 'AB'
      AND name IN ('http', 'https', 'smtp', 'imap', 'dhcp', 'ipv4', 'ipv6', 'ssl', 'pop3',
                   'json', 'svg', 'js', 'xml', 'jpg', 'ieee', 'ascii', 'api')
);

-- Protocols and networking
DELETE FROM tokens WHERE ns = 'AB' AND name IN ('http', 'https', 'smtp', 'imap', 'dhcp', 'ipv4', 'ipv6', 'ssl', 'pop3');

-- File formats / programming (no independent English meaning)
DELETE FROM tokens WHERE ns = 'AB' AND name IN ('json', 'svg', 'js', 'xml', 'jpg', 'ieee', 'ascii');

-- api: two duplicate entries — remove both
DELETE FROM tokens WHERE ns = 'AB' AND name = 'api';

-- ============================================================
-- 4. Add missing token: tremens
--    Token_id: AB.AA.AT.AG.Ch
--    Bucket: p3=AT (t-starting), p4=AG (7-char, index 6), p2=AA (primary bucket)
--    Max existing p5 in (AB, AA, AT, AG) = .Cg. (132) → next = .Ch. (133)
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name)
VALUES ('AB', 'AA', 'AT', 'AG', 'Ch', 'tremens');

-- Add token_pos: ADJ (Latin medical adjective, used as adjective in English)
INSERT INTO token_pos (token_id, pos, is_primary, morpheme_accept, characteristics)
VALUES ('AB.AA.AT.AG.Ch', 'ADJ', true, 0, 0);

-- Add gloss
INSERT INTO token_glosses (token_id, pos, gloss_text, status)
VALUES ('AB.AA.AT.AG.Ch', 'ADJ', 'trembling; chiefly in "delirium tremens" (medical Latin phrase used in English)', 'DRAFT');

-- Wire gloss_id
UPDATE token_pos tp
   SET gloss_id = tg.id
  FROM token_glosses tg
 WHERE tg.token_id = 'AB.AA.AT.AG.Ch'
   AND tg.pos      = tp.pos
   AND tp.token_id = 'AB.AA.AT.AG.Ch';

-- ============================================================
-- Verify
-- ============================================================

-- Confirm AD/AE URI tokens are gone
SELECT 'AD/AE URI tokens remaining' AS check_name,
       count(*) AS remaining
FROM tokens
WHERE (ns = 'AD' AND name IN ('www', 'php', 'utc'))
   OR (ns = 'AE' AND name IN ('www', 'html', 'css', 'cli', 'php', 'dns', 'tcp', 'tls', 'sftp', 'ico'));

-- Confirm AB URI tokens are gone
SELECT 'AB URI tokens remaining' AS check_name,
       count(*) AS remaining
FROM tokens
WHERE ns = 'AB'
  AND name IN ('http', 'https', 'smtp', 'imap', 'dhcp', 'ipv4', 'ipv6', 'ssl', 'pop3', 'json', 'svg', 'js', 'xml', 'jpg', 'ieee', 'ascii', 'api');

-- Confirm tremens added
SELECT t.name, t.token_id, tp.pos
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
WHERE t.name = 'tremens';

-- Summary counts
SELECT ns, count(*) FROM tokens GROUP BY ns ORDER BY ns;

COMMIT;
