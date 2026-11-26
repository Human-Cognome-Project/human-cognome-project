# Examine a study

| I want to | Route |
|---|---|
| See an actual study without building the engine | Open the [bundled viewer](../../tools/viewport/index.html) in a WebGL2 browser. Choose **Open snapshot** and load the [tick-zero](../../tools/viewport/examples/letter-study-tick0.json) or [tick-eight](../../tools/viewport/examples/letter-study.json) capture. Each contains 16 occurrences. |
| Follow the data that produced a native observation | Use the [recipe-to-report trace](INTEGRATED-PATH.md) and the [retained CPU capture instructions](../../engine/docs/RETAINED-POSITIVE-CAPTURE.md). That preset requires read-only source connections and a fresh, dedicated warm database. |
| Drive the editable study | Use the [native CLI guide](../../engine/docs/LIVE-EDITABLE-STUDY.md) and the [example commands](../../tools/viewport/examples/live-study.commands). |
| Change the live `í` study's level of detail | Select `--unicode-i` in the native CLI and use the commands below. |
| Open the retained qualification study | Select `--retained` and open the exact recipe offered by the publishing owner. |

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

To see what an exchange can change between ticks, compress and re-expand immediately after `tick 2`. Compare the completed tick, body positions and focal field across the three records. [Worked round trip and replay](../../engine/docs/LIVE-UNICODE-EXCHANGE.md#observe-a-round-trip-without-advancing-the-tick)

## Open the retained qualification study

Build with warm integration enabled. Set `HCP_CORE_TEST_CONNINFO` and
`HCP_LING_TEST_CONNINFO` to read-only retained sources. Set
`HCP_WARM_TEST_CONNINFO` to a fresh, dedicated warm fixture database with
schema v2. [Retained fixture setup](../../engine/docs/RETAINED-POSITIVE-CAPTURE.md)

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --retained --threads 2 --direct
```

The publishing owner prepares the retained English pair and prints a
`recipe_available` record. Enter `open SELECTION REVISION` with both values
from that record. After `create`, enter `observe`, `tick 1`, and `quit`. The
completed records identify the same study and the selected recipe. A wrong
pin is refused before acquisition. The `recipe_release` record confirms that
the owner released the lease after the command loop finished.

The first SIGINT or SIGTERM wakes an idle command wait or stops a batch at a
completed tick. A disconnected output reader also stops a batch between ticks.
If the output pipe is full during shutdown, the owner closes the session and
releases the lease even when the final record cannot be delivered. A malformed
command produces `invalid_input` and the next command can use the same study.

This mode qualifies startup on a recipe published by the same process.
Pinned catalogue selection across sessions and mixed-object edits remain in
the [recipe-selected live work](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432).

[Viewer controls](../../tools/viewport/README.md) · [Build the engine](FIRST-RUN.md)
