#!/bin/bash
# corpus pour driver — one doc through the one door (silas kernel lane)
set -euo pipefail
export PYTHONPATH=$HOME/engine/hcp/engine/timestep
DOC=$1; TICKS=${2:-200}; STATE_EVERY=${3:-20}
cd ~/engine/hcp/engine/kernel
RUN=corpus-pour-v0
~/engine/venv/bin/python - "$DOC" << 'PYEOF'
import json, sys
d = json.load(open("../ingest/dbdocs-ingest-manifests.json"))
doc = int(sys.argv[1])
m = [x for x in d if x["source"].endswith(f"id={doc}")][0]
json.dump(m, open(f"corpus-pour-v0/manifest-doc{doc}.json","w"), indent=1)
print("manifest split:", m["doc"]["name"])
PYEOF
NAME=$(~/engine/venv/bin/python -c "import json;print(json.load(open(\"$RUN/manifest-doc$DOC.json\"))[\"doc\"][\"name\"].split(\"_\")[0])")
POUR_ARCH=cuda POUR_FP=f32 ~/engine/venv/bin/python pour_raw.py \
  --manifest $RUN/manifest-doc$DOC.json --verify-record \
  --ticks $TICKS --sample-every 20 --state-every $STATE_EVERY \
  --out-prefix $RUN/pour-doc$DOC-$NAME \
  --state-dir $RUN/states-doc$DOC
