#!/bin/bash
set -uo pipefail
for d in 1 2 3 4 5 6 7 8 9; do
  echo "=== DOC $d start $(date -u +%H:%M:%SZ) ==="
  ~/engine/hcp/engine/kernel/corpus-pour-v0/run-doc.sh $d 200 20 || echo "DOC $d FAILED rc=$?"
done
echo "=== CORPUS RUN DONE $(date -u +%H:%M:%SZ) ==="
