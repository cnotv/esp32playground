---
description: "Use when compiling, uploading, or flashing this ESP32 sketch: Makefile targets, FQBN, port, and how Wokwi picks up build output."
applyTo: "Makefile, *.ino, wokwi.toml"
---

# IoT build and flash workflow

Use the Makefile. Do not hand roll `arduino-cli` invocations.

There is no root sketch, `modules/servo/`, `modules/ir/`, `modules/servo_web/`,
`modules/buzzer_web/`, `modules/scare_prop/`, and `modules/epaper/` are each
separate, standalone sketches (own `setup()`/`loop()`). The Makefile generates
`<name>-compile`/`<name>-upload`/`<name>-flash` targets from the `MODULES`
list, one line per module:

```
make servo-compile
make servo-upload
make servo-flash    # compile then upload

make ir-compile
make ir-upload
make ir-flash       # compile then upload

make servoweb-compile
make servoweb-upload
make servoweb-flash # compile then upload

make buzzerweb-compile
make buzzerweb-upload
make buzzerweb-flash # compile then upload

make scareprop-compile
make scareprop-upload
make scareprop-flash # compile then upload

make epaper-compile
make epaper-upload
make epaper-flash   # compile then upload

make monitor        # arduino-cli monitor, reads Serial output at 9600 baud,
                     # works for whichever module is currently flashed
```

Adding a new module: add its name to `MODULES` and a `MODULE_DIR_<name> :=
modules/<dir>` line in the Makefile, the three targets are generated
automatically from the `MODULE_TARGETS` template, don't hand-write another
compile/upload/flash block.

FQBN is `esp32:esp32:esp32`. Port is `/dev/cu.usbserial-0001`, overridable with
`make servo-flash PORT=...` (must be passed as a `make` argument, not a shell
environment variable prefix, `PORT=... make servo-flash` does not override the
Makefile's `PORT :=`). Baud rate for `make monitor` is 9600 (115200 was
confirmed unreliable, garbled serial, on this board/cable), overridable the
same way with `make monitor BAUD=...`. Press Ctrl+C to exit the monitor.

Wokwi reads the compiled artefacts, so `make <name>-compile` must run before
the simulation reflects a code change. Paths are declared in each folder's own
`wokwi.toml`.
