"""PBM storage — write/read from hcp_fic_pbm prefix tree schema.

Handles routing bonds to the correct subtable by B-side namespace,
storing only distinguishing segments per the schema design.
"""

import psycopg
from hcp.core.token_id import encode_pair


DB_CONFIG = {
    "dbname": "hcp_fic_pbm",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
    "port": 5432,
}


def connect():
    return psycopg.connect(**DB_CONFIG)


def _split_token_id(token_id):
    """Split a token_id into its component segments."""
    parts = token_id.split(".")
    # Pad to 5 segments
    while len(parts) < 5:
        parts.append(None)
    return parts[:5]


def _next_pbm_address(conn, ns, p2, p3):
    """Get next available PBM address for the given century slot.

    Returns (p4, p5) as base-50 encoded pair values.
    Uses the pbm_counters table for auto-increment.
    """
    with conn.cursor() as cur:
        # Ensure counter exists
        cur.execute("""
            INSERT INTO pbm_counters (ns, p2, p3, next_value)
            VALUES (%s, %s, %s, 1)
            ON CONFLICT (ns, p2, p3) DO NOTHING
        """, (ns, p2, p3))

        # Atomically get and increment
        cur.execute("""
            UPDATE pbm_counters
            SET next_value = next_value + 1
            WHERE ns = %s AND p2 = %s AND p3 = %s
            RETURNING next_value - 1
        """, (ns, p2, p3))
        seq = cur.fetchone()[0]

    # Encode as two base-50 pairs: p4 = seq // 2500, p5 = seq % 2500
    p4 = encode_pair(seq // 2500)
    p5 = encode_pair(seq % 2500)
    return p4, p5


def store_pbm(conn, name, century_code, pbm_data, category="book",
              subcategory="novel", metadata=None):
    """Store a PBM to the prefix tree schema.

    Args:
        conn: psycopg connection to hcp_fic_pbm
        name: Human-readable document name
        century_code: Century code (e.g., 'AS' for 19th century)
        pbm_data: dict from disassemble() — must have bonds, first_fpb
        category: Document category
        subcategory: Document subcategory
        metadata: Optional JSONB metadata

    Returns:
        doc_id string (e.g., 'vA.AB.AS.AA.AA')
    """
    import json

    ns = "vA"
    p2 = "AB"
    p3 = century_code

    # Get next available address
    p4, p5 = _next_pbm_address(conn, ns, p2, p3)

    first_a = pbm_data["first_fpb"][0] if pbm_data["first_fpb"] else None
    first_b = pbm_data["first_fpb"][1] if pbm_data["first_fpb"] else None

    with conn.cursor() as cur:
        # Insert document
        cur.execute("""
            INSERT INTO pbm_documents (ns, p2, p3, p4, p5, name, category,
                                       subcategory, first_fpb_a, first_fpb_b, metadata)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb)
            RETURNING id, doc_id
        """, (ns, p2, p3, p4, p5, name, category, subcategory,
              first_a, first_b, json.dumps(metadata or {})))
        doc_pk, doc_id = cur.fetchone()

        # Build starter lookup: token_a → starter_pk
        starter_pks = {}
        unique_starters = set(a for a, b, c in pbm_data["bonds"])

        for token_a in unique_starters:
            a_parts = _split_token_id(token_a)
            cur.execute("""
                INSERT INTO pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5)
                VALUES (%s, %s, %s, %s, %s, %s)
                RETURNING id
            """, (doc_pk, a_parts[0], a_parts[1], a_parts[2],
                  a_parts[3], a_parts[4]))
            starter_pks[token_a] = cur.fetchone()[0]

        # Route bonds to correct subtables
        word_bonds = []
        char_bonds = []
        marker_bonds = []

        for token_a, token_b, count in pbm_data["bonds"]:
            starter_id = starter_pks[token_a]
            b_parts = _split_token_id(token_b)
            b_ns = b_parts[0]
            b_p2 = b_parts[1]

            if b_ns == "AB" and b_p2 == "AB":
                # Word bond — store p3, p4, p5
                word_bonds.append((starter_id, b_parts[2], b_parts[3],
                                   b_parts[4], count))
            elif b_ns == "AA" and b_p2 == "AE" and b_parts[4] is None:
                # Marker bond (no p5) — store p3, p4
                marker_bonds.append((starter_id, b_parts[2], b_parts[3],
                                     count))
            elif b_ns == "AA":
                # Character bond — store p2, p3, p4, p5
                char_bonds.append((starter_id, b_parts[1], b_parts[2],
                                   b_parts[3], b_parts[4], count))
            else:
                # Fallback: treat as character bond with full B decomposition
                char_bonds.append((starter_id, b_parts[1], b_parts[2],
                                   b_parts[3], b_parts[4], count))

        # Bulk insert bonds
        if word_bonds:
            cur.executemany("""
                INSERT INTO pbm_word_bonds (starter_id, b_p3, b_p4, b_p5, count)
                VALUES (%s, %s, %s, %s, %s)
            """, word_bonds)

        if char_bonds:
            cur.executemany("""
                INSERT INTO pbm_char_bonds (starter_id, b_p2, b_p3, b_p4, b_p5, count)
                VALUES (%s, %s, %s, %s, %s, %s)
            """, char_bonds)

        if marker_bonds:
            cur.executemany("""
                INSERT INTO pbm_marker_bonds (starter_id, b_p3, b_p4, count)
                VALUES (%s, %s, %s, %s)
            """, marker_bonds)

    conn.commit()
    return doc_id


def load_pbm(conn, doc_id):
    """Load a PBM from the prefix tree schema.

    Args:
        conn: psycopg connection to hcp_fic_pbm
        doc_id: Document address string (e.g., 'vA.AB.AS.AA.AA')

    Returns:
        dict with:
          - doc_id: string
          - name: string
          - first_fpb: (token_a, token_b) or None
          - bonds: list of (token_a, token_b, count) tuples
    """
    with conn.cursor() as cur:
        # Get document
        cur.execute("""
            SELECT id, name, first_fpb_a, first_fpb_b
            FROM pbm_documents WHERE doc_id = %s
        """, (doc_id,))
        row = cur.fetchone()
        if not row:
            return None
        doc_pk, name, fpb_a, fpb_b = row

        # UNION ALL extraction — reconstruct full token_B from prefixes
        cur.execute("""
            SELECT s.token_a_id,
                   'AB.AB.' || wb.b_p3 || '.' || wb.b_p4
                             || COALESCE('.' || wb.b_p5, '') AS token_b_id,
                   wb.count
            FROM pbm_word_bonds wb
            JOIN pbm_starters s ON s.id = wb.starter_id
            WHERE s.doc_id = %s

            UNION ALL

            SELECT s.token_a_id,
                   'AA.' || cb.b_p2 || '.' || cb.b_p3 || '.' || cb.b_p4
                         || COALESCE('.' || cb.b_p5, '') AS token_b_id,
                   cb.count
            FROM pbm_char_bonds cb
            JOIN pbm_starters s ON s.id = cb.starter_id
            WHERE s.doc_id = %s

            UNION ALL

            SELECT s.token_a_id,
                   'AA.AE.' || mb.b_p3 || '.' || mb.b_p4 AS token_b_id,
                   mb.count
            FROM pbm_marker_bonds mb
            JOIN pbm_starters s ON s.id = mb.starter_id
            WHERE s.doc_id = %s
        """, (doc_pk, doc_pk, doc_pk))

        bonds = [(a, b, c) for a, b, c in cur.fetchall()]

    return {
        "doc_id": doc_id,
        "name": name,
        "first_fpb": (fpb_a, fpb_b) if fpb_a else None,
        "bonds": bonds,
    }
