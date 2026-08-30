#!/usr/bin/env bash
# build_spirv.sh — the portable transpile chain, all the way to Vulkan bytecode:
#   AZSL --(cpp include preprocess)--> azslc HLSL --(dxc -spirv)--> SPIR-V
# This is the exact chain O3DE's shader build uses. Produces settle.spv for the
# Vulkan harness (vk_settle_harness). Skips cleanly (exit 0) if O3DE absent.
set -euo pipefail
cd "$(dirname "$0")"

O3DE_ROOT="${O3DE_ROOT:-/opt/O3DE/25.10.2}"
BIN="$O3DE_ROOT/bin/Linux/profile/Default/Builders"
AZSLC="$(find "$O3DE_ROOT" -iname azslc -type f 2>/dev/null | head -1 || true)"
DXC="$(find "$BIN/DirectXShaderCompiler" -iname 'dxc*' -type f 2>/dev/null | head -1 || true)"
SRGLIB="$O3DE_ROOT/Gems/Atom/Feature/Common/Assets/ShaderLib"
OUT="${1:-settle.spv}"

if [[ -z "$AZSLC" || -z "$DXC" || ! -d "$SRGLIB" ]]; then
    echo "SKIP: O3DE azslc/dxc not found under $O3DE_ROOT"
    exit 0
fi

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

# Stage 1: include preprocessing (no line markers / system headers).
cpp -P -nostdinc -undef -I . -I "$SRGLIB" HCPSettleCompute.azsl > "$TMP/pp.azsl"
# Stage 2: AZSL -> HLSL (Vulkan namespace, unique register indices for Vulkan).
"$AZSLC" --namespace vk --unique-idx -o "$TMP/settle.hlsl" "$TMP/pp.azsl"
# Stage 3: HLSL -> SPIR-V (compute, SM6.3, entry SettleStep).
"$DXC" -T cs_6_3 -E SettleStep -spirv -Fo "$OUT" "$TMP/settle.hlsl"

echo "OK: wrote $OUT ($(stat -c%s "$OUT") bytes)"
