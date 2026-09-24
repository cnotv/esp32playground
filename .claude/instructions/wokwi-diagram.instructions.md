---
description: "Use when creating or editing Wokwi diagram.json files under modules/: connection array syntax, breadboard pin grammar, validating an edit, and available tooling."
applyTo: "modules/**"
---

# Wokwi diagram editing

## Diagram files

There is no root sketch, no root `diagram.json`, and no `diagrams/` folder
anymore, both were removed along with `sketch_sep13a.ino` when the project
split into standalone modules. Every diagram now lives inside its own
`modules/<name>/` folder, next to that module's `wokwi.toml`.

- `modules/servo/`, `modules/ir/`, and `modules/servo_web/` are standalone
  sketches (each with its own `.ino` defining `setup()`/`loop()`). Each
  `wokwi.toml` points at its own `modules/<name>/build/`, and each is
  compiled/flashed with its own `<name>-*` Makefile targets.
- `modules/breadboard/` and `modules/epaper/` are diagram-only folders with no
  `.ino` of their own. They predate the split into modules, no single sketch
  matches their wiring anymore (breadboard shows a servo and the now-dead IR
  receiver together, epaper's part is a stand-in Wokwi can't really simulate),
  so their `wokwi.toml` points at `modules/servo/`'s build purely so the
  simulator has something valid to load, not because that firmware matches
  the diagram. See each folder's own `diagram.md` for what's actually true.

### Companion ASCII diagram

Whenever a diagram is created or its wiring changes, generate or update a
matching `diagram.md` next to it (root pairs with `diagram.md` at the root; a
variant folder gets its own `diagram.md` inside it). It's an ASCII box diagram
of the ESP32 pinout, breadboard rails, and each connected part, each arrow
reflecting the real `connections` array, not a generic example. See
`diagram.md` at the root for the current format to follow.

`modules/breadboard/` (currently mirrored into the root `diagram.json`) wires
the ESP32 to a micro servo and an IR receiver, both seated on their own
### Companion ASCII diagram

Whenever a diagram is created or its wiring changes, generate or update a
matching `diagram.md` next to it, inside the same `modules/<name>/` folder.
It's an ASCII box diagram of the ESP32 pinout and each connected part, each
arrow reflecting the real `connections` array, not a generic example. See any
existing `modules/*/diagram.md` for the current format to follow.

`modules/breadboard/` documents wiring for a micro servo and an IR receiver,
both seated on their own breadboard so their power pins feed from ESP32 rails
instead of floating with bare wire ends, but see the caveat above about its
`wokwi.toml` being stale. `modules/epaper/` documents real wiring for a 4.2in
SPI e-paper module using a `wokwi-ili9341` stand-in (Wokwi has no e-paper
part), see its own `diagram.md` for the caveats.

### When the user says "add it in /diagram"

This means: create a new named folder under `modules/<name>/` with its own
`diagram.json` and `wokwi.toml`, rather than overwriting an existing module's
diagram in place. Ask for the folder name if it is not given. If the new
diagram is meant to simulate an existing standalone sketch, point its
`wokwi.toml` at that sketch's own `modules/<name>/build/`, don't invent a new
build path.

## Wokwi diagram.json conventions

Connections are **arrays**, not objects:

```json
[ "esp32:D13", "lcd1:RS", "gray", [] ]
```

Element 0 and 1 are `partId:pinName`, element 2 is the wire colour, element 3 is
the optional routing hint list.

Never use `brown` or `pink` for a wire colour. Pick a clearly distinct
alternative instead (`red`, `black`, `orange`, `yellow`, `green`, `blue`,
`teal`, `white`, `gray`, `purple`, `cyan`, `magenta`).

### Breadboard pin grammar

Verified against `packages/diagram-lint/src/rules/invalid-pin.ts` in
`wokwi/wokwi-cli`:

- Row tie point: `{column}{t|b}.{letter}` where column is 1 to 63 and letter is
  `a` to `j`. `t` is the half above the gully (rows a to e), `b` is below (rows f
  to j). Example: `bb1:5t.a`, `bb1:12b.j`.
