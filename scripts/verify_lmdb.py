import lmdb
import os
import argparse
import sys
from pathlib import Path

def get_human_size(bytes_size):
    """Converts bytes to a readable format (MB, GB) across all platforms."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_size < 1024:
            return f"{bytes_size:.2f} {unit}"
        bytes_size /= 1024
    return f"{bytes_size:.2f} TB"

def verify_hcp_databases(db_path_str, verbose=False):
    """
    Analyzes HCP LMDB sub-databases. 
    Uses pathlib to ensure compatibility with Linux, macOS, and Windows.
    """
    # Use Path for OS-agnostic path handling
    base_path = Path(db_path_str)
    mdb_file = base_path / "data.mdb"

    if not base_path.exists():
        print(f"Error: Database directory not found at: {base_path}")
        print("Tip: Run the appropriate 'compile_..._lmdb.py' script first.")
        return

    # Check physical size
    if mdb_file.exists():
        file_size = mdb_file.stat().st_size
        print(f"--- Storage Report ---")
        print(f"Location:  {base_path.absolute()}")
        print(f"Disk Usage: {get_human_size(file_size)}\n")
    
    try:
        # lock=False is important for Linux/macOS to avoid permission issues 
        # when reading a database that might be open by the engine.
        env = lmdb.open(str(base_path), readonly=True, max_dbs=20, lock=False)
        
        target_dbs = [
            'vbed_02', 'vbed_04', 'vbed_08', 'vbed_16', 
            'entities_fic', 'entities_nf'
        ]

        # Formatting the header for a clean CLI experience
        header = f"{'Sub-DB':<15} | {'Entries':<12} | {'Status'}"
        print(header)
        print("-" * len(header))

        for db_name in target_dbs:
            try:
                db_handle = env.open_db(db_name.encode())
                with env.begin(db=db_handle) as txn:
                    count = txn.stat()['entries']
                    
                    # Using text-based status for better compatibility with CI/CD logs
                    status = "[ OK ]" if count > 0 else "[ EMPTY ]"
                    print(f"{db_name:<15} | {count:<12,} | {status}")

                    if verbose and count > 0:
                        with txn.cursor() as cursor:
                            samples = []
                            for i, (key, _) in enumerate(cursor):
                                samples.append(key.decode('utf-8', errors='replace'))
                                if i >= 4: break
                            print(f"   > Samples: {', '.join(samples)}")
                        
            except lmdb.Error:
                print(f"{db_name:<15} | {'N/A':<12} | [ MISSING ]")

        env.close()
        print("\nVerification process finished.")

    except Exception as e:
        print(f"Critical error: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cross-platform HCP LMDB Verification Tool")
    # Default path uses a string that pathlib will convert correctly for any OS
    parser.add_argument("--path", default="data/vocab.lmdb/", help="Path to the LMDB directory")
    parser.add_argument("--verbose", action="store_true", help="Show sample entries")
    
    args = parser.parse_args()
    verify_hcp_databases(args.path, args.verbose)
