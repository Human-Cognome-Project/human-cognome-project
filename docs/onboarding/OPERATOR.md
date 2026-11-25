# Examine a study

| I want to | Route |
|---|---|
| See an actual study without building the engine | Open the [bundled viewer](../../tools/viewport/index.html) in a WebGL2 browser. Choose **Open snapshot** and load the [tick-zero](../../tools/viewport/examples/letter-study-tick0.json) or [tick-eight](../../tools/viewport/examples/letter-study.json) capture. Each contains 16 occurrences. |
| Follow the data that produced a native observation | Use the [recipe-to-report trace](INTEGRATED-PATH.md) and the [retained CPU capture instructions](../../engine/docs/RETAINED-POSITIVE-CAPTURE.md). That preset requires read-only source connections and a fresh, dedicated warm database. |
| Drive the editable study | Use the [native CLI guide](../../engine/docs/LIVE-EDITABLE-STUDY.md) and the [example commands](../../tools/viewport/examples/live-study.commands). |
| Change the live `í` study's level of detail | Select `--unicode-i` in the native CLI and use the commands below. |

From the repository root after building the native viewport target, run:

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct --commands tools/viewport/examples/live-study.commands
```

The fixture adds entry 20, ticks, removes it and ticks again in the same session. The completed records show the surviving occurrences. [Native claim counts](../../engine/docs/EXCHANGE-CLAIM-OBSERVATION.md) show occupied records before, during and after an edit.

## Change the live `í` study's level of detail

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct --unicode-i --response-work-limit 1000000
```

Enter `observe`, `lod parts`, `tick 3`, `observe`, `lod aggregate`, `tick 2`, `observe` and `quit`, one command per line. Choose your own tick counts to advance the same running study. The completed JSON records show `í` at mass 4 and the ordered bytes `C3` and `AD` at mass 2 each. `add 20` and `remove 2` report `unsupported` for the fixed cohort. The session accepts further commands after each refusal and exits with status 1 if a command was refused. [Live exchange evidence](../../engine/docs/LIVE-UNICODE-EXCHANGE.md)

Pass `--commands tools/viewport/examples/live-unicode.commands` to replay the [example](../../tools/viewport/examples/live-unicode.commands).

[Viewer controls](../../tools/viewport/README.md) · [Build the engine](FIRST-RUN.md)
