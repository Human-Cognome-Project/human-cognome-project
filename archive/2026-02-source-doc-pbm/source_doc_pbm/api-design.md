# HCP Public API Design

_Draft v0.1, 2026-02-06_

## Overview

A read-only JSON API hosted on WHC (PHP + MySQL) that serves two consumers:

1. **GitHub Pages UI** — public browse/search interface
2. **Chunking tool (online mode)** — token resolution for PBM generation

The API mirrors the Postgres schema but serves a read-optimized MySQL subset.

## Base URL

```
https://<tbd>.whc.ca/api/v1/
```

All responses are JSON. CORS enabled for `*.github.io`.

---

## Endpoints

### Token Operations

These are the core endpoints. The chunking tool's online mode lives or dies on these.

#### `GET /tokens/lookup`

Resolve a word/string to its token ID. **Primary endpoint for the chunking tool.**

```
GET /tokens/lookup?q=ephemeral&shard=english
```

Response:
```json
{
  "token_id": "AB.AB.CC.Bx.Kf",
  "name": "ephemeral",
  "layer": "word",
  "subcategory": "adj",
  "atomization": ["AB.AA.AA.AE.Ae", "AB.AA.AA.AE.Ap", "..."],
  "found": true
}
```

Not found:
```json
{
  "token_id": null,
  "name": "xyzzyplugh",
  "found": false,
  "suggestion": "Flag as TBD in PBM"
}
```

Parameters:
- `q` (required) — the string to look up
- `shard` — which shard to search: `core`, `english`, `names` (default: searches all)

#### `GET /tokens/{token_id}`

Look up a token by its ID.

```
GET /tokens/AB.AB.CC.Bx.Kf
```

Response:
```json
{
  "token_id": "AB.AB.CC.Bx.Kf",
  "name": "ephemeral",
  "layer": "word",
  "subcategory": "adj",
  "atomization": ["AB.AA.AA.AE.Ae", "AB.AA.AA.AE.Ap", "..."],
  "shard": "english",
  "metadata": {}
}
```

#### `GET /tokens/batch`

Batch lookup for the chunking tool — resolve multiple words in one call.

```
POST /tokens/batch
Content-Type: application/json

{
  "words": ["the", "quick", "brown", "fox"],
  "shard": "english"
}
```

Response:
```json
{
  "results": [
    {"word": "the", "token_id": "AB.AB.CN.AA.AC", "found": true},
    {"word": "quick", "token_id": "AB.AB.CC.Af.Rn", "found": true},
    {"word": "brown", "token_id": "AB.AB.CC.Ab.Pm", "found": true},
    {"word": "fox", "token_id": "AB.AB.CA.Ac.Jw", "found": true}
  ],
  "all_found": true,
  "tbd_count": 0
}
```

Note: POST despite being a read operation, because query strings have length limits and a chunking tool may send hundreds of words per batch.

#### `GET /tokens/exists`

Quick existence check — lighter than full lookup. For the chunking tool to pre-filter known vs. TBD tokens.

```
GET /tokens/exists?q=ephemeral&shard=english
```

Response:
```json
{
  "exists": true,
  "token_id": "AB.AB.CC.Bx.Kf"
}
```

#### `GET /tokens/search`

Prefix/substring search for the browse UI.

```
GET /tokens/search?q=ephemer&mode=prefix&shard=english&limit=20&offset=0
```

Response:
```json
{
  "query": "ephemer",
  "mode": "prefix",
  "total": 3,
  "results": [
    {"token_id": "AB.AB.CC.Bx.Kf", "name": "ephemeral", "subcategory": "adj"},
    {"token_id": "AB.AB.CA.Bx.Kg", "name": "ephemerality", "subcategory": "noun"},
    {"token_id": "AB.AB.CA.Bx.Kh", "name": "ephemeris", "subcategory": "noun"}
  ]
}
```

Parameters:
- `q` (required) — search string
- `mode` — `prefix` (default), `substring`, `exact`
- `shard` — `core`, `english`, `names`, or omit for all
- `limit` — results per page (default 20, max 100)
- `offset` — pagination offset

