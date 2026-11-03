#!/usr/bin/env bash
# Same-run transport only. This is not a cross-run build-directory cache.
set -Eeuo pipefail
action=${1:?pack|restore ARCHIVE [EXPECTED_SHA256]}
archive=$(realpath -m "${2:?Archive path required}")
case "$action" in
  pack)
    test -x engine/build/engine_devices
    mkdir -p engine/build/ci-provenance
    git rev-parse HEAD > engine/build/ci-provenance/commit.txt
    pwd -P > engine/build/ci-provenance/workspace.txt
    git rev-parse HEAD:engine/taichi > engine/build/ci-provenance/taichi-tree.txt
    git submodule status --recursive > engine/build/ci-provenance/submodules.txt
    cmake -LA -N engine/build > engine/build/ci-provenance/engine-cmake.txt
    cmake -LA -N engine/taichi/build-review > engine/build/ci-provenance/taichi-cmake.txt
    clang-15 --version > engine/build/ci-provenance/toolchain.txt
    ctest --test-dir engine/build --show-only=json-v1 > engine/build/ci-provenance/tests.json
    jq -e '.tests | length > 0' engine/build/ci-provenance/tests.json >/dev/null
    # Retain executables, runtime, generated test definitions and configuration.
    # No object files/archives, JIT results or test results are transported.
    tar --exclude=CMakeFiles --exclude=Testing --exclude='*.a' --exclude='*.o' \
      --exclude='.ninja*' -czf "$archive" engine/build
    sha256sum "$archive"
    ;;
  restore)
    expected=${3:?Producer archive digest required}
    [[ "$expected" =~ ^[0-9a-f]{64}$ ]]
    printf '%s  %s\n' "$expected" "$archive" | sha256sum --check
    test ! -e engine/build
    tar -xzf "$archive"
    test "$(git rev-parse HEAD)" = "$(cat engine/build/ci-provenance/commit.txt)"
    test "$(pwd -P)" = "$(cat engine/build/ci-provenance/workspace.txt)"
    test "$(git rev-parse HEAD:engine/taichi)" = "$(cat engine/build/ci-provenance/taichi-tree.txt)"
    # shellcheck source=/dev/null
    source engine/build/engine-env.sh
    test "$TI_LIB_DIR" = "$PWD/engine/build/taichi-runtime"
    engine/build/engine_devices --cpu-only
    ;;
  *) echo "Unknown bundle action: $action" >&2; exit 2 ;;
esac
