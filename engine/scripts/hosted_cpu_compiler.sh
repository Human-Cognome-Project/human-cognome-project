#!/usr/bin/env bash
# Preserve all 2 workers x 2 source paths x 2 compiler shapes on separate runners.
set -Eeuo pipefail
[[ "${HCP_WORKERS:?}" =~ ^[12]$ ]] || exit 2
[[ "${HCP_SOURCE_PATH:?}" =~ ^(bulk|direct)$ ]] || exit 2
: "${RUNNER_TEMP:?}"
# shellcheck disable=SC1091
. ./engine/build/engine-env.sh
mkdir -p "$RUNNER_TEMP/c3-compiler-ir"
/usr/lib/llvm-15/bin/clang++ --version > "$RUNNER_TEMP/c3-compiler-ir/toolchain.txt"
/usr/lib/llvm-15/bin/llvm-config --version >> "$RUNNER_TEMP/c3-compiler-ir/toolchain.txt"
/usr/lib/llvm-15/bin/llc --version >> "$RUNNER_TEMP/c3-compiler-ir/toolchain.txt"
git rev-parse HEAD HEAD:engine/taichi > "$RUNNER_TEMP/c3-compiler-ir/source.txt"
sha256sum engine/build/live_hierarchy_test \
  engine/build/taichi-runtime/runtime_x64.bc > "$RUNNER_TEMP/c3-compiler-ir/binaries.sha256"
cmake -LA -N engine/build > "$RUNNER_TEMP/c3-compiler-ir/cmake.txt"
workers=$HCP_WORKERS
source_path=$HCP_SOURCE_PATH
for compiler_shape in default no-unroll; do
  hcp_ir_dir="$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-${compiler_shape}"
  mkdir -p "$hcp_ir_dir"
  hcp_ir_args=()
  if [ "$source_path" = direct ]; then hcp_ir_args+=(--direct); fi
  if [ "$compiler_shape" = no-unroll ]; then hcp_ir_args+=(--compiler-no-unroll); fi
  HCP_C3_IR_DIR="$hcp_ir_dir" engine/build/live_hierarchy_test \
    --threads "$workers" --case ir "${hcp_ir_args[@]}" > "$hcp_ir_dir/collection.txt"
  engine/build/live_hierarchy_test --threads "$workers" \
    --audit-ir "$hcp_ir_dir" "${hcp_ir_args[@]}" > "$hcp_ir_dir/audit.txt"
  mkdir -p "$hcp_ir_dir/selection"
  HCP_B1_IR_DIR="$hcp_ir_dir/selection" engine/build/live_hierarchy_test \
    --threads "$workers" --case selection_ir "${hcp_ir_args[@]}" \
    > "$hcp_ir_dir/selection/collection.txt"
  mkdir -p "$hcp_ir_dir/contact-ticks"
  HCP_B1_TICK_IR_DIR="$hcp_ir_dir/contact-ticks" engine/build/live_hierarchy_test \
    --threads "$workers" --case contact_tick_ir "${hcp_ir_args[@]}" \
    > "$hcp_ir_dir/contact-ticks/collection.txt"
  mkdir -p "$hcp_ir_dir/placement"
  HCP_B2_IR_DIR="$hcp_ir_dir/placement" engine/build/live_hierarchy_test \
    --threads "$workers" --case placement_compiler "${hcp_ir_args[@]}" \
    > "$hcp_ir_dir/placement/collection.txt"
done

# The collection phase created the no-unroll evidence directory.
# shellcheck disable=SC1091
. ./engine/build/engine-env.sh
workers=$HCP_WORKERS
source_path=$HCP_SOURCE_PATH
hcp_ir_args=()
if [ "$source_path" = direct ]; then hcp_ir_args+=(--direct); fi
engine/build/live_hierarchy_test --threads "$workers" \
  --compiler-no-unroll "${hcp_ir_args[@]}" \
  > "$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-no-unroll/native.txt"
engine/build/live_hierarchy_test --threads "$workers" \
  --compiler-no-unroll --case refresh "${hcp_ir_args[@]}" \
  > "$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-no-unroll/refresh.txt"
engine/build/live_hierarchy_test --threads "$workers" \
  --compiler-no-unroll --case selection "${hcp_ir_args[@]}" \
  > "$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-no-unroll/selection.txt"
engine/build/live_hierarchy_test --threads "$workers" \
  --compiler-no-unroll --case contact_tick "${hcp_ir_args[@]}" \
  > "$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-no-unroll/contact-ticks.txt"
engine/build/live_hierarchy_test --threads "$workers" \
  --compiler-no-unroll --case placement "${hcp_ir_args[@]}" \
  > "$RUNNER_TEMP/c3-compiler-ir/${workers}-${source_path}-no-unroll/placement.txt"
if [ "$HCP_SOURCE_PATH" = bulk ]; then
  engine/build/placement_math_test --threads "$workers" --compiler-no-unroll \
    > "$RUNNER_TEMP/c3-compiler-ir/placement-math-${workers}-no-unroll.txt"
fi
