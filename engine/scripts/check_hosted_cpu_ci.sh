#!/usr/bin/env bash
# Small orchestration checks; no Taichi build, physics or PostgreSQL execution.
set -Eeuo pipefail
scripts=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
cd "$scratch"
mkdir -p fixture/engine/taichi fixture/bin fixture/source
cd fixture
git init -q
git config user.name 'CI contract fixture'
git config user.email 'ci-fixture@example.invalid'
echo fixture > engine/taichi/source.txt
git add engine/taichi/source.txt
git commit -qm fixture
# A nine-test CTest inventory exercises uneven shard sizes and escaped names.
cat > source/CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(ci_contract NONE)
enable_testing()
foreach(i RANGE 1 9)
  add_test(NAME "fixture.${i}" COMMAND "${CMAKE_COMMAND}" -E true)
endforeach()
CMAKE
cmake -S source -B engine/build >/dev/null
mkdir -p engine/build/ci-provenance engine/build/taichi-runtime
ctest --test-dir engine/build --show-only=json-v1 > engine/build/ci-provenance/tests.json
for shard in 1 2 3 4; do
  bash "$scripts/hosted_cpu_regression.sh" "$shard" "$scratch/shard-$shard" > "$scratch/shard-$shard.log"
done
cat "$scratch"/shard-*/selected.txt | sort > "$scratch/actual.txt"
jq -r '.tests[].name' engine/build/ci-provenance/tests.json | sort > "$scratch/expected.txt"
cmp "$scratch/expected.txt" "$scratch/actual.txt"
expect_failure() {
  if "$@" > "$scratch/rejection.log" 2>&1; then
    echo "Expected rejection: $*" >&2; exit 1
  fi
}
expect_failure bash "$scripts/hosted_cpu_regression.sh" 0 "$scratch/invalid"
# A failed test must propagate through the real runner.
sed -i 's/-E true/-E false/' source/CMakeLists.txt
cmake -S source -B engine/build >/dev/null
expect_failure bash "$scripts/hosted_cpu_regression.sh" 1 "$scratch/failing"
sed -i 's/-E false/-E true/' source/CMakeLists.txt
cmake -S source -B engine/build >/dev/null
# Fail closed if transported test registration no longer matches the build.
cp engine/build/ci-provenance/tests.json "$scratch/inventory.json"
printf '{"tests":[]}\n' > engine/build/ci-provenance/tests.json
expect_failure bash "$scripts/hosted_cpu_regression.sh" 1 "$scratch/missing"
cp "$scratch/inventory.json" engine/build/ci-provenance/tests.json
# Stub only the toolchain version reporter; all tar/checksum/git/CTest calls are real.
printf '#!/usr/bin/env bash\necho fixture-clang\n' > bin/clang-15
chmod +x bin/clang-15
export PATH="$PWD/bin:$PATH"
mkdir -p engine/taichi/build-review
cmake -S source -B engine/taichi/build-review >/dev/null
# shellcheck disable=SC2016
printf '#!/usr/bin/env bash\n[[ "$1" == --cpu-only ]]\n' > engine/build/engine_devices
chmod +x engine/build/engine_devices
printf 'export TI_LIB_DIR="%s/engine/build/taichi-runtime"\n' "$PWD" > engine/build/engine-env.sh
echo runtime > engine/build/taichi-runtime/runtime_x64.bc
touch engine/build/excluded.a engine/build/excluded.o
bash "$scripts/hosted_cpu_bundle.sh" pack "$scratch/build.tar.gz" > "$scratch/pack.log"
digest=$(sha256sum "$scratch/build.tar.gz" | cut -d ' ' -f 1)
tar -tzf "$scratch/build.tar.gz" > "$scratch/members.txt"
if grep -Eq 'excluded\.(a|o)$|/CMakeFiles/|/Testing/' "$scratch/members.txt"; then
  echo 'Build-only or test-result files leaked into transport' >&2; exit 1
fi
mv engine/build "$scratch/original-build"
bash "$scripts/hosted_cpu_bundle.sh" restore "$scratch/build.tar.gz" "$digest" > "$scratch/restore.log"
test -x engine/build/engine_devices
cmp engine/build/taichi-runtime/runtime_x64.bc "$scratch/original-build/taichi-runtime/runtime_x64.bc"
rm -rf engine/build
expect_failure bash "$scripts/hosted_cpu_bundle.sh" restore "$scratch/build.tar.gz" "$(printf '%064d' 0)"
test ! -e engine/build
git commit --allow-empty -qm changed-source
expect_failure bash "$scripts/hosted_cpu_bundle.sh" restore "$scratch/build.tar.gz" "$digest"
rm -rf engine/build
git checkout -q HEAD^
# Identical commit at a different path must reject CTest's absolute paths.
cd ..
mv fixture relocated
cd relocated
expect_failure bash "$scripts/hosted_cpu_bundle.sh" restore "$scratch/build.tar.gz" "$digest"
printf 'PASS: complete disjoint shards, failure propagation, inventory and bundle provenance guards\n'
