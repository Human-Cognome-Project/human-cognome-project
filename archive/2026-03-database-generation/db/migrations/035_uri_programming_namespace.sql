-- Migration 035: URI/Programming namespace (AA.AG) in hcp_core
--
-- Context:
--   "labels" (engine informal term for AD/AE namespace in hcp_english) was
--   causing confusion with "Labels" (engine proper-noun tier). The root cause:
--   Pass 6 inserted URI/programming tokens (www, html, css, http, etc.) into
--   hcp_english's AE (Initialisms) and AB (main vocab) namespaces. These tokens
--   are language-invariant — they appear identically in any language context.
--   Language-invariant tokens belong in hcp_core, not in language shards.
--
-- This migration:
--   1. Adds AA.AG namespace allocation ("URI Elements") to hcp_core
--   2. Inserts URI/programming tokens into AA.AG subcategories:
--        AA.AG.AA = Network Protocols  (http, https, ftp, sftp, smtp, imap, ...)
--        AA.AG.AB = File Formats       (html, xml, pdf, epub, txt, json, ...)
--        AA.AG.AC = Programming Tools  (css, php, cli, api, js, ...)
--        AA.AG.AD = Standards/IDs      (ascii, ieee, utc, url, uri, www, ...)
--        AA.AG.AE = TLDs               (com, net, org, edu, gov)
--
-- Migration 036 (hcp_english) removes the leaked copies from hcp_english.
--
-- Token_id scheme within AA.AG:
--   ns=AA, p2=AG, p3={subcategory}, p4=AA, p5=sequential (AA, AB, AC, ...)
--   All tokens stored lowercase. No token_pos/token_glosses in hcp_core.

\connect hcp_core

BEGIN;

-- ============================================================
-- 1. Namespace allocations
-- ============================================================

INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('AA.AG',    'URI Elements',        'Language-invariant URI, protocol, file format, and programming identifier tokens', 'category',    'AA'),
('AA.AG.AA', 'Network Protocols',   'Internet/network protocols: http, https, ftp, sftp, smtp, imap, pop3, ssh, ssl, tls, tcp, dns, dhcp, ip', 'subcategory', 'AA.AG'),
('AA.AG.AB', 'File Formats',        'File formats and extensions: html, xml, pdf, epub, txt, json, csv, zip, png, jpg, gif, svg, mp3, avi, ico, exe, js', 'subcategory', 'AA.AG'),
('AA.AG.AC', 'Programming Tools',   'Programming languages, tools, and frameworks: css, php, cli, api, gui, sql, sdk', 'subcategory', 'AA.AG'),
('AA.AG.AD', 'Standards/IDs',       'Technical standards and universal identifier schemes: ascii, unicode, ieee, iso, ansi, utc, url, uri, www', 'subcategory', 'AA.AG'),
('AA.AG.AE', 'TLDs',                'Top-level domain names: com, net, org, edu, gov, io', 'subcategory', 'AA.AG');

-- ============================================================
-- 2. Network Protocols  (AA.AG.AA.AA.{p5})
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory) VALUES
('AA', 'AG', 'AA', 'AA', 'AA', 'http',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AB', 'https',  'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AC', 'ftp',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AD', 'sftp',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AE', 'smtp',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AF', 'imap',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AG', 'pop3',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AH', 'ssh',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AI', 'ssl',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AJ', 'tls',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AK', 'tcp',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AL', 'dns',    'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AM', 'dhcp',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AN', 'ip',     'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AO', 'ipv4',   'uri_element', 'protocol'),
('AA', 'AG', 'AA', 'AA', 'AP', 'ipv6',   'uri_element', 'protocol');

-- ============================================================
-- 3. File Formats  (AA.AG.AB.AA.{p5})
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory) VALUES
('AA', 'AG', 'AB', 'AA', 'AA', 'html',   'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AB', 'xml',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AC', 'txt',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AD', 'pdf',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AE', 'epub',   'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AF', 'zip',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AG', 'json',   'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AH', 'csv',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AI', 'png',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AJ', 'jpg',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AK', 'gif',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AL', 'svg',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AM', 'mp3',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AN', 'avi',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AO', 'ico',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AP', 'exe',    'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AQ', 'js',     'uri_element', 'file_format'),
('AA', 'AG', 'AB', 'AA', 'AR', 'mp4',    'uri_element', 'file_format');

-- ============================================================
-- 4. Programming Tools  (AA.AG.AC.AA.{p5})
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory) VALUES
('AA', 'AG', 'AC', 'AA', 'AA', 'css',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AB', 'php',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AC', 'cli',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AD', 'api',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AE', 'gui',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AF', 'sql',    'uri_element', 'programming'),
('AA', 'AG', 'AC', 'AA', 'AG', 'sdk',    'uri_element', 'programming');

-- ============================================================
-- 5. Standards/IDs  (AA.AG.AD.AA.{p5})
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory) VALUES
('AA', 'AG', 'AD', 'AA', 'AA', 'ascii',   'uri_element', 'standard'),
('AA', 'AG', 'AD', 'AA', 'AB', 'ieee',    'uri_element', 'standard'),
('AA', 'AG', 'AD', 'AA', 'AC', 'iso',     'uri_element', 'standard'),
('AA', 'AG', 'AD', 'AA', 'AD', 'ansi',    'uri_element', 'standard'),
('AA', 'AG', 'AD', 'AA', 'AE', 'utc',     'uri_element', 'standard'),
('AA', 'AG', 'AD', 'AA', 'AF', 'url',     'uri_element', 'identifier'),
('AA', 'AG', 'AD', 'AA', 'AG', 'uri',     'uri_element', 'identifier'),
('AA', 'AG', 'AD', 'AA', 'AH', 'www',     'uri_element', 'identifier'),
('AA', 'AG', 'AD', 'AA', 'AI', 'unicode', 'uri_element', 'standard');

-- ============================================================
-- 6. TLDs  (AA.AG.AE.AA.{p5})
-- ============================================================

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory) VALUES
('AA', 'AG', 'AE', 'AA', 'AA', 'com',    'uri_element', 'tld'),
('AA', 'AG', 'AE', 'AA', 'AB', 'net',    'uri_element', 'tld'),
('AA', 'AG', 'AE', 'AA', 'AC', 'org',    'uri_element', 'tld'),
('AA', 'AG', 'AE', 'AA', 'AD', 'edu',    'uri_element', 'tld'),
('AA', 'AG', 'AE', 'AA', 'AE', 'gov',    'uri_element', 'tld'),
('AA', 'AG', 'AE', 'AA', 'AF', 'io',     'uri_element', 'tld');

-- ============================================================
-- Verify
-- ============================================================

SELECT
    (SELECT count(*) FROM namespace_allocations WHERE pattern LIKE 'AA.AG%') AS new_ns_entries,
    (SELECT count(*) FROM tokens WHERE ns = 'AA' AND p2 = 'AG') AS new_tokens;

SELECT p3, subcategory, count(*) AS tokens
FROM tokens
WHERE ns = 'AA' AND p2 = 'AG'
GROUP BY p3, subcategory
ORDER BY p3;

COMMIT;
