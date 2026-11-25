# Examine a study

| I want to | Route |
|---|---|
| See an actual study without building the engine | Open the [bundled viewer](../../tools/viewport/index.html) in a WebGL2 browser. Choose **Open snapshot** and load the [tick-zero](../../tools/viewport/examples/letter-study-tick0.json) or [tick-eight](../../tools/viewport/examples/letter-study.json) capture. Each contains 16 occurrences. |
| Follow the data that produced a native observation | Use the [recipe-to-report trace](INTEGRATED-PATH.md) and the [retained CPU capture instructions](../../engine/docs/RETAINED-POSITIVE-CAPTURE.md). That preset requires read-only source connections and a fresh, dedicated warm database. |
| Drive the editable study | Use the [native CLI guide](../../engine/docs/LIVE-EDITABLE-STUDY.md) and the [example commands](../../tools/viewport/examples/live-study.commands). |

From the repository root after building the native viewport target, run:

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct --commands tools/viewport/examples/live-study.commands
```

The fixture adds entry 20, ticks, removes it and ticks again in the same session. The completed records show the surviving occurrences. [Native claim counts](../../engine/docs/EXCHANGE-CLAIM-OBSERVATION.md) show occupied records before, during and after an edit.

[Viewer controls](../../tools/viewport/README.md) · [Build the engine](FIRST-RUN.md)
