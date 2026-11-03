#!/usr/bin/env bash
# Regression witness for inherited alternate CPU paths in an existing cache.
# Requires an already built local CPU checkout; retains all witness artifacts.
set -Eeuo pipefail
repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo"
test -z "$(git status --porcelain)"
out=$(realpath -m "${1:?Supply a new evidence directory outside the checkout}")
case "$out/" in "$repo/"*) exit 2 ;; esac
mkdir "$out"
build="$repo/engine/build"
test -f "$build/CMakeCache.txt"
exec 9>"$build/.local-ci.lock"
flock -n 9 || { echo 'Build is in use.'; exit 2; }
# Valid alternate paths to a CPU installation, without needing a second fork
# build. The runtime is a separate copy; all three cache values differ.
ln -s "$repo/engine/taichi" "$out/alternate-source"
ln -s "$repo/engine/taichi/build-review" "$out/alternate-build"
mkdir "$out/alternate-runtime"
cp "$repo/engine/taichi/taichi/runtime/llvm/runtime_module/runtime_x64.bc" \
  "$out/alternate-runtime/"
cmake -S engine -B "$build" \
  -DENGINE_TAICHI_ROOT="$out/alternate-source" \
  -DENGINE_TAICHI_BUILD="$out/alternate-build" \
  -DENGINE_TAICHI_RUNTIME_SOURCE="$out/alternate-runtime" \
  -DENGINE_TAICHI_CUDA=OFF > "$out/seed-configure.log" 2>&1
cp "$build/CMakeCache.txt" "$out/cache-before.txt"
for pair in ROOT:alternate-source BUILD:alternate-build RUNTIME_SOURCE:alternate-runtime; do
  grep -Fx "ENGINE_TAICHI_${pair%%:*}:PATH=$out/${pair#*:}" "$out/cache-before.txt"
done
flock -u 9
exec 9>&-
bash engine/scripts/run_local_ci.sh quick "$out/run"
cp "$build/CMakeCache.txt" "$out/cache-after.txt"
grep -Fx "ENGINE_TAICHI_ROOT:PATH=$repo/engine/taichi" "$out/cache-after.txt"
grep -Fx "ENGINE_TAICHI_BUILD:PATH=$repo/engine/taichi/build-review" "$out/cache-after.txt"
grep -Fx "ENGINE_TAICHI_RUNTIME_SOURCE:PATH=$repo/engine/taichi/taichi/runtime/llvm/runtime_module" "$out/cache-after.txt"
cmp "$build/taichi-runtime/runtime_x64.bc" \
  "$repo/engine/taichi/taichi/runtime/llvm/runtime_module/runtime_x64.bc"
find engine/taichi/build-review -type f -name '*.a' -printf '%p %s %T@\n' |
  sort > "$out/archives-before.txt"
test -s "$out/archives-before.txt"
cmake --build engine/taichi/build-review --target taichi_c_api -j 2 > "$out/incremental.log" 2>&1
# Taichi intentionally regenerates runtime bitcode even on an incremental run.
# Existing compiled archives must retain their paths, sizes and modification times.
find engine/taichi/build-review -type f -name '*.a' -printf '%p %s %T@\n' |
  sort > "$out/archives-after.txt"
cmp "$out/archives-before.txt" "$out/archives-after.txt"
printf 'PASS: alternate cache replaced; matched runtime; incremental backing reused\n' > "$out/result.txt"