- Power rail: `{t|b}{p|n}.{number}` where number is 1 to 50. `p` is positive, `n`
  is negative. Example: `bb1:tp.1`, `bb1:bn.45`.

Anything else is rejected by the linter. The breadboard part contributes no pins
to the registry (`registry/parts/wokwi-breadboard.json` has `"pins": []`), so
these two patterns are the only validation that exists.

Keep rail taps within positions 1 to 25 unless you have confirmed the rail is a
single continuous net across its full length.

### Declaring a breadboard does not wire it

A `wokwi-breadboard` part with no connections referencing its id is decorative.
The render will show wires flying point to point over an untouched board. If the
intent is breadboard routing, write explicit connections into `bb1` rows and
rails.

### Prefer explicit wires over overlap auto-connect

Wokwi auto-connects a part's pins to tie points when the part physically
overlaps the breadboard, which is why `noBreadboard` exists on `ElementPin`.
Hand-placing `top`/`left` to land pins in the right holes is fragile and a near
miss silently creates duplicate or missing nets. Place parts beside the board
and wire them explicitly.

### Part paint order matters when a part sits over the breadboard

Parts render in the order they appear in the `parts` array; later entries paint
on top of earlier ones. If a part is deliberately positioned so it visually sits
on the breadboard (e.g. a resistor placed over `bb1`), declare `bb1` **before**
that part, otherwise the breadboard paints over it and it disappears from the
render even though the connections are correct. Verified live: rendering a test
diagram in the Wokwi web editor (via a Playwright browser session pasting the
diagram JSON into a scratch project) showed a resistor invisible until `bb1`
was moved earlier in the `parts` array.

### Part types in use

`wokwi-esp32-devkit-v1`, `wokwi-lcd1602` (with `attrs.pins: "16"`),
`wokwi-potentiometer`, `wokwi-breadboard` (830 point full board; the `-half` and
`-mini` variants also exist), `wokwi-servo` (pins `GND`, `V+`, `PWM`).

ESP32 ground pins are `GND.1`, `GND.2`, `GND.3`. `VIN` is the 5V rail when the
board is USB powered, and is what the 5V LCD should run from.

## Validating a diagram edit

Always validate after editing, and make the validation prove it ran.

A validator that assumes object shaped connections will silently inspect nothing
and report success. This happened in a previous session. Any validation script
must print the count of endpoints it actually checked, and that count must be
greater than zero before the result is trusted.

Check: JSON parses, every `partId` resolves to a declared part, every breadboard
pin matches one of the two regexes above, and the root and variant files agree.

A JSON-level validator cannot see the render. Before calling a diagram edit
done, also verify it in the Wokwi simulator itself: start the simulation from
`diagram.json` in the Wokwi VS Code extension and inspect the rendered view
(or a screenshot of it) for parts landing off-canvas, wires crossing components,
or a red/highlighted pin indicating an unresolved connection. There is no tool
in this environment that can capture that webview directly, so ask the user to
run the simulation and share a screenshot (view it with the image tool) or
describe what they see, and do not report the task complete until that visual
check has happened.

## Tooling available

- Wokwi VS Code extension is installed. Its diagram schema lives at
  `~/.vscode/extensions/wokwi.wokwi-vscode-*/schemas/diagram.schema.json`. The
  schema does **not** contain part pin names.
- PlatformIO IDE is installed but unused by this project.
- `wokwi-cli` is **not** installed, and there is no npm package by that name, so
  `wokwi-cli lint` is unavailable. Validate with a script instead.
- No Arduino, ESP32 or electronics MCP server is configured.

## Verifying part pin names

The breadboard element is not in the open source `wokwi/wokwi-elements` repo and
`docs.wokwi.com/parts/wokwi-breadboard` returns 404, so pin names cannot be
looked up there. Component pin names for other parts are in
`wokwi/wokwi-elements` under `src/<part>-element.ts` as `pinInfo`.

Do not guess pin names or rely on recall. Verify against source, then record the
finding here.
