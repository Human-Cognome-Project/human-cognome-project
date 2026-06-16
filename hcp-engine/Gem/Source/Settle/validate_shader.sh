#!/usr/bin/env bash
# validate_shader.sh — the AZSL shader's "test": preprocess includes, run azslc
# semantic check, and assert the compute entry point + numthreads survive.
# Requires an O3DE install (for azslc + the Atom SrgSemantics ShaderLib). Skips
# cleanly (exit 0) if O3DE is not present, so it's safe in CI without O3DE.
set -euo pipefail
cd "$(dirname "$0")"

O3DE_ROOT="${O3DE_ROOT:-/opt/O3DE/25.10.2}"
AZSLC="$(find "$O3DE_ROOT" -iname azslc -type f 2>/dev/null | head -1 || true)"
SRGLIB="$O3DE_ROOT/Gems/Atom/Feature/Common/Assets/ShaderLib"

if [[ -z "$AZSLC" || ! -d "$SRGLIB" ]]; then
    echo "SKIP: O3DE/azslc not found under $O3DE_ROOT (set O3DE_ROOT to enable)"
    exit 0
fi

SHADER="HCPSettleCompute.azsl"
PP="$(mktemp --suffix=.azsl)"
trap 'rm -f "$PP"' EXIT

# Stage 1: mcpp-equivalent include preprocessing (no line markers / system hdrs).
cpp -P -nostdinc -undef -I . -I "$SRGLIB" "$SHADER" > "$PP"

# Stage 2: AZSL semantic check (no output = clean).
if ! "$AZSLC" --semantic "$PP"; then
    echo "FAIL: $SHADER failed azslc --semantic"; exit 1
fi

# Stage 3: assert the compute entry + numthreads survived.
IA="$("$AZSLC" --namespace vk --ia "$PP")"
if ! grep -q '"entry" : "SettleStep"' <<<"$IA"; then
    echo "FAIL: SettleStep compute entry not found in IA output"; exit 1
fi
if ! grep -Pzoq '"numthreads"\s*:\s*\[\s*64,\s*1,\s*1' <<<"$IA"; then
    echo "FAIL: SettleStep numthreads != [64,1,1]"; exit 1
fi

echo "PASS: $SHADER transpiles clean; SettleStep numthreads [64,1,1]"
