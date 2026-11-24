# Examine a study

| I want to | Route |
|---|---|
| See an actual study without building the engine | Open the [bundled viewer](../../tools/viewport/index.html) in a WebGL2 browser. Choose **Open snapshot** and load the [tick-zero](../../tools/viewport/examples/letter-study-tick0.json) or [tick-eight](../../tools/viewport/examples/letter-study.json) capture. Each contains 16 occurrences. |
| Follow the data that produced a native observation | Use the [recipe-to-report trace](INTEGRATED-PATH.md) and the [retained CPU capture instructions](../../engine/docs/RETAINED-POSITIVE-CAPTURE.md). That preset requires read-only source connections and a fresh, dedicated warm database. |
| Drive the editable study | Follow the [operator CLI implementation handoff](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343#issuecomment-7137040893). At the #422 baseline the viewer reads completed snapshots; operator commands for live edits are planned. |

In the planned live route, a human or agent drives one session. Follow a surviving occurrence across an addition, a tick, a removal and the next tick. Changing visible LoD exchanges active particles across the whole field. Watching recorded snapshots or zooming the camera only changes what you see. Publish runnable CLI examples here when the command path lands.

[Viewer controls](../../tools/viewport/README.md) · [Build the engine](FIRST-RUN.md)
