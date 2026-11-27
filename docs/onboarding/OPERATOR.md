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

`tick N` advances the field by up to `N` ticks. Its `ticks` count reports how
many committed in that command, and `completed_tick` gives the study total.
Use `tick N GEOMETRY_WORK` to request a per-tick geometry-work budget within
the study's prepared limits. A capacity refusal reports the native budget,
required amount and capacity. Earlier ticks in the batch remain committed.
The same study can accept a removal and resume ticking from that state.

Use `list` to read the catalogue’s `definition` and `revision` together.
Enter `add 27 DEFINITION REVISION` with that exact pair, then `tick 1`,
`remove 27` and `tick 1`. The definition stays in the catalogue after
removal. An unlisted pin produces `invalid_input` and leaves the study usable.

### Control the timestep and the clock

`dt` is the amount of simulated time advanced by one native tick. The clock period is the target wall-clock time between tick starts. At `dt 0.1` and a period of 250 ms, ten successful ticks advance one simulation-time unit and take about 2.5 seconds at the target rate. A slow tick can reduce the achieved rate. The browser can sample completed positions on its own schedule.

| Driver action | Live CLI |
|---|---|
| Choose the initial physical step | `--dt 0.1`. A fresh study defaults to `1`. |
| Query or change the step on an open study | `dt` reports it. `dt 0.05` applies to subsequent ticks. Both commands leave the completed tick count unchanged. |
| Run continuously | `run 250` requests one tick every 250 ms (4 Hz). `run` resumes at the configured period. |
| Pause and inspect | `pause` acknowledges the last completed tick. `observe` reads that state. |
| Single-step or advance a block while paused | `tick 1` advances one step. `tick 10` advances up to ten and emits one result. |
| Choose startup pacing | An interactive terminal starts paced at 250 ms after study creation. `--paused` starts in step mode. Pipes and command files start paused by default. `--pace-ms 250` selects paced mode for either driver. |

**Human example.** With the retained sources and warm database configured as above, start with two existing A definitions available and open the English A pin printed in `catalogue_available`.

```sh
engine/build/viewport/viewport_live_study --retained --paused --dt 0.1 \
  --letter 02.00.00.00.03 --letter 02.01.00.00.03 --focus-byte 00.00.00.00.23
```

Enter `open DEFINITION REVISION` using the values from the first catalogue row. After `create`, enter `add 2` for French A, then `run 250`. Enter `pause` when you see motion you want to examine. Enter `dt 0.05` and `tick 1` to advance one finer step. Enter `run` to continue, or `save study.json` and `quit` to close the session.

**Agent protocol.** Keep stdin open. Send one newline-terminated command and continuously drain the newline-delimited JSON on stdout. While running, automatic ticks can arrive before a command acknowledgement. Match the `command` field and use `automatic: false` to identify a requested tick. Wait for the `pause` acknowledgement before stepping. Check `status` and the count of committed `ticks` on a batch. A refused automatic tick pauses the clock. Read `completed_tick`, `dt`, `pace_ms` and `running` on state-bearing events. The embedded snapshot records the configured `source.parameters.dt` and the most recently committed `source.last_tick_dt`, which is null before the first receiver tick. Each successful batch advances `ticks × dt` units of simulation time.

A position save records the active timestep. Startup `--load FILE` uses that step unless you explicitly pass `--dt X`. Live `load FILE` restores the saved step when the replacement succeeds. Files from the original position-only format use their historical step of `1`. The local clock period and run/pause mode belong to the driver. Use `--paused` or pause before a live load when you want time to examine receiver tick zero.

### Open one letter from a retained catalogue

On a fresh dedicated fixture, supply the retained addresses the publishing
owner should offer. The English A and í witness uses these addresses.

```sh
engine/build/viewport/viewport_live_study --retained \
  --letter 02.00.00.00.03 --letter 02.00.00.00.1W --response-work-limit 1000000
```

Use the definition and revision from the first row of `catalogue_available`
in `open DEFINITION REVISION`. Only that root starts live. Use the second
row's pin in `add 2 DEFINITION REVISION`, then run `tick 1`, `lod parts`,
`lod aggregate`, `remove 2`, and `tick 1`. `list` still includes the
removed definition. The catalogue and study pins are released when their
owner is finished with them.

With `--letter`, `add ENTRY` uses the native owner's declared or remembered
template for that entry. Retrying a live entry is a no-op. Use `add ENTRY
TEMPLATE_ENTRY` or `add ENTRY DEFINITION REVISION` for a new, undeclared
entry, even in a single-letter catalogue. A selector that conflicts with a
declared or remembered template is refused.

English A and B share nibble `4` through their bytes `41` and `42`. Use
`--letter 02.00.00.00.04` as the second address to replay the pair. In parts
exposure, the response trace shows both scoped contributions under the same
nibble field. Each contributes through its own byte path. Run `tick 1` in
parts exposure to observe the shared field before rolling up.

### Open a retained English/French study

English A and French A share byte `41`. They belong to separate language and
alphabet groups. Select the shared byte explicitly when offering the two roots.

```sh
engine/build/viewport/viewport_live_study --retained \
  --letter 02.00.00.00.03 --letter 02.01.00.00.03 \
  --focus-byte 00.00.00.00.23 --response-work-limit 1000000
```

Open either root by its exact pin and `add` the other using its entry, definition
and revision. Tick, expand with `lod parts`, roll up with `lod aggregate` and
remove either entry. The `focus` and `live_fields` records identify the shared
byte and let you match response packet comparisons to each language’s fields.

The explicit byte focus currently requires every declared letter to consist of
that byte and have a complete membership path. The
[recipe-selected live work](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432)
tracks wider catalogue qualification.

### Save and reload a retained CPU study

Enter `save FILE` after a completed command. The file records the exact public
catalogue and recipe pins, live declared occurrences, exposure, selected focus,
reference frame and completed positions. Restart files cover the retained
catalogue's declared roots and its binary64 reference-local positions.

Quit the producer. Configure a second fresh warm v2 database with the same
read-only retained sources and start the receiver.

```sh
engine/build/viewport/viewport_live_study --retained --load FILE
```

The receiver checks the recorded catalogue against the retained sources, then
uses the saved pins to publish the catalogue in its own warm database. Its
`create` record shows `saved_tick` and a receiving `completed_tick` of zero.
The native owner restores positions and initializes motion under D3. A later
trajectory can differ from the producer’s. To inspect receiver tick zero interactively,
start with `--paused`, then enter `tick 1` to advance one step.

`load FILE` on an open study stages a replacement from the same study and exact
recipe. A stale pin or invalid position leaves the open study available for
further commands. The `observation` from a completed event is the input for
the browser’s **Open snapshot** action.

[Viewer controls](../../tools/viewport/README.md) · [Build the engine](FIRST-RUN.md)
