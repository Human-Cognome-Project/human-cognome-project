"""Test database connectivity and basic operations."""

import psycopg2

def test_connections():
    """Test connections to all required databases."""

    databases = ['hcp_en_pbm', 'hcp_english', 'hcp_names']

    for dbname in databases:
        try:
            conn = psycopg2.connect(
                host="localhost",
                database=dbname,
                user="hcp",
                password="hcp_dev"
            )
            with conn.cursor() as cur:
                cur.execute("SELECT 1")
                result = cur.fetchone()
            conn.close()
            print(f"✓ {dbname}: Connected successfully")
        except Exception as e:
            print(f"✗ {dbname}: {e}")

def test_simple_insert():
    """Test simple insert into hcp_en_pbm."""

    try:
        conn = psycopg2.connect(
            host="localhost",
            database="hcp_en_pbm",
            user="hcp",
            password="hcp_dev"
        )

        with conn.cursor() as cur:
            # Try inserting a test document
            cur.execute("""
                INSERT INTO documents (
                    token_id, external_id, source_token,
                    document_type_token, publication_year, language_token,
                    char_count, token_count, first_fpb
                ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
            """, (
                'zA.TEST1',
                'test123',
                'test_source',
                'fiction',
                2025,
                'en',
                1000,
                100,
                ['token1', 'token2']
            ))
            conn.commit()
            print("✓ Test document inserted successfully")

            # Query it back
            cur.execute("SELECT token_id, external_id FROM documents WHERE token_id = 'zA.TEST1'")
            row = cur.fetchone()
            print(f"  Retrieved: {row}")

            # Clean up
            cur.execute("DELETE FROM documents WHERE token_id = 'zA.TEST1'")
            conn.commit()
            print("✓ Test document deleted")

        conn.close()

    except Exception as e:
        print(f"✗ Insert test failed: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("Testing database connections...")
    test_connections()
    print("\nTesting simple insert...")
    test_simple_insert()
    print("\nAll tests complete!")
