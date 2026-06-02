#!/bin/bash
# Build the HCP Source Workstation installer (offline).
# Usage: ./build_installer.sh [output_name]
#
# Prerequisites:
#   - QtIFW binarycreator at ~/QtIFW-4.8.1/bin/binarycreator
#   - Workstation binary built at ../build/linux/bin/profile/HCPWorkstation
#   - LMDB vocab at ../data/vocab.lmdb/ (for vocab component)
#   - Engine daemon at ../build/linux/bin/profile/HeadlessServerLauncher (for engine component)
#
# The script copies binaries into package data/ directories before calling binarycreator.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BINARYCREATOR="${HOME}/QtIFW-4.8.1/bin/binarycreator"
OUTPUT="${1:-HCPWorkstation-Installer-linux-x64}"

echo "=== HCP Workstation Installer Build ==="
echo "Repo: $REPO_ROOT"
echo "Output: $OUTPUT"
echo ""

# --- Workstation binary ---
WS_BIN="$REPO_ROOT/build/linux/bin/profile/HCPWorkstation"
WS_DATA="$SCRIPT_DIR/packages/com.hcp.workstation/data/bin"
if [ -f "$WS_BIN" ]; then
    mkdir -p "$WS_DATA"
    cp "$WS_BIN" "$WS_DATA/HCPWorkstation"
    echo "[OK] Workstation binary copied"
else
    echo "[WARN] Workstation binary not found at $WS_BIN — skipping"
fi

# --- Engine daemon ---
ENGINE_BIN="$REPO_ROOT/build/linux/bin/profile/HeadlessServerLauncher"
ENGINE_DATA="$SCRIPT_DIR/packages/com.hcp.engine/data/daemon"
if [ -f "$ENGINE_BIN" ]; then
    mkdir -p "$ENGINE_DATA"
    cp "$ENGINE_BIN" "$ENGINE_DATA/HeadlessServerLauncher"
    echo "[OK] Engine daemon copied"
else
    echo "[WARN] Engine daemon not found at $ENGINE_BIN — skipping"
fi

# --- LMDB vocab ---
VOCAB_SRC="$REPO_ROOT/data/vocab.lmdb"
VOCAB_DATA="$SCRIPT_DIR/packages/com.hcp.vocab.lmdb/data/vocab"
if [ -d "$VOCAB_SRC" ]; then
    mkdir -p "$VOCAB_DATA"
    cp -r "$VOCAB_SRC" "$VOCAB_DATA/vocab.lmdb"
    echo "[OK] LMDB vocab copied"
else
    echo "[WARN] LMDB vocab not found at $VOCAB_SRC — skipping"
fi

# --- Build installer ---
echo ""
echo "Building installer..."
"$BINARYCREATOR" \
    --offline-only \
    -c "$SCRIPT_DIR/config/config.xml" \
    -p "$SCRIPT_DIR/packages" \
    "$SCRIPT_DIR/$OUTPUT"

echo ""
echo "=== Done: $SCRIPT_DIR/$OUTPUT ==="
