# Agent instructions

ESP32 DevKit v1 projects, built with arduino-cli and simulated in Wokwi. There
is no single root sketch, each `modules/<name>/` folder is its own standalone
sketch with its own `setup()`/`loop()`.

## Build and flash

See `.claude/instructions/iot-workflow.instructions.md` for the Makefile
targets, FQBN, port, and how Wokwi picks up build output.

## Modules

- `modules/servo/` - Elegoo example, sweeps a micro servo on `D33`.
- `modules/ir/` - reads an IR remote on `D4`/`3V3`, logs codes to Serial. The
  IR receiver module is currently dead (killed by an earlier 5V miswiring),
  see `/memories/repo/hardware-debugging.md` before assuming the wiring or
  code is at fault again.
- `modules/servo_web/` - controls the same servo over WiFi instead of IR
  (`WiFiManager` captive portal, no hardcoded credentials), the working
  replacement for the dead IR receiver.
- `modules/buzzer_web/` - passive buzzer played as a mini piano (`C4`-`C5`)
  from a web page, same `WiFiManager` captive portal pattern as `servo_web`.
- `modules/scare_prop/` - autonomous jump-scare prop, no WiFi: an HC-SR04
  distance sensor triggers a fan relay, LED, buzzer siren, and servo jerk when
  something gets close. Read its `diagram.md` before wiring the fan, a bare
  motor or bare relay coil on a GPIO will damage the pin, it needs an actual
  relay MODULE (onboard driver), confirm that's what you have first.
- `modules/epaper/` - drives a 4.2in SPI e-paper module via GxEPD2, currently
  just draws a placeholder, no calendar/notes/image source wired up yet.
  Wokwi has no e-paper part, its diagram is wiring documentation only, not a
  working simulation, see its own `diagram.md`.
- `modules/breadboard/` - diagram-only wiring reference, no `.ino`, see
  `.claude/instructions/wokwi-diagram.instructions.md` for the caveat about
  its stale `wokwi.toml`.

Each module's own `.ino` is the source of truth for its wiring. Read it before
changing that module's diagram. If a diagram and its sketch disagree, the
sketch wins. Never invent a pin assignment to make a diagram look tidy.

## Adding new rules to this file

Before appending a new rule here, check whether it describes a distinct area or
workflow that doesn't already have its own file. If it does, give it its own
file instead of growing this one:

- `.claude/instructions/*.instructions.md` (with `applyTo`) for conventions
  scoped to a set of files, discovered automatically when those files are
  touched.
- `.claude/skills/<name>/SKILL.md` for an on-demand, multi-step workflow that
  bundles its own scripts or templates.
- `.claude/agents/*.md` for a persona that needs its own tool restrictions or
  context isolation, not just different advice.

Keep this file for what's always relevant regardless of which file is being
touched. Wokwi diagram conventions, for example, live in
`.claude/instructions/wokwi-diagram.instructions.md` instead of here, since they
only matter when a `modules/**/diagram.json` is being edited.

## Diagram files

See `.claude/instructions/wokwi-diagram.instructions.md` for diagram file
locations and variants, `diagram.json` connection syntax, breadboard pin
grammar, validating an edit, available tooling, and how to verify part pin
names.

