"""Cache miss resolver — Postgres → LMDB backfill pipeline.

Handles three request types from the engine:
  1. "chunk"                → single token lookup (word, char, or label)
  2. "chunk_0 chunk_1"      → boolean forward walk (boilerplate prefix check)
  3. "AA.AE.AF.AA.AC chunk" → var DB mint-or-return (var_request token prefix)

Every resolved value gets written to LMDB so subsequent lookups are hits.
All LMDB values are plain UTF-8 strings — zero-copy mmap reads in C++.

Sub-databases (shared contract with C++ HCPVocabulary):
  w2t     word form   → token_id
  c2t     char (byte) → token_id
  l2t     label       → token_id
  t2w     token_id    → word form   (reverse, for reconstruction)
  t2c     token_id    → char byte   (reverse, for reconstruction)
  forward prefix      → "0"/"1"/token_id  (boilerplate walk)
  meta    (reserved for eviction tracking)

Forward walk encoding:
  Key not found → uncached (resolver needs to query Postgres)
  "0"           → cached negative (no match)
  "1"           → partial match (keep walking)
  token_id      → complete match (emit this sequence token)

Boilerplate stores are scoped by document source metadata.
"""

import lmdb
import psycopg

# System token IDs from hcp_core (pbm_anchor / boundary)
VAR_REQUEST = "AA.AE.AF.AA.AC"   # var_request — prepended to var DB requests
STREAM_START = "AA.AE.AF.AA.AA"  # stream_start
STREAM_END = "AA.AE.AF.AA.AB"    # stream_end


