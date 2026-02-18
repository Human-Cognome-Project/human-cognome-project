"""Cache miss resolver — Postgres → LMDB backfill pipeline.

Handles three request types from the engine:
  1. "chunk"      → single token lookup (normal vocab resolution)
  2. "chunk_0 chunk_1" → boolean forward walk (boilerplate prefix check)
  3. "AA.AE.AF.AA.AC chunk" → var DB mint-or-return (var_request token prefix)

Every resolved token gets written to LMDB so subsequent lookups are hits.
LMDB values are msgpack-encoded: {"t": token_id} for vocab entries.

Forward walk is a simple boolean loop driven by the engine:
  - Engine submits "chunk_0 chunk_1" → true/false against boilerplate stores
  - True → engine concatenates next word, asks again
  - False → engine falls back to individual tokens
  - Token_id string → sequence complete, engine emits the sequence token

Boilerplate stores are scoped by document source metadata.
"""

import lmdb
import msgpack
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
        self.env = lmdb.open(lmdb_path, map_size=map_size, max_dbs=3)
        self.vocab_db = self.env.open_db(b"vocab")       # chunk → {t}
        self.fwd_db = self.env.open_db(b"forward")        # prefix → bool/token_id
        self.meta_db = self.env.open_db(b"meta")           # eviction metadata

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
                     "chunk"           → vocab lookup
                     "AA.AE.AF.AA.AC chunk" → var DB (var_request prefix)
            doc_id: Document being processed (for var source tracking).
            position: Position in document (for var source tracking).
            source_tags: Active source tags for boilerplate scoping.

        Returns:
            For vocab: {"t": token_id}
            For var:   {"t": var_id}
            None if unresolvable.
        """
        if request.startswith(VAR_REQUEST):
            chunk = request[len(VAR_REQUEST):]
            return self._resolve_var(chunk, doc_id, position)
        else:
            return self._resolve_vocab(request)

    def check_forward(self, prefix, source_tags):
        """Boolean forward walk check — is this prefix in any boilerplate store?

        Args:
            prefix: Space-separated token sequence, e.g. "the end"
            source_tags: Source tags from document metadata for scoping.

        Returns:
            True if prefix matches a partial boilerplate sequence (keep going).
            Token_id string if prefix matches a complete sequence (done).
            False if no match (fall back to individual tokens).

        Postgres handles the terminal check internally — if there are
        no more positions after the prefix, it returns the boilerplate
        entity's token_id directly.
        """
        if not source_tags:
            return False

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

        Checks hcp_core (single chars, punctuation) and hcp_english (words).
        Writes result to LMDB.
        """
        token_id = None

        # Single character → core byte code
        if len(chunk) == 1:
            token_id = self._lookup_core_char(chunk)

        # Word lookup in hcp_english
        if token_id is None:
            token_id = self._lookup_english_word(chunk)

        if token_id is None:
            return None

        result = {"t": token_id}

        # Write to LMDB
        self._lmdb_put_vocab(chunk, result)

        return result

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

    # ------------------------------------------------------------------
    # Var DB resolution — mint or return existing
    # ------------------------------------------------------------------

    def _resolve_var(self, chunk, doc_id=None, position=None):
        """Mint or return a var token for an unresolved chunk.

        Checks existing active var tokens first (handles LMDB eviction).
        Logs source location for librarian promotion workflow.
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

        result = {"t": var_id}

        # Write to LMDB
        self._lmdb_put_vocab(chunk, result)

        return result

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
            True if prefix matches a partial boilerplate sequence.
            Token_id string if prefix completes a sequence.
            False if no match.
        """
        # TODO: Wire to actual entity position table queries when
        # boilerplate entities are populated. The query will:
        #
        # 1. Check document meta for source/type
        # 2. Check source/type for any boilerplate stores
        # 3. Check if prefix tokens match sequential positions in those stores
        # 4. If match and more positions remain → return True
        # 5. If match and no more positions → return entity token_id
        # 6. No match → return False
        #
        # For now, return False — no boilerplate entities exist yet.
        return False

    # ------------------------------------------------------------------
    # LMDB write operations
    # ------------------------------------------------------------------

    def _lmdb_put_vocab(self, chunk, result):
        """Write a vocab entry to LMDB."""
        with self.env.begin(write=True) as txn:
            txn.put(
                chunk.encode("utf-8"),
                msgpack.packb(result),
                db=self.vocab_db
            )

    def _lmdb_put_forward(self, prefix, result):
        """Write a forward walk result to LMDB.

        Result is True (partial match), token_id string (complete), or False.
        """
        with self.env.begin(write=True) as txn:
            txn.put(
                prefix.encode("utf-8"),
                msgpack.packb(result),
                db=self.fwd_db
            )


    # ------------------------------------------------------------------
    # LMDB read operations
    # ------------------------------------------------------------------

    def lmdb_get_vocab(self, chunk):
        """Read a vocab entry from LMDB. Returns dict or None."""
        with self.env.begin(db=self.vocab_db) as txn:
            val = txn.get(chunk.encode("utf-8"))
            if val is None:
                return None
            return msgpack.unpackb(val)

    def _lmdb_get_forward(self, prefix):
        """Read a forward walk result from LMDB. Returns bool/token_id or None."""
        with self.env.begin(db=self.fwd_db) as txn:
            val = txn.get(prefix.encode("utf-8"))
            if val is None:
                return None
            return msgpack.unpackb(val)

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self):
        """Close LMDB environment."""
        self.env.close()
