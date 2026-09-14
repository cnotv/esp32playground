# Agent instructions

ESP32 DevKit v1 driving a 16 pin HD44780 LCD1602 in 4 bit mode, built with
arduino-cli and simulated in Wokwi.

## Build and flash

Use the Makefile. Do not hand roll `arduino-cli` invocations.

```
make compile   # arduino-cli compile, output to build/esp32.esp32.esp32
make upload    # arduino-cli upload
make flash     # compile then upload
```

FQBN is `esp32:esp32:esp32`. Port is `/dev/cu.usbserial-0001`, overridable with
`make PORT=...`.

Wokwi reads the compiled artefacts, so `make compile` must run before the
simulation reflects a code change. Paths are declared in `wokwi.toml`.

## Pin map

The sketch is the single source of truth for wiring. Read it before changing any
diagram:

```
sketch_sep13a.ino:4   rs=13, en=12, d4=14, d5=27, d6=26, d7=25
```

If a diagram and the sketch disagree, the sketch wins. Never invent a pin
assignment to make a diagram look tidy.

## Diagram files

- `diagram.json` at the root is the file Wokwi actually simulates.
- `diagrams/breadboard/` and `diagrams/direct/` are variants, each with its own
  `wokwi.toml` pointing at `../../build/`.
- The root file mirrors the selected variant. When you edit a variant, mirror it
  into the root, and verify the two `connections` arrays are byte identical.

## Wokwi diagram.json conventions

Connections are **arrays**, not objects:

```json
[ "esp32:D13", "lcd1:RS", "gray", [] ]
```

Element 0 and 1 are `partId:pinName`, element 2 is the wire colour, element 3 is
the optional routing hint list.

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

### Part types in use

`wokwi-esp32-devkit-v1`, `wokwi-lcd1602` (with `attrs.pins: "16"`),
`wokwi-potentiometer`, `wokwi-breadboard` (830 point full board; the `-half` and
`-mini` variants also exist).

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