---

### Dictionary Data (Kaikki layers)

For the browse UI — lets people explore the linguistic data behind the tokens.

#### `GET /entries/{word}`

Get dictionary entries for a word.

```
GET /entries/ephemeral
```

Response:
```json
{
  "word": "ephemeral",
  "entries": [
    {
      "id": 45231,
      "word_token": "AB.AB.CC.Bx.Kf",
      "pos_token": "AB.AB.CC",
      "etymology_num": 1,
      "senses": [
        {
          "id": 89012,
          "gloss_tokens": ["..."],
          "tag_tokens": ["..."]
        }
      ],
      "forms": [
        {
          "form_text": "ephemerals",
          "form_token": "AB.AB.CC.Bx.Kg",
          "tag_tokens": ["..."]
        }
      ],
      "relations": [
        {
          "relation_token": "...",
          "target_word": "fleeting",
          "target_token": "AB.AB.CC.Af.Rn",
          "tag_tokens": ["..."]
        }
      ]
    }
  ]
}
```

#### `GET /entries/{word}/senses`

Just the senses for a word (lighter payload).

#### `GET /entries/{word}/forms`

Just the forms for a word.

#### `GET /entries/{word}/relations`

Just the relations for a word.

---

### Namespace & Status

For the browse UI and general project visibility.

#### `GET /namespaces`

List all namespace allocations.

```
GET /namespaces
```

Response:
```json
{
  "namespaces": [
    {
      "pattern": "AA",
      "name": "Universal",
      "description": "Universal / computational — byte codes, NSM primitives...",
      "alloc_type": "mode",
      "parent": null
    },
    ...
  ]
}
```

#### `GET /status`

Shard health and summary statistics.

```
GET /status
```

Response:
```json
{
  "shards": {
    "core": {
      "token_count": 2450,
      "last_updated": "2026-02-05",
      "size_mb": 2.3
    },
    "english": {
      "token_count": 1252854,
      "last_updated": "2026-02-05",
      "size_mb": 685,
      "layers": {
        "affix": 3696,
        "word": 1146520,
        "derivative": 3979,
        "multiword": 9084
      }
    },
    "names": {
      "token_count": 150528,
      "last_updated": "2026-02-05",
      "size_mb": 58
    }
  },
  "total_tokens": 1405832
}
```

#### `GET /status/coverage`

What percentage of common English words have tokens? Namespace utilization? Useful for project dashboards.

---

## Design Notes

### For the Chunking Tool (Instance B)

The critical path is: **word → token_id**. The tool needs:
1. `/tokens/lookup` — single word resolution
2. `/tokens/batch` — bulk resolution (main workhorse)
3. `/tokens/exists` — quick pre-check

Everything else is for the browse UI.

When a word isn't found, the tool flags it as TBD in the PBM. The TBD list ships with the submission so Patrick can resolve them during curation.

### Rate Limiting

Light touch. Public project, generous hosting. Suggested:
- 60 requests/minute for single endpoints
- 10 requests/minute for batch (which can carry hundreds of words each)
- No auth required for reads

### MySQL Schema

Not a 1:1 mirror of Postgres. Read-optimized:
- Denormalize where it speeds queries
- Add full-text indexes on `name` fields for search
- Skip construction-only columns/tables
- Prefix compression can happen at export time (Actions job)

### Data Freshness

Updated when Patrick pushes a new dump. Actions detects the change and runs the transform/sync pipeline. Could be daily, could be weekly — depends on project velocity.

---

## Open Questions for Instance B

1. **Batch size expectations?** How many words per batch call for typical chunking runs?
2. **TBD handling:** Should the API return suggested TBD identifiers, or does the tool generate its own?
3. **Name resolution:** When the tool hits a capitalized word, does it check `names` shard first, or does the API route automatically?
4. **Offline parity:** Should the API response format exactly match what the offline DB queries return, so the tool has one parser?
