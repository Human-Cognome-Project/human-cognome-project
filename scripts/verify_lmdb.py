#!/usr/bin/env python3
"""
HCP LMDB Verification Tool
Author: dhitalprashant77-lab
Description: 
A diagnostic utility to check the integrity of the Human Cognome Project's 
vocabulary and entity databases. It dynamically discovers sub-databases 
and decodes fixed-width records for human-readable inspection.
"""

import lmdb
import os
import argparse
import sys
from pathlib import Path

def get_human_size(bytes_size):
    """Converts raw bytes into a format humans actually want to read."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_size < 1024:
            return f"{bytes_size:.2f} {unit}"
        bytes_size /= 1024
    return f"{bytes_size:.2f} TB"

def decode_hcp_record(db_name, raw_data):
    """
    Decodes the HCP fixed-width format: [Word (N bytes) | Token ID (14 bytes)].
    The 'N' is determined by the number in the vbed_XX name.
    """
    if db_name.startswith("vbed_") and "meta" not in db_name:
        try:
            # vbed_08 means the first 8 bytes are the word
            word_len = int(db_name.split('_')[1])
            word = raw_data[:word_len].decode('utf-8', errors='replace').strip()
            # The remaining 14 bytes are the Token ID
            token_id = raw_data[word_len:].decode('utf-8', errors='replace').strip()
            return f"Word: '{word}' | ID: {token_id}"
        except (ValueError, IndexError):
            pass
    
    # If it's not a vbed or decoding fails, show a snippet of the data
    return raw_data[:30].hex() + "..."

def verify_hcp_databases(db_path_str, verbose=False):
    """
    The main engine for checking our LMDB environment.
    """
    # Use Pathlib to ensure this works on Windows, Linux (ARM/x64), and macOS
    base_path = Path(db_path_str)
    
    if not base_path.exists():
        print(f"❌ Error: We couldn't find a database at {base_path.absolute()}")
        print("Tip: Make sure you've run the 'compile_vocab_lmdb.py' script first!")
        return

    print(f"🔍 Inspecting Cognome Database: {base_path.name}")
    print(f"📍 Location: {base_path.absolute()}")

    try:
        # max_dbs=64: We use 48 currently (6 core + 30 vbed + 1 manifest + 4 entity + headroom).
        # lock=False: Essential so we don't crash if the C++ engine is running.
        env = lmdb.open(str(base_path), readonly=True, max_dbs=64, lock=False)
        
        # DYNAMIC DISCOVERY: Instead of hardcoding names, we ask the DB what it has.
        # This is done by opening the 'unnamed' database which acts as a master list.
        with env.begin() as txn:
            master_db = env.open_db()
            cursor = txn.cursor(master_db)
            sub_dbs = [key.decode('utf-8') for key, _ in cursor]
        
        if not sub_dbs:
            print("⚠️ The database environment is open, but it appears to be empty.")
            return

        print(f"📊 Found {len(sub_dbs)} sub-databases. Starting scan...\n")
        print(f"{'Sub-Database':<20} | {'Entries':<12} | {'Status'}")
        print("-" * 50)

        for name in sorted(sub_dbs):
            try:
                handle = env.open_db(name.encode())
                with env.begin(db=handle) as txn:
                    entry_count = txn.stat()['entries']
                    status = "✅ OK" if entry_count > 0 else "⚠️ EMPTY"
                    print(f"{name:<20} | {entry_count:<12,} | {status}")

                    # If the user wants to see the data, we'll peek at the first 3 records
                    if verbose and entry_count > 0:
                        cursor = txn.cursor()
                        for i, (key, value) in enumerate(cursor):
                            # In HCP vbed dbs, the 'value' is often empty; data is in the key.
                            # We'll decode the key as that's where the Word|ID lives.
                            decoded = decode_hcp_record(name, key)
                            print(f"   └─ Sample [{i}]: {decoded}")
                            if i >= 2: break # Only show 3 samples so we don't flood the terminal
            
            except lmdb.Error as e:
                print(f"{name:<20} | {'Error':<12} | ❌ Could not open: {e}")

        env.close()
        print("\n✨ Verification complete. Everything looks good for the pipeline!")

    except Exception as e:
        print(f"\n💥 A critical error stopped the scan: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="HCP LMDB Diagnostic Tool")
    parser.add_argument("--path", default="data/vocab.lmdb/", help="Path to your LMDB folder")
    parser.add_argument("--verbose", action="store_true", help="Peek inside the records to see decoded words/IDs")
    
    args = parser.parse_args()
    verify_hcp_databases(args.path, args.verbose)
