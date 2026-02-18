"""Cache miss resolver — Postgres → LMDB backfill pipeline.

Handles three request types from the engine:
  1. "chunk"      → single token lookup (normal vocab resolution)
  2. "chunk *"    → forward walk start (continuation index query)
  3. "<var>chunk" → var DB mint-or-return for unresolved sequences

Every resolved token gets written to LMDB so subsequent lookups are hits.
LMDB values are msgpack-encoded: {"t": token_id, "c": [continuations]}.

For forward walk:
  - "chunk *"           → array of next words (or empty)
  - "chunk nextword *"  → array of next words, or [stream_end] if complete
  - "compiled string"   → sequence token_id (final resolution)
"""

import lmdb
import msgpack
import psycopg

# Stream end marker token — signals completion of a forward walk sequence
STREAM_END = "AA.AE.AF.AA.AB"


class CacheMissResolver:
    """Resolves LMDB cache misses from PostgreSQL vocab shards."""

    def __init__(self, lmdb_path, map_size=100 * 1024 * 1024,
                 db_host="localhost", db_user="hcp", db_pass="hcp_dev"):
        self.db_host = db_host
        self.db_user = db_user
        self.db_pass = db_pass

        # Open LMDB environment with named sub-databases
        self.env = lmdb.open(lmdb_path, map_size=map_size, max_dbs=4)
        self.vocab_db = self.env.open_db(b"vocab")       # chunk → {t, c}
        self.fwd_db = self.env.open_db(b"forward")        # "prefix *" → [next words]
        self.compiled_db = self.env.open_db(b"compiled")   # compiled string → token_id
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
                     "chunk"      → vocab lookup
                     "chunk *"    → forward walk
                     "<var>chunk" → var DB
            doc_id: Document being processed (for var source tracking).
            position: Position in document (for var source tracking).
            source_tags: Active source tags for continuation scoping.

        Returns:
            For vocab:    {"t": token_id, "c": [continuations]}
            For forward:  [next_word, ...] or [STREAM_END]
            For var:      {"t": var_id, "c": []}
            None if unresolvable.
        """
        if request.startswith("<var>"):
            chunk = request[5:]
            return self._resolve_var(chunk, doc_id, position)
        elif request.endswith(" *"):
            prefix = request[:-2]
            return self._resolve_forward(prefix, source_tags)
        else:
            return self._resolve_vocab(request, source_tags)

    # ------------------------------------------------------------------
    # Vocab resolution — normal token lookup
    # ------------------------------------------------------------------

    def _resolve_vocab(self, chunk, source_tags=None):
        """Resolve a chunk to a token_id via Postgres vocab shards.

        Checks hcp_core (single chars, punctuation) and hcp_english (words).
        Builds continuation data from stored sequences if source_tags given.
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

        # Build continuation data
        continuations = []
        if source_tags:
            continuations = self._query_continuations(chunk, source_tags)

        result = {"t": token_id, "c": continuations}

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

        result = {"t": var_id, "c": []}

        # Write to LMDB
        self._lmdb_put_vocab(chunk, result)

        return result

    # ------------------------------------------------------------------
    # Forward walk — continuation index queries
    # ------------------------------------------------------------------

    def _resolve_forward(self, prefix, source_tags=None):
        """Query stored sequences for next-word continuations.

        Args:
            prefix: Space-separated token sequence so far.
                    "the" → find sequences starting with "the"
                    "the quick" → find sequences matching "the quick" at pos 1-2
            source_tags: Active source tags for scoping.

        Returns:
            List of next words, or [STREAM_END] if prefix completes a sequence,
            or [] if no continuations.
        """
        if not source_tags:
            return []

        parts = prefix.split()
        match_position = len(parts)  # 1-indexed position to check next

        conn = self._pg_connect("hcp_nf_entities")
        try:
            with conn.cursor() as cur:
                # Find sequences where all prefix positions match
                # and return the next position's token
                #
                # This uses the entity shard: boilerplate sequences are
                # Thing entities with category='boilerplate'.
                # Their internal tokens are stored in position tables.
                #
                # For now, query sequence_positions equivalent.
                # TODO: adapt to actual entity position table structure
                # when boilerplate entities are populated.
                next_words = self._query_sequence_next(
                    cur, parts, match_position, source_tags
                )
        finally:
            conn.close()

        # Cache in LMDB
        fwd_key = prefix + " *"
        self._lmdb_put_forward(fwd_key, next_words)

        return next_words

    def _query_sequence_next(self, cur, prefix_parts, next_pos, source_tags):
        """Query entity shard for next word in matching sequences.

        Returns list of next words, or [STREAM_END] if a sequence completes.
        """
        # This is a placeholder for the actual entity position table query.
        # The real implementation will:
        # 1. Find all boilerplate entities (category='boilerplate',
        #    subcategory IN source_tags) whose position data matches
        #    the prefix at positions 1..N
        # 2. Return the tokens at position N+1
        # 3. If position N+1 is stream_end marker, return [STREAM_END]
        #
        # For now, return empty — no boilerplate entities exist yet.
        return []

    def _query_continuations(self, chunk, source_tags):
        """Query continuation index for a vocab entry.

        Returns list of next-word strings for sequences starting with chunk.
        """
        # Same placeholder — queries boilerplate entities where position 1
        # matches the chunk token, returns position 2 values.
        return []

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

    def _lmdb_put_forward(self, key, next_words):
        """Write a forward walk entry to LMDB."""
        with self.env.begin(write=True) as txn:
            txn.put(
                key.encode("utf-8"),
                msgpack.packb(next_words),
                db=self.fwd_db
            )

    def _lmdb_put_compiled(self, compiled_string, token_id):
        """Write a compiled sequence entry to LMDB."""
        with self.env.begin(write=True) as txn:
            txn.put(
                compiled_string.encode("utf-8"),
                msgpack.packb(token_id),
                db=self.compiled_db
            )

    # ------------------------------------------------------------------
    # LMDB read operations (called by engine before triggering miss)
    # ------------------------------------------------------------------

    def lmdb_get_vocab(self, chunk):
        """Read a vocab entry from LMDB. Returns dict or None."""
        with self.env.begin(db=self.vocab_db) as txn:
            val = txn.get(chunk.encode("utf-8"))
            if val is None:
                return None
            return msgpack.unpackb(val)

    def lmdb_get_forward(self, prefix):
        """Read a forward walk entry from LMDB. Returns list or None."""
        key = prefix + " *"
        with self.env.begin(db=self.fwd_db) as txn:
            val = txn.get(key.encode("utf-8"))
            if val is None:
                return None
            return msgpack.unpackb(val)

    def lmdb_get_compiled(self, compiled_string):
        """Read a compiled sequence entry from LMDB. Returns token_id or None."""
        with self.env.begin(db=self.compiled_db) as txn:
            val = txn.get(compiled_string.encode("utf-8"))
            if val is None:
                return None
            return msgpack.unpackb(val)

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self):
        """Close LMDB environment."""
        self.env.close()
