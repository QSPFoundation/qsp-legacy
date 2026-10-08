# Tests of qsp-legacy

The tests are scenario files (`engine/*.qspt`) executed by a small runner
(`runner/qsp_test_runner.c`) against the public API of the library
(the default binding). Most of them are ported from
[qsp-wasm-engine](https://github.com/QSPFoundation/qsp-wasm-engine/tree/main/tests/engine)
and adapted to the semantics of the 5.7 engine; every adaptation is
commented in place with the original expectation (`wasm: ...`).

## Building and running

```sh
cmake -B build -DQSP_LEGACY_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Every `.qspt` file is registered as a separate CTest test (`engine.<name>`).
The runner can also be called directly: `build/tests/qsp-legacy-tests FILE.qspt...`.
It prints the failed checks with the events recorded so far, and the
summary `N passed, M failed, K skipped`; the exit code is non-zero if
anything failed.

The tests require the default binding (`BUILD_JVM=OFF`).

## Format of `.qspt` files

A file is a sequence of tests. Lines starting with `//` and empty lines
between commands are ignored. Every command starts with `@` at the start
of the line. Commands marked as *block* take the following lines up to the
next `@`-line as their text; `@@` at the start of a block line stands for a
literal `@`, trailing empty lines and `//` lines are not part of the block.

The engine is reinitialized (`QSPTerminate`/`QSPInit`) for every test;
the game is built from the definitions and loaded right before the first
action of the test.

### Test and game definition

| Command | Meaning |
|---|---|
| `@test NAME` | starts a new test |
| `@src` *(block)* | code of location `test`; the game also gets an empty first location `start` (the same wrapper as `runTestFile()` in qsp-wasm-engine) |
| `@game` *(block)* | locations in the qsps format: `# name`, code lines, a line starting with `--` |
| `@module NAME` *(block)* | a separate game in the qsps format, loaded when the game calls `ADDQST 'NAME'` / `OPENGAME 'NAME'` |
| `@desc LOC` *(block)* | base description of location `LOC` |
| `@action LOC \| NAME [\| IMAGE]` *(block)* | base action of location `LOC`, the block is its code |

### Replies of the "user"

| Command | Meaning |
|---|---|
| `@reply input TEXT` | queues a reply to `INPUT` (escapes `\n`, `\r`, `\\`) |
| `@reply menu N` | queues the item index chosen in `MENU` (`-1` closes the menu) |
| `@reply isplay 0/1` | queues a reply to the "is playing" callback |
| `@reply mscount N` | value returned by `MSECSCOUNT` |

### Actions

| Command | API call |
|---|---|
| `@run [LOC]` | `QSPExecLocationCode` (default location `test`) |
| `@exec CODE` | `QSPExecString` |
| `@counter` | `QSPExecCounter` |
| `@set-input TEXT` | `QSPSetInputStrText` |
| `@user-input` | `QSPExecUserInput` |
| `@select-act N` / `@exec-act` | `QSPSetSelActionIndex` / `QSPExecuteSelActionCode` |
| `@select-obj N` | `QSPSetSelObjectIndex` |
| `@restart` | `QSPRestartGame` |
| `@save NAME` / `@load-save NAME` | `QSPSaveGameAsData` / `QSPOpenSavedGameFromData` into/from an in-memory slot |
| `@debug 0/1` | `QSPEnableDebugMode` |
| `@clear-events` | forgets the recorded events |

An error of the engine is recorded as the event `error NAME` and must be
consumed with `@expect error`; an unconsumed error fails the test at the
next action or at the end of the test.

### Checks

| Command | Checks |
|---|---|
| `@expect EXPR => VALUE` | value of a QSP expression (`QSPGetExprValue`); `VALUE` is a number or a string in single quotes |
| `@expect main TEXT` / `@expect stat TEXT` | main / additional description (escaped) |
| `@expect acts [a; b\|img]` / `@expect objs [...]` | current actions / objects |
| `@expect selact N` / `@expect selobj N` | selected action / object |
| `@expect curloc NAME` | current location |
| `@expect error NAME [(location 'L', line N, action A)]` | the pending error, by name or with its position |
| `@expect event TEXT` / `@expect no-event PREFIX` | an event was / no event starting with `PREFIX` was recorded |
| `@expect count N PREFIX` | number of events starting with `PREFIX` |
| `@expect events` *(block)* | the exact list of recorded events |
| `@skip` | the test is skipped (the feature doesn't exist in the 5.7 engine); a comment says why |

### Events

Callbacks are recorded with the names used by qsp-wasm-engine:
`main_changed [TEXT]`, `stats_changed [TEXT]`, `actions_changed [...]`,
`objects_changed [...]`, `msg TEXT`, `view [FILE]`, `system_cmd TEXT`,
`user_input [TEXT]`, `input TEXT`, `menu [...]`, `wait N`, `timer N`,
`play_file FILE VOLUME`, `is_play FILE`, `close_file [FILE]`,
`panel_visibility TYPE 0/1`, `open_game FILE IS_ADD`, `save_game [FILE]`,
`load_save [FILE]`, `debug LOC:LINE:ACT CODE`, `error NAME`.
An argument in brackets is omitted when the engine passes `NULL`
(for the descriptions: when they are empty).