class CacheMissResolver:
    """Resolves LMDB cache misses from PostgreSQL vocab shards."""

    def __init__(self, lmdb_path, map_size=100 * 1024 * 1024,
                 db_host="localhost", db_user="hcp", db_pass="hcp_dev"):
        self.db_host = db_host
        self.db_user = db_user
        self.db_pass = db_pass

        # Open LMDB environment with named sub-databases
        # 8 max_dbs: w2t, c2t, l2t, t2w, t2c, forward, meta (+1 spare)
        self.env = lmdb.open(lmdb_path, map_size=map_size, max_dbs=8)
        self.w2t_db = self.env.open_db(b"w2t")          # word → token_id
        self.c2t_db = self.env.open_db(b"c2t")          # char → token_id
        self.l2t_db = self.env.open_db(b"l2t")          # label → token_id
        self.t2w_db = self.env.open_db(b"t2w")          # token_id → word
        self.t2c_db = self.env.open_db(b"t2c")          # token_id → char
        self.fwd_db = self.env.open_db(b"forward")       # prefix → 0/1/token_id
        self.meta_db = self.env.open_db(b"meta")          # eviction metadata

    def _pg_connect(self, dbname):
        return psycopg.connect(
            dbname=dbname, user=self.db_user,
            password=self.db_pass, host=self.db_host
        )

    # ------------------------------------------------------------------
    # Public API — called by engine on LMDB miss
    # ------------------------------------------------------------------

    def resolve(self, request, doc_id=None, position=None,
                source_tags=None):
        """Route a cache miss request to the appropriate handler.

        Args:
            request: The lookup string from the engine.
                     "chunk"                → vocab lookup
                     "AA.AE.AF.AA.AC chunk" → var DB (var_request prefix)
            doc_id: Document being processed (for var source tracking).
            position: Position in document (for var source tracking).
            source_tags: Active source tags for boilerplate scoping.

        Returns:
            token_id string, or None if unresolvable.
        """
        if request.startswith(VAR_REQUEST):
            chunk = request[len(VAR_REQUEST):].lstrip()
            return self._resolve_var(chunk, doc_id, position)
        else:
            return self._resolve_vocab(request)

    def check_forward(self, prefix, source_tags):
        """Boolean forward walk check — is this prefix in any boilerplate store?

        Args:
            prefix: Space-separated token sequence, e.g. "the end"
            source_tags: Source tags from document metadata for scoping.

        Returns:
            "1" if prefix matches a partial boilerplate sequence (keep going).
            Token_id string if prefix matches a complete sequence (done).
            "0" if no match (fall back to individual tokens).

        Postgres handles the terminal check internally — if there are
        no more positions after the prefix, it returns the boilerplate
        entity's token_id directly.
        """
        if not source_tags:
            return "0"

        # Check LMDB cache first
        cached = self._lmdb_get_forward(prefix)
        if cached is not None:
            return cached

        # Query Postgres
        result = self._query_boilerplate_prefix(prefix, source_tags)

        # Cache the result
        self._lmdb_put_forward(prefix, result)

        return result

    # ------------------------------------------------------------------
    # Vocab resolution — normal token lookup
    # ------------------------------------------------------------------

    def _resolve_vocab(self, chunk):
        """Resolve a chunk to a token_id via Postgres vocab shards.

        Determines token type (char, word, or label) and writes to
        the appropriate LMDB sub-databases including reverse lookups.
        """
        # Single character → core byte code (c2t + t2c)
        if len(chunk) == 1:
            token_id = self._lookup_core_char(chunk)
            if token_id:
                self._lmdb_put(self.c2t_db, chunk, token_id)
                self._lmdb_put(self.t2c_db, token_id, chunk)
                return token_id

        # Word lookup in hcp_english (w2t + t2w)
        token_id = self._lookup_english_word(chunk)
        if token_id:
            self._lmdb_put(self.w2t_db, chunk.lower(), token_id)
            self._lmdb_put(self.t2w_db, token_id, chunk.lower())
            return token_id

        return None

    def resolve_label(self, label):
        """Resolve a label (structural token name) to a token_id.

        Labels are system names like 'newline', 'tab', etc.
        Writes to l2t sub-database.
        """
        token_id = self._lookup_label(label)
        if token_id:
            self._lmdb_put(self.l2t_db, label, token_id)
        return token_id

    def _lookup_core_char(self, ch):
        """Look up a single character in hcp_core."""
        byte_val = ord(ch)
        if byte_val > 127:
            return None
        # Compute token_id directly — ASCII byte codes have deterministic addresses
        from hcp.core.token_id import encode_pair
        return f"AA.AA.AA.AA.{encode_pair(byte_val)}"

    def _lookup_english_word(self, word):
        """Look up a word in hcp_english."""
        conn = self._pg_connect("hcp_english")
        try:
            with conn.cursor() as cur:
                # Try lowercase first (standard words)
                cur.execute(
                    "SELECT token_id FROM tokens WHERE name = %s LIMIT 1",
                    (word.lower(),)
                )
                row = cur.fetchone()
                if row:
                    return row[0]

                # Try exact match (label/proper noun tokens)
                cur.execute(
                    "SELECT token_id FROM tokens "
                    "WHERE name = %s AND subcategory = 'label' LIMIT 1",
                    (word,)
                )
                row = cur.fetchone()
                return row[0] if row else None
        finally:
            conn.close()

    def _lookup_label(self, label):
        """Look up a label in hcp_english or hcp_core."""
        conn = self._pg_connect("hcp_english")
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT token_id FROM tokens "
                    "WHERE name = %s AND layer = 'label' LIMIT 1",
                    (label,)
                )
                row = cur.fetchone()
                return row[0] if row else None
        finally:
            conn.close()

    # ------------------------------------------------------------------
    # Var DB resolution — mint or return existing
    # ------------------------------------------------------------------

    def _resolve_var(self, chunk, doc_id=None, position=None):
        """Mint or return a var token for an unresolved chunk.

        Checks existing active var tokens first (handles LMDB eviction).
        Logs source location for librarian promotion workflow.
        Writes to w2t (var tokens are usable as word-position tokens).
        """
        conn = self._pg_connect("hcp_var")
        try:
            with conn.cursor() as cur:
                # Mint or return existing (atomic function)
                cur.execute("SELECT mint_var_token(%s)", (chunk,))
                var_id = cur.fetchone()[0]

                # Log source for librarian update index
                if doc_id is not None and position is not None:
                    cur.execute(
                        "INSERT INTO var_sources (var_id, doc_id, position) "
                        "VALUES (%s, %s, %s)",
                        (var_id, doc_id, position)
                    )

                conn.commit()
        finally:
            conn.close()

        # Var tokens go in w2t (they occupy word positions in documents)
        self._lmdb_put(self.w2t_db, chunk, var_id)
        self._lmdb_put(self.t2w_db, var_id, chunk)

        return var_id

    # ------------------------------------------------------------------
    # Forward walk — boolean boilerplate prefix check
    # ------------------------------------------------------------------

    def _query_boilerplate_prefix(self, prefix, source_tags):
        """Check if prefix matches any boilerplate sequence.

        Boilerplate sequences are Thing entities in hcp_nf_entities with
        category='boilerplate', subcategory matching source_tags.
        Their internal tokens are in position tables.

        Postgres handles the terminal check: if the prefix matches and
        there are no more positions, returns the boilerplate entity's
        token_id directly.

        Wiring route: document meta → source/type → boilerplate stores
        for that source/type → add to query set.

        Returns:
            "1" if prefix matches a partial boilerplate sequence.
            Token_id string if prefix completes a sequence.
            "0" if no match.
        """
        # TODO: Wire to actual entity position table queries when
        # boilerplate entities are populated. The query will:
        #
        # 1. Check document meta for source/type
        # 2. Check source/type for any boilerplate stores
        # 3. Check if prefix tokens match sequential positions in those stores
        # 4. If match and more positions remain → return "1"
        # 5. If match and no more positions → return entity token_id
        # 6. No match → return "0"
        #
        # For now, return "0" — no boilerplate entities exist yet.
        return "0"

    # ------------------------------------------------------------------
    # LMDB operations — all values are plain UTF-8 strings
    # ------------------------------------------------------------------

    def _lmdb_put(self, db, key, value):
        """Write a key → value pair to an LMDB sub-database.

        Both key and value are UTF-8 encoded strings.
        """
        with self.env.begin(write=True) as txn:
            txn.put(
                key.encode("utf-8"),
                value.encode("utf-8"),
                db=db
            )

    def _lmdb_get(self, db, key):
        """Read a value from an LMDB sub-database. Returns string or None."""
        with self.env.begin(db=db) as txn:
            val = txn.get(key.encode("utf-8"))
            if val is None:
                return None
            return val.decode("utf-8")

    def _lmdb_put_forward(self, prefix, result):
        """Write a forward walk result to LMDB.

        Result is "0" (no match), "1" (partial match), or token_id (complete).
        """
        self._lmdb_put(self.fwd_db, prefix, result)

    def _lmdb_get_forward(self, prefix):
        """Read a forward walk result from LMDB.

        Returns "0"/"1"/token_id string, or None if uncached.
        """
        return self._lmdb_get(self.fwd_db, prefix)

    # Public read helpers

    def lmdb_get_word(self, word):
        """Read a word → token_id from LMDB. Returns string or None."""
        return self._lmdb_get(self.w2t_db, word)

    def lmdb_get_char(self, ch):
        """Read a char → token_id from LMDB. Returns string or None."""
        return self._lmdb_get(self.c2t_db, ch)

    def lmdb_get_label(self, label):
        """Read a label → token_id from LMDB. Returns string or None."""
        return self._lmdb_get(self.l2t_db, label)

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self):
        """Close LMDB environment."""
        self.env.close()
